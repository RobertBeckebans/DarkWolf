<!-- doxygenix: sha256=a581e1a508fafd993ac052a8346dc6bd575af4355a59659d01e3d437114b9f7a -->

# engine/botlib/be_aas_reach.cpp

## File Purpose
- Implement all logic for calculating reachability links used by bots to plan paths across an AAS area network.
- Provide geometry helpers that determine whether a bot can move from one area to another using various movement primitives.
- Expose API functions such as AAS_BestReachableArea, AAS_ReachabilityExists and the incremental init routine AAS_ContinueInitReachability.

## Core Responsibilities
- Build the reach‑ability graph of the AAS navigation mesh, representing all viable one‑off movements between navigation areas.
- Compute per‑area travel links for various traversal types (walk, swim, jump, ladder, gravity‑affected moves, teleport, elevator, function‑bobbing, etc.).
- Allocate, reuse and free temporary reachability objects from a fixed heap for memory efficiency.
- Provide utility queries for area properties (grounded, crouch, liquid, ladder, jump‑pad, etc.) used during link construction.
- Store the finalized reachabilities into the AAS world structure for fast access by the bot navigation code.

## Key Types and Functions
- aas_lreachability_t ‑ temporary reach‑link structure used during graph construction.
- AAS_BestReachableLinkArea(aas_link_t* areas) ‑ selects the best candidate area from a linked list.
- AAS_BestReachableArea(vec3_t origin, vec3_t mins, vec3_t maxs, vec3_t goalorigin) ‑ finds a reachable area for a bounding box.
- AAS_SetupReachabilityHeap() ‑ pre‑allocates the memory pool for aas_lreachability_t.
- AAS_ShutDownReachabilityHeap() ‑ frees the heap.
- AAS_AllocReachability() ‑ pulls a link from the free pool.
- AAS_FreeReachability(aas_lreachability_t* lreach) ‑ returns a link to the pool.
- AAS_AreaReachability(int areanum) ‑ returns how many links an area currently has.
- AAS_FaceArea(aas_face_t* face) ‑ computes the area of a face.
- AAS_AreaVolume(int areanum) ‑ calculates the convex-hull volume of an area.
- AAS_AreaGroundFaceArea(int areanum) ‑ sums the areas of ground faces of an area.
- AAS_FaceCenter(int facenum, vec3_t center) ‑ arithmetic mean of a face’s vertices.
- AAS_FallDamageDistance() ‑ maximum height before fall damage.
- AAS_FallDelta(float distance) ‑ damage factor from falling a given distance.
- AAS_MaxJumpHeight(float sv_jumpvel) ‑ theoretical peak height for a given jump velocity.
- AAS_MaxJumpDistance(float sv_jumpvel) ‑ maximum horizontal distance attainable in a jump.
- AAS_AreaCrouch(int areanum) ‑ whether the area contains only crouch presence.
- AAS_AreaSwim(int areanum), AAS_AreaLiquid(aasworld, areanum), AAS_AreaLava(aasworld, areanum), AAS_AreaSlime(aasworld, areanum), AAS_AreaGrounded(aasworld, areanum), AAS_AreaLadder(aasworld, areanum), AAS_AreaJumpPad(aasworld, areanum), AAS_AreaTeleporter(aasworld, areanum), AAS_AreaDoNotEnter(aasworld, areanum), AAS_AreaDoNotEnterLarge(aasworld, areanum) ‑ various presence & content checks.
- AAS_BarrierJumpTravelTime() ‑ computes time for a barrier jump.
- AAS_ReachabilityExists(int area1num, int area2num) ‑ whether a reach link already exists.
- AAS_NearbySolidOrGap(vec3_t start, vec3_t end) ‑ tests for obstacles or gaps ahead.
- AAS_Reachability_Swim(int area1num, int area2num) ‑ creates swim links between adjacent swim areas.
- AAS_Reachability_EqualFloorHeight(int area1num, int area2num) ‑ links ground areas with equal floor height.
- AAS_Reachability_Step_Barrier_WaterJump_WalkOffLedge(int area1num, int area2num) ‑ links for step, barrier, water‑jump, and walk‑off‑ledge moves.
- AAS_Reachability_Ladder(int area1num, int area2num) ‑ links where ladder faces share an edge.
- AAS_Reachability_Teleport() ‑ links between trigger_teleport and target_teleporter destinations.
- AAS_Reachability_Elevator() ‑ links for func_plat elevator movement.
- AAS_FindFaceReachabilities(vec3_t* facepoints, int numpoints, aas_plane_t* plane, int towardsface) ‑ helper for func_bobbing.
- AAS_Reachability_FuncBobbing() ‑ processes func_bobbing entities to create links.
- AAS_Reachability_JumpPad() ‑ handles trigger_push / jumppad reachabilities.
- AAS_Reachability_Grapple(int area1num, int area2num) ‑ creates grapple‑hook links (currently disabled).
- AAS_SetWeaponJumpAreaFlags() ‑ marks areas that can be weapon‑jumped to.
- AAS_Reachability_WeaponJump(int area1num, int area2num) ‑ creates rocket/bfg jump links to weapon‑jump areas.
- AAS_Reachability_WalkOffLedge(int areanum) ‑ adds walk‑off‑ledge links for grounded areas.
- AAS_StoreReachability() ‑ copies the temporary linked list into the final linear reachability array in aasworld.
- AAS_ContinueInitReachability(float time) ‑ incremental construction loop; returns qtrue until all reachabilities are built.
- AAS_InitReachability() ‑ entry point that sets everything up and triggers the incremental loop.

## Important Control Flow
- When an AAS world is loaded, AAS_InitReachability initializes structures, the heap, per‑area reachability pointers, and may set a savefile flag for incremental building.
- AAS_ContinueInitReachability is repeatedly called with a time budget; it iterates over unprocessed areas and for each distinct pair (i,j) attempts to create reachability links for movement types such as swim, equal‑floor walk, step/​barrier/​water‑jump/​walk‑off‑ledge, ladder, jump, and subsequently grapple/​weapon‑jump (currently disabled).
- If the time slice expires or the progress percentage changes, the function returns qtrue to request another iteration; once all links are built it stores them with AAS_StoreReachability, frees the heap, and initiates cluster calculation.
- The construction uses AAS_AllocReachability / AAS_FreeReachability to manage a pre‑allocated pool, and geometry checks like AAS_PointAreaNum, AAS_TraceClientBBox, and face‑area calculations to validate feasibility.

## External Dependencies
- "q_shared.h", "l_log.h", "l_memory.h", "l_script.h", "l_libvar.h", "l_precomp.h", "l_struct.h"
- "aasfile.h", "botshared/botlib.h", "botshared/be_aas.h", "be_aas_funcs.h", "be_aas_def.h"
- "botimport" global table for printing, logging and Sys_MilliSeconds()
- "aassettings" for gravity, jump velocity, step height and other engine constants

## How It Fits
- This file is part of the bot library subsystem; it transforms static level geometry into a graph of actionable moves for the AI path‑finder.
- It interfaces with the AAS world data generated by the level loader and with the main bot code through exported functions.
- The reach‑ability graph is the foundation for all higher‑level navigation: after building the links, the AI can perform a graph search (e.g., Dijkstra/A*).
