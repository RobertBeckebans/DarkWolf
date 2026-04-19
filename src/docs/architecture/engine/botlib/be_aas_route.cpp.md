<!-- doxygenix: sha256=8e1d6f3ee9e87fd7cacc76fef1411c9ee83fa18f6cd9027854ee3f5723fb24cb -->

# engine/botlib/be_aas_route.cpp

## File Purpose
- Implement the Area Awareness System (AAS) routing subsystem used by bots to compute travel times, reachability, and visibility.
- Define helper routines for memory allocation, cache management, and data persistence specific to routing.
- Provide high‑level query interfaces such as AAS_AreaTravelTimeToGoalArea, AAS_AreaVisible, and path selection utilities.
- Bridge between raw AAS world data (areas, clusters, portals, reachabilities) and bot navigation logic.

## Core Responsibilities
- Maintain an in‑memory routing cache for every area/portal pair, storing travel times and best reachability indices.
- Compute travel times between points inside areas, adjusting for terrain steepness and movement type, and cache them per‑area for rapid reuse.
- Generate visibility bitsets for all routable areas, allowing quick line‑of‑sight tests without expensive tracers.
- Provide pathfinding helpers that return travel times, reachabilities, and safe hiding or attack spots while respecting travel restrictions.
- Persist routing caches to disk (.rcd files) and reload them, ensuring consistency across map loads.
- Offer debugging hooks to report cache update statistics and cache size.

## Key Types and Functions
- aas_routingcache_t — stores per‑area/portal travel time and reachability data, linked into cluster or portal lists.
- aas_reversedreachability_t — per‑area structure holding a singly linked list of reversed reachability entries.
- aas_reversedlink_t — represents a single reversed link storing the source area number and the original reachability link index.
- aas_routingupdate_t — temporary node used during cache updates to propagate travel-time improvements.
- AAS_RoutingGetMemory(int size) — abstraction over GetClearedMemory that returns zero‑initialised block used for any routing allocation.
- AAS_RoutingFreeMemory(void *ptr) — frees routing‑allocated memory via FreeMemory.
- AAS_InitTravelFlagFromType() — populates global travelflagfortype[] mapping each travel type ID to its corresponding travel flag.
- AAS_TravelFlagForType(int traveltype) — retrieves the travel flag for a given type, returning TFL_INVALID for out‑of‑range inputs.
- AAS_ClusterAreaNum(int cluster, int areanum) — translates a global area number into a local cluster area index, handling portals appropriately.
- AAS_AreaTravelTime(int areanum, vec3_t start, vec3_t end) — calculates raw travel time between two points inside an area, applying speed factors and ground steepness.
- AAS_CalculateAreaTravelTimes() — builds a 3‑dimensional array of travel times for every area‑reachable pair, factoring in pre‑calculated travel times and directionality.
- AAS_CreateReversedReachability() — creates reverse links for each reachability, enabling efficient backward searches during cache updates.
- AAS_AreaGroundSteepnessScale(int areanum) — returns a scaling factor derived from an area’s ground steepness.
- AAS_PortalMaxTravelTime(int portalnum) — scans reachable areas from a portal returning the worst‑case travel time.
- AAS_InitPortalMaxTravelTimes() — populates portalmaxtraveltimes[] with per‑portal maximum travel times.
- AAS_FreeRoutingCache(aas_routingcache_t *cache) — decrements global cache size and frees the cache.
- AAS_FreeAllClusterAreaCache() — traverses clusterareacache[][], freeing every node.
- AAS_FreeAllPortalCache() — frees the portalcache[] linked lists.
- AAS_FreeAreaVisibility() — releases visibility bitset storage and the compressed buffers.
- AAS_InitRoutingUpdate() — allocates per‑area and per‑portal working buffers used for incremental cache updates.
- AAS_CreateVisibility() — performs visibility checks for every routable pair, compresses results, and stores them.
- AAS_GetAreaRoutingCache(int clusternum, int areanum, int travelflags, qboolean forceUpdate) — fetches or allocates an area cache and triggers UpdateAreaRoutingCache().
- AAS_UpdateAreaRoutingCache(aas_routingcache_t *areacache) — breadth‑first propagates travel times to all reachable areas within the same cluster.
- AAS_GetPortalRoutingCache(int clusternum, int areanum, int travelflags) — retrieves or builds a portal routing cache, driving UpdatePortalRoutingCache().
- AAS_UpdatePortalRoutingCache(aas_routingcache_t *portalcache) — expands reachability through portals, collecting minimal travel times per portal.
- AAS_AreaRouteToGoalArea(int areanum, vec3_t origin, int goalareanum, int travelflags, int *traveltime, int *reachnum) — the core path planner; returns travel time or zero if no route.
- AAS_AreaTravelTimeToGoalArea(int areanum, vec3_t origin, int goalareanum, int travelflags) — wrapper around AreaRouteToGoalArea that returns only the travel time.
- AAS_AreaTravelTimeToGoalAreaCheckLoop(...) — same as above but rejects routes that loop back to a given area.
- AAS_AreaReachabilityToGoalArea(int areanum, vec3_t origin, int goalareanum, int travelflags) — retrieves the reachability index for a successful route.
- AAS_ReachabilityFromNum(int num, struct aas_reachability_s *reach) — copies a reachability entry from the global array to user buffer.
- AAS_AreaVisible(int srcarea, int destarea) — returns true if destarea appears in srcarea’s decompressed visibility bitset, decompressing it on first call.
- AAS_NearestHideArea(...) — finds the closest safe hiding spot for a bot relative to an enemy, respecting travel flags and visibility constraints.
- AAS_FindAttackSpotWithinRange(...) — selects an attack position that can see the enemy, lies within a specified range, and is reachable without violating flags.
- AAS_GetRouteFirstVisPos(vec3_t srcpos, vec3_t destpos, int travelflags, vec3_t retpos) — walks along a generated route and returns the first point that can see the destination.

## Important Control Flow
- When initialized, the routing subsystem builds reversed reachability links, travel flag tables, area travel time matrices, and visibility caches by sequentially calling setup functions such as AAS_InitTravelFlagFromType, AAS_CreateReversedReachability, AAS_CalculateAreaTravelTimes, and AAS_CreateVisibility; these run once during AAS_InitRouting().
- AAS_CreateAllRoutingCache() iterates over all areas, marks routable ones, and requests per‑area/portal caches via AAS_GetAreaRoutingCache() and AAS_GetPortalRoutingCache(); each cache update traverses reversed links or portal links using breadth‑first search, filling travel times and reachability indices.
- While running queries, AAS_AreaRouteToGoalArea() ensures caching limits by freeing oldest caches, determines cluster relations, and picks between area and portal caching paths, then returns traversal time and reachability number.
- Visibility queries decompress cached RLE vectors on demand (AAS_AreaVisible), while AAS_CreateVisibility precomputes bidirectional visibility using PVS and precise tracers.
- Stateful searches such as AAS_NearestHideArea and AAS_FindAttackSpotWithinRange use global routing update arrays as work queues, iterating through reachability lists, computing travel times, and rejecting paths based on travel flags, visibility, bounds, or entity presence.
- AAS_GetRouteFirstVisPos() walks along a computed route, checking visibility at each segment until a visible point is found.

## External Dependencies
- "aasfile.h", "botshared/botlib.h", "botshared/be_aas.h", "be_aas_funcs.h", "be_interface.h", "be_aas_def.h"
- "q_shared.h", "l_memory.h", "l_log.h", "l_libvar.h", "l_script.h", "l_utils.h", "l_precomp.h", "l_struct.h", "l_crc.h"

## How It Fits
- Be_aas_route.cpp is part of the bot navigation stack; it operates atop the AAS data structures populated during level load, adding caching layers for runtime speed.
- It exposes a stable C API that other bot modules call to obtain travel times or visibility, enabling high‑level behaviour like pathing, hiding, or attack positioning.
- The route cache mechanism allows the engine to reuse expensive pre‑computed path matrices across level loads, reducing bot startup costs.
- The module is tightly coupled to the global aasworld instance but is designed to support multiple worlds if the engine is extended, indicated by the external aas_t array.
