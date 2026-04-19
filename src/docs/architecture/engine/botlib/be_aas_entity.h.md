<!-- doxygenix: sha256=7aa1f1723d377347713537ce9e75149541fd1567ce4b00385a91d76aae031b9b -->

# engine/botlib/be_aas_entity.h

## File Purpose
- Declares the public API for querying and modifying entity data in the AI spatial analysis system (AAS) of the Return to Castle Wolfenstein engine.

## Core Responsibilities
- Exposes a set of API functions for reading and updating entity information within the bot AI path‑finding world (AAS).
- Provides internal utilities (AASINTERN) for world maintenance and entity data extraction used by the AAS subsystem.
- Validates entity indices and world state, printing fatal errors via botimport.Print when misused.

## Key Types and Functions
- AAS_InvalidateEntities — clears the validity flag on every entity in the AAS world.
- AAS_ResetEntityLinks — resets area/leaf pointers for all entities to NULL.
- AAS_UpdateEntity(int, bot_entitystate_t*) — updates an entity’s state and collision data in the world.
- AAS_EntityBSPData(int, bsp_entdata_t*) — writes BSP model and collision info for a given entity into the provided structure.
- AAS_EntitySize(int, vec3_t, vec3_t) — copies an entity’s stored minimum and maximum coordinates into out‑parameters.
- AAS_EntityModelNum(int) — returns the model index stored in the entity’s record.
- AAS_OriginOfEntityWithModelNum(int, vec3_t) — finds the world origin of the first ET_MOVER entity whose model matches the argument and writes it to an output vector.
- AAS_BestReachableEntityArea(int) — queries the best reachable AAS area for a specified entity.
- AAS_EntityInfo(int, aas_entityinfo_t*) — copies the full internal data record for the entity into the supplied buffer.
- AAS_NextEntity(int) — returns the next valid entity index after the given one, or 0 if none.
- AAS_EntityOrigin(int, vec3_t) — outputs the world origin of the given entity.
- AAS_EntityType(int) — returns the entity type enumeration value.
- AAS_EntityModelindex(int) — returns the model index for the entity (alias of EntityModelNum).
- AAS_IsEntityInArea(int, int, int) — supposed to determine if a non‑ignored entity occupies an area; currently a stub that always returns false.

## Important Control Flow
- AAS world initialization creates an array of entity structures and sets default values.
- AAS_InvalidateEntities clears the valid flag on all entities, marking them uninitialized.
- AAS_ResetEntityLinks sets all area and leaf pointers for each entity to NULL, disconnecting them from the spatial graph.
- AAS_UpdateEntity copies the supplied bot_entitystate_t data into the entity record, updates collision data, and recalculates linking information.
- AAS_EntityBSPData fills a bsp_entdata_t with the entity’s model reference and collision information for use in physics queries.
- Lookup functions (e.g., AAS_EntitySize, AAS_EntityModelNum, AAS_EntityOrigin, etc.) perform bounds checks on the entity index, read pre‑validated arrays or run linear scans, and return the requested data or zero/defaults on error.

## External Dependencies
- Requires the definitions of bot_entitystate_t, bsp_entdata_t, aas_entityinfo_t, vec3_t from the botlib headers such as "botlib.h" and "botdefs.h".
- Relies on the global AAS world data structure (defaultaasworld) defined in "be_aas.h" or equivalent.
- Uses the botimport interface for error reporting, declared in "botlib_imports.h".
- Requires standard math/vector types defined in "q_shared.h" or "q_math.h".

## How It Fits
- It is part of the bot navigation stack; the functions allow bots to query world entities for collision, location, and model data needed for path planning, obstacle avoidance, and behavior logic.
- Internal functions aid in maintaining the AAS entity list during map loading and runtime updates.
