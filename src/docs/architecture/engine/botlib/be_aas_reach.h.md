<!-- doxygenix: sha256=e1a8c0447c6cb0aaf580a6455694fee0886e777a3021bfc5fd70719d899c90c1 -->

# engine/botlib/be_aas_reach.h

## File Purpose
- Header for functions that initialize reachability data and query navigation area properties within the Return to Castle Wolfenstein bot AAS subsystem.

## Core Responsibilities
- Build and maintain the per‑area reachability tables used by AI navigation.
- Expose fast lookup routines to determine area properties such as liquid, crouch‑only, ladder, or flood conditions.
- Provide utilities that allow the bot library to find reachable areas for a bounding box and compute associated goal origins.

## Key Types and Functions
- AAS_InitReachability() — initializes full reachability graph for the AAS world.
- AAS_ContinueInitReachability(float time) — continues incremental reachability construction over a time slice.
- AAS_BestReachableLinkArea(aas_link_t* areas) — returns the best reachable link area for a given array of links.
- AAS_AreaReachability(int areanum) — returns the number of reachable areas from the specified area.
- AAS_BestReachableArea(vec3_t origin, vec3_t mins, vec3_t maxs, vec3_t goalorigin) — finds a reachable area for a bounding box and sets a goal point within it.
- AAS_NextModelReachability(int num, int modelnum) — iterates the reachability list to find the next entry whose face references the specified model.
- AAS_AreaGroundFaceArea(int areanum) — sums the surface areas of all ground faces in an area.
- AAS_AreaCrouch(int areanum) — returns true if the area is designated as crouch‑only.
- AAS_AreaSwim(int areanum) — indicates whether the area contains swimable liquid.
- AAS_AreaLiquid(int areanum) — tests if the area contains any liquid content.
- AAS_AreaLava(int areanum) — checks if the area contains lava.
- AAS_AreaSlime(int areanum) — checks if the area contains slime.
- AAS_AreaGrounded(int areanum) — reports if the area has at least one ground face.
- AAS_AreaLadder(int areanum) — reports if the area contains ladder geometry.
- AAS_AreaJumpPad(int areanum) — reports if the area contains a jump pad.
- AAS_AreaDoNotEnter(int areanum) — reports if the area is marked as no‑enter.
- AAS_AreaDoNotEnterLarge(int areanum) — reports if the large no‑enter flag is set for the area.

## Important Control Flow
- Functions read global navigation data from the `aasworld` object, accessing area definitions, reachability tables, and flag bitmasks.
- During reachability initialization, `AAS_InitReachability` builds the full reachability graph, while `AAS_ContinueInitReachability` performs incremental updates over a given time budget.
- `AAS_AreaReachability` queries the pre‑computed reachability count for an area, validating the index before returning the stored value.
- `AAS_BestReachableArea` locates a reachable AAS area for a bounding box at a given origin; it performs collision traces, checks ground placement, and, if necessary, searches linked invisible entities to determine the best reachable area.
- `AAS_NextModelReachability` scans the global reachability array for entries whose face references a specified model index; it returns the next matching index or 0 if none is found.
- Other helper functions (`AAS_AreaSwim`, `AAS_AreaLava`, etc.) fetch flag bits from an area’s settings and return boolean results.

## External Dependencies
- Requires the `engine/botlib/be_aas.h` header for definitions of `aasworld`, `aas_link_t`, `vec3_t`, and flag constants.
- Depends on global state declared and initialized in the AAS subsystem (e.g., `aasworld` pointer).
- Implicitly uses standard integer and float types from `<cstdint>` and `<cfloat>`.

## How It Fits
- Part of the AI navigation engine, this component enables bots to assess which areas are reachable from a given point and to gather terrain characteristics needed for movement planning.
- Provides a stable, low‑level API consumed by path planners, decision trees, and animation controllers to compute movement trajectories and environmental interactions.
- Interfaces with the global AAS world data, allowing other subsystems to avoid direct access to raw navigation structures.
