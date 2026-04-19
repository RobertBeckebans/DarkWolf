<!-- doxygenix: sha256=22115d730e35ba3cb7e300ab23f1826cc30947d268fbbe941d53f739c854ca99 -->

# engine/botlib/be_aas_routealt.cpp

## File Purpose
- Implement the optional alternative routing logic for the AAS-based bot pathfinding engine, enabling bots to discover and use alternate paths when the optimal path is unsuitable or blocked.
- Provide data structures, initialization, shutdown, and the core algorithm that selects valid, near‑optimal alternative route goals.

## Core Responsibilities
- Maintain global arrays used to store mid‑range area candidates and temporary cluster lists.
- Provide an alternative route goal selection algorithm that expands from a set of mid‑range area nodes, clusters them, and chooses representative goal positions.
- Offer initialization and shutdown helpers that allocate and free the data structures, ensuring proper resource management in the AAS subsystem.

## Key Types and Functions
- midrangearea_t — structure holding validity flag and travel times to start/goal for mid‑range area nodes.
- midrangeareas — pointer to dynamic array of midrangearea_t objects sized by the world’s area count.
- clusterareas — pointer to dynamic array of integers used to collect connected mid‑range area numbers during flood fill.
- numclusterareas — integer counter of current cluster size during flooding.
- AAS_AltRoutingFloodCluster_r(int areanum) — recursively marks an area as part of the current cluster and propagates through all reachable neighboring mid‑range areas.
- AAS_AlternativeRouteGoals(vec3_t start, vec3_t goal, int travelflags, aas_altroutegoal_t* altroutegoals, int maxaltroutegoals, int color) — main routine that evaluates all potential mid‑range areas, clusters them, and fills the output array with alternative route goals.
- AAS_InitAlternativeRouting() — allocates or reallocates the global midrangeareas and clusterareas arrays when alternative routing is enabled.
- AAS_ShutdownAlternativeRouting() — frees the allocated arrays, clears global pointers, and resets cluster count when alternative routing is disabled.

## Important Control Flow
- When enabled by the ENABLE_ALTROUTING macro, AAS_AlternativeRouteGoals is the entry point. It first validates the start and goal positions, retrieves their area numbers, and computes the fastest travel time from start to goal.
- It then iterates over all areas, selecting those that contain the AREACONTENTS_ROUTEPORTAL flag, have reachable paths, and whose travel time to/from the start and goal areas is within 1.5× the fastest travel time; such areas are marked as mid‑range candidates in the midrangeareas array.
- For each valid mid‑range area, the recursive function AAS_AltRoutingFloodCluster_r flood‑fills through reachable faces, building a cluster of connected mid‑range areas in the clusterareas array and marking processed areas as invalid.
- The cluster’s centroid is computed, then the area within the cluster closest to that centroid is chosen as an alternative route goal. Its position, travel times, and extra time are written into the caller‑supplied altroutegoals buffer.
- The process stops when either all clusters are processed or the caller’s maximum goal count is reached, after which a diagnostic message reports the number of goals produced.

## External Dependencies
- "q_shared.h", "l_utils.h", "l_memory.h", "l_log.h", "l_script.h", "l_precomp.h", "l_struct.h" (standard core headers for utilities and memory management)
- "aasfile.h", "botshared/botlib.h", "botshared/be_aas.h", "be_aas_funcs.h", "be_interface.h", "be_aas_def.h" (AAS engine and bot library interfaces)
- Global symbols AAS_PointAreaNum, AAS_AreaTravelTimeToGoalArea, AAS_AreaReachability, and several vector operations (VectorClear, VectorAdd, VectorScale, VectorSubtract, VectorLength, VectorCopy).
- Bot import functions for diagnostics (botimport.Print) and drawing primitives (AAS_DrawPermanentCross).

## How It Fits
- This component sits within the bot AAS module, extending core travel‑time calculations to support strategic route diversification.
- It is invoked by caller code that requires a set of fallback goal positions, typically when a primary path is compromised or when multiple concurrent bot paths need spreading.
- By grouping mid‑range nodes into clusters, it reduces per‑bot decision overhead and encourages spatially distinct alternative goals.
