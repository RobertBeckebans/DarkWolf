<!-- doxygenix: sha256=2523deddf88bb51d666812635376a4d0386692c60292bfdd69c2ec9b62ffc99c -->

# engine/botlib/be_aas_entity.cpp

## File Purpose
- Implement the entity management layer of the AAS navigation system used by the bot library, providing CRUD and query operations for entities within the AI spatial world.

## Core Responsibilities
- Maintain a per-entity state table in the default AAS world.
- Translate bot entity state updates into the navigation system’s internal representation.
- Handle spatial linking and unlinking of entities to AAS areas and BSP leaves when their geometry or position changes.
- Provide read‑only accessors for entity properties such as origin, size, model index, type, and bounding boxes.
- Support entity search operations for model index matching or nearest entity queries.
- Adjust routing constraints (blocking) for rectangular volumes across all loaded worlds.
- Ensure consistency of area routing cache when enabling or disabling routing on individual areas.

## Key Types and Functions
- aas_t – struct representing an AAS world, containing arrays of aas_entity_t and link lists.
- aas_entity_t – struct containing an aas_entityinfo_t and pointers to area and leaf links.
- aas_entityinfo_t – struct holding per‑entity fields such as type, flags, origin, mins/maxs, modelindex, animation frames, etc.
- AAS_UpdateEntity(int entnum, bot_entitystate_t* state) — updates the internal entity record and relinks it in the world if necessary.
- AAS_EntityInfo(int entnum, aas_entityinfo_t* info) — copies internal entity info to caller buffer.
- AAS_EntityOrigin(int entnum, vec3_t origin) — returns entity origin.
- AAS_EntityModelindex(int entnum) — returns entity’s model index.
- AAS_EntityModelNum(int entnum) — synonym for model index accessor.
- AAS_EntityType(int entnum) — returns entity type enum value.
- AAS_OriginOfEntityWithModelNum(int modelnum, vec3_t origin) — finds first mover entity with given model number, returns qtrue if found.
- AAS_EntitySize(int entnum, vec3_t mins, vec3_t maxs) — returns bounding box dimensions.
- AAS_EntityBSPData(int entnum, bsp_entdata_t* entdata) — populates BSP entity data structure for world‑space geometry.
- AAS_ResetEntityLinks() — clears area and leaf pointers for all entities in default world.
- AAS_InvalidateEntities() — clears valid flag and resets entity numbers for all entities.
- AAS_NearestEntity(vec3_t origin, int modelindex) — returns closest entity with matching model index within 40 units in X/Y.
- AAS_BestReachableEntityArea(int entnum) — returns index of best reachable AAS area for the entity via AAS_BestReachableLinkArea.
- AAS_NextEntity(int entnum) — iterates to find the next entity with valid flag set.
- AAS_IsEntityInArea(int entnumIgnore, int entnumIgnore2, int areanum) — stub that always returns qfalse, intended to test area occupancy.
- AAS_SetAASBlockingEntity(vec3_t absmin, vec3_t absmax, qboolean blocking) — toggles routing state for all areas intersecting a box, optionally resetting all worlds when min==max and blocking<0.

## Important Control Flow
- At startup, the module initializes with a global AAS world pointer to the first world in the array, ensuring that all subsequent calls operate on the same spatial data structure.
- AAS_UpdateEntity first confirms that the default world is loaded, then copies entity state from a bot_entitystate_t into the world’s aas_entity_t, updating timestamps, flags, pose, and model indices.
- The function evaluates whether position, angle, collision volume or model data changed; if so, a relink flag is set.
- When relinking, the entity is removed from any prior area and BSP leaf associations, then its absolute bounding box is computed, and it is reinserted into the AAS area list via AAS_LinkEntityClientBBox and into the BSP tree via AAS_BSPLinkEntity.
- Query functions like AAS_EntityInfo, AAS_EntityOrigin, AAS_EntityModelindex read back data from the same aas_entity_t structure, often after validity checks against maxentities and initialized status.
- Utility functions such as AAS_EntitySize, AAS_EntityBSPData, and AAS_EntityModelNum expose specific fields from the internal entity record to callers.
- More complex helpers such as AAS_OriginOfEntityWithModelNum or AAS_NearestEntity iterate the world’s entity array to find matches based on type or model index, applying simple spatial heuristics.
- Functions that modify area routing (AAS_SetAASBlockingEntity) iterate over all loaded AAS worlds, using AAS_BBoxAreas to find affected areas and toggling the routing flag via AAS_EnableRoutingArea.
- The entity list can be cleared or reinitialized by AAS_ResetEntityLinks and AAS_InvalidateEntities, resetting area/leaf pointers or validity flags before a rebuild.

## External Dependencies
- "q_shared.h"
- "l_memory.h"
- "l_script.h"
- "l_precomp.h"
- "l_struct.h"
- "l_utils.h"
- "l_log.h"
- "aasfile.h"
- "botshared/botlib.h"
- "botshared/be_aas.h"
- "be_aas_funcs.h"
- "be_interface.h"
- "be_aas_def.h"

## How It Fits
- Resides in the bot shared library; exposes the AAS entity API to the rest of the engine.
- Depends on the global AAS world data structure managed elsewhere; interacts with area and BSP linking utilities defined in aasfile.c and related files.
