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
 * name:		be_ai_weap.c
 *
 * desc:		weapon AI
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_libvar.h"
#include "l_log.h"
#include "l_memory.h"
#include "l_utils.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "botshared/botlib.h"
#include "botshared/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_ai_weight.h" //fuzzy weights
#include "botshared/be_ai_weap.h"

// #define DEBUG_AI_WEAP

// structure field offsets
#define WEAPON_OFS( x )		( int )&( ( ( weaponinfo_t* )0 )->x )
#define PROJECTILE_OFS( x ) ( int )&( ( ( projectileinfo_t* )0 )->x )

// weapon definition
fielddef_t	weaponinfo_fields[] = { { "number", WEAPON_OFS( number ), FT_INT }, // weapon number
	 { "name", WEAPON_OFS( name ), FT_STRING },									// name of the weapon
	 { "level", WEAPON_OFS( level ), FT_INT },
	 { "model", WEAPON_OFS( model ), FT_STRING },						   // model of the weapon
	 { "weaponindex", WEAPON_OFS( weaponindex ), FT_INT },				   // index of weapon in inventory
	 { "flags", WEAPON_OFS( flags ), FT_INT },							   // special flags
	 { "projectile", WEAPON_OFS( projectile ), FT_STRING },				   // projectile used by the weapon
	 { "numprojectiles", WEAPON_OFS( numprojectiles ), FT_INT },		   // number of projectiles
	 { "hspread", WEAPON_OFS( hspread ), FT_FLOAT },					   // horizontal spread of projectiles (degrees from middle)
	 { "vspread", WEAPON_OFS( vspread ), FT_FLOAT },					   // vertical spread of projectiles (degrees from middle)
	 { "speed", WEAPON_OFS( speed ), FT_FLOAT },						   // speed of the projectile (0 = instant hit)
	 { "acceleration", WEAPON_OFS( acceleration ), FT_FLOAT },			   //"acceleration" * time (in seconds) + "speed" = projectile speed
	 { "recoil", WEAPON_OFS( recoil ), FT_FLOAT | FT_ARRAY, 3 },		   // amount of recoil the player gets from the weapon
	 { "offset", WEAPON_OFS( offset ), FT_FLOAT | FT_ARRAY, 3 },		   // projectile start offset relative to eye and view angles
	 { "angleoffset", WEAPON_OFS( angleoffset ), FT_FLOAT | FT_ARRAY, 3 }, // offset of the shoot angles relative to the view angles
	 { "extrazvelocity", WEAPON_OFS( extrazvelocity ), FT_FLOAT },		   // extra z velocity the projectile gets
	 { "ammoamount", WEAPON_OFS( ammoamount ), FT_INT },				   // ammo amount used per shot
	 { "ammoindex", WEAPON_OFS( ammoindex ), FT_INT },					   // index of ammo in inventory
	 { "activate", WEAPON_OFS( activate ), FT_FLOAT },					   // time it takes to select the weapon
	 { "reload", WEAPON_OFS( reload ), FT_FLOAT },						   // time it takes to reload the weapon
	 { "spinup", WEAPON_OFS( spinup ), FT_FLOAT },						   // time it takes before first shot
	 { "spindown", WEAPON_OFS( spindown ), FT_FLOAT },					   // time it takes before weapon stops firing
	 { NULL, 0, 0, 0 } };

// projectile definition
fielddef_t	projectileinfo_fields[] = { { "name", PROJECTILE_OFS( name ), FT_STRING }, // name of the projectile
	 { "model", WEAPON_OFS( model ), FT_STRING },									   // model of the projectile
	 { "flags", PROJECTILE_OFS( flags ), FT_INT },									   // special flags
	 { "gravity", PROJECTILE_OFS( gravity ), FT_FLOAT },							   // amount of gravity applied to the projectile [0,1]
	 { "damage", PROJECTILE_OFS( damage ), FT_INT },								   // damage of the projectile
	 { "radius", PROJECTILE_OFS( radius ), FT_FLOAT },								   // radius of damage
	 { "visdamage", PROJECTILE_OFS( visdamage ), FT_INT },							   // damage of the projectile to visible entities
	 { "damagetype", PROJECTILE_OFS( damagetype ), FT_INT },						   // type of damage (combination of the DAMAGETYPE_? flags)
	 { "healthinc", PROJECTILE_OFS( healthinc ), FT_INT },							   // health increase the owner gets
	 { "push", PROJECTILE_OFS( push ), FT_FLOAT },									   // amount a player is pushed away from the projectile impact
	 { "detonation", PROJECTILE_OFS( detonation ), FT_FLOAT },						   // time before projectile explodes after fire pressed
	 { "bounce", PROJECTILE_OFS( bounce ), FT_FLOAT },								   // amount the projectile bounces
	 { "bouncefric", PROJECTILE_OFS( bouncefric ), FT_FLOAT },						   // amount the bounce decreases per bounce
	 { "bouncestop", PROJECTILE_OFS( bouncestop ), FT_FLOAT },						   // minimum bounce value before bouncing stops
	 // recurive projectile definition??
	 { NULL, 0, 0, 0 } };

structdef_t weaponinfo_struct	  = { sizeof( weaponinfo_t ), weaponinfo_fields };
structdef_t projectileinfo_struct = { sizeof( projectileinfo_t ), projectileinfo_fields };

// weapon configuration: set of weapons with projectiles
typedef struct weaponconfig_s {
	int				  numweapons;
	int				  numprojectiles;
	projectileinfo_t* projectileinfo;
	weaponinfo_t*	  weaponinfo;
} weaponconfig_t;

// the bot weapon state
typedef struct bot_weaponstate_s {
	struct weightconfig_s* weaponweightconfig; // weapon weight configuration
	int*				   weaponweightindex;  // weapon weight index
} bot_weaponstate_t;

bot_weaponstate_t* botweaponstates[MAX_CLIENTS + 1];
weaponconfig_t*	   weaponconfig;

/*!
	\brief Checks if the supplied weapon number lies within the valid range defined by weaponconfig and reports an error if it does not.

	The function examines the global weapon configuration to see if the provided weapon number is greater than zero and not larger than the number of weapons available. If the number is out of range
   it prints an error message via botimport.Print and returns qfalse; otherwise it returns qtrue. The return value is interpreted as a boolean, where non‑zero indicates a valid weapon number.

	\param weaponnum The 1‑based index of the weapon to validate.
	\return An integer indicating validity: non‑zero if the weapon number is valid, zero if it is out of range.
*/
int				   BotValidWeaponNumber( int weaponnum )
{
	if( weaponnum <= 0 || weaponnum > weaponconfig->numweapons ) {
		botimport.Print( PRT_ERROR, "weapon number out of range\n" );
		return qfalse;
	}

	return qtrue;
}

/*!
	\brief Returns the weapon state for a given handle, or null if the handle is invalid.

	The function checks that the handle is within the valid range of 1 to MAX_CLIENTS, inclusive. If the handle is out of range or the corresponding entry in the botweaponstates array is null, a fatal
   message is printed and the function returns null. Otherwise it returns the pointer stored in botweaponstates at the given index.

	\param handle An integer identifier for a bot weapon state, expected to be between 1 and MAX_CLIENTS inclusive.
	\return Pointer to a bot_weaponstate_t instance, or null when the handle is out of bounds or uninitialized.
*/
bot_weaponstate_t* BotWeaponStateFromHandle( int handle )
{
	if( handle <= 0 || handle > MAX_CLIENTS ) {
		botimport.Print( PRT_FATAL, "move state handle %d out of range\n", handle );
		return NULL;
	}

	if( !botweaponstates[handle] ) {
		botimport.Print( PRT_FATAL, "invalid move state %d\n", handle );
		return NULL;
	}

	return botweaponstates[handle];
}

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
#ifdef DEBUG_AI_WEAP
void DumpWeaponConfig( weaponconfig_t* wc )
{
	FILE* fp;
	int	  i;

	fp = Log_FileStruct();

	if( !fp ) {
		return;
	}

	for( i = 0; i < wc->numprojectiles; i++ ) {
		WriteStructure( fp, &projectileinfo_struct, ( char* )&wc->projectileinfo[i] );
		Log_Flush();
	}

	for( i = 0; i < wc->numweapons; i++ ) {
		WriteStructure( fp, &weaponinfo_struct, ( char* )&wc->weaponinfo[i] );
		Log_Flush();
	}
}

#endif // DEBUG_AI_WEAP

/*!
	\brief Loads a weapon configuration file, parses its contents into a weaponconfig_t structure, and returns a pointer to it.

	The function first obtains the limits for weapons and projectiles, defaulting to 32 if unset or negative, and corrects any out‑of‑range values. It copies the provided filename into a local
   buffer and attempts to load the file via LoadSourceFile. If the file cannot be opened the function logs an error and returns NULL.

	Memory for a weaponconfig_t structure and contiguous arrays of weaponinfo_t and projectileinfo_t entries is allocated from the hunk allocator. The file is parsed token by token. For a
   "weaponinfo" token a structure is read, its number is validated against the maximum, and it is copied into the weaponinfo array where the entry is marked as valid. For a "projectileinfo" token
   the array is filled similarly, ensuring the number of projectiles does not exceed the limit.

	When an unexpected token occurs the function cleans up any allocated memory, frees the source, logs an error, and returns NULL.

	After parsing, the function validates each weapon entry that was marked as valid: it must have a non‑empty name and a non‑empty projectile string. For each such weapon it searches the
   projectile array for a matching projectile name and copies that projectile’s data into the weapon’s proj field. If the referenced projectile is missing, or if any other validation fails, the
   function cleans up and returns NULL.

	If no weapon entries were loaded a warning is logged; otherwise a message indicating success is printed. The parsed configuration structure is returned to the caller, which is responsible for
   freeing it with FreeMemory.


	\param filename Path to the weapon configuration file to load.
	\return A pointer to the allocated weaponconfig_t structure, or NULL if loading or validation fails.
*/
weaponconfig_t* LoadWeaponConfig( char* filename )
{
	int				max_weaponinfo, max_projectileinfo;
	token_t			token;
	char			path[MAX_PATH];
	int				i, j;
	source_t*		source;
	weaponconfig_t* wc;
	weaponinfo_t	weaponinfo;

	max_weaponinfo = ( int )LibVarValue( "max_weaponinfo", "32" );

	if( max_weaponinfo < 0 ) {
		botimport.Print( PRT_ERROR, "max_weaponinfo = %d\n", max_weaponinfo );
		max_weaponinfo = 32;
		LibVarSet( "max_weaponinfo", "32" );
	}

	max_projectileinfo = ( int )LibVarValue( "max_projectileinfo", "32" );

	if( max_projectileinfo < 0 ) {
		botimport.Print( PRT_ERROR, "max_projectileinfo = %d\n", max_projectileinfo );
		max_projectileinfo = 32;
		LibVarSet( "max_projectileinfo", "32" );
	}

	strncpy( path, filename, MAX_PATH );
	source = LoadSourceFile( path );

	if( !source ) {
		botimport.Print( PRT_ERROR, "counldn't load %s\n", path );
		return NULL;
	}

	// initialize weapon config
	wc				   = ( weaponconfig_t* )GetClearedHunkMemory( sizeof( weaponconfig_t ) + max_weaponinfo * sizeof( weaponinfo_t ) + max_projectileinfo * sizeof( projectileinfo_t ) );
	wc->weaponinfo	   = ( weaponinfo_t* )( ( char* )wc + sizeof( weaponconfig_t ) );
	wc->projectileinfo = ( projectileinfo_t* )( ( char* )wc->weaponinfo + max_weaponinfo * sizeof( weaponinfo_t ) );
	wc->numweapons	   = max_weaponinfo;
	wc->numprojectiles = 0;

	// parse the source file
	while( PC_ReadToken( source, &token ) ) {
		if( !strcmp( token.string, "weaponinfo" ) ) {
			memset( &weaponinfo, 0, sizeof( weaponinfo_t ) );

			if( !ReadStructure( source, &weaponinfo_struct, ( char* )&weaponinfo ) ) {
				FreeMemory( wc );
				FreeSource( source );
				return NULL;
			}

			if( weaponinfo.number < 0 || weaponinfo.number >= max_weaponinfo ) {
				botimport.Print( PRT_ERROR, "weapon info number %d out of range in %s\n", weaponinfo.number, path );
				FreeMemory( wc );
				FreeSource( source );
				return NULL;
			}

			memcpy( &wc->weaponinfo[weaponinfo.number], &weaponinfo, sizeof( weaponinfo_t ) );
			wc->weaponinfo[weaponinfo.number].valid = qtrue;

		} else if( !strcmp( token.string, "projectileinfo" ) ) {
			if( wc->numprojectiles >= max_projectileinfo ) {
				botimport.Print( PRT_ERROR, "more than %d projectiles defined in %s\n", max_projectileinfo, path );
				FreeMemory( wc );
				FreeSource( source );
				return NULL;
			}

			memset( &wc->projectileinfo[wc->numprojectiles], 0, sizeof( projectileinfo_t ) );

			if( !ReadStructure( source, &projectileinfo_struct, ( char* )&wc->projectileinfo[wc->numprojectiles] ) ) {
				FreeMemory( wc );
				FreeSource( source );
				return NULL;
			}

			wc->numprojectiles++;

		} else {
			botimport.Print( PRT_ERROR, "unknown definition %s in %s\n", token.string, path );
			FreeMemory( wc );
			FreeSource( source );
			return NULL;
		}
	}

	FreeSource( source );

	// fix up weapons
	for( i = 0; i < wc->numweapons; i++ ) {
		if( !wc->weaponinfo[i].valid ) {
			continue;
		}

		if( !wc->weaponinfo[i].name[0] ) {
			botimport.Print( PRT_ERROR, "weapon %d has no name in %s\n", i, path );
			FreeMemory( wc );
			return NULL;
		}

		if( !wc->weaponinfo[i].projectile[0] ) {
			botimport.Print( PRT_ERROR, "weapon %s has no projectile in %s\n", wc->weaponinfo[i].name, path );
			FreeMemory( wc );
			return NULL;
		}

		// find the projectile info and copy it to the weapon info
		for( j = 0; j < wc->numprojectiles; j++ ) {
			if( !strcmp( wc->projectileinfo[j].name, wc->weaponinfo[i].projectile ) ) {
				memcpy( &wc->weaponinfo[i].proj, &wc->projectileinfo[j], sizeof( projectileinfo_t ) );
				break;
			}
		}

		if( j == wc->numprojectiles ) {
			botimport.Print( PRT_ERROR, "weapon %s uses undefined projectile in %s\n", wc->weaponinfo[i].name, path );
			FreeMemory( wc );
			return NULL;
		}
	}

	if( !wc->numweapons ) {
		botimport.Print( PRT_WARNING, "no weapon info loaded\n" );
	}

	botimport.Print( PRT_MESSAGE, "loaded %s\n", path );
	return wc;
}

/*!
	\brief Creates an array of weapon weight indices based on the provided weight and weapon configurations.

	This function allocates an integer array sized to the number of weapons in the weapon configuration. For each weapon it calls FindFuzzyWeight with the weight configuration and the weapon's name to
   obtain an index representing that weapon's weight. The resulting array is returned, with each position corresponding to the weapon index in the weapon configuration.

	\param wwc pointer to the weight configuration used to determine weapon weights
	\param wc pointer to the weapon configuration that provides the list of weapons and their names
	\return pointer to newly allocated integer array of size wc->numweapons containing the matched weight indices
*/
int* WeaponWeightIndex( weightconfig_t* wwc, weaponconfig_t* wc )
{
	int *index, i;

	// initialize item weight index
	index = ( int* )GetClearedMemory( sizeof( int ) * wc->numweapons );

	for( i = 0; i < wc->numweapons; i++ ) {
		index[i] = FindFuzzyWeight( wwc, wc->weaponinfo[i].name );
	}

	return index;
}

/*!
	\brief Frees weapon weight configuration and index data for the specified bot weapon state handle

	The function obtains a bot weapon state structure from its handle. If the handle is invalid it simply returns. When valid, it deallocates the weight configuration if it exists, and then frees the
   weight index array, ensuring no memory leaks remain for that weapon state.

	\param weaponstate Integer handle referencing a bot weapon state whose data should be released
*/
void BotFreeWeaponWeights( int weaponstate )
{
	bot_weaponstate_t* ws;

	ws = BotWeaponStateFromHandle( weaponstate );

	if( !ws ) {
		return;
	}

	if( ws->weaponweightconfig ) {
		FreeWeightConfig( ws->weaponweightconfig );
	}

	if( ws->weaponweightindex ) {
		FreeMemory( ws->weaponweightindex );
	}
}

int BotLoadWeaponWeights( int weaponstate, char* filename )
{
	bot_weaponstate_t* ws;

	ws = BotWeaponStateFromHandle( weaponstate );

	if( !ws ) {
		return BLERR_CANNOTLOADWEAPONWEIGHTS;
	}

	BotFreeWeaponWeights( weaponstate );
	//
	ws->weaponweightconfig = ReadWeightConfig( filename );

	if( !ws->weaponweightconfig ) {
		botimport.Print( PRT_FATAL, "couldn't load weapon config %s\n", filename );
		return BLERR_CANNOTLOADWEAPONWEIGHTS;
	}

	if( !weaponconfig ) {
		return BLERR_CANNOTLOADWEAPONCONFIG;
	}

	ws->weaponweightindex = WeaponWeightIndex( ws->weaponweightconfig, weaponconfig );
	return BLERR_NOERROR;
}

void BotGetWeaponInfo( int weaponstate, int weapon, weaponinfo_t* weaponinfo )
{
	bot_weaponstate_t* ws;

	if( !BotValidWeaponNumber( weapon ) ) {
		return;
	}

	ws = BotWeaponStateFromHandle( weaponstate );

	if( !ws ) {
		return;
	}

	if( !weaponconfig ) {
		return;
	}

	memcpy( weaponinfo, &weaponconfig->weaponinfo[weapon], sizeof( weaponinfo_t ) );
}

int BotChooseBestFightWeapon( int weaponstate, int* inventory )
{
	int				   i, index, bestweapon;
	float			   weight, bestweight;
	weaponconfig_t*	   wc;
	bot_weaponstate_t* ws;

	ws = BotWeaponStateFromHandle( weaponstate );

	if( !ws ) {
		return 0;
	}

	wc = weaponconfig;

	if( !weaponconfig ) {
		return 0;
	}

	// if the bot has no weapon weight configuration
	if( !ws->weaponweightconfig ) {
		return 0;
	}

	bestweight = 0;
	bestweapon = 0;

	for( i = 0; i < wc->numweapons; i++ ) {
		if( !wc->weaponinfo[i].valid ) {
			continue;
		}

		index = ws->weaponweightindex[i];

		if( index < 0 ) {
			continue;
		}

		weight = FuzzyWeight( inventory, ws->weaponweightconfig, index );

		if( weight > bestweight ) {
			bestweight = weight;
			bestweapon = i;
		}
	}

	return bestweapon;
}

void BotResetWeaponState( int weaponstate )
{
	struct weightconfig_s* weaponweightconfig;
	int*				   weaponweightindex;
	bot_weaponstate_t*	   ws;

	ws = BotWeaponStateFromHandle( weaponstate );

	if( !ws ) {
		return;
	}

	weaponweightconfig = ws->weaponweightconfig;
	weaponweightindex  = ws->weaponweightindex;

	// memset(ws, 0, sizeof(bot_weaponstate_t));
	ws->weaponweightconfig = weaponweightconfig;
	ws->weaponweightindex  = weaponweightindex;
}

int BotAllocWeaponState()
{
	int i;

	for( i = 1; i <= MAX_CLIENTS; i++ ) {
		if( !botweaponstates[i] ) {
			botweaponstates[i] = ( bot_weaponstate_t* )GetClearedMemory( sizeof( bot_weaponstate_t ) );
			return i;
		}
	}

	return 0;
}

/*!
	\brief Releases the allocated resources for a specific weapon state handle

	The function first verifies that the supplied handle is within the valid client range and that a state exists for that handle. If either check fails, a fatal error message is printed and execution
   returns immediately. For a valid handle, it first frees any weapon weights associated with the state by calling BotFreeWeaponWeights, then deallocates the memory for the state itself and clears its
   pointer in the global array.

	\param handle The identifier of the weapon state to free, which must be a valid, allocated handle.
*/
void BotFreeWeaponState( int handle )
{
	if( handle <= 0 || handle > MAX_CLIENTS ) {
		botimport.Print( PRT_FATAL, "move state handle %d out of range\n", handle );
		return;
	}

	if( !botweaponstates[handle] ) {
		botimport.Print( PRT_FATAL, "invalid move state %d\n", handle );
		return;
	}

	BotFreeWeaponWeights( handle );
	FreeMemory( botweaponstates[handle] );
	botweaponstates[handle] = NULL;
}

int BotSetupWeaponAI()
{
	char* file;

	file		 = LibVarString( "weaponconfig", "weapons.c" );
	weaponconfig = LoadWeaponConfig( file );

	if( !weaponconfig ) {
		botimport.Print( PRT_FATAL, "couldn't load the weapon config\n" );
		return BLERR_CANNOTLOADWEAPONCONFIG;
	}

#ifdef DEBUG_AI_WEAP
	DumpWeaponConfig( weaponconfig );
#endif // DEBUG_AI_WEAP
	//
	return BLERR_NOERROR;
}

void BotShutdownWeaponAI()
{
	int i;

	if( weaponconfig ) {
		FreeMemory( weaponconfig );
	}

	weaponconfig = NULL;

	for( i = 1; i <= MAX_CLIENTS; i++ ) {
		if( botweaponstates[i] ) {
			BotFreeWeaponState( i );
		}
	}
}
