<!-- doxygenix: sha256=25f864f27e028d7f947a234b79926ebf2e61e45f70d5fbe4e231766fbdfca9e1 -->

# engine/botlib/be_aas_bsp.h

## File Purpose
- Declares the public API for AAS-related operations used by the bot subsystem, including collision tracing, content querying, visibility testing, area connectivity, entity iteration, and BSP‑model utilities.
- Provides internal helper prototypes (guarded by AASINTERN) for loading and manipulating the underlying BSP structure used by bots.

## Core Responsibilities
- Provide an AAS (Area Awareness System) interface for bot AI to query world geometry, entity properties, and spatial relationships.
- Wrap the underlying bot import subsystem so that bots can perform collision traces, find surface contents, and assess visibility.
- Expose entity management helpers: linking entities to BSP leaves, iterating over entities, and retrieving key/value pairs from epair maps.
- Offer geometry utilities for BSP models: computing transformed bounds and origins based on orientation.
- Support debugging and introspection of BSP data for development purposes.

## Key Types and Functions
- AAS_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask) — forwards a bounding‑box trace request to the bot import subsystem and returns a bsp_trace_t containing hit information.
- AAS_PointContents(vec3_t point) — returns the world content bitmask at the specified point via botimport.
- AAS_inPVS(vec3_t p1, vec3_t p2) — asks botimport.inPVS whether p2 lies in p1’s potential visibility set.
- AAS_inPHS(vec3_t p1, vec3_t p2) — stub that always returns qtrue, intended to be replaced by a real hidden‑set test.
- AAS_AreasConnected(int area1, int area2) — reports whether two area indices are connected in the navigation graph.
- AAS_BoxEntities(vec3_t absmins, vec3_t absmaxs, int* list, int maxcount) — scans all entities, writes those overlapping the box into list, and returns the count.
- AAS_BSPModelMinsMaxsOrigin(int modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin) — computes transformed bounds and origin of a BSP model given its Euler orientation.
- AAS_NextBSPEntity(int ent) — returns the next valid entity index after ent, or 0 if no further entities exist.
- AAS_ValueForBSPEpairKey(int ent, char* key, char* value, int size) — copies the value string of the specified key from an entity’s epair list into value.
- AAS_VectorForBSPEpairKey(int ent, char* key, vec3_t v) — retrieves a string value and parses up to three floats into a vec3_t.
- AAS_FloatForBSPEpairKey(int ent, char* key, float* value) — fetches a key’s string value, converts it to a float, and stores it in value.
- AAS_IntForBSPEpairKey(int ent, char* key, int* value) — fetches a key’s string value, converts it to an integer, and stores it in value.
- AAS_LoadBSPFile() (internal) — loads a BSP map file and populates bspworld structures.
- AAS_DumpBSPData() (internal) — outputs debugging information about the loaded BSP hierarchy.
- AAS_UnlinkFromBSPLeaves(bsp_link_t* leaves) (internal) — removes an entity’s link from the leaf list of the BSP tree.
- AAS_BSPLinkEntity(vec3_t absmins, vec3_t absmaxs, int entnum, int modelnum) (internal) — creates and returns a bsp_link_t that attaches an entity to the appropriate leaves in the BSP tree.
- AAS_EntityCollision(int entnum, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int contentmask, bsp_trace_t* trace) (internal) — performs a collision test for a particular entity against a capsule trace.
- AAS_PrintFreeBSPLinks(char* str) (internal) — writes a textual dump of free link list nodes into the provided buffer for debugging purposes.

## Important Control Flow
- AAS_Trace receives a bounding‑box trace request and delegates the actual collision test to the bot import subsystem, then returns a bsp_trace_t containing the resulting hit data.
- AAS_PointContents queries the world contents at a given point via the bot import interface and returns the content flag bitmask.
- AAS_inPVS passes two points to botimport.inPVS to determine whether they are in each other’s potential visibility set.
- AAS_inPHS returns true unconditionally, acting as a stub until replaced by a full hidden‑set test.
- AAS_AreasConnected accesses shared area‑connectivity data to report whether area1 can reach area2.
- AAS_BoxEntities scans all entities in bspworld, tests each bounding box against the supplied absmins/absmaxs, writes matching entity numbers into the supplied list, and returns the count.
- AAS_BSPModelMinsMaxsOrigin transforms a BSP model’s local extents by the provided Euler angles to compute world‑space mins, maxs, and origin.
- AAS_NextBSPEntity increments an entity index, checks against bspworld.numentities, and returns the next valid entity number or 0 if none remain.
- AAS_ValueForBSPEpairKey locates a key in an entity’s epair list, copies its value into a user‑supplied buffer, and indicates success.
- AAS_VectorForBSPEpairKey retrieves the string value via AAS_ValueForBSPEpairKey and parses up to three floats into a vec3_t.
- AAS_FloatForBSPEpairKey obtains a key’s string value and converts it to a float using atof.
- AAS_IntForBSPEpairKey obtains a key’s string value and converts it to an integer using atoi.
- Internal functions under AASINTERN (e.g., AAS_LoadBSPFile, AAS_DumpBSPData) modify the global BSP world structure: loading map data, dumping debugging information, linking/unlinking entities, performing per‑entity collision tests, and outputting link list state.

## External Dependencies
- Requires definitions of vec3_t, bsp_link_t, bsp_trace_t, qboolean, and other core bot structures from the bot library.
- Calls into the botimport interface (botimport.Trace, botimport.inPVS, etc.) defined elsewhere in the engine.
- Accesses global bspworld, bspeworld, and related data structures that represent the loaded map’s BSP hierarchy.
- Uses standard C string functions and types such as char, int, float, sscanf, atof, and atoi.
- Relies on the MAX_EPAIRKEY macro for epair key buffer sizing.

## How It Fits
- Serves as the contract between the bot AI code and the engine’s world representation, hiding engine-specific details behind a high‑level interface.
- Allows bot modules to remain portable across different map formats by delegating to the same set of functions, regardless of the underlying BSP loader implementation.
- Functions as the entry point for bot decision logic that requires spatial awareness: navigation, visibility checks, and interaction with world objects.
- In the larger engine, this header is included by bot modules and the AAS implementation source file, which in turn links against the global BSP world data managed by the level loader.
