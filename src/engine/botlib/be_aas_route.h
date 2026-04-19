/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU
General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*!
	\file engine/botlib/be_aas_route.h
	\brief Header defining the routing interface of the AAS subsystem for bots and other AI agents.
	\note doxygenix: sha256=9a5101d0e1192f119f0e3712e33b9c4e0211db8d90b545cf392ed5a9a1ac0946

	\par File Purpose
	- Header defining the routing interface of the AAS subsystem for bots and other AI agents.
	- Declares routing initialization/finalization, travel‑time queries, reachability enumeration, random goal selection, and fuzzy point resolution functions.
	- Separates internal implementation (`#ifdef AASINTERN`) from the public API exposed to other parts of the engine.

	\par Core Responsibilities
	- Provide the public API for area‑based pathfinding and travel calculations within the AAS (Area Awareness System) module.
	- Encapsulate internal routing data initialisation, caching, and cleanup.
	- Translate area content descriptors into travel‑flag bitmasks for use by navigation algorithms.
	- Expose helper functions for enumerating reachabilities, retrieving reachability data, and selecting random goal areas.
	- Offer travel‑time computation utilities for intra‑area movement and inter‑area routing, including loop‑guarded path evaluation.
	- Provide a fuzzy point‑to‑area resolver used by bots to find a valid start or goal area when their position is not directly inside a reachable zone.

	\par Key Types and Functions
	- AAS_InitRouting — Initialise routing tables and caches for the current world.
	- AAS_FreeRoutingCaches — Release all cached routing data.
	- AAS_AreaTravelTime (internal) — Compute intra‑area travel time based on terrain steepness and movement type.
	- AAS_AreaTravelTime — Public alias of the internal travel‑time computation or duplicate declaration (used for forward compatibility).
	- AAS_CreateAllRoutingCache — Populate all area routing caches (often called during AAS initialisation).
	- AAS_RoutingInfo — Output diagnostic information about routing caches (debug helper).
	- AAS_TravelFlagForType(int) — Retrieve the travel‑flag corresponding to a travel type index.
	- AAS_AreaContentsTravelFlag(int) — Derive travel flags for an area from its content descriptors.
	- AAS_NextAreaReachability(int, int) — Enumerate the next reachability index for a given area.
	- AAS_ReachabilityFromNum(int, aas_reachability_s*) — Copy a reachability entry identified by its global index.
	- AAS_RandomGoalArea(int, int, int*, vec3_t) — Select a random reachable goal area satisfying given travel flags.
	- AAS_AreaTravelTimeToGoalArea(int, vec3_t, int, int) — Compute travel time from an origin to a goal area using a complete route.
	- AAS_AreaTravelTimeToGoalAreaCheckLoop(int, vec3_t, int, int, int) — Compute travel time while rejecting routes that loop back to a specified area.
	- BotFuzzyPointReachabilityArea(vec3_t) — Resolve the nearest reachable area for an arbitrary point using trace searches.

	\par Control Flow
	- During initialization (AAS_InitRouting) the routing tables are built and cached for the current world, enabling subsequent path queries.
	- When AAS_FreeRoutingCaches is called the cached routing data is freed, clearing memory used for reachability and area traversal.
	- AAS_AreaTravelTime computes an intra‑area traversal time by scaling distance with terrain steepness and movement type, which is used by higher‑level travel time functions.
	- AAS_AreaTravelTimeToGoalArea requests a full area‑to‑area route via AAS_AreaRouteToGoalArea, aggregates the per‑edge travel times, and returns the total.
	- AAS_AreaTravelTimeToGoalAreaCheckLoop extends the previous behavior by rejecting routes that would return to a specified loop area, thereby avoiding circular paths.
	- AAS_RandomGoalArea scans the world for a random reachable goal, respecting travelflags, and returns the chosen area and destination coordinates.
	- AAS_NextAreaReachability iterates over the linked list of reachabilities stored per area, enabling enumeration of all outgoing edges.
	- AAS_ReachabilityFromNum copies a reachability entry from the internal array into user‑supplied memory for direct inspection.
	- AAS_AreaContentsTravelFlag derives a travel‑flag bitmask from an area’s content bits (water, lava, slime, not‑enter, etc.), informing pathfinding about traversal constraints.
	- AAS_TravelFlagForType simply looks up the flag corresponding to a particular travel type index in the global AAS world data.
	- BotFuzzyPointReachabilityArea performs a multi‑step trace search (point, upward, expanded grid) to locate the nearest reachable area for a supplied world position.

	\par Dependencies
	- vec3_t, aas_reachability_s, and other AAS world structures are defined in the core AAS headers (e.g., `<engine/botlib/be_aas.h>`).
	- The implementation relies on global AAS world state typically declared in `aas.c` or `aas_global.h`.
	- Functions such as `AAS_AreaRouteToGoalArea` and `AAS_AreaGroundSteepnessScale` are declared elsewhere in the AAS subsystem.
	- Tracing utilities referenced in `BotFuzzyPointReachabilityArea` (e.g., `trap_AAS_Trace()` or a similar trace API) are provided by the game engine’s trace subsystem.

	\par How It Fits
	- The AAS routing layer sits between low‑level collision/tracing and high‑level bot decision logic:
	-     • Bots request routes and travel times via these functions, enabling strategic navigation decisions.
	-     • The AAS routing populates reachability graphs from the static world definition, allowing efficient pathfinding.
	-     • The API abstracts pathfinding details, letting upper‑level AI use simple calls like `AAS_AreaTravelTimeToGoalArea` or `AAS_RandomGoalArea` without needing to understand reachability internals.
	-     • Runtime routing caches improve performance for repeated queries, and `AAS_FreeRoutingCaches` keeps memory usage manageable across level loads.
*/

/*****************************************************************************
 * name:		be_aas_route.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#ifdef AASINTERN
// initialize the AAS routing
void			   AAS_InitRouting();
// free the AAS routing caches
void			   AAS_FreeRoutingCaches();
// returns the travel time from start to end in the given area
unsigned short int AAS_AreaTravelTime( int areanum, vec3_t start, vec3_t end );
//
void			   AAS_CreateAllRoutingCache();
//
void			   AAS_RoutingInfo();
#endif // AASINTERN

/*!
	\brief Retrieves the travel flag for a given travel type.

	If the provided type is outside the valid range, the function returns the special constant TFL_INVALID. Otherwise it returns the flag located in the global AAS world structure for that type.

	\param traveltype index of the travel type whose flag is requested.
	\return An integer representing the travel flag, or the constant TFL_INVALID for an out‑of‑range type.
*/
int				   AAS_TravelFlagForType( int traveltype );

/*!
	\brief Retrieves travel flags for a specified area based on its content descriptors.

	The function looks up the area settings for the given area number and examines the contents bitfield. Depending on which content bits are set, it returns a bitmask of travel flags: a water, slime,
   or lava area yields the corresponding travel flag; if none of these are set, the area is considered air. It also adds the large not‑enter or standard not‑enter flags if the respective content bits
   are present, returning immediately when the standard not‑enter flag is set. The resulting bitmask can be used by pathfinding code to determine which travel types are prohibited for the area.

	\param areanum Index of the area whose contents should be queried in the current AAS world.
	\return A bitmask of travel flags representing the area’s contents, such as water, slime, lava, air, and not‑enter restrictions.
*/
int				   AAS_AreaContentsTravelFlag( int areanum );

/*!
	\brief Retrieves the next reachability index for a specified area.

	Given an area number and a current reachability index, the function returns the subsequent reachability index in the area’s routing list.

	If the provided current index is 0, the function returns the first reachable area index. Any index less than the first reachable area triggers a fatal error message. After incrementing the current
   index, if the result exceeds the number of reachable areas, 0 is returned to indicate that no further reachabilities exist for the area.

	\param areanum Numerical identifier of the area whose reachabilities are queried
	\param reachnum Current reachability index; zero starts enumeration
	\return The next reachability index, or zero if there are none left.
*/
int				   AAS_NextAreaReachability( int areanum, int reachnum );

/*!
	\brief Retrieves a reachability entry by index and stores it in the supplied buffer.

	If the navigation system has not been initialized or the index is outside the valid range, the reachability structure is zeroed before returning. Otherwise it copies the entry from the internal
   reachability array into the supplied structure.

	\param num Index of the desired reachability within the internal array
	\param reach Pointer to a reachability structure that will receive a copy of the requested entry
*/
void			   AAS_ReachabilityFromNum( int num, struct aas_reachability_s* reach );

/*!
	\brief Selects a random reachable goal area from a given area and provides the area index and coordinates.

	The function begins by verifying that the supplied area has any reachability information; if none, it fails immediately. It then selects a random target area among all areas in the world, wrapping
   around to stay within bounds. For each candidate, it checks that a travel path exists to the candidate area with the given travel flags. If the candidate is a swim area, its center point is
   returned as the goal. Otherwise, a downward trace from the candidate’s center is performed, and if the trace ends in the candidate area and the area has a sufficient ground face height, the trace
   end position is returned. The function continues scanning until a suitable area is found or all areas have been considered, returning success or failure accordingly.

	\param areanum Index of the starting area to search from
	\param travelflags Travel flags used when determining reachability
	\param goalareanum Pointer to an integer where the chosen goal area number will be stored
	\param goalorigin Vector to receive the coordinates of the goal location
	\return Non‑zero integer (true) if a goal area was found; zero (false) otherwise.
*/
int				   AAS_RandomGoalArea( int areanum, int travelflags, int* goalareanum, vec3_t goalorigin );

/*!
	\brief Computes the time needed to move between two points inside a specified area, adjusting for terrain steepness and movement type.

	The function first calculates the straight‑line distance between the start and end coordinates. It then scales this distance by the area’s terrain steepness factor obtained via
   AAS_AreaGroundSteepnessScale. Depending on the area’s movement properties, the distance is multiplied by one of three predefined factors: crouch, swim, or normal walk. The resulting value is
   rounded up to the nearest integer and returned as an unsigned short. If the computed distance is zero, it is set to one to avoid returning an invalid travel time. The function returns the estimated
   travel time in milliseconds for moving within the area between the two points.

	\param areanum identifies the area whose properties are used for scaling and movement type selection
	\param start the starting position vector
	\param end the ending position vector
	\return the travel time in milliseconds to traverse from start to end within the specified area
*/
unsigned short int AAS_AreaTravelTime( int areanum, vec3_t start, vec3_t end );

/*!
	\brief Returns the travel time between a starting area and a goal area using the specified travel flags.

	The function first attempts to compute a route using AAS_AreaRouteToGoalArea. If a route is found, the accumulated travel time (in ticks) for that route is returned. If no route can be found, the
   function returns 0. The resulting time reflects the time required by the underlying navigation system to move from the origin point to the goal area while obeying the provided travel constraints.

	\param areanum Identifier of the starting area number
	\param origin World‑space origin point from which the travel time is calculated
	\param goalareanum Identifier of the destination area number
	\param travelflags Bit mask specifying allowed travel types (e.g., walk, swim)
	\return The travel time in ticks between the start and goal areas, or 0 if no path exists.
*/
int				   AAS_AreaTravelTimeToGoalArea( int areanum, vec3_t origin, int goalareanum, int travelflags );

/*!
	\brief Calculates the time to travel from a starting area to a goal area while avoiding loops

	The function first attempts to find a route from the current area to the goal area using the supplied travel flags. If a route is found, the resulting travel time is examined. Additionally, if the
   proposed destination area equals a provided loop safeguard area, the route is rejected to prevent a circular path and a zero time is returned. All other unreachable or looped scenarios also result
   in a zero return value.

	\param areanum the numeric identifier of the starting area
	\param origin the position vector within the starting area
	\param goalareanum the numeric identifier of the destination area
	\param travelflags bitmask specifying allowed traversal types
	\param loopareanum numeric identifier of an area that should not be targeted to avoid looping
	\return The computed travel time in game ticks; zero indicates the goal is unreachable or would create a looped route
*/
int				   AAS_AreaTravelTimeToGoalAreaCheckLoop( int areanum, vec3_t origin, int goalareanum, int travelflags, int loopareanum );

/*!
	\brief Compute a reachable area number for a point, using fuzzy search when direct reachability fails

	This function attempts to locate the navigation area that contains the given point. It first queries the exact area at the point; if that area is reachable it is returned immediately. If not, an
   upwards trace by 4 units is performed to locate a nearby reachable area. When still no reachable area is found, the function expands the search in a 3×3×3 grid around the origin, offsetting the
   target by 256 units horizontally and 200 units vertically in the positive and negative directions, and traces areas along these directions. The closest reachable area intersected during this
   expanded search is chosen as the best candidate. If no reachable area is identified, the function falls back to the first area encountered during any trace and finally returns 0 if all attempts
   fail.

	The function is intended for AI navigation to determine a suitable start or target area when a point may lie outside of a reachable zone.

	\param origin 3‑D world position of the point to test
	\return Area number (integer) for the point; 0 indicates no reachable area could be determined
*/
extern int		   BotFuzzyPointReachabilityArea( vec3_t origin );
