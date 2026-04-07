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
 * name:		be_ai_goal.c
 *
 * desc:		goal AI
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_utils.h"
#include "l_libvar.h"
#include "l_memory.h"
#include "l_log.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "botshared/botlib.h"
#include "botshared/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_ai_weight.h"
#include "botshared/be_ai_goal.h"
#include "botshared/be_ai_move.h"

// #define DEBUG_AI_GOAL
#ifdef RANDOMIZE
	#define UNDECIDEDFUZZY
#endif // RANDOMIZE
#define DROPPEDWEIGHT
// avoid goal time
#define AVOID_TIME		  30
// avoid dropped goal time
#define AVOIDDROPPED_TIME 5
//
#define TRAVELTIME_SCALE  0.01

// location in the map "target_location"
typedef struct maplocation_s {
	vec3_t				  origin;
	int					  areanum;
	char				  name[MAX_EPAIRKEY];
	struct maplocation_s* next;
} maplocation_t;

// camp spots "info_camp"
typedef struct campspot_s {
	vec3_t			   origin;
	int				   areanum;
	char			   name[MAX_EPAIRKEY];
	float			   range;
	float			   weight;
	float			   wait;
	float			   random;
	struct campspot_s* next;
} campspot_t;

// FIXME: these are game specific
typedef enum {
	GT_FFA,			  // free for all
	GT_TOURNAMENT,	  // one on one tournament
	GT_SINGLE_PLAYER, // single player tournament

	//-- team games go after this --

	GT_TEAM, // team deathmatch
	GT_CTF,	 // capture the flag

	GT_MAX_GAME_TYPE
} gametype_t;

// Rafael gameskill
typedef enum {
	GSKILL_EASY,
	GSKILL_MEDIUM,
	GSKILL_HARD, // normal default level
	GSKILL_MAX
} gameskill_t;

typedef struct levelitem_s {
	int					number;		 // number of the level item
	int					iteminfo;	 // index into the item info
	int					notteam;	 // true if not in teamplay
	int					notfree;	 // true if not in ffa
	int					notsingle;	 // true if not in single
	vec3_t				origin;		 // origin of the item
	int					goalareanum; // area the item is in
	vec3_t				goalorigin;	 // goal origin within the area
	int					entitynum;	 // entity number
	float				timeout;	 // item is removed after this time
	struct levelitem_s *prev, *next;
} levelitem_t;

typedef struct iteminfo_s {
	char   classname[32];		   // classname of the item
	char   name[MAX_STRINGFIELD];  // name of the item
	char   model[MAX_STRINGFIELD]; // model of the item
	int	   modelindex;			   // model index
	int	   type;				   // item type
	int	   index;				   // index in the inventory
	float  respawntime;			   // respawn time
	vec3_t mins;				   // mins of the item
	vec3_t maxs;				   // maxs of the item
	int	   number;				   // number of the item info
} iteminfo_t;

#define ITEMINFO_OFS( x ) ( int )&( ( ( iteminfo_t* )0 )->x )

fielddef_t	iteminfo_fields[] = { { "name", ITEMINFO_OFS( name ), FT_STRING },
	 { "model", ITEMINFO_OFS( model ), FT_STRING },
	 { "modelindex", ITEMINFO_OFS( modelindex ), FT_INT },
	 { "type", ITEMINFO_OFS( type ), FT_INT },
	 { "index", ITEMINFO_OFS( index ), FT_INT },
	 { "respawntime", ITEMINFO_OFS( respawntime ), FT_FLOAT },
	 { "mins", ITEMINFO_OFS( mins ), FT_FLOAT | FT_ARRAY, 3 },
	 { "maxs", ITEMINFO_OFS( maxs ), FT_FLOAT | FT_ARRAY, 3 },
	 { 0, 0, 0 } };

structdef_t iteminfo_struct = { sizeof( iteminfo_t ), iteminfo_fields };

typedef struct itemconfig_s {
	int			numiteminfo;
	iteminfo_t* iteminfo;
} itemconfig_t;

// goal state
typedef struct bot_goalstate_s {
	struct weightconfig_s* itemweightconfig; // weight config
	int*				   itemweightindex;	 // index from item to weight
	//
	int					   client;				 // client using this goal state
	int					   lastreachabilityarea; // last area with reachabilities the bot was in
	//
	bot_goal_t			   goalstack[MAX_GOALSTACK]; // goal stack
	int					   goalstacktop;			 // the top of the goal stack
	//
	int					   avoidgoals[MAX_AVOIDGOALS];	   // goals to avoid
	float				   avoidgoaltimes[MAX_AVOIDGOALS]; // times to avoid the goals
} bot_goalstate_t;

bot_goalstate_t* botgoalstates[MAX_CLIENTS + 1];
// item configuration
itemconfig_t*	 itemconfig = NULL;
// level items
levelitem_t*	 levelitemheap	= NULL;
levelitem_t*	 freelevelitems = NULL;
levelitem_t*	 levelitems		= NULL;
int				 numlevelitems	= 0;
// map locations
maplocation_t*	 maplocations = NULL;
// camp spots
campspot_t*		 campspots = NULL;
// the game type
int				 g_gametype;

// Rafael gameskill
int				 g_gameskill;

/*!
	\brief Retrieve the goal state associated with a handle, returning null if the handle is invalid or unused.

	The function first verifies that the handle is within the valid range (1 to MAX_CLIENTS). If the handle is out of bounds, it logs a fatal error and returns null. It then checks whether a goal
   state exists at that handle in the global botgoalstates array; if not, it logs another fatal error and returns null. If a goal state is present, the function returns a pointer to it.

	\param handle Index of the goal state to retrieve.
	\return Pointer to the corresponding bot_goalstate_t, or null on error.
*/
bot_goalstate_t* BotGoalStateFromHandle( int handle )
{
	if( handle <= 0 || handle > MAX_CLIENTS ) {
		botimport.Print( PRT_FATAL, "goal state handle %d out of range\n", handle );
		return NULL;
	}

	if( !botgoalstates[handle] ) {
		botimport.Print( PRT_FATAL, "invalid goal state %d\n", handle );
		return NULL;
	}

	return botgoalstates[handle];
}

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotInterbreedGoalFuzzyLogic( int parent1, int parent2, int child )
{
	bot_goalstate_t *p1, *p2, *c;

	p1 = BotGoalStateFromHandle( parent1 );
	p2 = BotGoalStateFromHandle( parent2 );
	c  = BotGoalStateFromHandle( child );

	InterbreedWeightConfigs( p1->itemweightconfig, p2->itemweightconfig, c->itemweightconfig );
}

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotSaveGoalFuzzyLogic( int goalstate, char* filename )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );

	// WriteWeightConfig(filename, gs->itemweightconfig);
}

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotMutateGoalFuzzyLogic( int goalstate, float range )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );

	EvolveWeightConfig( gs->itemweightconfig );
}

/*!
	\brief Loads item configuration data from a specified file into memory and returns the resulting configuration structure.

	The function first obtains a maximum number of item entries from a library variable, enforcing a sane default if the value is invalid. It copies the supplied file name into a fixed‑size buffer and
   attempts to open the source file. On success, it allocates a contiguous memory block large enough to hold an itemconfig structure followed by an array of iteminfo structures. The file is parsed
   token by token; whenever the keyword "iteminfo" is encountered, a new entry is created. The class name string is read, surrounding quotes are stripped, and the name is copied into the structure,
   after which the remaining fields are populated by a call to ReadStructure. If the number of entries exceeds the maximum, required tokens are missing, or parsing fails, the function frees any
   allocated memory and the source handle before returning NULL. After successful parsing, the source file is closed, a warning is printed if no entries were loaded, and a success message containing
   the file path is displayed. The populated itemconfig structure is then returned.

	\param filename The path or name of the configuration file to load.
	\return A pointer to an itemconfig_t object containing the loaded item information, or NULL if an error occurred.
*/
itemconfig_t* LoadItemConfig( char* filename )
{
	int			  max_iteminfo;
	token_t		  token;
	char		  path[MAX_PATH];
	source_t*	  source;
	itemconfig_t* ic;
	iteminfo_t*	  ii;

	max_iteminfo = ( int )LibVarValue( "max_iteminfo", "256" );

	if( max_iteminfo < 0 ) {
		botimport.Print( PRT_ERROR, "max_iteminfo = %d\n", max_iteminfo );
		max_iteminfo = 128;
		LibVarSet( "max_iteminfo", "128" );
	}

	strncpy( path, filename, MAX_PATH );
	source = LoadSourceFile( path );

	if( !source ) {
		botimport.Print( PRT_ERROR, "counldn't load %s\n", path );
		return NULL;
	}

	// initialize item config
	ic				= ( itemconfig_t* )GetClearedHunkMemory( sizeof( itemconfig_t ) + max_iteminfo * sizeof( iteminfo_t ) );
	ic->iteminfo	= ( iteminfo_t* )( ( char* )ic + sizeof( itemconfig_t ) );
	ic->numiteminfo = 0;

	// parse the item config file
	while( PC_ReadToken( source, &token ) ) {
		if( !strcmp( token.string, "iteminfo" ) ) {
			if( ic->numiteminfo >= max_iteminfo ) {
				SourceError( source, "more than %d item info defined\n", max_iteminfo );
				FreeMemory( ic );
				FreeSource( source );
				return NULL;
			}

			ii = &ic->iteminfo[ic->numiteminfo];
			memset( ii, 0, sizeof( iteminfo_t ) );

			if( !PC_ExpectTokenType( source, TT_STRING, 0, &token ) ) {
				FreeMemory( ic );
				FreeMemory( source );
				return NULL;
			}

			StripDoubleQuotes( token.string );
			strncpy( ii->classname, token.string, sizeof( ii->classname ) - 1 );

			if( !ReadStructure( source, &iteminfo_struct, ( char* )ii ) ) {
				FreeMemory( ic );
				FreeSource( source );
				return NULL;
			}

			ii->number = ic->numiteminfo;
			ic->numiteminfo++;

		} else {
			SourceError( source, "unknown definition %s\n", token.string );
			FreeMemory( ic );
			FreeSource( source );
			return NULL;
		}
	}

	FreeSource( source );

	//
	if( !ic->numiteminfo ) {
		botimport.Print( PRT_WARNING, "no item info loaded\n" );
	}

	botimport.Print( PRT_MESSAGE, "loaded %s\n", path );
	return ic;
}

/*!
	\brief Creates an array mapping each iteminfo to its associated fuzzy weight index

	The function allocates a new array of int, sized to the number of iteminfos in the supplied item configuration. It then iterates over every iteminfo, calling FindFuzzyWeight to locate the
   corresponding weight function in the provided weight configuration. If a weight function cannot be found, a diagnostic message is written via Log_Write and the array entry is left negative. The
   returned array contains the index of the weight function for each iteminfo, or a negative value when none exists. The caller is responsible for freeing this memory.

	\param iwc The weight configuration used to resolve weight indices
	\param ic The item configuration containing the list of iteminfos to process
	\return A pointer to a newly allocated int array whose length equals ic->numiteminfo. Each element holds the index of the fuzzy weight function for the corresponding iteminfo, or a negative value
   if no weight function could be found.
*/
int* ItemWeightIndex( weightconfig_t* iwc, itemconfig_t* ic )
{
	int *index, i;

	// initialize item weight index
	index = ( int* )GetClearedMemory( sizeof( int ) * ic->numiteminfo );

	for( i = 0; i < ic->numiteminfo; i++ ) {
		index[i] = FindFuzzyWeight( iwc, ic->iteminfo[i].classname );

		if( index[i] < 0 ) {
			Log_Write( "item info %d \"%s\" has no fuzzy weight\r\n", i, ic->iteminfo[i].classname );
		}
	}

	return index;
}

/*!
	\brief Initializes or resets the global level item heap.

	If an existing heap exists, it is freed first. The maximum number of level items is obtained from the variable "max_levelitems" (defaulting to 256). Memory sufficient for that many levelitem_t
   structures is allocated. The structures are then linked together into a singly linked free list by assigning each element's next pointer to the following one; the last element’s next is set to
   null. The global pointer freelevelitems is set to the head of this list, preparing the system for future item allocations.

*/
void InitLevelItemHeap()
{
	int i, max_levelitems;

	if( levelitemheap ) {
		FreeMemory( levelitemheap );
	}

	max_levelitems = ( int )LibVarValue( "max_levelitems", "256" );
	levelitemheap  = ( levelitem_t* )GetMemory( max_levelitems * sizeof( levelitem_t ) );

	for( i = 0; i < max_levelitems - 2; i++ ) {
		levelitemheap[i].next = &levelitemheap[i + 1];
	}

	levelitemheap[max_levelitems - 1].next = NULL;
	//
	freelevelitems = levelitemheap;
}

/*!
	\brief Allocates and returns a level item from the free list, or NULL if none are available.

	The function grabs the first element in the global free list. If no element exists, it logs a fatal error and returns NULL. Otherwise it advances the free list head, zeros the memory of the chosen
   item with memset, and returns its pointer.

	\return Pointer to the allocated levelitem_t or NULL when no free items remain.
*/
levelitem_t* AllocLevelItem()
{
	levelitem_t* li;

	li = freelevelitems;

	if( !li ) {
		botimport.Print( PRT_FATAL, "out of level items\n" );
		return NULL;
	}

	//
	freelevelitems = freelevelitems->next;
	memset( li, 0, sizeof( levelitem_t ) );
	return li;
}

/*!
	\brief Adds a level item back to the global free list for later reuse.

	The function inserts the provided levelitem_t pointed to by li at the head of the free list. It assigns li->next to the current free list head and then updates the free list head pointer to li.
   This operation merely recycles the item; no memory deallocation occurs.

	\param li Pointer to the level item to be returned to the free list.
*/
void FreeLevelItem( levelitem_t* li )
{
	li->next	   = freelevelitems;
	freelevelitems = li;
}

/*!
	\brief Insert a level item node at the beginning of the global levelitems list and update its links.

	If the list already contains nodes, the previous head’s prev pointer is set to the new node. The new node’s prev pointer is set to NULL and its next pointer is set to the former head. Finally, the
   global head pointer is updated to the new node, ensuring the list remains consistent.

	\param li pointer to the level item to be added; the caller must have allocated and initialized the node prior to the call.
*/
void AddLevelItemToList( levelitem_t* li )
{
	if( levelitems ) {
		levelitems->prev = li;
	}

	li->prev   = NULL;
	li->next   = levelitems;
	levelitems = li;
}

/*!
	\brief Detaches a level item from the doubly linked list and updates the list head if necessary.

	The function adjusts the neighboring nodes’ pointers: if the removed item has a previous node it connects that node’s next pointer to the removed node’s next; otherwise the global head pointer
   levelitems is set to the removed node’s next. Finally, if a next node exists its prev pointer is set to the removed node’s previous. This operation removes the node from the list without
   deallocating it.

	\param li Pointer to the level item to be removed; must already belong to the list and not be null.
*/
void RemoveLevelItemFromList( levelitem_t* li )
{
	if( li->prev ) {
		li->prev->next = li->next;

	} else {
		levelitems = li->next;
	}

	if( li->next ) {
		li->next->prev = li->prev;
	}
}

/*!
	\brief Releases all allocated map location and camp spot structures and resets the global lists.

	The function iterates over the linked lists of map locations and camp spots, freeing each node using FreeMemory. After all nodes are freed, the global pointers representing the lists are set to
   null to indicate that no entities remain.

*/
void BotFreeInfoEntities()
{
	maplocation_t *ml, *nextml;
	campspot_t *   cs, *nextcs;

	for( ml = maplocations; ml; ml = nextml ) {
		nextml = ml->next;
		FreeMemory( ml );
	}

	maplocations = NULL;

	for( cs = campspots; cs; cs = nextcs ) {
		nextcs = cs->next;
		FreeMemory( cs );
	}

	campspots = NULL;
}

/*!
	\brief Initializes map location and camp spot entities for bot pathfinding.

	First clears any existing entity information, then walks through all entities in the map's BSP data. For each entity named "target_location" it stores its position, name and area number in a
   linked list of map locations. For each entity named "info_camp" it collects camp spot parameters such as origin, name, range, weight, wait, and random, verifies that the spot is in a valid area,
   and then adds it to a list of camp spots. If developer mode is enabled it reports the counts of locations and spots processed. Invalid camp spots are discarded with a message message.

*/
void BotInitInfoEntities()
{
	char		   classname[MAX_EPAIRKEY];
	maplocation_t* ml;
	campspot_t*	   cs;
	int			   ent, numlocations, numcampspots;

	BotFreeInfoEntities();
	//
	numlocations = 0;
	numcampspots = 0;

	for( ent = AAS_NextBSPEntity( 0 ); ent; ent = AAS_NextBSPEntity( ent ) ) {
		if( !AAS_ValueForBSPEpairKey( ent, "classname", classname, MAX_EPAIRKEY ) ) {
			continue;
		}

		// map locations
		if( !strcmp( classname, "target_location" ) ) {
			ml = ( maplocation_t* )GetClearedMemory( sizeof( maplocation_t ) );
			AAS_VectorForBSPEpairKey( ent, "origin", ml->origin );
			AAS_ValueForBSPEpairKey( ent, "message", ml->name, sizeof( ml->name ) );
			ml->areanum	 = AAS_PointAreaNum( ml->origin );
			ml->next	 = maplocations;
			maplocations = ml;
			numlocations++;
		}

		// camp spots
		else if( !strcmp( classname, "info_camp" ) ) {
			cs = ( campspot_t* )GetClearedMemory( sizeof( campspot_t ) );
			AAS_VectorForBSPEpairKey( ent, "origin", cs->origin );
			// cs->origin[2] += 16;
			AAS_ValueForBSPEpairKey( ent, "message", cs->name, sizeof( cs->name ) );
			AAS_FloatForBSPEpairKey( ent, "range", &cs->range );
			AAS_FloatForBSPEpairKey( ent, "weight", &cs->weight );
			AAS_FloatForBSPEpairKey( ent, "wait", &cs->wait );
			AAS_FloatForBSPEpairKey( ent, "random", &cs->random );
			cs->areanum = AAS_PointAreaNum( cs->origin );

			if( !cs->areanum ) {
				botimport.Print( PRT_MESSAGE, "camp spot at %1.1f %1.1f %1.1f in solid\n", cs->origin[0], cs->origin[1], cs->origin[2] );
				FreeMemory( cs );
				continue;
			}

			cs->next  = campspots;
			campspots = cs;
			// AAS_DrawPermanentCross(cs->origin, 4, LINECOLOR_YELLOW);
			numcampspots++;
		}
	}

	if( bot_developer ) {
		botimport.Print( PRT_MESSAGE, "%d map locations\n", numlocations );
		botimport.Print( PRT_MESSAGE, "%d camp spots\n", numcampspots );
	}
}

void BotInitLevelItems()
{
	int			  i, spawnflags;
	char		  classname[MAX_EPAIRKEY];
	vec3_t		  origin;
	int			  ent;
	itemconfig_t* ic;
	levelitem_t*  li;

	// initialize the map locations and camp spots
	BotInitInfoEntities();

	// initialize the level item heap
	InitLevelItemHeap();
	levelitems	  = NULL;
	numlevelitems = 0;
	//
	ic = itemconfig;

	if( !ic ) {
		return;
	}

	// if there's no AAS file loaded
	if( !AAS_Loaded() ) {
		return;
	}

	// update the modelindexes of the item info
	for( i = 0; i < ic->numiteminfo; i++ ) {
		// ic->iteminfo[i].modelindex = AAS_IndexFromModel(ic->iteminfo[i].model);
		if( !ic->iteminfo[i].modelindex ) {
			Log_Write( "item %s has modelindex 0", ic->iteminfo[i].classname );
		}
	}

	for( ent = AAS_NextBSPEntity( 0 ); ent; ent = AAS_NextBSPEntity( ent ) ) {
		if( !AAS_ValueForBSPEpairKey( ent, "classname", classname, MAX_EPAIRKEY ) ) {
			continue;
		}

		//
		spawnflags = 0;
		AAS_IntForBSPEpairKey( ent, "spawnflags", &spawnflags );

		// FIXME: don't do this
		//  for now skip all floating entities
		if( spawnflags & 1 ) {
			continue;
		}

		//
		for( i = 0; i < ic->numiteminfo; i++ ) {
			if( !strcmp( classname, ic->iteminfo[i].classname ) ) {
				// get the origin of the item
				if( AAS_VectorForBSPEpairKey( ent, "origin", origin ) ) {
					li = AllocLevelItem();

					if( !li ) {
						return;
					}

					//
					li->number	  = ++numlevelitems;
					li->timeout	  = 0;
					li->entitynum = 0;
					//
					AAS_IntForBSPEpairKey( ent, "notfree", &li->notfree );
					AAS_IntForBSPEpairKey( ent, "notteam", &li->notteam );
					AAS_IntForBSPEpairKey( ent, "notsingle", &li->notsingle );

					// if not a stationary item
					if( !( spawnflags & 1 ) ) {
						if( !AAS_DropToFloor( origin, ic->iteminfo[i].mins, ic->iteminfo[i].maxs ) ) {
							botimport.Print( PRT_MESSAGE, "%s in solid at (%1.1f %1.1f %1.1f)\n", classname, origin[0], origin[1], origin[2] );
						}
					}

					// item info of the level item
					li->iteminfo = i;
					// origin of the item
					VectorCopy( origin, li->origin );
					// get the item goal area and goal origin
					li->goalareanum = AAS_BestReachableArea( origin, ic->iteminfo[i].mins, ic->iteminfo[i].maxs, li->goalorigin );
					//
					AddLevelItemToList( li );

				} else {
					botimport.Print( PRT_ERROR, "item %s without origin\n", classname );
				}

				break;
			}
		}

		if( i >= ic->numiteminfo ) {
			Log_Write( "entity %s unknown item\r\n", classname );
		}
	}

	botimport.Print( PRT_MESSAGE, "found %d level items\n", numlevelitems );
}

void BotGoalName( int number, char* name, int size )
{
	levelitem_t* li;

	if( !itemconfig ) {
		return;
	}

	//
	for( li = levelitems; li; li = li->next ) {
		if( li->number == number ) {
			strncpy( name, itemconfig->iteminfo[li->iteminfo].name, size - 1 );
			name[size - 1] = '\0';
			return;
		}
	}

	strcpy( name, "" );
	return;
}

void BotResetAvoidGoals( int goalstate )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );

	if( !gs ) {
		return;
	}

	memset( gs->avoidgoals, 0, MAX_AVOIDGOALS * sizeof( int ) );
	memset( gs->avoidgoaltimes, 0, MAX_AVOIDGOALS * sizeof( float ) );
}

void BotDumpAvoidGoals( int goalstate )
{
	int				 i;
	bot_goalstate_t* gs;
	char			 name[32];

	gs = BotGoalStateFromHandle( goalstate );

	if( !gs ) {
		return;
	}

	for( i = 0; i < MAX_AVOIDGOALS; i++ ) {
		if( gs->avoidgoaltimes[i] >= AAS_Time() ) {
			BotGoalName( gs->avoidgoals[i], name, 32 );
			Log_Write( "avoid goal %s, number %d for %f seconds", name, gs->avoidgoals[i], gs->avoidgoaltimes[i] - AAS_Time() );
		}
	}
}

/*!
	\brief Adds an avoidance goal to the bot’s goal state and records its expiration time.

	The function iterates over the array of stored avoidance goals and finds the first slot whose timestamp has passed relative to the current time returned by AAS_Time(). Once found, it stores the
   provided goal number in that slot and sets its timeout to the current time plus the specified avoidtime. This marks the goal as one that the bot should avoid for the given period.

	\param gs Pointer to the bot’s goal state structure in which avoidance goals are tracked.
	\param number Identifier of the goal that the bot should avoid.
	\param avoidtime Duration in seconds that the goal should be avoided before it can be considered again.
*/
void BotAddToAvoidGoals( bot_goalstate_t* gs, int number, float avoidtime )
{
	int i;

	for( i = 0; i < MAX_AVOIDGOALS; i++ ) {
		// if this avoid goal has expired
		if( gs->avoidgoaltimes[i] < AAS_Time() ) {
			gs->avoidgoals[i]	  = number;
			gs->avoidgoaltimes[i] = AAS_Time() + avoidtime;
			return;
		}
	}
}

void BotRemoveFromAvoidGoals( int goalstate, int number )
{
	int				 i;
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );

	if( !gs ) {
		return;
	}

	// don't use the goals the bot wants to avoid
	for( i = 0; i < MAX_AVOIDGOALS; i++ ) {
		if( gs->avoidgoals[i] == number && gs->avoidgoaltimes[i] >= AAS_Time() ) {
			gs->avoidgoaltimes[i] = 0;
			return;
		}
	}
}

float BotAvoidGoalTime( int goalstate, int number )
{
	int				 i;
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );

	if( !gs ) {
		return 0;
	}

	// don't use the goals the bot wants to avoid
	for( i = 0; i < MAX_AVOIDGOALS; i++ ) {
		if( gs->avoidgoals[i] == number && gs->avoidgoaltimes[i] >= AAS_Time() ) {
			return gs->avoidgoaltimes[i] - AAS_Time();
		}
	}

	return 0;
}

/*!
	\brief Retrieves the goal data for a level item matching a specified classname and index, returning the item's number or -1 if not found.

	The function iterates over the global levelitems list starting with the first item whose number exceeds the supplied index. For each candidate it verifies that the item is appropriate to the
   current gametype: non‑single‑player, team or free play items are skipped as needed. When the classname matches the item’s name from itemconfig, the goal structure is populated with the item's area,
   origin, entity number, bounding boxes, and number, and that number is returned. If itemconfig is missing or no matching item is found, the function returns -1.

	\param index The index after which to start searching; items with numbers less than or equal to this value are ignored.
	\param name The classname to match against the level item's name.
	\param goal Pointer to a bot_goal_t structure that will receive the goal information for the matched item.
	\return The unique number of the matched level item, or -1 if no matching item is found.
*/
int BotGetLevelItemGoal( int index, char* name, bot_goal_t* goal )
{
	levelitem_t* li;

	if( !itemconfig ) {
		return -1;
	}

	for( li = levelitems; li; li = li->next ) {
		if( li->number <= index ) {
			continue;
		}

		//
		if( g_gametype == GT_SINGLE_PLAYER ) {
			if( li->notsingle ) {
				continue;
			}

		} else if( g_gametype >= GT_TEAM ) {
			if( li->notteam ) {
				continue;
			}

		} else {
			if( li->notfree ) {
				continue;
			}
		}

		//
		if( !Q_stricmp( name, itemconfig->iteminfo[li->iteminfo].name ) ) {
			goal->areanum = li->goalareanum;
			VectorCopy( li->goalorigin, goal->origin );
			goal->entitynum = li->entitynum;
			VectorCopy( itemconfig->iteminfo[li->iteminfo].mins, goal->mins );
			VectorCopy( itemconfig->iteminfo[li->iteminfo].maxs, goal->maxs );
			goal->number = li->number;
			// botimport.Print(PRT_MESSAGE, "found li %s\n", itemconfig->iteminfo[li->iteminfo].name);
			return li->number;
		}
	}

	return -1;
}

int BotGetMapLocationGoal( char* name, bot_goal_t* goal )
{
	maplocation_t* ml;
	vec3_t		   mins = { -8, -8, -8 }, maxs = { 8, 8, 8 };

	for( ml = maplocations; ml; ml = ml->next ) {
		if( !Q_stricmp( ml->name, name ) ) {
			goal->areanum = ml->areanum;
			VectorCopy( ml->origin, goal->origin );
			goal->entitynum = 0;
			VectorCopy( mins, goal->mins );
			VectorCopy( maxs, goal->maxs );
			return qtrue;
		}
	}

	return qfalse;
}

int BotGetNextCampSpotGoal( int num, bot_goal_t* goal )
{
	int			i;
	campspot_t* cs;
	vec3_t		mins = { -8, -8, -8 }, maxs = { 8, 8, 8 };

	if( num < 0 ) {
		num = 0;
	}

	i = num;

	for( cs = campspots; cs; cs = cs->next ) {
		if( --i < 0 ) {
			goal->areanum = cs->areanum;
			VectorCopy( cs->origin, goal->origin );
			goal->entitynum = 0;
			VectorCopy( mins, goal->mins );
			VectorCopy( maxs, goal->maxs );
			return num + 1;
		}
	}

	return 0;
}

// NOTE: enum entityType_t in bg_public.h
#define ET_ITEM 2

void BotUpdateEntityItems()
{
	int				 ent, i, modelindex;
	vec3_t			 dir;
	levelitem_t *	 li, *nextli;
	aas_entityinfo_t entinfo;
	itemconfig_t*	 ic;

	// timeout current entity items if necessary
	for( li = levelitems; li; li = nextli ) {
		nextli = li->next;

		// if it is a item that will time out
		if( li->timeout ) {
			// timeout the item
			if( li->timeout < AAS_Time() ) {
				RemoveLevelItemFromList( li );
				FreeLevelItem( li );
			}
		}
	}

	// find new entity items
	ic = itemconfig;

	if( !itemconfig ) {
		return;
	}

	//
	for( ent = AAS_NextEntity( 0 ); ent; ent = AAS_NextEntity( ent ) ) {
		if( AAS_EntityType( ent ) != ET_ITEM ) {
			continue;
		}

		// get the model index of the entity
		modelindex = AAS_EntityModelindex( ent );

		//
		if( !modelindex ) {
			continue;
		}

		// get info about the entity
		AAS_EntityInfo( ent, &entinfo );

		// FIXME: don't do this
		// skip all floating items for now
		if( entinfo.groundent != ENTITYNUM_WORLD ) {
			continue;
		}

		// if the entity is still moving
		if( entinfo.origin[0] != entinfo.lastvisorigin[0] || entinfo.origin[1] != entinfo.lastvisorigin[1] || entinfo.origin[2] != entinfo.lastvisorigin[2] ) {
			continue;
		}

		// check if the level item isn't already stored
		for( li = levelitems; li; li = li->next ) {
			// if the model of the level item and the entity are different
			if( ic->iteminfo[li->iteminfo].modelindex != modelindex ) {
				continue;
			}

			// if the level item is linked to an entity
			if( li->entitynum ) {
				if( li->entitynum == ent ) {
					VectorCopy( entinfo.origin, li->origin );
					break;
				}

			} else {
				// check if the entity is very close
				VectorSubtract( li->origin, entinfo.origin, dir );

				if( VectorLength( dir ) < 30 ) {
					// found an entity for this level item
					li->entitynum = ent;
					// keep updating the entity origin
					VectorCopy( entinfo.origin, li->origin );
					// also update the goal area number
					li->goalareanum = AAS_BestReachableArea( li->origin, ic->iteminfo[li->iteminfo].mins, ic->iteminfo[li->iteminfo].maxs, li->goalorigin );
					// Log_Write("found item %s entity", ic->iteminfo[li->iteminfo].classname);
					break;
				}

				// else botimport.Print(PRT_MESSAGE, "item %s has no attached entity\n",
				//						ic->iteminfo[li->iteminfo].name);
			}
		}

		if( li ) {
			continue;
		}

		// check if the model is from a known item
		for( i = 0; i < ic->numiteminfo; i++ ) {
			if( ic->iteminfo[i].modelindex == modelindex ) {
				break;
			}
		}

		// if the model is not from a known item
		if( i >= ic->numiteminfo ) {
			continue;
		}

		// allocate a new level item
		li = AllocLevelItem();

		//
		if( !li ) {
			continue;
		}

		// entity number of the level item
		li->entitynum = ent;
		// number for the level item
		li->number = numlevelitems + ent;
		// set the item info index for the level item
		li->iteminfo = i;
		// origin of the item
		VectorCopy( entinfo.origin, li->origin );
		// get the item goal area and goal origin
		li->goalareanum = AAS_BestReachableArea( li->origin, ic->iteminfo[i].mins, ic->iteminfo[i].maxs, li->goalorigin );

		//
		if( AAS_AreaJumpPad( li->goalareanum ) ) {
			FreeLevelItem( li );
			continue;
		}

		// time this item out after 30 seconds
		// dropped items disappear after 30 seconds
		li->timeout = AAS_Time() + 30;
		// add the level item to the list
		AddLevelItemToList( li );
		// botimport.Print(PRT_MESSAGE, "found new level item %s\n", ic->iteminfo[i].classname);
	}
}

void BotDumpGoalStack( int goalstate )
{
	int				 i;
	bot_goalstate_t* gs;
	char			 name[32];

	gs = BotGoalStateFromHandle( goalstate );

	if( !gs ) {
		return;
	}

	for( i = 1; i <= gs->goalstacktop; i++ ) {
		BotGoalName( gs->goalstack[i].number, name, 32 );
		Log_Write( "%d: %s", i, name );
	}
}

void BotPushGoal( int goalstate, bot_goal_t* goal )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );

	if( !gs ) {
		return;
	}

	if( gs->goalstacktop >= MAX_GOALSTACK - 1 ) {
		botimport.Print( PRT_ERROR, "goal heap overflow\n" );
		BotDumpGoalStack( goalstate );
		return;
	}

	gs->goalstacktop++;
	memcpy( &gs->goalstack[gs->goalstacktop], goal, sizeof( bot_goal_t ) );
}

void BotPopGoal( int goalstate )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );

	if( !gs ) {
		return;
	}

	if( gs->goalstacktop > 0 ) {
		gs->goalstacktop--;
	}
}

void BotEmptyGoalStack( int goalstate )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );

	if( !gs ) {
		return;
	}

	gs->goalstacktop = 0;
}

int BotGetTopGoal( int goalstate, bot_goal_t* goal )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );

	if( !gs ) {
		return qfalse;
	}

	if( !gs->goalstacktop ) {
		return qfalse;
	}

	memcpy( goal, &gs->goalstack[gs->goalstacktop], sizeof( bot_goal_t ) );
	return qtrue;
}

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotGetSecondGoal( int goalstate, bot_goal_t* goal )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );

	if( !gs ) {
		return qfalse;
	}

	if( gs->goalstacktop <= 1 ) {
		return qfalse;
	}

	memcpy( goal, &gs->goalstack[gs->goalstacktop - 1], sizeof( bot_goal_t ) );
	return qtrue;
}

//===========================================================================
// pops a new long term goal on the goal stack in the goalstate
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotChooseLTGItem( int goalstate, vec3_t origin, int* inventory, int travelflags )
{
	int				 areanum, t, weightnum;
	float			 weight, bestweight, avoidtime;
	iteminfo_t*		 iteminfo;
	itemconfig_t*	 ic;
	levelitem_t *	 li, *bestitem;
	bot_goal_t		 goal;
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );

	if( !gs ) {
		return qfalse;
	}

	if( !gs->itemweightconfig ) {
		return qfalse;
	}

	// get the area the bot is in
	areanum = BotReachabilityArea( origin, gs->client );

	// if the bot is in solid or if the area the bot is in has no reachability links
	if( !areanum || !AAS_AreaReachability( areanum ) ) {
		// use the last valid area the bot was in
		areanum = gs->lastreachabilityarea;
	}

	// remember the last area with reachabilities the bot was in
	gs->lastreachabilityarea = areanum;

	// if still in solid
	if( !areanum ) {
		return qfalse;
	}

	// the item configuration
	ic = itemconfig;

	if( !itemconfig ) {
		return qfalse;
	}

	// best weight and item so far
	bestweight = 0;
	bestitem   = NULL;
	memset( &goal, 0, sizeof( bot_goal_t ) );

	// go through the items in the level
	for( li = levelitems; li; li = li->next ) {
		if( g_gametype == GT_SINGLE_PLAYER ) {
			if( li->notsingle ) {
				continue;
			}

		} else if( g_gametype >= GT_TEAM ) {
			if( li->notteam ) {
				continue;
			}

		} else {
			if( li->notfree ) {
				continue;
			}
		}

		// if the item is not in a possible goal area
		if( !li->goalareanum ) {
			continue;
		}

		// get the fuzzy weight function for this item
		iteminfo  = &ic->iteminfo[li->iteminfo];
		weightnum = gs->itemweightindex[iteminfo->number];

		if( weightnum < 0 ) {
			continue;
		}

		// if this goal is in the avoid goals
		if( BotAvoidGoalTime( goalstate, li->number ) > 0 ) {
			continue;
		}

#ifdef UNDECIDEDFUZZY
		weight = FuzzyWeightUndecided( inventory, gs->itemweightconfig, weightnum );
#else
		weight = FuzzyWeight( inventory, gs->itemweightconfig, weightnum );
#endif // UNDECIDEDFUZZY
#ifdef DROPPEDWEIGHT

		// HACK: to make dropped items more attractive
		if( li->timeout ) {
			weight += 1000;
		}

#endif // DROPPEDWEIGHT

		if( weight > 0 ) {
			// get the travel time towards the goal area
			t = AAS_AreaTravelTimeToGoalArea( areanum, origin, li->goalareanum, travelflags );

			// if the goal is reachable
			if( t > 0 ) {
				weight /= ( float )t * TRAVELTIME_SCALE;

				//
				if( weight > bestweight ) {
					bestweight = weight;
					bestitem   = li;
				}
			}
		}
	}

	// if no goal item found
	if( !bestitem ) {
		/*
		//if not in lava or slime
		if (!AAS_AreaLava(areanum) && !AAS_AreaSlime(areanum))
		{
			if (AAS_RandomGoalArea(areanum, travelflags, &goal.areanum, goal.origin))
			{
				VectorSet(goal.mins, -15, -15, -15);
				VectorSet(goal.maxs, 15, 15, 15);
				goal.entitynum = 0;
				goal.number = 0;
				goal.flags = GFL_ROAM;
				goal.iteminfo = 0;
				//push the goal on the stack
				BotPushGoal(goalstate, &goal);
				//
		#ifdef DEBUG
				botimport.Print(PRT_MESSAGE, "chosen roam goal area %d\n", goal.areanum);
		#endif //DEBUG
				return qtrue;
			}
		}
		*/
		return qfalse;
	}

	// create a bot goal for this item
	iteminfo = &ic->iteminfo[bestitem->iteminfo];
	VectorCopy( bestitem->goalorigin, goal.origin );
	VectorCopy( iteminfo->mins, goal.mins );
	VectorCopy( iteminfo->maxs, goal.maxs );
	goal.areanum   = bestitem->goalareanum;
	goal.entitynum = bestitem->entitynum;
	goal.number	   = bestitem->number;
	goal.flags	   = GFL_ITEM;
	goal.iteminfo  = bestitem->iteminfo;
	// add the chosen goal to the goals to avoid for a while
	avoidtime = iteminfo->respawntime * 0.5;

	if( avoidtime < 10 ) {
		avoidtime = AVOID_TIME;
	}

	// if it's a dropped item
	if( bestitem->timeout ) {
		avoidtime = AVOIDDROPPED_TIME;
	}

	BotAddToAvoidGoals( gs, bestitem->number, avoidtime );
	// push the goal on the stack
	BotPushGoal( goalstate, &goal );
	//
#ifdef DEBUG_AI_GOAL

	if( bestitem->timeout ) {
		botimport.Print( PRT_MESSAGE, "new ltg dropped item %s\n", ic->iteminfo[bestitem->iteminfo].classname );
	}

	iteminfo = &ic->iteminfo[bestitem->iteminfo];
	botimport.Print( PRT_MESSAGE, "new ltg \"%s\"\n", iteminfo->classname );
#endif // DEBUG_AI_GOAL
	return qtrue;
}

int BotChooseNBGItem( int goalstate, vec3_t origin, int* inventory, int travelflags, bot_goal_t* ltg, float maxtime )
{
	int				 areanum, t, weightnum, ltg_time;
	float			 weight, bestweight, avoidtime;
	iteminfo_t*		 iteminfo;
	itemconfig_t*	 ic;
	levelitem_t *	 li, *bestitem;
	bot_goal_t		 goal;
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );

	if( !gs ) {
		return qfalse;
	}

	if( !gs->itemweightconfig ) {
		return qfalse;
	}

	// get the area the bot is in
	areanum = BotReachabilityArea( origin, gs->client );

	// if the bot is in solid or if the area the bot is in has no reachability links
	if( !areanum || !AAS_AreaReachability( areanum ) ) {
		// use the last valid area the bot was in
		areanum = gs->lastreachabilityarea;
	}

	// remember the last area with reachabilities the bot was in
	gs->lastreachabilityarea = areanum;

	// if still in solid
	if( !areanum ) {
		return qfalse;
	}

	//
	if( ltg ) {
		ltg_time = AAS_AreaTravelTimeToGoalArea( areanum, origin, ltg->areanum, travelflags );

	} else {
		ltg_time = 99999;
	}

	// the item configuration
	ic = itemconfig;

	if( !itemconfig ) {
		return qfalse;
	}

	// best weight and item so far
	bestweight = 0;
	bestitem   = NULL;
	memset( &goal, 0, sizeof( bot_goal_t ) );

	// go through the items in the level
	for( li = levelitems; li; li = li->next ) {
		if( g_gametype == GT_SINGLE_PLAYER ) {
			if( li->notsingle ) {
				continue;
			}

		} else if( g_gametype >= GT_TEAM ) {
			if( li->notteam ) {
				continue;
			}

		} else {
			if( li->notfree ) {
				continue;
			}
		}

		// if the item is in a possible goal area
		if( !li->goalareanum ) {
			continue;
		}

		// get the fuzzy weight function for this item
		iteminfo  = &ic->iteminfo[li->iteminfo];
		weightnum = gs->itemweightindex[iteminfo->number];

		if( weightnum < 0 ) {
			continue;
		}

		// if this goal is in the avoid goals
		if( BotAvoidGoalTime( goalstate, li->number ) > 0 ) {
			continue;
		}

		//
#ifdef UNDECIDEDFUZZY
		weight = FuzzyWeightUndecided( inventory, gs->itemweightconfig, weightnum );
#else
		weight = FuzzyWeight( inventory, gs->itemweightconfig, weightnum );
#endif // UNDECIDEDFUZZY
#ifdef DROPPEDWEIGHT

		// HACK: to make dropped items more attractive
		if( li->timeout ) {
			weight += 1000;
		}

#endif // DROPPEDWEIGHT

		if( weight > 0 ) {
			// get the travel time towards the goal area
			t = AAS_AreaTravelTimeToGoalArea( areanum, origin, li->goalareanum, travelflags );

			// if the goal is reachable
			if( t > 0 && t < maxtime ) {
				weight /= ( float )t * TRAVELTIME_SCALE;

				//
				if( weight > bestweight ) {
					t = 0;

					if( ltg && !li->timeout ) {
						// get the travel time from the goal to the long term goal
						t = AAS_AreaTravelTimeToGoalArea( li->goalareanum, li->goalorigin, ltg->areanum, travelflags );
					}

					// if the travel back is possible and doesn't take too long
					if( t <= ltg_time ) {
						bestweight = weight;
						bestitem   = li;
					}
				}
			}
		}
	}

	// if no goal item found
	if( !bestitem ) {
		return qfalse;
	}

	// create a bot goal for this item
	iteminfo = &ic->iteminfo[bestitem->iteminfo];
	VectorCopy( bestitem->goalorigin, goal.origin );
	VectorCopy( iteminfo->mins, goal.mins );
	VectorCopy( iteminfo->maxs, goal.maxs );
	goal.areanum   = bestitem->goalareanum;
	goal.entitynum = bestitem->entitynum;
	goal.number	   = bestitem->number;
	goal.flags	   = GFL_ITEM;
	goal.iteminfo  = bestitem->iteminfo;
	// add the chosen goal to the goals to avoid for a while
	avoidtime = iteminfo->respawntime * 0.5;

	if( avoidtime < 10 ) {
		avoidtime = AVOID_TIME;
	}

	// if it's a dropped item
	if( bestitem->timeout ) {
		avoidtime = AVOIDDROPPED_TIME;
	}

	BotAddToAvoidGoals( gs, bestitem->number, avoidtime );
	// push the goal on the stack
	BotPushGoal( goalstate, &goal );
	//
#ifdef DEBUG_AI_GOAL

	if( bestitem->timeout ) {
		botimport.Print( PRT_MESSAGE, "new nbg dropped item %s\n", ic->iteminfo[bestitem->iteminfo].classname );
	}

	iteminfo = &ic->iteminfo[bestitem->iteminfo];
	botimport.Print( PRT_MESSAGE, "new nbg \"%s\"\n", iteminfo->classname );
#endif // DEBUG_AI_GOAL
	return qtrue;
}

int BotTouchingGoal( vec3_t origin, bot_goal_t* goal )
{
	int	   i;
	vec3_t boxmins, boxmaxs;
	vec3_t absmins, absmaxs;
	vec3_t safety_maxs = { 0, 0, 0 }; //{4, 4, 10};
	vec3_t safety_mins = { 0, 0, 0 }; //{-4, -4, 0};

	AAS_PresenceTypeBoundingBox( PRESENCE_NORMAL, boxmins, boxmaxs );
	VectorSubtract( goal->mins, boxmaxs, absmins );
	VectorSubtract( goal->maxs, boxmins, absmaxs );
	VectorAdd( absmins, goal->origin, absmins );
	VectorAdd( absmaxs, goal->origin, absmaxs );
	// make the box a little smaller for safety
	VectorSubtract( absmaxs, safety_maxs, absmaxs );
	VectorSubtract( absmins, safety_mins, absmins );

	for( i = 0; i < 3; i++ ) {
		if( origin[i] < absmins[i] || origin[i] > absmaxs[i] ) {
			return qfalse;
		}
	}

	return qtrue;
}

int BotItemGoalInVisButNotVisible( int viewer, vec3_t eye, vec3_t viewangles, bot_goal_t* goal )
{
	aas_entityinfo_t entinfo;
	bsp_trace_t		 trace;
	vec3_t			 middle;

	if( !( goal->flags & GFL_ITEM ) ) {
		return qfalse;
	}

	//
	VectorAdd( goal->mins, goal->mins, middle );
	VectorScale( middle, 0.5, middle );
	VectorAdd( goal->origin, middle, middle );
	//
	trace = AAS_Trace( eye, NULL, NULL, middle, viewer, CONTENTS_SOLID );

	// if the goal middle point is visible
	if( trace.fraction >= 1 ) {
		// the goal entity number doesn't have to be valid
		// just assume it's valid
		if( goal->entitynum <= 0 ) {
			return qfalse;
		}

		//
		// if the entity data isn't valid
		AAS_EntityInfo( goal->entitynum, &entinfo );

		// NOTE: for some wacko reason entities are sometimes
		//  not updated
		// if (!entinfo.valid) return qtrue;
		if( entinfo.ltime < AAS_Time() - 0.5 ) {
			return qtrue;
		}
	}

	return qfalse;
}

void BotResetGoalState( int goalstate )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );

	if( !gs ) {
		return;
	}

	memset( gs->goalstack, 0, MAX_GOALSTACK * sizeof( bot_goal_t ) );
	gs->goalstacktop = 0;
	BotResetAvoidGoals( goalstate );
}

int BotLoadItemWeights( int goalstate, char* filename )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );

	if( !gs ) {
		return BLERR_CANNOTLOADITEMWEIGHTS;
	}

	// load the weight configuration
	gs->itemweightconfig = ReadWeightConfig( filename );

	if( !gs->itemweightconfig ) {
		botimport.Print( PRT_FATAL, "couldn't load weights\n" );
		return BLERR_CANNOTLOADITEMWEIGHTS;
	}

	// if there's no item configuration
	if( !itemconfig ) {
		return BLERR_CANNOTLOADITEMWEIGHTS;
	}

	// create the item weight index
	gs->itemweightindex = ItemWeightIndex( gs->itemweightconfig, itemconfig );
	// everything went ok
	return BLERR_NOERROR;
}

void BotFreeItemWeights( int goalstate )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );

	if( !gs ) {
		return;
	}

	if( gs->itemweightconfig ) {
		FreeWeightConfig( gs->itemweightconfig );
	}

	if( gs->itemweightindex ) {
		FreeMemory( gs->itemweightindex );
	}
}

int BotAllocGoalState( int client )
{
	int i;

	for( i = 1; i <= MAX_CLIENTS; i++ ) {
		if( !botgoalstates[i] ) {
			botgoalstates[i]		 = ( bot_goalstate_t* )GetClearedMemory( sizeof( bot_goalstate_t ) );
			botgoalstates[i]->client = client;
			return i;
		}
	}

	return 0;
}

void BotFreeGoalState( int handle )
{
	if( handle <= 0 || handle > MAX_CLIENTS ) {
		botimport.Print( PRT_FATAL, "goal state handle %d out of range\n", handle );
		return;
	}

	if( !botgoalstates[handle] ) {
		botimport.Print( PRT_FATAL, "invalid goal state handle %d\n", handle );
		return;
	}

	BotFreeItemWeights( handle );
	FreeMemory( botgoalstates[handle] );
	botgoalstates[handle] = NULL;
}

int BotSetupGoalAI()
{
	char* filename;

	// check if teamplay is on
	g_gametype = LibVarValue( "g_gametype", "0" );
	// item configuration file
	filename = LibVarString( "itemconfig", "items.c" );
	// load the item configuration
	itemconfig = LoadItemConfig( filename );

	if( !itemconfig ) {
		botimport.Print( PRT_FATAL, "couldn't load item config\n" );
		return BLERR_CANNOTLOADITEMCONFIG;
	}

	// everything went ok
	return BLERR_NOERROR;
}

void BotShutdownGoalAI()
{
	int i;

	if( itemconfig ) {
		FreeMemory( itemconfig );
	}

	itemconfig = NULL;

	if( levelitemheap ) {
		FreeMemory( levelitemheap );
	}

	levelitemheap  = NULL;
	freelevelitems = NULL;
	levelitems	   = NULL;
	numlevelitems  = 0;

	BotFreeInfoEntities();

	for( i = 1; i <= MAX_CLIENTS; i++ ) {
		if( botgoalstates[i] ) {
			BotFreeGoalState( i );
		}
	}
}
