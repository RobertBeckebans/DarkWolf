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

/*****************************************************************************
 * name:		be_aas_entity.c
 *
 * desc:		AAS entities
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_utils.h"
#include "l_log.h"
#include "aasfile.h"
#include "botshared/botlib.h"
#include "botshared/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"

#define MASK_SOLID CONTENTS_PLAYERCLIP

// Ridah, always use the default world for entities
extern aas_t aasworlds[MAX_AAS_WORLDS];

aas_t*		 defaultaasworld = aasworlds;

// FIXME: these might change
enum { ET_GENERAL, ET_PLAYER, ET_ITEM, ET_MISSILE, ET_MOVER };

/*!
	\brief Updates entity information in the AI awareness system using the supplied state.

	The function first ensures the default AAS world is loaded; if not, it prints a message and returns BLERR_NOAASFILE. It then updates the internal entity structure with data from the state
   parameter, recording timestamps and copying origins, flags, solid type, model indices and animations.  It flags the entity as valid and, depending on the number of AAS frames, determines whether a
   relink is necessary.  For solid BSP models it recomputes mins/maxs with the current angles; for solid BBOX models it checks if the bounding box dimensions changed.  If the origin changes or if any
   of the above properties changed, it sets the relink flag.  When relinking, the entity is unlinked from its previous areas and BSP leaves, then relinked using the largest bounding box, and finally
   reinserted into the world’s BSP tree. It returns BLERR_NOERROR on successful update.

	\param entnum the identifier of the entity to be updated
	\param state a pointer to a bot_entitystate_t structure containing the new entity state
	\return BLERR_NOERROR on success or BLERR_NOAASFILE if the AAS world is not loaded.
*/
int AAS_UpdateEntity( int entnum, bot_entitystate_t* state )
{
	int			  relink;
	aas_entity_t* ent;
	vec3_t		  absmins, absmaxs;

	if( !( *defaultaasworld ).loaded ) {
		botimport.Print( PRT_MESSAGE, "AAS_UpdateEntity: not loaded\n" );
		return BLERR_NOAASFILE;
	}

	ent = &( *defaultaasworld ).entities[entnum];

	ent->i.update_time = AAS_Time() - ent->i.ltime;
	ent->i.type		   = state->type;
	ent->i.flags	   = state->flags;
	ent->i.ltime	   = AAS_Time();
	VectorCopy( ent->i.origin, ent->i.lastvisorigin );
	VectorCopy( state->old_origin, ent->i.old_origin );
	ent->i.solid	   = state->solid;
	ent->i.groundent   = state->groundent;
	ent->i.modelindex  = state->modelindex;
	ent->i.modelindex2 = state->modelindex2;
	ent->i.frame	   = state->frame;
	// ent->i.event = state->event;
	ent->i.eventParm = state->eventParm;
	ent->i.powerups	 = state->powerups;
	ent->i.weapon	 = state->weapon;
	ent->i.legsAnim	 = state->legsAnim;
	ent->i.torsoAnim = state->torsoAnim;

	//	ent->i.weapAnim = state->weapAnim;	//----(SA)
	//----(SA)	didn't want to comment in as I wasn't sure of any implications of changing the aas_entityinfo_t and bot_entitystate_t structures.

	// number of the entity
	ent->i.number = entnum;
	// updated so set valid flag
	ent->i.valid = qtrue;
	// link everything the first frame

	if( ( *defaultaasworld ).numframes == 1 ) {
		relink = qtrue;

	} else {
		relink = qfalse;
	}

	//
	if( ent->i.solid == SOLID_BSP ) {
		// if the angles of the model changed
		if( !VectorCompare( state->angles, ent->i.angles ) ) {
			VectorCopy( state->angles, ent->i.angles );
			relink = qtrue;
		}

		// get the mins and maxs of the model
		// FIXME: rotate mins and maxs
		AAS_BSPModelMinsMaxsOrigin( ent->i.modelindex, ent->i.angles, ent->i.mins, ent->i.maxs, NULL );

	} else if( ent->i.solid == SOLID_BBOX ) {
		// if the bounding box size changed
		if( !VectorCompare( state->mins, ent->i.mins ) || !VectorCompare( state->maxs, ent->i.maxs ) ) {
			VectorCopy( state->mins, ent->i.mins );
			VectorCopy( state->maxs, ent->i.maxs );
			relink = qtrue;
		}
	}

	// if the origin changed
	if( !VectorCompare( state->origin, ent->i.origin ) ) {
		VectorCopy( state->origin, ent->i.origin );
		relink = qtrue;
	}

	// if the entity should be relinked
	if( relink ) {
		// don't link the world model
		if( entnum != ENTITYNUM_WORLD ) {
			// absolute mins and maxs
			VectorAdd( ent->i.mins, ent->i.origin, absmins );
			VectorAdd( ent->i.maxs, ent->i.origin, absmaxs );

			// unlink the entity
			AAS_UnlinkFromAreas( ent->areas );
			// relink the entity to the AAS areas (use the larges bbox)
			ent->areas = AAS_LinkEntityClientBBox( absmins, absmaxs, entnum, PRESENCE_NORMAL );
			// unlink the entity from the BSP leaves
			AAS_UnlinkFromBSPLeaves( ent->leaves );
			// link the entity to the world BSP tree
			ent->leaves = AAS_BSPLinkEntity( absmins, absmaxs, entnum, 0 );
		}
	}

	return BLERR_NOERROR;
}

void AAS_EntityInfo( int entnum, aas_entityinfo_t* info )
{
	if( !( *defaultaasworld ).initialized ) {
		botimport.Print( PRT_FATAL, "AAS_EntityInfo: (*defaultaasworld) not initialized\n" );
		memset( info, 0, sizeof( aas_entityinfo_t ) );
		return;
	}

	if( entnum < 0 || entnum >= ( *defaultaasworld ).maxentities ) {
		botimport.Print( PRT_FATAL, "AAS_EntityInfo: entnum %d out of range\n", entnum );
		memset( info, 0, sizeof( aas_entityinfo_t ) );
		return;
	}

	memcpy( info, &( *defaultaasworld ).entities[entnum].i, sizeof( aas_entityinfo_t ) );
}

void AAS_EntityOrigin( int entnum, vec3_t origin )
{
	if( entnum < 0 || entnum >= ( *defaultaasworld ).maxentities ) {
		botimport.Print( PRT_FATAL, "AAS_EntityOrigin: entnum %d out of range\n", entnum );
		VectorClear( origin );
		return;
	}

	VectorCopy( ( *defaultaasworld ).entities[entnum].i.origin, origin );
}

int AAS_EntityModelindex( int entnum )
{
	if( entnum < 0 || entnum >= ( *defaultaasworld ).maxentities ) {
		botimport.Print( PRT_FATAL, "AAS_EntityModelindex: entnum %d out of range\n", entnum );
		return 0;
	}

	return ( *defaultaasworld ).entities[entnum].i.modelindex;
}

int AAS_EntityType( int entnum )
{
	if( !( *defaultaasworld ).initialized ) {
		return 0;
	}

	if( entnum < 0 || entnum >= ( *defaultaasworld ).maxentities ) {
		botimport.Print( PRT_FATAL, "AAS_EntityType: entnum %d out of range\n", entnum );
		return 0;
	}

	return ( *defaultaasworld ).entities[entnum].i.type;
} // end of the AAS_EntityType

int AAS_EntityModelNum( int entnum )
{
	if( !( *defaultaasworld ).initialized ) {
		return 0;
	}

	if( entnum < 0 || entnum >= ( *defaultaasworld ).maxentities ) {
		botimport.Print( PRT_FATAL, "AAS_EntityModelNum: entnum %d out of range\n", entnum );
		return 0;
	}

	return ( *defaultaasworld ).entities[entnum].i.modelindex;
}

int AAS_OriginOfEntityWithModelNum( int modelnum, vec3_t origin )
{
	int			  i;
	aas_entity_t* ent;

	for( i = 0; i < ( *defaultaasworld ).maxentities; i++ ) {
		ent = &( *defaultaasworld ).entities[i];

		if( ent->i.type == ET_MOVER ) {
			if( ent->i.modelindex == modelnum ) {
				VectorCopy( ent->i.origin, origin );
				return qtrue;
			}
		}
	}

	return qfalse;
}

void AAS_EntitySize( int entnum, vec3_t mins, vec3_t maxs )
{
	aas_entity_t* ent;

	if( !( *defaultaasworld ).initialized ) {
		return;
	}

	if( entnum < 0 || entnum >= ( *defaultaasworld ).maxentities ) {
		botimport.Print( PRT_FATAL, "AAS_EntitySize: entnum %d out of range\n", entnum );
		return;
	}

	ent = &( *defaultaasworld ).entities[entnum];
	VectorCopy( ent->i.mins, mins );
	VectorCopy( ent->i.maxs, maxs );
}

/*!
	\brief Populates a bsp_entdata_t structure with the specified entity's data.

	The function looks up the entity in the global AAS world structure using entnum, then copies its local origin and angles. It calculates the world-space bounding box by adding the origin to the
   entity's local mins and maxs, storing the results in absmins and absmaxs. Finally it stores the entity's solid flag and model index (offset by one) in the provided structure.

	\param entnum The index of the entity to query.
	\param entdata Pointer to the structure to receive entity data.
*/
void AAS_EntityBSPData( int entnum, bsp_entdata_t* entdata )
{
	aas_entity_t* ent;

	ent = &( *defaultaasworld ).entities[entnum];
	VectorCopy( ent->i.origin, entdata->origin );
	VectorCopy( ent->i.angles, entdata->angles );
	VectorAdd( ent->i.origin, ent->i.mins, entdata->absmins );
	VectorAdd( ent->i.origin, ent->i.maxs, entdata->absmaxs );
	entdata->solid	  = ent->i.solid;
	entdata->modelnum = ent->i.modelindex - 1;
}

/*!
	\brief Resets each entity's area and leaf link information in the default AAS world.

	Iterates over all entities stored in the global AAS world and clears the associations to area and leaf structures by assigning NULL to the respective pointers.

*/
void AAS_ResetEntityLinks()
{
	int i;

	for( i = 0; i < ( *defaultaasworld ).maxentities; i++ ) {
		( *defaultaasworld ).entities[i].areas	= NULL;
		( *defaultaasworld ).entities[i].leaves = NULL;
	}
}

/*!
	\brief Marks all entities in the global AAS world as invalid and resets their indices.

	Iterates over all entities up to the maximum defined in defaultaasworld, setting each entity's valid flag to false and storing its index as the entity number. This prepares the entity list for a
   subsequent rebuild or update process.

*/
void AAS_InvalidateEntities()
{
	int i;

	for( i = 0; i < ( *defaultaasworld ).maxentities; i++ ) {
		( *defaultaasworld ).entities[i].i.valid  = qfalse;
		( *defaultaasworld ).entities[i].i.number = i;
	}
}

/*!
	\brief Searches for the nearest entity with a specified model index relative to a given point.

	The function iterates through all entities in the world, filtering by the provided modelindex. For each matching entity, it computes the vector from the input origin to the entity's origin and
   restricts consideration to entities whose x and y differences are less than 40 units. It then calculates the Euclidean length of that vector and keeps track of the smallest distance found. The
   index of the closest entity is returned; if no qualified entity is found, 0 is returned.

	\param origin the location from which distances are measured
	\param modelindex the model identifier used to select candidate entities
	\return the index of the nearest matching entity, or 0 if none found
*/
int AAS_NearestEntity( vec3_t origin, int modelindex )
{
	int			  i, bestentnum;
	float		  dist, bestdist;
	aas_entity_t* ent;
	vec3_t		  dir;

	bestentnum = 0;
	bestdist   = 99999;

	for( i = 0; i < ( *defaultaasworld ).maxentities; i++ ) {
		ent = &( *defaultaasworld ).entities[i];

		if( ent->i.modelindex != modelindex ) {
			continue;
		}

		VectorSubtract( ent->i.origin, origin, dir );

		if( fabs( dir[0] ) < 40 ) {
			if( fabs( dir[1] ) < 40 ) {
				dist = VectorLength( dir );

				if( dist < bestdist ) {
					bestdist   = dist;
					bestentnum = i;
				}
			}
		}
	}

	return bestentnum;
}

int AAS_BestReachableEntityArea( int entnum )
{
	aas_entity_t* ent;

	ent = &( *defaultaasworld ).entities[entnum];
	return AAS_BestReachableLinkArea( ent->areas );
}

int AAS_NextEntity( int entnum )
{
	if( !( *defaultaasworld ).loaded ) {
		return 0;
	}

	if( entnum < 0 ) {
		entnum = -1;
	}

	while( ++entnum < ( *defaultaasworld ).maxentities ) {
		if( ( *defaultaasworld ).entities[entnum].i.valid ) {
			return entnum;
		}
	}

	return 0;
}

int AAS_IsEntityInArea( int entnumIgnore, int entnumIgnore2, int areanum )
{
	aas_link_t*	  link;
	aas_entity_t* ent;
	//	int i;

	// RF, not functional (doesnt work with multiple areas)
	return qfalse;

	for( link = ( *aasworld ).arealinkedentities[areanum]; link; link = link->next_ent ) {
		// ignore the pass entity
		if( link->entnum == entnumIgnore ) {
			continue;
		}

		if( link->entnum == entnumIgnore2 ) {
			continue;
		}

		//
		ent = &( *defaultaasworld ).entities[link->entnum];

		if( !ent->i.valid ) {
			continue;
		}

		if( !ent->i.solid ) {
			continue;
		}

		return qtrue;
	}

	/*
		ent = (*defaultaasworld).entities;
		for (i = 0; i < (*defaultaasworld).maxclients; i++, ent++)
		{
			if (!ent->i.valid)
				continue;
			if (!ent->i.solid)
				continue;
			if (i == entnumIgnore)
				continue;
			if (i == entnumIgnore2)
				continue;
			for (link = ent->areas; link; link = link->next_area)
			{
				if (link->areanum == areanum)
				{
					return qtrue;
				}
			}
		}
	*/
	return qfalse;
}

/*!
	\brief Updates the routing status of a specified area, optionally returning its previous state

	Validates that the supplied area number is within the valid range of the world. If the area number is invalid, an error message is printed (when bot developer mode is active) and the function
   returns 0. When enable is negative, the function merely reports whether the area was previously enabled or disabled, returning non‑zero if it was enabled and zero otherwise. For non‑negative
   values, the AREA_DISABLED flag is cleared (enable) or set (disable). If the area’s disabled flag changes as a result, all routing cache entries that reference this area are purged to maintain
   consistency. The function then returns the former enabled status of the area – non‑zero if it was enabled before the call, zero if it was disabled.

	\param areanum number of the area to modify
	\return Non‑zero if the area was enabled before the call; zero if it was disabled or if the area number is out of range.
*/
int	 AAS_EnableRoutingArea( int areanum, int enable );

/*!
	\brief Marks or clears blocking status for AAS areas inside a bounding box

	If the bounding box is degenerate (min equals max) and the blocking flag is negative, the function clears all area blocking for every loaded world and restores routing. Otherwise, for each loaded
   world the function identifies all AAS areas that overlap the supplied bounding box and sets their routing state to the opposite of the blocking flag. A value of qtrue for blocking disables routing
   through those areas; qfalse enables routing again.

	\param absmin minimum corner of the bounding box
	\param absmax maximum corner of the bounding box
	\param blocking qboolean indicating desired blocking state; if negative and bounding box is degenerate a global reset is performed
*/
void AAS_SetAASBlockingEntity( vec3_t absmin, vec3_t absmax, qboolean blocking )
{
	int areas[128];
	int numareas, i, w;

	//
	// check for resetting AAS blocking
	if( VectorCompare( absmin, absmax ) && blocking < 0 ) {
		for( w = 0; w < MAX_AAS_WORLDS; w++ ) {
			AAS_SetCurrentWorld( w );

			//
			if( !( *aasworld ).loaded ) {
				continue;
			}

			// now clear blocking status
			for( i = 1; i < ( *aasworld ).numareas; i++ ) {
				AAS_EnableRoutingArea( i, qtrue );
			}
		}

		//
		return;
	}

	//
	for( w = 0; w < MAX_AAS_WORLDS; w++ ) {
		AAS_SetCurrentWorld( w );

		//
		if( !( *aasworld ).loaded ) {
			continue;
		}

		// grab the list of areas
		numareas = AAS_BBoxAreas( absmin, absmax, areas, 128 );

		// now set their blocking status
		for( i = 0; i < numareas; i++ ) {
			AAS_EnableRoutingArea( areas[i], !blocking );
		}
	}
}
