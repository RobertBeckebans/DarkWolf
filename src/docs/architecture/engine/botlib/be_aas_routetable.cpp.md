<!-- doxygenix: sha256=898e2e0d73f8d3d2c22ca3ed16ae6d09537a2befb2401105efb52fb1198137ed -->

# engine/botlib/be_aas_routetable.cpp

## File Purpose
- Define and manage the runtime route table used by the bot navigation system; enable fast area‑to‑area path queries, persistence of pre‑computed route data, and support debug and utility operations.

## Core Responsibilities
- Provide an on‑disk cache of AAS navigation information (route tables).
- Compute and store parent/child relationships, visibility lists, travel times, and parent link indices for all routable areas.
- Offer fast look‑ups of reachability and travel time between any two areas via AAS_RT_GetRoute() and AAS_RT_GetChild().
- Manage memory for the runtime route tables, reporting usage with AAS_RT_PrintMemoryUsage().
- Handle IO of serialized route tables (.rtb files) in little‑endian format.
- Facilitate advanced bot behaviors such as hiding positions (AAS_RT_GetHidePos) and debug visualization (AAS_RT_ShowRoute()).

## Key Types and Functions
- aas_rt_t – top‑level runtime route table structure containing arrays of children, parents, and link tables.
- aas_rt_child_t – runtime data for a single area, including child index offset and parent link metadata.
- aas_rt_parent_t – runtime representation of a parent area, with child lists and visibility data.
- aas_rt_parent_link_t – a single parent‑child link, storing childIndex and parent.
- aas_rt_route_t – runtime route entry, holding reachable_index and travel_time.
- AAS_RT_GetClearedMemory(size_t size) – allocates a zeroed block and updates global memory counters.
- AAS_RT_FreeMemory(void* ptr) – frees a block and adjusts tracking counters.
- AAS_RT_PrintMemoryUsage() – prints current memory usage when enabled.
- AAS_RT_GetValidVisibleAreasCount(aas_area_buildlocalinfo_t* localinfo, aas_area_childlocaldata_t** childlocaldata) – counts unlinked visible areas for a given area.
- AAS_RT_CalcTravelTimesToGoalArea(int goalarea) – populates travel times for all filtered routes to goalarea.
- AAS_RT_CalculateRouteTable(aas_rt_route_t** parmroutetable) – iterates filtered areas, calling AAS_RT_CalcTravelTimesToGoalArea for each.
- AAS_RT_AddParentLink(aas_area_childlocaldata_t* child, int parentindex, int childindex) – inserts a new parent link in the child’s list.
- AAS_RT_WriteShort(unsigned short int si, fileHandle_t fp) & AAS_RT_WriteByte(int si, fileHandle_t fp) – helper writers for little‑endian serialization.
- AAS_RT_WriteRouteTable() – serializes the entire aas_rt_t to a .rtb file, including identifiers, version, CRC, and all child/parent/link data.
- AAS_RT_DBG_Read(void* buf, int size, int fp) – thin wrapper around botimport.FS_Read.
- AAS_RT_ReadRouteTable(fileHandle_t fp) – deserializes a .rtb, performs CRC validation, swaps endianness if needed, and reconstructs all runtime tables and indices.
- AAS_RT_NumParentLinks(aas_area_childlocaldata_t* child) – walks a child’s parentlink list counting entries.
- AAS_RT_BuildRouteTable() – high‑level constructor that builds or loads the route table, including filtering, parent selection, index building, and cleanup.
- AAS_RT_ShutdownRouteTable() – releases all runtime data and nulls the global reference.
- AAS_RT_GetFirstParentLink(aas_rt_child_t* child) – returns a pointer to the first aas_rt_parent_link_t for a child.
- AAS_RT_GetChild(int areanum) – returns the runtime child structure for an area or NULL.
- AAS_RT_GetRoute(int srcnum, vec3_t origin, int destnum) – round‑robin cache of 64 routes; calls AAS_AreaRouteToGoalArea and fills reachable_index and travel_time.
- BotGetReachabilityToGoal(...) – selects a suitable reachability toward a goal respecting flags and avoidance lists.
- AAS_RT_ShowRoute(vec3_t srcpos, int srcnum, int destnum) – debug drawing of a route; only when compiled with DEBUG.
- AAS_RT_GetHidePos(vec3_t srcpos, int srcnum, int srcarea, vec3_t destpos, int destnum, int destarea, vec3_t returnPos) – breadth‑first search for a hiding spot, using the route table.
- AAS_RT_GetReachabilityIndex(int areanum, int reachIndex) – currently returns reachIndex; intended to apply an offset of firstreachablearea (TODO).

## Important Control Flow
- When a map loads, AAS_RT_BuildRouteTable() is invoked; it first tries to read a pre‑generated .rtb file.
- If the file is missing or incompatible, the function builds a route table from scratch: it filters routable areas, computes visibility lists, selects parent–child associations to minimize travel times, allocates runtime structures, fills aas_rt_t, and finally writes the table via AAS_RT_WriteRouteTable().
- Runtime queries such as AAS_RT_GetRoute() call the low‑level AAS_AreaRouteToGoalArea() over relevant parent/child links and return a cached route structure.
- AAS_RT_ShutdownRouteTable() deallocates all runtime tables when the bot world is torn down.

## External Dependencies
- "q_shared.h" – common engine definitions.
- "l_memory.h", "l_script.h", "l_precomp.h", "l_struct.h", "l_libvar.h", "l_utils.h" – memory, script, and utility support.
- "aasfile.h" – AAS area definitions.
- "botshared/botlib.h", "botshared/be_aas.h", "be_interface.h", "be_aas_def.h" – Bot library and AAS interfaces.
- "botshared/be_ai_goal.h" – goal types for bot navigation.
- Standard C library (malloc, free, memcpy, memset, etc.).

## How It Fits
- The file supplies the core runtime representation (aas_rt_t) that the bot library uses to plan movements, avoiding complex pathfinding each frame.
- It sits between the AAS world data and the decision‑making code: after the AAS world is loaded, this module creates a lightweight cache that bots query for navigation.
- It abstracts away the heavy pre‑computation of visibility and travel times, allowing other modules to remain agnostic of the underlying graph structure.
