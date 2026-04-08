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
 * name:		be_ai_weap.h
 *
 * desc:		weapon AI
 *
 *
 *****************************************************************************/

// projectile flags
#define PFL_WINDOWDAMAGE   1 // projectile damages through window
#define PFL_RETURN		   2 // set when projectile returns to owner
// weapon flags
#define WFL_FIRERELEASED   1 // set when projectile is fired with key-up event
// damage types
#define DAMAGETYPE_IMPACT  1 // damage on impact
#define DAMAGETYPE_RADIAL  2 // radial damage
#define DAMAGETYPE_VISIBLE 4 // damage to all entities visible to the projectile

typedef struct projectileinfo_s {
	char  name[MAX_STRINGFIELD];
	char  model[MAX_STRINGFIELD];
	int	  flags;
	float gravity;
	int	  damage;
	float radius;
	int	  visdamage;
	int	  damagetype;
	int	  healthinc;
	float push;
	float detonation;
	float bounce;
	float bouncefric;
	float bouncestop;
} projectileinfo_t;

typedef struct weaponinfo_s {
	int				 valid;	 // true if the weapon info is valid
	int				 number; // number of the weapon
	char			 name[MAX_STRINGFIELD];
	char			 model[MAX_STRINGFIELD];
	int				 level;
	int				 weaponindex;
	int				 flags;
	char			 projectile[MAX_STRINGFIELD];
	int				 numprojectiles;
	float			 hspread;
	float			 vspread;
	float			 speed;
	float			 acceleration;
	vec3_t			 recoil;
	vec3_t			 offset;
	vec3_t			 angleoffset;
	float			 extrazvelocity;
	int				 ammoamount;
	int				 ammoindex;
	float			 activate;
	float			 reload;
	float			 spinup;
	float			 spindown;
	projectileinfo_t proj; // pointer to the used projectile
} weaponinfo_t;

/*!
	\brief Initializes the weapon AI configuration for bot entities

	Sets up the weapon AI system by loading the weapon configuration file and performing necessary initialization steps. The function loads weapon data from a configuration file specified by the
   libvar "weaponconfig" with a default of "weapons.c". If the configuration cannot be loaded, it prints a fatal error and returns an error code. If debugging is enabled, it dumps the loaded weapon
   configuration for inspection. The function is typically called during the bot library initialization process before other AI systems are set up.

	\return Error code indicating success or failure of the weapon AI setup process
*/
int	 BotSetupWeaponAI();

/*!
	\brief Shuts down the weapon AI system by freeing all allocated memory and resetting states

	This function cleans up the weapon AI subsystem by freeing the global weapon configuration memory and iterating through all client slots to free individual weapon states. It is designed to be
   called during bot library shutdown to ensure proper cleanup of all weapon AI related resources

*/
void BotShutdownWeaponAI();

/*!
	\brief Returns the best fighting weapon for a bot based on inventory and weapon state.

	This function evaluates the bot's inventory against weapon weight configurations to determine the most suitable weapon for combat. It iterates through all valid weapons, calculates a fuzzy weight
   for each based on the inventory and weapon state, and returns the index of the weapon with the highest weight. If no valid weapon configuration is found, it returns 0.

	\param weaponstate The handle to the bot's current weapon state configuration
	\param inventory Pointer to an array representing the bot's current inventory items
	\return Index of the best weapon to use for fighting, or 0 if no suitable weapon is found
*/
int	 BotChooseBestFightWeapon( int weaponstate, int* inventory );

/*!
	\brief Retrieves information about a specified weapon for a bot based on the weapon state and weapon number.

	This function fetches weapon information for a bot by using the provided weapon state and weapon number. It first validates the weapon number and checks if the weapon state and weapon
   configuration are valid. If all validations pass, it copies the weapon information from the configuration into the provided weaponinfo structure. The function does not perform any error handling
   beyond validation checks.

	\param weaponstate The handle or identifier for the bot's weapon state
	\param weapon The weapon number to retrieve information for
	\param weaponinfo Pointer to a structure where the weapon information will be copied
*/
void BotGetWeaponInfo( int weaponstate, int weapon, weaponinfo_t* weaponinfo );

/*!
	\brief Loads weapon weight configurations for a specified bot weapon state from a file

	This function initializes or updates the weapon weight configuration for a bot weapon state by loading data from a specified file. It first retrieves the weapon state structure, then frees any
   existing weapon weights, and attempts to read the new configuration from the provided filename. If the configuration file cannot be loaded or if the weapon configuration is invalid, the function
   returns an appropriate error code. The function is typically called during bot initialization or when switching weapon configurations during gameplay.

	\param weaponstate Handle to the bot weapon state to load weights into
	\param filename Path to the weapon weight configuration file to load
	\return Error code indicating success or failure of the operation, where BLERR_NOERROR indicates success and other values indicate specific types of failures
*/
int	 BotLoadWeaponWeights( int weaponstate, char* filename );

/*!
	\brief Allocates and returns a new weapon state handle for bot AI systems

	This function allocates a new weapon state structure for use in bot AI systems. It searches through the available bot weapon state slots, which are indexed from 1 to MAX_CLIENTS. When it finds an
   unused slot, it initializes the slot with a cleared memory allocation of the size required for a bot_weaponstate_t structure and returns the corresponding index. If all slots are occupied, it
   returns 0 to indicate failure. The function is typically called during bot initialization or when a new bot needs to manage weapon states independently.

	\return A handle to the newly allocated weapon state, or 0 if no slots are available
*/
int	 BotAllocWeaponState();

/*!
	\brief Frees the memory allocated for a bot weapon state identified by the given handle

	This function releases the memory associated with a bot weapon state structure. It first validates the handle to ensure it is within the valid range of client indices. If the handle is invalid or
   the weapon state has not been allocated, it logs a fatal error and returns without taking action. Otherwise, it frees the weapon weights associated with the state, then frees the memory allocated
   for the weapon state structure itself and sets the corresponding entry in the global array to NULL

	\param weaponstate Handle to the bot weapon state to be freed
*/
void BotFreeWeaponState( int weaponstate );

/*!
	\brief Resets the specified bot weapon state to its initial configuration

	This function resets a bot weapon state by restoring the weapon weight configuration and index pointers while preserving the configuration data structure. It retrieves the weapon state using a
   handle, validates the existence of the state, and then resets the state's internal pointers to their original values. The function ensures that the memory used for weapon weights is preserved but
   the state is reset to a clean configuration. This is typically used during bot initialization or when resetting bot behavior during gameplay.

	\param weaponstate Handle to the bot weapon state to reset
*/
void BotResetWeaponState( int weaponstate );
