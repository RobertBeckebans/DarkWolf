<!-- doxygenix: sha256=9a5101d0e1192f119f0e3712e33b9c4e0211db8d90b545cf392ed5a9a1ac0946 -->

# engine/botlib/be_aas_route.h

## File Purpose
- Header defining the routing interface of the AAS subsystem for bots and other AI agents.
- Declares routing initialization/finalization, travel‑time queries, reachability enumeration, random goal selection, and fuzzy point resolution functions.
- Separates internal implementation (`#ifdef AASINTERN`) from the public API exposed to other parts of the engine.

## Core Responsibilities
- Provide the public API for area‑based pathfinding and travel calculations within the AAS (Area Awareness System) module.
- Encapsulate internal routing data initialisation, caching, and cleanup.
- Translate area content descriptors into travel‑flag bitmasks for use by navigation algorithms.
- Expose helper functions for enumerating reachabilities, retrieving reachability data, and selecting random goal areas.
- Offer travel‑time computation utilities for intra‑area movement and inter‑area routing, including loop‑guarded path evaluation.
- Provide a fuzzy point‑to‑area resolver used by bots to find a valid start or goal area when their position is not directly inside a reachable zone.

## Key Types and Functions
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

## Important Control Flow
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

## External Dependencies
- vec3_t, aas_reachability_s, and other AAS world structures are defined in the core AAS headers (e.g., `<engine/botlib/be_aas.h>`).
- The implementation relies on global AAS world state typically declared in `aas.c` or `aas_global.h`.
- Functions such as `AAS_AreaRouteToGoalArea` and `AAS_AreaGroundSteepnessScale` are declared elsewhere in the AAS subsystem.
- Tracing utilities referenced in `BotFuzzyPointReachabilityArea` (e.g., `trap_AAS_Trace()` or a similar trace API) are provided by the game engine’s trace subsystem.

## How It Fits
- The AAS routing layer sits between low‑level collision/tracing and high‑level bot decision logic:
-     • Bots request routes and travel times via these functions, enabling strategic navigation decisions.
-     • The AAS routing populates reachability graphs from the static world definition, allowing efficient pathfinding.
-     • The API abstracts pathfinding details, letting upper‑level AI use simple calls like `AAS_AreaTravelTimeToGoalArea` or `AAS_RandomGoalArea` without needing to understand reachability internals.
-     • Runtime routing caches improve performance for repeated queries, and `AAS_FreeRoutingCaches` keeps memory usage manageable across level loads.
