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
	\file engine/botlib/be_aas_routealt.h
	\brief Declares the public interface for the alternative routing part of the AAS subsystem; includes init/shutdown hooks used only by internal AAS code paths.
	\note doxygenix: sha256=4aa48da6ac911e842ecaebda480497c35d99d0444e72548882f6a80777e65c65

	\par File Purpose
	- Declares the public interface for the alternative routing part of the AAS subsystem; includes init/shutdown hooks used only by internal AAS code paths.

	\par Core Responsibilities
	- Initialize and shutdown data structures required for computing alternative routing paths.
	- Compute a set of alternative route goal candidates between two world positions, constrained by travel flags and time limits.
	- Populate an array of `aas_altroutegoal_t` structures with the results and report the total count.

	\par Key Types and Functions
	- AAS_InitAlternativeRouting() — initializes bookkeeping and lookup tables needed for alternative routing before AAS usage begins.
	- AAS_ShutdownAlternativeRouting() — frees resources allocated by the initialization routine.
	- AAS_AlternativeRouteGoals(vec3_t start, vec3_t goal, int travelflags, aas_altroutegoal_t* altroutegoals, int maxaltroutegoals, int color) — enumerates up to `maxaltroutegoals` alternative routing goals between `start` and `goal`, respecting `travelflags`, while optionally rendering debug color.
	- aas_altroutegoal_t — structure that holds an area index, travel times, and a world position for an alternative goal.
	- vec3_t — 3‑component floating point vector representing world positions.

	\par Control Flow
	- The `AAS_AlternativeRouteGoals` routine receives world coordinates for a start and a target position, along with travel flags that mask allowed path types, then internally queries the AAS graph to locate candidate mid‑area nodes that lie on valid travel paths between the two endpoints. It calculates the shortest travel time for the direct route, filters all mid‑areas whose travel time does not exceed a threshold relative to that shortest path, clusters the surviving nodes to avoid redundancy, and then populates an output array with the best alternative route goal data (area number, travel time, and position). It returns the count of goals written, and may emit debug geometry in the specified color if debugging is enabled.

	\par Dependencies
	- vec3_t type (defined in a shared math header, e.g., `math/tr_vec3.h`).
	- aas_altroutegoal_t struct (defined in an AAS types header, e.g., `engine/botlib/be_aas_types.h`).

	\par How It Fits
	- Part of the bot navigation engine; extends core AAS pathfinding by providing non‑shortest alternatives that bots may choose to avoid congested or unsafe routes.
	- Interacts with the AAS world graph and travel‐time computations, enabling higher‑level AI decision layers to request alternative objectives.
*/

/*****************************************************************************
 * name:		be_aas_routealt.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#ifdef AASINTERN
void AAS_InitAlternativeRouting();
void AAS_ShutdownAlternativeRouting();
#endif // AASINTERN

/*!
	\brief Finds alternative route goals between a start and goal position using AAS data structures

	This function computes alternative route goals by identifying mid-range areas that connect the start and goal positions through valid travel paths. It filters areas based on travel time constraints relative to the shortest path and then clusters these areas to find optimal alternative goal positions. The function returns the number of alternative goals found, up to the maximum allowed, and populates the provided array with the details of each alternative goal including area number, travel times, and position coordinates.

	\param start Starting position in the world
	\param goal Target position in the world
	\param travelflags Travel flags to determine valid paths
	\param altroutegoals Output array to store alternative route goals
	\param maxaltroutegoals Maximum number of alternative goals to return
	\param color Debug color for visualization (if enabled)
	\return The number of alternative route goals found and stored in the altroutegoals array
*/
int AAS_AlternativeRouteGoals( vec3_t start, vec3_t goal, int travelflags, aas_altroutegoal_t* altroutegoals, int maxaltroutegoals, int color );
