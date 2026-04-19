<!-- doxygenix: sha256=4aa48da6ac911e842ecaebda480497c35d99d0444e72548882f6a80777e65c65 -->

# engine/botlib/be_aas_routealt.h

## File Purpose
- Declares the public interface for the alternative routing part of the AAS subsystem; includes init/shutdown hooks used only by internal AAS code paths.

## Core Responsibilities
- Initialize and shutdown data structures required for computing alternative routing paths.
- Compute a set of alternative route goal candidates between two world positions, constrained by travel flags and time limits.
- Populate an array of `aas_altroutegoal_t` structures with the results and report the total count.

## Key Types and Functions
- AAS_InitAlternativeRouting() — initializes bookkeeping and lookup tables needed for alternative routing before AAS usage begins.
- AAS_ShutdownAlternativeRouting() — frees resources allocated by the initialization routine.
- AAS_AlternativeRouteGoals(vec3_t start, vec3_t goal, int travelflags, aas_altroutegoal_t* altroutegoals, int maxaltroutegoals, int color) — enumerates up to `maxaltroutegoals` alternative routing goals between `start` and `goal`, respecting `travelflags`, while optionally rendering debug color.
- aas_altroutegoal_t — structure that holds an area index, travel times, and a world position for an alternative goal.
- vec3_t — 3‑component floating point vector representing world positions.

## Important Control Flow
- The `AAS_AlternativeRouteGoals` routine receives world coordinates for a start and a target position, along with travel flags that mask allowed path types, then internally queries the AAS graph to locate candidate mid‑area nodes that lie on valid travel paths between the two endpoints. It calculates the shortest travel time for the direct route, filters all mid‑areas whose travel time does not exceed a threshold relative to that shortest path, clusters the surviving nodes to avoid redundancy, and then populates an output array with the best alternative route goal data (area number, travel time, and position). It returns the count of goals written, and may emit debug geometry in the specified color if debugging is enabled.

## External Dependencies
- vec3_t type (defined in a shared math header, e.g., `math/tr_vec3.h`).
- aas_altroutegoal_t struct (defined in an AAS types header, e.g., `engine/botlib/be_aas_types.h`).

## How It Fits
- Part of the bot navigation engine; extends core AAS pathfinding by providing non‑shortest alternatives that bots may choose to avoid congested or unsafe routes.
- Interacts with the AAS world graph and travel‐time computations, enabling higher‑level AI decision layers to request alternative objectives.
