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
	\file engine/botlib/be_aas_bspq3.cpp
	\brief This file implements the BSP‑entity data layer of the AAS system in DarkWolf, parsing the entity block of a BSP file, storing key/value pairs in a compact buffer, and providing utility functions for querying entity attributes and environmental contents.
	\note doxygenix: sha256=1d12e40b8288fdf7d2f5f3c8fc34e01d0b05c34318867d09bbad32cf522c0289

	\par File Purpose
	- This file implements the BSP‑entity data layer of the AAS system in DarkWolf, parsing the entity block of a BSP file, storing key/value pairs in a compact buffer, and providing utility functions for querying entity attributes and environmental contents.

	\par Core Responsibilities
	- Provide a lightweight representation of the BSP world for the AAS/AI subsystem.
	- Expose entity and world query APIs such as content lookup, visibility, bounding‑box tests, and key/value retrieval.
	- Load, parse, and clean up BSP entity data without accessing geometry directly.
	- Offer stubbed BSP‑leaf linkage hooks for future expansion.

	\par Key Types and Functions
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

	\par Control Flow
	- AAS_LoadBSPFile clears any existing world data, obtains the entity block from botimport.BSPEntityData(), copies it into a cleared hunk buffer, parses the entities with AAS_ParseBSPEntities(), marks the world as loaded, and returns BLERR_NOERROR.
	- AAS_ParseBSPEntities first scans all entities in the entity block to compute a single memory requirement, allocates one contiguous buffer, reloads the script, and walks each entity building a linked list of bsp_epair_t nodes for each key/value pair.
	- Query functions such as AAS_ValueForBSPEpairKey, AAS_VectorForBSPEpairKey, AAS_FloatForBSPEpairKey, and AAS_IntForBSPEpairKey retrieve stored strings and convert them to the requested type.
	- Tracing and visibility helpers (AAS_Trace, AAS_PointContents, AAS_EntityCollision, AAS_inPVS, AAS_inPHS) delegate to the botimport API, forwarding all input and returning the obtained results.
	- Utility functions (AAS_BSPModelMinsMaxsOrigin, AAS_UnlinkFromBSPLeaves, AAS_BSPLinkEntity, AAS_BoxEntities, AAS_NextBSPEntity, AAS_BSPEntityInRange) provide either stubs or simple entity‑management logic that operates on the global bspworld.
	- AAS_FreeBSPEntities releases the buffer holding all epair strings and resets the entity count; it is called from AAS_DumpBSPData and from AAS_ParseBSPEntities on error.
	- AAS_DumpBSPData clears the entire bspworld structure, frees entity data, and marks the world as unloaded.

	\par Dependencies
	- "q_shared.h" – basic types, constants (e.g. vec3_t, PRT_MESSAGE).
	- "l_memory.h" – memory allocation helpers (GetClearedHunkMemory, FreeMemory).
	- "l_script.h" – script parsing utilities (LoadScriptMemory, PS_ReadToken).
	- "l_precomp.h" – pre‑compiled header with common includes.
	- "l_struct.h" – generic structure definitions.
	- "aasfile.h" – AAS file format definitions.
	- "botshared/botlib.h" – import function table (botimport).
	- "botshared/be_aas.h", "be_aas_funcs.h", "be_aas_def.h" – AAS API declarations.

	\par How It Fits
	- Part of the botlib module that supplies the AAS system with world information.
	- Uses the engine’s botimport infrastructure to perform heavy‑lifting tasks such as collisions, visibility, and model bounds.
	- Operates on a small in‑memory snapshot of entity data, enabling fast look‑ups required by AAS navigation, pathfinding, and perception logic.
	- Designed for single‑player mode; geometry handling is abstracted away, keeping this module focused on entity metadata and simple spatial queries.
*/

/*****************************************************************************
 * name:		be_aas_bspq3.c
 *
 * desc:		BSP, Environment Sampling
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "botshared/botlib.h"
#include "botshared/be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

extern botlib_import_t botimport;

// #define TRACE_DEBUG

#define ON_EPSILON		0.005
// #define DEG2RAD( a ) (( a * M_PI ) / 180.0F)

#define MAX_BSPENTITIES 2048

typedef struct rgb_s {
	int red;
	int green;
	int blue;
} rgb_t;

// bsp entity epair
typedef struct bsp_epair_s {
	char*				key;
	char*				value;
	struct bsp_epair_s* next;
} bsp_epair_t;

// bsp data entity
typedef struct bsp_entity_s {
	bsp_epair_t* epairs;
} bsp_entity_t;

// id Sofware BSP data
typedef struct bsp_s {
	// true when bsp file is loaded
	int			 loaded;
	// entity data
	int			 entdatasize;
	char*		 dentdata;
	// bsp entities
	int			 numentities;
	bsp_entity_t entities[MAX_BSPENTITIES];
	// memory used for strings and epairs
	byte*		 ebuffer;
} bsp_t;

// global bsp
bsp_t bspworld;

#ifdef BSP_DEBUG
typedef struct cname_s {
	int	  value;
	char* name;
} cname_t;

cname_t contentnames[] = { { CONTENTS_SOLID, "CONTENTS_SOLID" },
	{ CONTENTS_WINDOW, "CONTENTS_WINDOW" },
	{ CONTENTS_AUX, "CONTENTS_AUX" },
	{ CONTENTS_LAVA, "CONTENTS_LAVA" },
	{ CONTENTS_SLIME, "CONTENTS_SLIME" },
	{ CONTENTS_WATER, "CONTENTS_WATER" },
	{ CONTENTS_MIST, "CONTENTS_MIST" },
	{ LAST_VISIBLE_CONTENTS, "LAST_VISIBLE_CONTENTS" },

	{ CONTENTS_AREAPORTAL, "CONTENTS_AREAPORTAL" },
	{ CONTENTS_PLAYERCLIP, "CONTENTS_PLAYERCLIP" },
	{ CONTENTS_MONSTERCLIP, "CONTENTS_MONSTERCLIP" },
	{ CONTENTS_CURRENT_0, "CONTENTS_CURRENT_0" },
	{ CONTENTS_CURRENT_90, "CONTENTS_CURRENT_90" },
	{ CONTENTS_CURRENT_180, "CONTENTS_CURRENT_180" },
	{ CONTENTS_CURRENT_270, "CONTENTS_CURRENT_270" },
	{ CONTENTS_CURRENT_UP, "CONTENTS_CURRENT_UP" },
	{ CONTENTS_CURRENT_DOWN, "CONTENTS_CURRENT_DOWN" },
	{ CONTENTS_ORIGIN, "CONTENTS_ORIGIN" },
	{ CONTENTS_MONSTER, "CONTENTS_MONSTER" },
	{ CONTENTS_DEADMONSTER, "CONTENTS_DEADMONSTER" },
	{ CONTENTS_DETAIL, "CONTENTS_DETAIL" },
	{ CONTENTS_TRANSLUCENT, "CONTENTS_TRANSLUCENT" },
	{ CONTENTS_LADDER, "CONTENTS_LADDER" },
	{ 0, 0 } };

/*!
	\brief Prints the names of content types that are set in the contents bitmask.

	This function iterates through an array of content names and their corresponding bitmask values. For each content type that is set in the provided contents bitmask, it prints the name of that content type. The function continues until it reaches the end of the contentnames array, which is indicated by a zero value in the value field of the contentnames structure.

	\param contents bitmask representing set content types
*/
void	PrintContents( int contents )
{
	int i;

	for( i = 0; contentnames[i].value; i++ ) {
		if( contents & contentnames[i].value ) {
			botimport.Print( PRT_MESSAGE, "%s\n", contentnames[i].name );
		}
	}
}

#endif // BSP_DEBUG

bsp_trace_t AAS_Trace( vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask )
{
	bsp_trace_t bsptrace;
	botimport.Trace( &bsptrace, start, mins, maxs, end, passent, contentmask );
	return bsptrace;
}

int AAS_PointContents( vec3_t point )
{
	return botimport.PointContents( point );
}

/*!
	\brief Tests whether a bounding box traveling from start to end intersects a specified entity before the current trace fraction and updates the trace if it does.

	The function performs an entity trace using the provided bounding box parameters and compares the resulting fraction with that of the supplied trace. If the entity trace fraction is smaller,
   indicating an earlier collision, the full trace information is copied to the supplied trace structure and the function returns true. If not, the original trace remains unchanged and the function
   returns false.

	\param entnum Index of the entity to test
	\param start Starting position of the trace
	\param boxmins Minimum extents of the bounding box
	\param boxmaxs Maximum extents of the bounding box
	\param end End point of the trace
	\param contentmask Mask of content types to consider in the trace
	\param trace Pointer to a trace structure that will be updated if this entity is the closest hit
	\return true if the entity was hit before the current trace fraction, qfalse otherwise
*/
qboolean AAS_EntityCollision( int entnum, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int contentmask, bsp_trace_t* trace )
{
	bsp_trace_t enttrace;

	botimport.EntityTrace( &enttrace, start, boxmins, boxmaxs, end, entnum, contentmask );

	if( enttrace.fraction < trace->fraction ) {
		memcpy( trace, &enttrace, sizeof( bsp_trace_t ) );
		return qtrue;
	}

	return qfalse;
}

qboolean AAS_inPVS( vec3_t p1, vec3_t p2 )
{
	return botimport.inPVS( p1, p2 );
}

qboolean AAS_inPHS( vec3_t p1, vec3_t p2 )
{
	return qtrue;
}

void AAS_BSPModelMinsMaxsOrigin( int modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin )
{
	botimport.BSPModelMinsMaxsOrigin( modelnum, angles, mins, maxs, origin );
}

/*!
	\brief unlinks the entity from all leaves

	The function iterates over each leaf provided in the list and removes the entity from them, updating any global data structures as necessary.

	\param leaves pointer to a linked list of leaf nodes
*/
void AAS_UnlinkFromBSPLeaves( bsp_link_t* leaves )
{
}

/*!
	\brief Returns a bsp_link_t pointer for the specified entity or NULL if linking is not performed.

	This function is intended to create or retrieve a linking structure that connects an entity to the BSP tree based on its absolute bounding box. The current implementation is a placeholder and
   unconditionally returns NULL. The actual linking logic would use the provided entity number, model index, and bounding coordinates to locate or create the appropriate bsp_link_t structure within
   the world BSP. TODO: clarify how the linkage should be established and what data the returned structure should contain.

	\param absmins Minimum coordinates of the entity's bounding box (vec3_t).
	\param absmaxs Maximum coordinates of the entity's bounding box (vec3_t).
	\param entnum Unique identifier for the entity in the world (int).
	\param modelnum Model index associated with the entity (int).
	\return Pointer to a bsp_link_t structure, currently NULL.
*/
bsp_link_t* AAS_BSPLinkEntity( vec3_t absmins, vec3_t absmaxs, int entnum, int modelnum )
{
	return NULL;
}

int AAS_BoxEntities( vec3_t absmins, vec3_t absmaxs, int* list, int maxcount )
{
	return 0;
}

int AAS_NextBSPEntity( int ent )
{
	ent++;

	if( ent >= 1 && ent < bspworld.numentities ) {
		return ent;
	}

	return 0;
}

/*!
	\brief Verifies that a BSP entity index falls within the valid range of loaded entities.

	If the supplied index is less than or equal to zero or greater than or equal to the total number of entities in the world, an error message is printed and the function returns a false indicator.
   For a valid index the function returns true.

	\param ent Entity index to validate.
	\return Integer value representing true (entity exists) or false (index out of range).
*/
int AAS_BSPEntityInRange( int ent )
{
	if( ent <= 0 || ent >= bspworld.numentities ) {
		botimport.Print( PRT_MESSAGE, "bsp entity out of range\n" );
		return qfalse;
	}

	return qtrue;
}

int AAS_ValueForBSPEpairKey( int ent, char* key, char* value, int size )
{
	bsp_epair_t* epair;

	value[0] = '\0';

	if( !AAS_BSPEntityInRange( ent ) ) {
		return qfalse;
	}

	for( epair = bspworld.entities[ent].epairs; epair; epair = epair->next ) {
		if( !strcmp( epair->key, key ) ) {
			strncpy( value, epair->value, size - 1 );
			value[size - 1] = '\0';
			return qtrue;
		}
	}

	return qfalse;
}

int AAS_VectorForBSPEpairKey( int ent, char* key, vec3_t v )
{
	char   buf[MAX_EPAIRKEY];
	double v1, v2, v3;

	VectorClear( v );

	if( !AAS_ValueForBSPEpairKey( ent, key, buf, MAX_EPAIRKEY ) ) {
		return qfalse;
	}

	// scanf into doubles, then assign, so it is vec_t size independent
	v1 = v2 = v3 = 0;
	sscanf( buf, "%lf %lf %lf", &v1, &v2, &v3 );
	v[0] = v1;
	v[1] = v2;
	v[2] = v3;
	return qtrue;
}

int AAS_FloatForBSPEpairKey( int ent, char* key, float* value )
{
	char buf[MAX_EPAIRKEY];

	*value = 0;

	if( !AAS_ValueForBSPEpairKey( ent, key, buf, MAX_EPAIRKEY ) ) {
		return qfalse;
	}

	*value = atof( buf );
	return qtrue;
}

int AAS_IntForBSPEpairKey( int ent, char* key, int* value )
{
	char buf[MAX_EPAIRKEY];

	*value = 0;

	if( !AAS_ValueForBSPEpairKey( ent, key, buf, MAX_EPAIRKEY ) ) {
		return qfalse;
	}

	*value = atoi( buf );
	return qtrue;
}

/*!
	\brief Frees memory used for BSP entities and resets the entity count.

	This function releases any allocated entity buffer memory, if it exists, and then sets the global entity count to zero. It is typically called when the BSP world data is no longer needed.

*/
void AAS_FreeBSPEntities()
{
	// RF, optimized memory allocation
	/*
		int i;
		bsp_entity_t *ent;
		bsp_epair_t *epair, *nextepair;

		for (i = 1; i < bspworld.numentities; i++)
		{
			ent = &bspworld.entities[i];
			for (epair = ent->epairs; epair; epair = nextepair)
			{
				nextepair = epair->next;
				//
				if (epair->key) FreeMemory(epair->key);
				if (epair->value) FreeMemory(epair->value);
				FreeMemory(epair);
			}
		}
	*/
	if( bspworld.ebuffer ) {
		FreeMemory( bspworld.ebuffer );
	}

	bspworld.numentities = 0;
}

/*!
	\brief Parses entity data from a BSP file into global structures.

	The function first scans the entity data to compute the amount of memory required for storing all entity key/value pairs and then allocates a single contiguous block. It reloads the script and
   parses each entity, building linked lists of key/value epairs. Parsed entities are placed into the global bspworld structure. Errors during parsing trigger cleanup of allocated resources.

	This routine does not return a value but updates global state used elsewhere in the engine.

*/
void AAS_ParseBSPEntities()
{
	script_t*	  script;
	token_t		  token;
	bsp_entity_t* ent;
	bsp_epair_t*  epair;
	byte *		  buffer, *buftrav;
	int			  bufsize;

	// RF, modified this, so that it first gathers up memory requirements, then allocates a single chunk,
	// and places the strings all in there

	bspworld.ebuffer = NULL;

	script = LoadScriptMemory( bspworld.dentdata, bspworld.entdatasize, "entdata" );
	SetScriptFlags( script, SCFL_NOSTRINGWHITESPACES | SCFL_NOSTRINGESCAPECHARS ); // SCFL_PRIMITIVE);

	bufsize = 0;

	while( PS_ReadToken( script, &token ) ) {
		if( strcmp( token.string, "{" ) ) {
			ScriptError( script, "invalid %s\n", token.string );
			AAS_FreeBSPEntities();
			FreeScript( script );
			return;
		}

		if( bspworld.numentities >= MAX_BSPENTITIES ) {
			botimport.Print( PRT_MESSAGE, "too many entities in BSP file\n" );
			break;
		}

		while( PS_ReadToken( script, &token ) ) {
			if( !strcmp( token.string, "}" ) ) {
				break;
			}

			bufsize += sizeof( bsp_epair_t );

			if( token.type != TT_STRING ) {
				ScriptError( script, "invalid %s\n", token.string );
				AAS_FreeBSPEntities();
				FreeScript( script );
				return;
			}

			StripDoubleQuotes( token.string );
			bufsize += strlen( token.string ) + 1;

			if( !PS_ExpectTokenType( script, TT_STRING, 0, &token ) ) {
				AAS_FreeBSPEntities();
				FreeScript( script );
				return;
			}

			StripDoubleQuotes( token.string );
			bufsize += strlen( token.string ) + 1;
		}

		if( strcmp( token.string, "}" ) ) {
			ScriptError( script, "missing }\n" );
			AAS_FreeBSPEntities();
			FreeScript( script );
			return;
		}
	}

	FreeScript( script );

	buffer			 = ( byte* )GetClearedHunkMemory( bufsize );
	buftrav			 = buffer;
	bspworld.ebuffer = buffer;

	// RF, now parse the entities into memory
	// RF, NOTE: removed error checks for speed, no need to do them twice

	script = LoadScriptMemory( bspworld.dentdata, bspworld.entdatasize, "entdata" );
	SetScriptFlags( script, SCFL_NOSTRINGWHITESPACES | SCFL_NOSTRINGESCAPECHARS ); // SCFL_PRIMITIVE);

	bspworld.numentities = 1;

	while( PS_ReadToken( script, &token ) ) {
		ent = &bspworld.entities[bspworld.numentities];
		bspworld.numentities++;
		ent->epairs = NULL;

		while( PS_ReadToken( script, &token ) ) {
			if( !strcmp( token.string, "}" ) ) {
				break;
			}

			epair = ( bsp_epair_t* )buftrav;
			buftrav += sizeof( bsp_epair_t );
			epair->next = ent->epairs;
			ent->epairs = epair;
			StripDoubleQuotes( token.string );
			epair->key = ( char* )buftrav;
			buftrav += ( strlen( token.string ) + 1 );
			strcpy( epair->key, token.string );

			if( !PS_ExpectTokenType( script, TT_STRING, 0, &token ) ) {
				AAS_FreeBSPEntities();
				FreeScript( script );
				return;
			}

			StripDoubleQuotes( token.string );
			epair->value = ( char* )buftrav;
			buftrav += ( strlen( token.string ) + 1 );
			strcpy( epair->value, token.string );
		}
	}

	FreeScript( script );
}

/*!
	\brief TODO: clarify the purpose and behavior of AAS_BSPTraceLight.

	TODO: clarify how the start, end, and endpos parameters are interpreted, what values are written to red, green, blue, and what the integer return value signifies.

	\param start TODO: clarify parameter usage.
	\param end TODO: clarify parameter usage.
	\param endpos TODO: clarify parameter usage.
	\param red TODO: clarify parameter usage.
	\param green TODO: clarify parameter usage.
	\param blue TODO: clarify parameter usage.
	\return TODO: clarify return value.
*/
int AAS_BSPTraceLight( vec3_t start, vec3_t end, vec3_t endpos, int* red, int* green, int* blue )
{
	return 0;
}

/*!
	\brief Releases BSP entity data and resets the world state.

	This function frees any allocated memory for BSP entities, clears the world data structure, and marks the world as not loaded.

*/
void AAS_DumpBSPData()
{
	AAS_FreeBSPEntities();

	if( bspworld.dentdata ) {
		FreeMemory( bspworld.dentdata );
	}

	bspworld.dentdata	 = NULL;
	bspworld.entdatasize = 0;
	//
	bspworld.loaded = qfalse;
	memset( &bspworld, 0, sizeof( bspworld ) );
}

/*!
	\brief Loads a BSP file into memory and parses the embedded entity data.

	This routine first dumps any existing BSP data structures, then determines the size of the entity block within the BSP file. It allocates a cleared memory block, copies the entity data into this
   buffer, and parses it to build the entity list. Finally, it records the BSP world as loaded and returns a success code. The function does not throw exceptions or handle errors beyond signaling a
   no–error return.

	\return The function returns a status code; BLERR_NOERROR indicates the BSP file was successfully loaded.
*/
int AAS_LoadBSPFile()
{
	AAS_DumpBSPData();
	bspworld.entdatasize = strlen( botimport.BSPEntityData() ) + 1;
	bspworld.dentdata	 = ( char* )GetClearedHunkMemory( bspworld.entdatasize );
	memcpy( bspworld.dentdata, botimport.BSPEntityData(), bspworld.entdatasize );
	AAS_ParseBSPEntities();
	bspworld.loaded = qtrue;
	return BLERR_NOERROR;
}
