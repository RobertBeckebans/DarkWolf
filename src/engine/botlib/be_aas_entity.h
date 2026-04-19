/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU
General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*!
	\file engine/botlib/be_aas_entity.h
	\brief Declares the public API for querying and modifying entity data in the AI spatial analysis system (AAS) of the Return to Castle Wolfenstein engine.
	\note doxygenix: sha256=7aa1f1723d377347713537ce9e75149541fd1567ce4b00385a91d76aae031b9b

	\par File Purpose
	- Declares the public API for querying and modifying entity data in the AI spatial analysis system (AAS) of the Return to Castle Wolfenstein engine.

	\par Core Responsibilities
	- Exposes a set of API functions for reading and updating entity information within the bot AI path‑finding world (AAS).
	- Provides internal utilities (AASINTERN) for world maintenance and entity data extraction used by the AAS subsystem.
	- Validates entity indices and world state, printing fatal errors via botimport.Print when misused.

	\par Key Types and Functions
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

	\par Control Flow
	- AAS world initialization creates an array of entity structures and sets default values.
	- AAS_InvalidateEntities clears the valid flag on all entities, marking them uninitialized.
	- AAS_ResetEntityLinks sets all area and leaf pointers for each entity to NULL, disconnecting them from the spatial graph.
	- AAS_UpdateEntity copies the supplied bot_entitystate_t data into the entity record, updates collision data, and recalculates linking information.
	- AAS_EntityBSPData fills a bsp_entdata_t with the entity’s model reference and collision information for use in physics queries.
	- Lookup functions (e.g., AAS_EntitySize, AAS_EntityModelNum, AAS_EntityOrigin, etc.) perform bounds checks on the entity index, read pre‑validated arrays or run linear scans, and return the requested data or zero/defaults on error.

	\par Dependencies
	- Requires the definitions of bot_entitystate_t, bsp_entdata_t, aas_entityinfo_t, vec3_t from the botlib headers such as "botlib.h" and "botdefs.h".
	- Relies on the global AAS world data structure (defaultaasworld) defined in "be_aas.h" or equivalent.
	- Uses the botimport interface for error reporting, declared in "botlib_imports.h".
	- Requires standard math/vector types defined in "q_shared.h" or "q_math.h".

	\par How It Fits
	- It is part of the bot navigation stack; the functions allow bots to query world entities for collision, location, and model data needed for path planning, obstacle avoidance, and behavior logic.
	- Internal functions aid in maintaining the AAS entity list during map loading and runtime updates.
*/

/*****************************************************************************
 * name:		be_aas_entity.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#ifdef AASINTERN
// invalidates all entity infos
void AAS_InvalidateEntities();
// resets the entity AAS and BSP links (sets areas and leaves pointers to NULL)
void AAS_ResetEntityLinks();
// updates an entity
int	 AAS_UpdateEntity( int ent, bot_entitystate_t* state );
// gives the entity data used for collision detection
void AAS_EntityBSPData( int entnum, bsp_entdata_t* entdata );
#endif // AASINTERN

/*!
	\brief Retrieves an entity's axis-aligned bounding box size into the supplied vectors.

	If the AAS world has not been initialized, the function exits quietly. It then validates that the given entity number is within the range of known entities, reporting a fatal error via
   botimport.Print if it is not. Upon success, it copies the entity's stored minimum and maximum coordinates into the provided arrays.

	\param entnum index of the entity within the AAS world
	\param maxs vector to receive the entity's maximum coordinate limits
	\param mins vector to receive the entity's minimum coordinate limits
*/
void AAS_EntitySize( int entnum, vec3_t mins, vec3_t maxs );

/*!
	\brief Retrieves the model index of a specified entity.

	The function checks that the global AI spatial world has been initialized and that the given entity number is within bounds. If either condition fails it logs a fatal error for an out‑of‑range
   entity and returns 0. Otherwise it returns the model index stored in the entity’s data structure.

	\param entnum The index of the entity whose model number is requested.
	\return The BSP model number (model index) for the entity, or 0 if the world is not initialized or the entity number is invalid.
*/
int	 AAS_EntityModelNum( int entnum );

/*!
	\brief Returns the world origin of the first mover entity that uses the specified BSP model number, if any.

	The function scans all entities in the global AI world for those of type ET_MOVER whose model index matches the supplied modelnum. When a match is found, the entity’s origin is copied into the
   provided vec3_t and a positive result is returned. If no matching entity exists, the function leaves origin unchanged and returns a negative value.

	The search is performed by iterating over the array of entities stored in *defaultaasworld, making this function quick for small worlds but linear in the number of entities.

	\param modelnum The BSP model number to search for.
	\param origin Output vector that receives the origin of the found mover entity.
	\return qtrue (1) if a matching entity is found; otherwise qfalse (0).
*/
int	 AAS_OriginOfEntityWithModelNum( int modelnum, vec3_t origin );

/*!
	\brief Determine the best reachable AAS area for a specified entity

	The function accepts an entity index, retrieves the corresponding entity structure from the global AAS world, and then calls AAS_BestReachableLinkArea on the entity’s link list. The result is the
   index of the AAS area that is considered best reachable from the entity’s current position.

	\param entnum The index of the entity within the AAS world’s entity array.
	\return The index of the area deemed best reachable for the entity
*/
int	 AAS_BestReachableEntityArea( int entnum );

/*!
	\brief Copies the stored information for a specified entity into the supplied structure

	If the AAS world is not initialized or the entity number is out of range, the function logs a fatal error and zeroes the output structure before returning. Otherwise, it copies the entity's
   internal information record from the AAS world into the caller-provided buffer.

	\param entnum identifier of the entity within the AAS world
	\param info pointer to a structure that will receive the entity's information
*/
void AAS_EntityInfo( int entnum, aas_entityinfo_t* info );

/*!
	\brief Finds the next valid entity index after a given index.

	The function first checks that a default AAS world has been loaded; if not it returns 0 to indicate no result.  It then normalises a supplied entity number: if the argument is negative, it is set
   to -1 so that the following pre‑increment will start the search at index 0.  Starting from that point it iterates through the world’s entity array until it finds an entity whose validity flag is
   set and returns that index.  If no further valid entity exists before the maximum entity count, the function returns 0 to signal the end of the list.

	\param entnum The index of the entity that was most recently processed; the function will search for the next entity with a valid flag after this index.  A negative value causes the search to
   begin at the first entity. \return The index of the next valid entity, or 0 if there are none left or the world has not been loaded.
*/
int	 AAS_NextEntity( int entnum );

/*!
	\brief Retrieves the world model origin of the specified entity and stores it in the provided vector.

	If the entity number is outside the valid range, a fatal message is printed and the output vector is cleared. Otherwise, the origin vector is filled with the entity's stored origin from the world
   data structure.

	\param entnum The numeric identifier of the entity to query.
	\param origin The vector that will receive the entity's origin. It is treated as an output parameter.
*/
void AAS_EntityOrigin( int entnum, vec3_t origin );

/*!
	\brief Returns the type of an entity given its index.

	Retrieves the entity type from the AAS world data after confirming that the AAS world has been initialized and the supplied entity number is within the valid range. If the world is not initialized
   or the entity number is invalid, a fatal error is logged and zero is returned.

	\param entnum Index of the entity whose type is requested.
	\return An integer representing the entity type; zero indicates an error or undefined type.
*/
int	 AAS_EntityType( int entnum );

/*!
	\brief Returns the model index for a specified entity number

	The function first validates that the supplied entity number lies within the bounds of the current AAS world. If the entity number is out of range, a fatal message is printed and zero is returned.
   Otherwise, the model index stored in the entity’s data structure is retrieved and returned.

	\param entnum the index of the entity whose model index is being requested
	\return The model index integer for the entity, or 0 if the entity number is invalid or out of range
*/
int	 AAS_EntityModelindex( int entnum );

/*!
	\brief Checks whether a solid entity other than specified ones occupies a particular area, but currently always returns false.

	The function was intended to iterate over entities linked to the specified area, excluding up to two ignored entities, and return true if any other solid entity was present. However, the
   implementation contains a provisional early return of qfalse and the actual iteration code is commented out or unreachable, rendering the function non-functional for multi‑area handling.

	This stub is a placeholder and does not perform area occupancy tests.

	It is often called to avoid paths that might be occupied by other entities.


	\param entnumIgnore Entity number to ignore in the check
	\param entnumIgnore2 Second optional entity number to ignore
	\param areanum Area number to search for occupancy
	\return 0 for false, 1 for true (currently always returns 0).
*/
int	 AAS_IsEntityInArea( int entnumIgnore, int entnumIgnore2, int areanum );
