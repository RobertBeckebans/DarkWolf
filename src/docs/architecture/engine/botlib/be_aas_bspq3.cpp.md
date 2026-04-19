<!-- doxygenix: sha256=1d12e40b8288fdf7d2f5f3c8fc34e01d0b05c34318867d09bbad32cf522c0289 -->

# engine/botlib/be_aas_bspq3.cpp

## File Purpose
- This file implements the BSP‑entity data layer of the AAS system in DarkWolf, parsing the entity block of a BSP file, storing key/value pairs in a compact buffer, and providing utility functions for querying entity attributes and environmental contents.

## Core Responsibilities
- Provide a lightweight representation of the BSP world for the AAS/AI subsystem.
- Expose entity and world query APIs such as content lookup, visibility, bounding‑box tests, and key/value retrieval.
- Load, parse, and clean up BSP entity data without accessing geometry directly.
- Offer stubbed BSP‑leaf linkage hooks for future expansion.

## Key Types and Functions
- rgb_t – simple struct holding red, green, and blue color components.
- bsp_epair_t – node of a key/value pair list for a BSP entity.
- bsp_entity_t – container holding the head of an epair list for a single entity.
- bsp_t – global state of the loaded BSP world: load flag, entity data pointers, entity list, and string buffer.
- bspworld – the sole instance of bsp_t, representing the current world.
- AAS_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask) – forwards parameters to botimport.Trace and returns the resulting bsp_trace_t.
- AAS_PointContents(vec3_t point) – calls botimport.PointContents and returns the content bitmask.
- AAS_EntityCollision(int entnum, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int contentmask, bsp_trace_t* trace) – performs an entity trace via botimport.EntityTrace, updating trace if the hit occurs earlier.
- AAS_inPVS(vec3_t p1, vec3_t p2) – returns the result of botimport.inPVS.
- AAS_inPHS(vec3_t p1, vec3_t p2) – stub that always returns qtrue.
- AAS_BSPModelMinsMaxsOrigin(int modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin) – forwards to botimport.BSPModelMinsMaxsOrigin.
- AAS_UnlinkFromBSPLeaves(bsp_link_t* leaves) – empty stub for future leaf unlink logic.
- AAS_BSPLinkEntity(vec3_t absmins, vec3_t absmaxs, int entnum, int modelnum) – currently returns NULL.
- AAS_BoxEntities(vec3_t absmins, vec3_t absmaxs, int* list, int maxcount) – currently returns 0.
- AAS_NextBSPEntity(int ent) – returns the next valid entity index or 0 if none.
- AAS_BSPEntityInRange(int ent) – validates an entity index against bspworld.numentities.
- AAS_ValueForBSPEpairKey(int ent, char* key, char* value, int size) – searches an entity’s epair list for key and copies the value into value.
- AAS_VectorForBSPEpairKey(int ent, char* key, vec3_t v) – retrieves a key’s string value and parses up to three floats into v.
- AAS_FloatForBSPEpairKey(int ent, char* key, float* value) – fetches a key’s string and converts it to a float.
- AAS_IntForBSPEpairKey(int ent, char* key, int* value) – fetches a key’s string and converts it to an int.
- AAS_FreeBSPEntities() – releases the global ebuffer and resets bspworld.numentities.
- AAS_ParseBSPEntities() – reads the entity block as a script, allocates a contiguous buffer, and populates bspworld.entities.
- AAS_BSPTraceLight(vec3_t start, vec3_t end, vec3_t endpos, int* red, int* green, int* blue) – stub that returns 0.
- AAS_DumpBSPData() – frees entity data, clears dentdata, and resets bspworld.
- AAS_LoadBSPFile() – orchestrates the load sequence: dump old data, copy entity block from botimport, parse it, mark loaded, and return BLERR_NOERROR.

## Important Control Flow
- AAS_LoadBSPFile clears any existing world data, obtains the entity block from botimport.BSPEntityData(), copies it into a cleared hunk buffer, parses the entities with AAS_ParseBSPEntities(), marks the world as loaded, and returns BLERR_NOERROR.
- AAS_ParseBSPEntities first scans all entities in the entity block to compute a single memory requirement, allocates one contiguous buffer, reloads the script, and walks each entity building a linked list of bsp_epair_t nodes for each key/value pair.
- Query functions such as AAS_ValueForBSPEpairKey, AAS_VectorForBSPEpairKey, AAS_FloatForBSPEpairKey, and AAS_IntForBSPEpairKey retrieve stored strings and convert them to the requested type.
- Tracing and visibility helpers (AAS_Trace, AAS_PointContents, AAS_EntityCollision, AAS_inPVS, AAS_inPHS) delegate to the botimport API, forwarding all input and returning the obtained results.
- Utility functions (AAS_BSPModelMinsMaxsOrigin, AAS_UnlinkFromBSPLeaves, AAS_BSPLinkEntity, AAS_BoxEntities, AAS_NextBSPEntity, AAS_BSPEntityInRange) provide either stubs or simple entity‑management logic that operates on the global bspworld.
- AAS_FreeBSPEntities releases the buffer holding all epair strings and resets the entity count; it is called from AAS_DumpBSPData and from AAS_ParseBSPEntities on error.
- AAS_DumpBSPData clears the entire bspworld structure, frees entity data, and marks the world as unloaded.

## External Dependencies
- "q_shared.h" – basic types, constants (e.g. vec3_t, PRT_MESSAGE).
- "l_memory.h" – memory allocation helpers (GetClearedHunkMemory, FreeMemory).
- "l_script.h" – script parsing utilities (LoadScriptMemory, PS_ReadToken).
- "l_precomp.h" – pre‑compiled header with common includes.
- "l_struct.h" – generic structure definitions.
- "aasfile.h" – AAS file format definitions.
- "botshared/botlib.h" – import function table (botimport).
- "botshared/be_aas.h", "be_aas_funcs.h", "be_aas_def.h" – AAS API declarations.

## How It Fits
- Part of the botlib module that supplies the AAS system with world information.
- Uses the engine’s botimport infrastructure to perform heavy‑lifting tasks such as collisions, visibility, and model bounds.
- Operates on a small in‑memory snapshot of entity data, enabling fast look‑ups required by AAS navigation, pathfinding, and perception logic.
- Designed for single‑player mode; geometry handling is abstracted away, keeping this module focused on entity metadata and simple spatial queries.
