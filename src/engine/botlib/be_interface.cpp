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
	\file engine/botlib/be_interface.cpp
	\brief Implements the bot library interface, providing exported functions for initialization, shutdown, entity management, and debugging.
	\note doxygenix: sha256=1aad9171335b10b541868724c3ead7f63bba9d13edbd7104cd8c5645e34c7f04

	\par File Purpose
	- Implements the bot library interface, providing exported functions for initialization, shutdown, entity management, and debugging.

	\par Core Responsibilities
	- Initializes and exposes bot library API to the engine.
	- Configures AAS, EA, and AI subsystem export tables.

	\par Key Types and Functions
	- botlib_globals_t botlibglobals — structure holding global bot state such as maxclients, maxentities, and setup flag.
	- botlib_export_t be_botlib_export — the exported API table.
	- botlib_import_t botimport — imported engine callbacks used by bot library.
	- int botlibsetup — flag marking whether library has been initialized.
	- int Sys_MilliSeconds() — platform‑agnostic millisecond counter.
	- qboolean ValidClientNumber(int num, char* str) — checks client index against botlibglobals.maxclients.
	- qboolean ValidEntityNumber(int num, char* str) — checks entity index against botlibglobals.maxentities.
	- qboolean BotLibSetup(char* str) — verifies that bot library is currently set up.
	- int Export_BotLibSetup() — performs full bot library initialization, configuring debug flags, setting up AAS, EA, movement AI, and global variables.
	- int Export_BotLibShutdown() — orderly de‑initializes subsystems, cleans globals, and handles recursion guard.
	- int Export_BotLibVarSet(char* var_name, char* value) — sets a libvar.
	- int Export_BotLibVarGet(char* var_name, char* value, int size) — retrieves a libvar string.
	- int Export_BotLibStartFrame(float time) — begins a frame after verifying setup and delegates to AAS_StartFrame.
	- int Export_BotLibLoadMap(const char* mapname) — loads AAS data for a map and initializes level items.
	- int Export_BotLibUpdateEntity(int ent, bot_entitystate_t* state) — updates an entity state in AAS after validation.
	- int BotExportTest(int parm0, char* parm1, vec3_t parm2, vec3_t parm3) — debug routine that visualizes and logs navigation data; active only in DEBUG builds.
	- static void Init_AAS_Export(aas_export_t* aas) — populates AAS export table with function pointers to AAS subsystem implementations.
	- static void Init_EA_Export(ea_export_t* ea) — populates EA export table with function pointers to EA subsystem implementations.
	- static void Init_AI_Export(ai_export_t* ai) — populates AI export table with function pointers to AI subsystem implementations.
	- botlib_export_t* GetBotLibAPI(int apiVersion, botlib_import_t* import) — validates API version, sets up export tables, and returns pointer to be_botlib_export.

	\par Control Flow
	- botimport is set from import argument in GetBotLibAPI; be_botlib_export table zeroed and filled using Init_*Export functions; botlibglobals initialized during Export_BotLibSetup; subsequent bot operations call exported functions, which check BotLibSetup and delegate to AAS, EA, AI subsystems; Export_BotLibShutdown reverse order cleans up; BotExportTest provides debug diagnostics

	\par Dependencies
	- q_shared.h
	- l_memory.h
	- l_log.h
	- l_libvar.h
	- l_script.h
	- l_precomp.h
	- l_struct.h
	- aasfile.h
	- botshared/botlib.h
	- botshared/be_aas.h
	- be_aas_funcs.h
	- be_aas_def.h
	- be_interface.h
	- botshared/be_ea.h
	- be_ai_weight.h
	- botshared/be_ai_goal.h
	- botshared/be_ai_move.h
	- botshared/be_ai_weap.h
	- botshared/be_ai_chat.h
	- botshared/be_ai_char.h
	- botshared/be_ai_gen.h

	\par How It Fits
	- This file is the central entry point for the bot subsystem; it constructs the exported function table used by the game engine to interact with bot logic. It wires together lower‑level AAS, EA, and AI components and exposes configuration functions.
*/

/*****************************************************************************
 * name:		be_interface.c
 *
 * desc:		bot library interface
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_memory.h"
#include "l_log.h"
#include "l_libvar.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "botshared/botlib.h"
#include "botshared/be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"
#include "be_interface.h"

#include "botshared/be_ea.h"
#include "be_ai_weight.h"
#include "botshared/be_ai_goal.h"
#include "botshared/be_ai_move.h"
#include "botshared/be_ai_weap.h"
#include "botshared/be_ai_chat.h"
#include "botshared/be_ai_char.h"
#include "botshared/be_ai_gen.h"

// library globals in a structure
botlib_globals_t botlibglobals;

botlib_export_t	 be_botlib_export;
botlib_import_t	 botimport;
//
int				 bot_developer;
// qtrue if the library is setup
int				 botlibsetup = qfalse;

//===========================================================================
//
// several functions used by the exported functions
//
//===========================================================================

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
// Ridah, faster Win32 code
#ifdef _WIN32
	#undef MAX_PATH // this is an ugly hack, to temporarily ignore the current definition, since it's also defined in windows.h
	#include <windows.h>
	#undef MAX_PATH
	#define MAX_PATH MAX_QPATH
#endif

/*!
	\brief Returns the number of milliseconds elapsed since the system was started

	This function provides a monotonic time source suitable for measuring elapsed time intervals. On Windows platforms, it uses the Windows multimedia timer API for higher precision timing. On other platforms, it uses the standard C library clock function converted to milliseconds. The function maintains internal state to ensure consistent time progression across multiple calls

	\return The number of milliseconds since the system was started
*/
int Sys_MilliSeconds()
{
	// Ridah, faster Win32 code
#ifdef _WIN32
	int				sys_curtime;
	static qboolean initialized = qfalse;
	static int		sys_timeBase;

	if( !initialized ) {
		sys_timeBase = timeGetTime();
		initialized	 = qtrue;
	}

	sys_curtime = timeGetTime() - sys_timeBase;

	return sys_curtime;
#else
	return clock() * 1000 / CLOCKS_PER_SEC;
#endif
}

/*!
	\brief Validates a client number against the allowed range and logs an error when it is outside this range.

	The function ensures that the supplied client number falls between 0 and the maximum number of supported clients (inclusive). If it lies outside this inclusive boundary, an error message is output
   using botimport.Print, indicating the provided number and the valid range, and the function returns false. A valid number causes the function to return true.

	\param num the client number to be validated
	\param str a contextual string used in the error message
	\return Returns true if the client number is within the valid range, false otherwise.
*/
qboolean ValidClientNumber( int num, char* str )
{
	if( num < 0 || num > botlibglobals.maxclients ) {
		// weird: the disabled stuff results in a crash
		botimport.Print( PRT_ERROR, "%s: invalid client number %d, [0, %d]\n", str, num, botlibglobals.maxclients );
		return qfalse;
	}

	return qtrue;
}

/*!
	\brief Verifies that a supplied entity number is in the valid range and logs an error if it is not.

	The function checks whether the entity number falls within the inclusive interval [0, maxentities] as defined by botlibglobals.maxentities. If the number is outside this range, it logs an error
   using botimport.Print, including the supplied string for context, the invalid number, and the maximum allowed value. After reporting the error it returns false, otherwise it returns true to
   indicate a valid entity number.

	\param num the entity number to validate
	\param str a context string used as a prefix in the error message
	\return true if the number is in the valid range, false otherwise
*/
qboolean ValidEntityNumber( int num, char* str )
{
	if( num < 0 || num > botlibglobals.maxentities ) {
		botimport.Print( PRT_ERROR, "%s: invalid entity number %d, [0, %d]\n", str, num, botlibglobals.maxentities );
		return qfalse;
	}

	return qtrue;
}

/*!
	\brief Checks whether the bot library has been initialized and reports an error if not.

	The function verifies the global flag that signals whether the bot library has been set up. If the flag is false, it prints a message indicating that the bot library was used before being set up,
   using the supplied string to identify the calling function, and returns false. If the flag is true, it returns true.

	\param str Identifier of the calling function, used in the error message.
	\return True if the bot library is set up; otherwise false.
*/
qboolean BotLibSetup( char* str )
{
	//	return qtrue;

	if( !botlibglobals.botlibsetup ) {
		botimport.Print( PRT_ERROR, "%s: bot library used before being setup\n", str );
		return qfalse;
	}

	return qtrue;
}

/*!
	\brief Initializes the bot library and runs setup for AI, entity assistance, movement, and related subsystems.

	The function first updates the bot developer flag from a configuration variable, then performs byte‑swapping initialization and opens the botlib log file. A message announcing the start of
   initialization is printed. Global limits for clients and entities are loaded from configuration values. Sequentially, the core setup routines for AAS, EA, and movement AI are invoked; any error
   returned by these functions causes early exit with that error. If all steps succeed, flags indicating completion of botlib setup are set to true and a success code is returned.

	\return On success returns BLERR_NOERROR; otherwise returns the specific error code from the first failing subsystem.
*/
int Export_BotLibSetup()
{
	int errnum;

	bot_developer = LibVarGetValue( "bot_developer" );
	// initialize byte swapping (litte endian etc.)
	Swap_Init();
	Log_Open( "botlib.log" );
	//
	botimport.Print( PRT_MESSAGE, "------- BotLib Initialization -------\n" );
	//
	botlibglobals.maxclients  = ( int )LibVarValue( "maxclients", "128" );
	botlibglobals.maxentities = ( int )LibVarValue( "maxentities", "2048" );

	errnum = AAS_Setup(); // be_aas_main.c

	if( errnum != BLERR_NOERROR ) {
		return errnum;
	}

	errnum = EA_Setup(); // be_ea.c

	if( errnum != BLERR_NOERROR ) {
		return errnum;
	}

	//	errnum = BotSetupWeaponAI();	//be_ai_weap.c
	//	if (errnum != BLERR_NOERROR)return errnum;
	//	errnum = BotSetupGoalAI();		//be_ai_goal.c
	//	if (errnum != BLERR_NOERROR) return errnum;
	//	errnum = BotSetupChatAI();		//be_ai_chat.c
	//	if (errnum != BLERR_NOERROR) return errnum;
	errnum = BotSetupMoveAI(); // be_ai_move.c

	if( errnum != BLERR_NOERROR ) {
		return errnum;
	}

	botlibsetup				  = qtrue;
	botlibglobals.botlibsetup = qtrue;

	return BLERR_NOERROR;
}

/*!
	\brief Shuts down the bot library, releasing all associated resources and subsystems.

	The function first ensures the library was previously initialized via BotLibSetup; if not, it returns BLERR_LIBRARYNOTSETUP. A recursion guard protects against re‑entrant calls; subsequent
   invocations merely return BLERR_NOERROR. On a clean shutdown, it sequentially deactivates chat, movement, goal, weapon, weights, and character AI modules, shuts down the AAS and elementary actions
   systems, deallocates all libvars, removes global pre‑compiler defines, and shuts down the logging facility. Global flags botlibsetup and botlibglobals.botlibsetup are reset, the recursion counter
   cleared, and any still‑open source handles are reported. In a debug build, memory usage is logged before final shutdown.


	\return An error code; BLERR_NOERROR on success, BLERR_LIBRARYNOTSETUP if the library had not been set up.
*/
int Export_BotLibShutdown()
{
	static int recursive = 0;

	if( !BotLibSetup( "BotLibShutdown" ) ) {
		return BLERR_LIBRARYNOTSETUP;
	}

	//
	if( recursive ) {
		return BLERR_NOERROR;
	}

	recursive = 1;
	// shutdown all AI subsystems
	BotShutdownChatAI();	 // be_ai_chat.c
	BotShutdownMoveAI();	 // be_ai_move.c
	BotShutdownGoalAI();	 // be_ai_goal.c
	BotShutdownWeaponAI();	 // be_ai_weap.c
	BotShutdownWeights();	 // be_ai_weight.c
	BotShutdownCharacters(); // be_ai_char.c
	// shutdown AAS
	AAS_Shutdown();
	// shutdown bot elemantary actions
	EA_Shutdown();
	// free all libvars
	LibVarDeAllocAll();
	// remove all global defines from the pre compiler
	PC_RemoveAllGlobalDefines();
	// shut down library log file
	Log_Shutdown();
	//
	botlibsetup				  = qfalse;
	botlibglobals.botlibsetup = qfalse;
	recursive				  = 0;
	// print any files still open
	PC_CheckOpenSourceHandles();
	//
#ifdef _DEBUG
	Log_AlwaysOpen( "memory.log" );
	PrintMemoryLabels();
	Log_Shutdown();
#endif
	return BLERR_NOERROR;
}

/*!
	\brief Sets a bot library variable identified by name to a supplied value and signals success via return code.

	The function forwards the variable name and value to the underlying LibVarSet mechanism, which updates the associated global state within the bot framework. After the update, it returns a status
   code indicating that the operation completed successfully. There are no conditional failure paths in this implementation; it always reports success.

	\param var_name C style string containing the name of the variable to set
	\param value C style string containing the new value for the variable
	\return Zero (BLERR_NOERROR) to indicate the variable was set without error
*/
int Export_BotLibVarSet( char* var_name, char* value )
{
	LibVarSet( var_name, value );
	return BLERR_NOERROR;
}

/*!
	\brief Retrieve a bot library variable string into a supplied buffer

	The function obtains the value of a bot library variable named by var_name using LibVarGetString, then copies up to size-1 characters into the buffer pointed to by value, ensuring the buffer is
   null terminated. It returns BLERR_NOERROR to indicate success.

	\param var_name Name of the variable whose value is to be retrieved
	\param value Pointer to a buffer where the retrieved string will be stored
	\param size Maximum length of the destination buffer, including space for the terminating null character
	\return BLERR_NOERROR on success
*/
int Export_BotLibVarGet( char* var_name, char* value, int size )
{
	char* varvalue;

	varvalue = LibVarGetString( var_name );
	strncpy( value, varvalue, size - 1 );
	value[size - 1] = '\0';
	return BLERR_NOERROR;
}

/*!
	\brief Starts a new frame for the bot library, ensuring initialization before delegating to AAS_StartFrame.

	The function first calls BotLibSetup with the name "BotStartFrame" to make sure the bot library is properly initialized.
	If the setup fails, it immediately returns the error code BLERR_LIBRARYNOTSETUP.
	On success, it forwards the supplied time to AAS_StartFrame and returns the status code obtained from that call.
	Thus, the function acts as a guard and wrapper around the core frame‑starting routine.

	\param time the current frame time used by AAS_StartFrame
	\return An integer status code: BLERR_LIBRARYNOTSETUP if the library is not initialized, otherwise the result of AAS_StartFrame.
*/
int Export_BotLibStartFrame( float time )
{
	if( !BotLibSetup( "BotStartFrame" ) ) {
		return BLERR_LIBRARYNOTSETUP;
	}

	return AAS_StartFrame( time );
}

/*!
	\brief Loads the bot infrastructure for a map, initializing AAS data and level items.

	The function first ensures that the bot library is properly set up. If setup succeeds, it prints a header, loads AAS data for the specified map, and returns an error code if the load fails. On
   success, it initializes level items and brush model types, prints a footer, and may output timing information in debug builds. Finally it returns a zero error code indicating success.

	\param mapname Name of the map to load; a C‑style string
	\return Zero on success or a BLERR_ error code indicating the nature of the failure.
*/
int Export_BotLibLoadMap( const char* mapname )
{
#ifdef DEBUG
	int starttime = Sys_MilliSeconds();
#endif
	int errnum;

	if( !BotLibSetup( "BotLoadMap" ) ) {
		return BLERR_LIBRARYNOTSETUP;
	}

	//
	botimport.Print( PRT_MESSAGE, "------------ Map Loading ------------\n" );
	// startup AAS for the current map, model and sound index
	errnum = AAS_LoadMap( mapname );

	if( errnum != BLERR_NOERROR ) {
		return errnum;
	}

	// initialize the items in the level
	BotInitLevelItems();	 // be_ai_goal.h
	BotSetBrushModelTypes(); // be_ai_move.h
	//
	botimport.Print( PRT_MESSAGE, "-------------------------------------\n" );
#ifdef DEBUG
	botimport.Print( PRT_MESSAGE, "map loaded in %d msec\n", Sys_MilliSeconds() - starttime );
#endif
	//
	return BLERR_NOERROR;
}

/*!
	\brief Updates the state of a specified entity in the bot library after confirming setup and validity.

	The routine first ensures that the bot library is initialized by calling BotLibSetup. If the library is not set up, it returns BLERR_LIBRARYNOTSETUP. It then checks that the provided entity number
   is valid; if not, it returns BLERR_INVALIDENTITYNUMBER. When both checks pass, the update request is forwarded to AAS_UpdateEntity, whose return value is propagated back to the caller.

	\param ent The identifier of the entity to update.
	\param state A pointer to the new bot entity state data that will be used for the update.
	\return The integer result from AAS_UpdateEntity, which may indicate success or specific error codes such as BLERR_LIBRARYNOTSETUP or BLERR_INVALIDENTITYNUMBER.
*/
int Export_BotLibUpdateEntity( int ent, bot_entitystate_t* state )
{
	if( !BotLibSetup( "BotUpdateEntity" ) ) {
		return BLERR_LIBRARYNOTSETUP;
	}

	if( !ValidEntityNumber( ent, "BotUpdateEntity" ) ) {
		return BLERR_INVALIDENTITYNUMBER;
	}

	return AAS_UpdateEntity( ent, state );
}

void	 AAS_TestMovementPrediction( int entnum, vec3_t origin, vec3_t dir );
void	 ElevatorBottomCenter( aas_reachability_t* reach, vec3_t bottomcenter );
int		 BotGetReachabilityToGoal( vec3_t origin,
		 int							  areanum,
		 int							  entnum,
		 int							  lastgoalareanum,
		 int							  lastareanum,
		 int*							  avoidreach,
		 float*							  avoidreachtimes,
		 int*							  avoidreachtries,
		 bot_goal_t*					  goal,
		 int							  travelflags,
		 int							  movetravelflags );

int		 AAS_PointLight( vec3_t origin, int* red, int* green, int* blue );

int		 AAS_TraceAreas( vec3_t start, vec3_t end, int* areas, vec3_t* points, int maxareas );

int		 AAS_Reachability_WeaponJump( int area1num, int area2num );

int		 BotFuzzyPointReachabilityArea( vec3_t origin );

float	 BotGapDistance( vec3_t origin, vec3_t hordir, int entnum );

int		 AAS_NearestHideArea( int srcnum, vec3_t origin, int areanum, int enemynum, vec3_t enemyorigin, int enemyareanum, int travelflags );

int		 AAS_FindAttackSpotWithinRange( int srcnum, int rangenum, int enemynum, float rangedist, int travelflags, float* outpos );

qboolean AAS_GetRouteFirstVisPos( vec3_t srcpos, vec3_t destpos, int travelflags, vec3_t retpos );

void	 AAS_SetAASBlockingEntity( vec3_t absmin, vec3_t absmax, qboolean blocking );

/*!
	\brief Perform diagnostic visualizations and debug logging based on global settings for a specified world location.

	This function examines several global debug flags and, depending on their values, performs visual tracing and logging for various aspects of the AI navigation world. It typically displays the
   current area, the target area, a hide area, or a route visibility area, and prints travel times or area properties. In all cases, it returns 0, so the integer return value is not used for any
   further logic.

	\param parm0 Bitfield flags controlling which diagnostics to perform; bit 0 requests setting a new goal area based on the supplied coordinates.
	\param parm1 Unreferenced string parameter; historically intended for an optional label or identifier but currently unused.
	\param parm2 3‑D position vector used as the reference point for diagnostics; normally the enemy or test location.
	\param parm3 Unreferenced 3‑D vector; reserved for future extensions but not used in the present implementation.
	\return Zero, indicating success (the return value is not used elsewhere).
*/
int		 BotExportTest( int parm0, char* parm1, vec3_t parm2, vec3_t parm3 )
{
	//	return AAS_PointLight(parm2, NULL, NULL, NULL);

#ifdef DEBUG
	static int area = -1;
	static int line[2];
	int		   newarea, i, highlightarea, bot_testhidepos, hideposarea, bot_testroutevispos;
	//	int reachnum;
	vec3_t	   eye, forward, right, end, origin;
	//	vec3_t bottomcenter;
	//	aas_trace_t trace;
	//	aas_face_t *face;
	//	aas_entity_t *ent;
	//	bsp_trace_t bsptrace;
	//	aas_reachability_t reach;
	//	bot_goal_t goal;

	//	clock_t start_time, end_time;
	vec3_t	   mins = { -16, -16, -24 };
	vec3_t	   maxs = { 16, 16, 32 };
	//	int areas[10], numareas;

	// return 0;

	if( !( *aasworld ).loaded ) {
		return 0;
	}

	AAS_SetCurrentWorld( 0 );

	for( i = 0; i < 2; i++ )
		if( !line[i] ) {
			line[i] = botimport.DebugLineCreate();
		}

	//	AAS_ClearShownDebugLines();
	bot_testhidepos = LibVarGetValue( "bot_testhidepos" );

	if( bot_testhidepos ) {
		VectorCopy( parm2, origin );
		newarea = BotFuzzyPointReachabilityArea( origin );

		if( parm0 & 1 ) {
			botlibglobals.goalareanum = newarea;
			VectorCopy( origin, botlibglobals.goalorigin );
			botimport.Print( PRT_MESSAGE, "new enemy position %2.1f %2.1f %2.1f area %d\n", origin[0], origin[1], origin[2], newarea );
		}

		AAS_ClearShownPolygons();
		AAS_ClearShownDebugLines();
		hideposarea = AAS_NearestHideArea( -1, origin, AAS_PointAreaNum( origin ), 0, botlibglobals.goalorigin, botlibglobals.goalareanum, TFL_DEFAULT );

		if( bot_testhidepos > 1 ) {
			if( hideposarea ) {
				botimport.Print( PRT_MESSAGE,
					"hidepos (%i) %2.1f %2.1f %2.1f\n",
					hideposarea,
					( *aasworld ).areawaypoints[hideposarea][0],
					( *aasworld ).areawaypoints[hideposarea][1],
					( *aasworld ).areawaypoints[hideposarea][2] );

			} else {
				botimport.Print( PRT_MESSAGE, "no hidepos found\n" );
			}
		}

		// area we are currently in
		AAS_ShowAreaPolygons( newarea, 1, qtrue );
		// enemy position
		AAS_ShowAreaPolygons( botlibglobals.goalareanum, 2, qtrue );
		// area we should go hide
		AAS_ShowAreaPolygons( hideposarea, 4, qtrue );
		return 0;
	}

	bot_testroutevispos = LibVarGetValue( "bot_testroutevispos" );

	if( bot_testroutevispos ) {
		VectorCopy( parm2, origin );
		newarea = BotFuzzyPointReachabilityArea( origin );

		if( parm0 & 1 ) {
			botlibglobals.goalareanum = newarea;
			VectorCopy( origin, botlibglobals.goalorigin );
			botimport.Print( PRT_MESSAGE, "new enemy position %2.1f %2.1f %2.1f area %d\n", origin[0], origin[1], origin[2], newarea );
		}

		AAS_ClearShownPolygons();
		AAS_ClearShownDebugLines();
		AAS_GetRouteFirstVisPos( botlibglobals.goalorigin, origin, TFL_DEFAULT, eye );
		// area we are currently in
		AAS_ShowAreaPolygons( newarea, 1, qtrue );
		// enemy position
		AAS_ShowAreaPolygons( botlibglobals.goalareanum, 2, qtrue );
		// area that is visible in path from enemy pos
		hideposarea = BotFuzzyPointReachabilityArea( eye );
		AAS_ShowAreaPolygons( hideposarea, 4, qtrue );
		return 0;
	}

	// if (AAS_AgainstLadder(parm2)) botimport.Print(PRT_MESSAGE, "against ladder\n");
	// BotOnGround(parm2, PRESENCE_NORMAL, 1, &newarea, &newarea);
	// botimport.Print(PRT_MESSAGE, "%f %f %f\n", parm2[0], parm2[1], parm2[2]);
	//*
	highlightarea = LibVarGetValue( "bot_highlightarea" );

	if( highlightarea > 0 ) {
		newarea = highlightarea;

	} else {
		VectorCopy( parm2, origin );
		origin[2] += 0.5;
		// newarea = AAS_PointAreaNum(origin);
		newarea = BotFuzzyPointReachabilityArea( origin );
	}

	botimport.Print( PRT_MESSAGE, "\rtravel time to goal (%d) = %d  ", botlibglobals.goalareanum, AAS_AreaTravelTimeToGoalArea( newarea, origin, botlibglobals.goalareanum, TFL_DEFAULT ) );

	// newarea = BotReachabilityArea(origin, qtrue);
	if( newarea != area ) {
		botimport.Print( PRT_MESSAGE, "origin = %f, %f, %f\n", origin[0], origin[1], origin[2] );
		area = newarea;
		botimport.Print( PRT_MESSAGE, "new area %d, cluster %d, presence type %d\n", area, AAS_AreaCluster( area ), AAS_PointPresenceType( origin ) );

		if( ( *aasworld ).areasettings[area].areaflags & AREA_LIQUID ) {
			botimport.Print( PRT_MESSAGE, "liquid area\n" );
		}

		botimport.Print( PRT_MESSAGE, "area contents: " );

		if( ( *aasworld ).areasettings[area].contents & AREACONTENTS_WATER ) {
			botimport.Print( PRT_MESSAGE, "water " );
		}

		if( ( *aasworld ).areasettings[area].contents & AREACONTENTS_LAVA ) {
			botimport.Print( PRT_MESSAGE, "lava " );
		}

		if( ( *aasworld ).areasettings[area].contents & AREACONTENTS_SLIME ) {
			//			botimport.Print(PRT_MESSAGE, "slime ");
			botimport.Print( PRT_MESSAGE, "slag " );
		}

		if( ( *aasworld ).areasettings[area].contents & AREACONTENTS_JUMPPAD ) {
			botimport.Print( PRT_MESSAGE, "jump pad " );
		}

		if( ( *aasworld ).areasettings[area].contents & AREACONTENTS_CLUSTERPORTAL ) {
			botimport.Print( PRT_MESSAGE, "cluster portal " );
		}

		if( ( *aasworld ).areasettings[area].contents & AREACONTENTS_DONOTENTER ) {
			botimport.Print( PRT_MESSAGE, "do not enter " );
		}

		if( ( *aasworld ).areasettings[area].contents & AREACONTENTS_DONOTENTER_LARGE ) {
			botimport.Print( PRT_MESSAGE, "do not enter large " );
		}

		if( !( *aasworld ).areasettings[area].contents ) {
			botimport.Print( PRT_MESSAGE, "empty " );
		}

		if( ( *aasworld ).areasettings[area].areaflags & AREA_DISABLED ) {
			botimport.Print( PRT_MESSAGE, "DISABLED" );
		}

		botimport.Print( PRT_MESSAGE, "\n" );
		botimport.Print(
			PRT_MESSAGE, "travel time to goal (%d) = %d\n", botlibglobals.goalareanum, AAS_AreaTravelTimeToGoalArea( newarea, origin, botlibglobals.goalareanum, TFL_DEFAULT | TFL_ROCKETJUMP ) );
		/*
		VectorCopy(origin, end);
		end[2] += 5;
		numareas = AAS_TraceAreas(origin, end, areas, NULL, 10);
		AAS_TraceClientBBox(origin, end, PRESENCE_CROUCH, -1);
		botimport.Print(PRT_MESSAGE, "num areas = %d, area = %d\n", numareas, areas[0]);
		*/
		/*
		botlibglobals.goalareanum = newarea;
		VectorCopy(parm2, botlibglobals.goalorigin);
		botimport.Print(PRT_MESSAGE, "new goal %2.1f %2.1f %2.1f area %d\n",
								origin[0], origin[1], origin[2], newarea);
		*/
	}

	//*
	if( parm0 & 1 ) {
		botlibglobals.goalareanum = newarea;
		VectorCopy( parm2, botlibglobals.goalorigin );
		botimport.Print( PRT_MESSAGE, "new goal %2.1f %2.1f %2.1f area %d\n", origin[0], origin[1], origin[2], newarea );
	} // end if*/

	//	if (parm0 & BUTTON_USE)
	//	{
	//		botlibglobals.runai = !botlibglobals.runai;
	//		if (botlibglobals.runai) botimport.Print(PRT_MESSAGE, "started AI\n");
	//		else botimport.Print(PRT_MESSAGE, "stopped AI\n");
	//* /
	/*
	goal.areanum = botlibglobals.goalareanum;
	reachnum = BotGetReachabilityToGoal(parm2, newarea, 1,
									ms.avoidreach, ms.avoidreachtimes,
									&goal, TFL_DEFAULT);
	if (!reachnum)
	{
		botimport.Print(PRT_MESSAGE, "goal not reachable\n");
	}
	else
	{
		AAS_ReachabilityFromNum(reachnum, &reach);
		AAS_ClearShownDebugLines();
		AAS_ShowArea(area, qtrue);
		AAS_ShowArea(reach.areanum, qtrue);
		AAS_DrawCross(reach.start, 6, LINECOLOR_BLUE);
		AAS_DrawCross(reach.end, 6, LINECOLOR_RED);
		//
		if (reach.traveltype == TRAVEL_ELEVATOR)
		{
			ElevatorBottomCenter(&reach, bottomcenter);
			AAS_DrawCross(bottomcenter, 10, LINECOLOR_GREEN);
		}
	} //end else*/
	//		botimport.Print(PRT_MESSAGE, "travel time to goal = %d\n",
	//					AAS_AreaTravelTimeToGoalArea(area, origin, botlibglobals.goalareanum, TFL_DEFAULT));
	//		botimport.Print(PRT_MESSAGE, "test rj from 703 to 716\n");
	//		AAS_Reachability_WeaponJump(703, 716);
	//	} //end if*/

	/*	face = AAS_AreaGroundFace(newarea, parm2);
		if (face)
		{
			AAS_ShowFace(face - (*aasworld).faces);
		} //end if*/
	/*
	AAS_ClearShownDebugLines();
	AAS_ShowArea(newarea, parm0 & BUTTON_USE);
	AAS_ShowReachableAreas(area);
	*/
	AAS_ClearShownPolygons();
	AAS_ClearShownDebugLines();
	AAS_ShowAreaPolygons( newarea, 1, parm0 & 4 );

	if( parm0 & 2 ) {
		AAS_ShowReachableAreas( area );

	} else {
		static int		   lastgoalareanum, lastareanum;
		static int		   avoidreach[MAX_AVOIDREACH];
		static float	   avoidreachtimes[MAX_AVOIDREACH];
		static int		   avoidreachtries[MAX_AVOIDREACH];
		int				   reachnum;
		bot_goal_t		   goal;
		aas_reachability_t reach;

		goal.areanum = botlibglobals.goalareanum;
		VectorCopy( botlibglobals.goalorigin, goal.origin );
		reachnum =
			BotGetReachabilityToGoal( origin, newarea, -1, lastgoalareanum, lastareanum, avoidreach, avoidreachtimes, avoidreachtries, &goal, TFL_DEFAULT | TFL_FUNCBOB, TFL_DEFAULT | TFL_FUNCBOB );
		AAS_ReachabilityFromNum( reachnum, &reach );
		AAS_ShowReachability( &reach );
	}

	VectorClear( forward );
	// BotGapDistance(origin, forward, 0);
	/*
	if (parm0 & BUTTON_USE)
	{
		botimport.Print(PRT_MESSAGE, "test rj from 703 to 716\n");
		AAS_Reachability_WeaponJump(703, 716);
	} //end if*/

	AngleVectors( parm3, forward, right, NULL );
	// get the eye 16 units to the right of the origin
	VectorMA( parm2, 8, right, eye );
	// get the eye 24 units up
	eye[2] += 24;
	// get the end point for the line to be traced
	VectorMA( eye, 800, forward, end );

	//	AAS_TestMovementPrediction(1, parm2, forward);
	/*	//trace the line to find the hit point
		trace = AAS_TraceClientBBox(eye, end, PRESENCE_NORMAL, 1);
		if (!line[0]) line[0] = botimport.DebugLineCreate();
		botimport.DebugLineShow(line[0], eye, trace.endpos, LINECOLOR_BLUE);
		//
		AAS_ClearShownDebugLines();
		if (trace.ent)
		{
			ent = &(*aasworld).entities[trace.ent];
			AAS_ShowBoundingBox(ent->origin, ent->mins, ent->maxs);
		} //end if*/

	/*
		start_time = clock();
		for (i = 0; i < 2000; i++)
		{
			AAS_Trace2(eye, mins, maxs, end, 1, MASK_PLAYERSOLID);
	//		AAS_TraceClientBBox(eye, end, PRESENCE_NORMAL, 1);
		}
		end_time = clock();
		botimport.Print(PRT_MESSAGE, "me %lu clocks, %lu CLOCKS_PER_SEC\n", end_time - start_time, CLOCKS_PER_SEC);
		start_time = clock();
		for (i = 0; i < 2000; i++)
		{
			AAS_Trace(eye, mins, maxs, end, 1, MASK_PLAYERSOLID);
		}
		end_time = clock();
		botimport.Print(PRT_MESSAGE, "id %lu clocks, %lu CLOCKS_PER_SEC\n", end_time - start_time, CLOCKS_PER_SEC);
	*/

	/*
	AAS_ClearShownDebugLines();
	//bsptrace = AAS_Trace(eye, NULL, NULL, end, 1, MASK_PLAYERSOLID);
	bsptrace = AAS_Trace(eye, mins, maxs, end, 1, MASK_PLAYERSOLID);
	if (!line[0]) line[0] = botimport.DebugLineCreate();
	botimport.DebugLineShow(line[0], eye, bsptrace.endpos, LINECOLOR_YELLOW);
	if (bsptrace.fraction < 1.0)
	{
		face = AAS_TraceEndFace(&trace);
		if (face)
		{
			AAS_ShowFace(face - (*aasworld).faces);
		}
		AAS_DrawPlaneCross(bsptrace.endpos,
									bsptrace.plane.normal,
									bsptrace.plane.dist + bsptrace.exp_dist,
									bsptrace.plane.type, LINECOLOR_GREEN);
		if (trace.ent)
		{
			ent = &(*aasworld).entities[trace.ent];
			AAS_ShowBoundingBox(ent->origin, ent->mins, ent->maxs);
		}
	} //end if*/
	/*/
	//bsptrace = AAS_Trace2(eye, NULL, NULL, end, 1, MASK_PLAYERSOLID);
	bsptrace = AAS_Trace2(eye, mins, maxs, end, 1, MASK_PLAYERSOLID);
	botimport.DebugLineShow(line[1], eye, bsptrace.endpos, LINECOLOR_BLUE);
	if (bsptrace.fraction < 1.0)
	{
		AAS_DrawPlaneCross(bsptrace.endpos,
									bsptrace.plane.normal,
									bsptrace.plane.dist,// + bsptrace.exp_dist,
									bsptrace.plane.type, LINECOLOR_RED);
		if (bsptrace.ent)
		{
			ent = &(*aasworld).entities[bsptrace.ent];
			AAS_ShowBoundingBox(ent->origin, ent->mins, ent->maxs);
		}
	}
	*/
#endif
	return 0;
}

/*!
	\brief Populate the AAS export structure with pointers to the AAS subsystem implementation functions.

	The function assigns each function pointer field in the aas_export_t structure to the corresponding implementation function located in various source files such as be_aas_entity.c, be_aas_main.c,
   be_aas_sample.c, be_aas_bspq3.c, be_aas_reach.c, be_aas_route.c, be_aas_move.c, and be_aas_routetable.c. By doing so it exposes the AAS API to other parts of the engine or game code. The routine
   should be invoked once during initialization before any code attempts to use the exported AAS functions.

	\param aas pointer to the export table to populate
*/
static void Init_AAS_Export( aas_export_t* aas )
{
	//--------------------------------------------
	// be_aas_entity.c
	//--------------------------------------------
	aas->AAS_EntityInfo = AAS_EntityInfo;
	//--------------------------------------------
	// be_aas_main.c
	//--------------------------------------------
	aas->AAS_Initialized			 = AAS_Initialized;
	aas->AAS_PresenceTypeBoundingBox = AAS_PresenceTypeBoundingBox;
	aas->AAS_Time					 = AAS_Time;
	//--------------------------------------------
	// be_aas_sample.c
	//--------------------------------------------
	aas->AAS_PointAreaNum = AAS_PointAreaNum;
	aas->AAS_TraceAreas	  = AAS_TraceAreas;
	//--------------------------------------------
	// be_aas_bspq3.c
	//--------------------------------------------
	aas->AAS_PointContents		  = AAS_PointContents;
	aas->AAS_NextBSPEntity		  = AAS_NextBSPEntity;
	aas->AAS_ValueForBSPEpairKey  = AAS_ValueForBSPEpairKey;
	aas->AAS_VectorForBSPEpairKey = AAS_VectorForBSPEpairKey;
	aas->AAS_FloatForBSPEpairKey  = AAS_FloatForBSPEpairKey;
	aas->AAS_IntForBSPEpairKey	  = AAS_IntForBSPEpairKey;
	//--------------------------------------------
	// be_aas_reach.c
	//--------------------------------------------
	aas->AAS_AreaReachability = AAS_AreaReachability;
	//--------------------------------------------
	// be_aas_route.c
	//--------------------------------------------
	aas->AAS_AreaTravelTimeToGoalArea = AAS_AreaTravelTimeToGoalArea;
	//--------------------------------------------
	// be_aas_move.c
	//--------------------------------------------
	aas->AAS_Swimming			   = AAS_Swimming;
	aas->AAS_PredictClientMovement = AAS_PredictClientMovement;

	// Ridah, route-tables
	//--------------------------------------------
	// be_aas_routetable.c
	//--------------------------------------------
	aas->AAS_RT_ShowRoute			   = AAS_RT_ShowRoute;
	aas->AAS_RT_GetHidePos			   = AAS_RT_GetHidePos;
	aas->AAS_FindAttackSpotWithinRange = AAS_FindAttackSpotWithinRange;
	aas->AAS_GetRouteFirstVisPos	   = AAS_GetRouteFirstVisPos;
	aas->AAS_SetAASBlockingEntity	   = AAS_SetAASBlockingEntity;
	// done.

	// Ridah, multiple AAS files
	aas->AAS_SetCurrentWorld = AAS_SetCurrentWorld;
	// done.
}

/*!
	\brief Initializes an ea_export_t instance with function pointers to client command actions.

	This function populates all members of the ea_export_t structure with the appropriate EA_ prefixed functions. Each member pointer such as EA_Say, EA_Talk, or EA_Move is set to the corresponding
   implementation function. The structure then provides a table of callbacks that the rest of the system can use to invoke client commands.

	\param ea pointer to the structure that will be initialized and populated with function callbacks
*/
static void Init_EA_Export( ea_export_t* ea )
{
	// ClientCommand elementary actions
	ea->EA_Say			= EA_Say;
	ea->EA_SayTeam		= EA_SayTeam;
	ea->EA_UseItem		= EA_UseItem;
	ea->EA_DropItem		= EA_DropItem;
	ea->EA_UseInv		= EA_UseInv;
	ea->EA_DropInv		= EA_DropInv;
	ea->EA_Gesture		= EA_Gesture;
	ea->EA_Command		= EA_Command;
	ea->EA_SelectWeapon = EA_SelectWeapon;
	ea->EA_Talk			= EA_Talk;
	ea->EA_Attack		= EA_Attack;
	ea->EA_Reload		= EA_Reload;
	ea->EA_Use			= EA_Use;
	ea->EA_Respawn		= EA_Respawn;
	ea->EA_Jump			= EA_Jump;
	ea->EA_DelayedJump	= EA_DelayedJump;
	ea->EA_Crouch		= EA_Crouch;
	ea->EA_MoveUp		= EA_MoveUp;
	ea->EA_MoveDown		= EA_MoveDown;
	ea->EA_MoveForward	= EA_MoveForward;
	ea->EA_MoveBack		= EA_MoveBack;
	ea->EA_MoveLeft		= EA_MoveLeft;
	ea->EA_MoveRight	= EA_MoveRight;
	ea->EA_Move			= EA_Move;
	ea->EA_View			= EA_View;
	ea->EA_GetInput		= EA_GetInput;
	ea->EA_EndRegular	= EA_EndRegular;
	ea->EA_ResetInput	= EA_ResetInput;
}

/*!
	\brief Sets all function pointers in an ai_export_t instance to the corresponding AI subsystem implementations.

	The function assigns each function pointer field of the supplied ai_export_t structure to the matching implementation from the character, chat, goal, movement, weapon, and genetics modules. It
   prepares the ai object for use by the game engine, ensuring that any AI queries will delegate to the correct subsystem functions. The function assumes the caller has allocated and passed a valid
   ai_export_t pointer; no additional memory is allocated or freed within this routine.

	\param ai A pointer to the ai_export_t structure that will be populated with function pointers. It must be non‑null when passed to this function.
*/
static void Init_AI_Export( ai_export_t* ai )
{
	//-----------------------------------
	// be_ai_char.h
	//-----------------------------------
	ai->BotLoadCharacter		= BotLoadCharacter;
	ai->BotFreeCharacter		= BotFreeCharacter;
	ai->Characteristic_Float	= Characteristic_Float;
	ai->Characteristic_BFloat	= Characteristic_BFloat;
	ai->Characteristic_Integer	= Characteristic_Integer;
	ai->Characteristic_BInteger = Characteristic_BInteger;
	ai->Characteristic_String	= Characteristic_String;
	//-----------------------------------
	// be_ai_chat.h
	//-----------------------------------
	ai->BotAllocChatState		= BotAllocChatState;
	ai->BotFreeChatState		= BotFreeChatState;
	ai->BotQueueConsoleMessage	= BotQueueConsoleMessage;
	ai->BotRemoveConsoleMessage = BotRemoveConsoleMessage;
	ai->BotNextConsoleMessage	= BotNextConsoleMessage;
	ai->BotNumConsoleMessages	= BotNumConsoleMessages;
	ai->BotInitialChat			= BotInitialChat;
	ai->BotNumInitialChats		= BotNumInitialChats;
	ai->BotReplyChat			= BotReplyChat;
	ai->BotChatLength			= BotChatLength;
	ai->BotEnterChat			= BotEnterChat;
	ai->BotGetChatMessage		= BotGetChatMessage;
	ai->StringContains			= StringContains;
	ai->BotFindMatch			= BotFindMatch;
	ai->BotMatchVariable		= BotMatchVariable;
	ai->UnifyWhiteSpaces		= UnifyWhiteSpaces;
	ai->BotReplaceSynonyms		= BotReplaceSynonyms;
	ai->BotLoadChatFile			= BotLoadChatFile;
	ai->BotSetChatGender		= BotSetChatGender;
	ai->BotSetChatName			= BotSetChatName;
	//-----------------------------------
	// be_ai_goal.h
	//-----------------------------------
	ai->BotResetGoalState			  = BotResetGoalState;
	ai->BotResetAvoidGoals			  = BotResetAvoidGoals;
	ai->BotRemoveFromAvoidGoals		  = BotRemoveFromAvoidGoals;
	ai->BotPushGoal					  = BotPushGoal;
	ai->BotPopGoal					  = BotPopGoal;
	ai->BotEmptyGoalStack			  = BotEmptyGoalStack;
	ai->BotDumpAvoidGoals			  = BotDumpAvoidGoals;
	ai->BotDumpGoalStack			  = BotDumpGoalStack;
	ai->BotGoalName					  = BotGoalName;
	ai->BotGetTopGoal				  = BotGetTopGoal;
	ai->BotGetSecondGoal			  = BotGetSecondGoal;
	ai->BotChooseLTGItem			  = BotChooseLTGItem;
	ai->BotChooseNBGItem			  = BotChooseNBGItem;
	ai->BotTouchingGoal				  = BotTouchingGoal;
	ai->BotItemGoalInVisButNotVisible = BotItemGoalInVisButNotVisible;
	ai->BotGetLevelItemGoal			  = BotGetLevelItemGoal;
	ai->BotGetNextCampSpotGoal		  = BotGetNextCampSpotGoal;
	ai->BotGetMapLocationGoal		  = BotGetMapLocationGoal;
	ai->BotAvoidGoalTime			  = BotAvoidGoalTime;
	ai->BotInitLevelItems			  = BotInitLevelItems;
	ai->BotUpdateEntityItems		  = BotUpdateEntityItems;
	ai->BotLoadItemWeights			  = BotLoadItemWeights;
	ai->BotFreeItemWeights			  = BotFreeItemWeights;
	ai->BotInterbreedGoalFuzzyLogic	  = BotInterbreedGoalFuzzyLogic;
	ai->BotSaveGoalFuzzyLogic		  = BotSaveGoalFuzzyLogic;
	ai->BotMutateGoalFuzzyLogic		  = BotMutateGoalFuzzyLogic;
	ai->BotAllocGoalState			  = BotAllocGoalState;
	ai->BotFreeGoalState			  = BotFreeGoalState;
	//-----------------------------------
	// be_ai_move.h
	//-----------------------------------
	ai->BotResetMoveState		  = BotResetMoveState;
	ai->BotMoveToGoal			  = BotMoveToGoal;
	ai->BotMoveInDirection		  = BotMoveInDirection;
	ai->BotResetAvoidReach		  = BotResetAvoidReach;
	ai->BotResetLastAvoidReach	  = BotResetLastAvoidReach;
	ai->BotReachabilityArea		  = BotReachabilityArea;
	ai->BotMovementViewTarget	  = BotMovementViewTarget;
	ai->BotPredictVisiblePosition = BotPredictVisiblePosition;
	ai->BotAllocMoveState		  = BotAllocMoveState;
	ai->BotFreeMoveState		  = BotFreeMoveState;
	ai->BotInitMoveState		  = BotInitMoveState;
	// Ridah
	ai->BotInitAvoidReach = BotInitAvoidReach;
	// done.
	//-----------------------------------
	// be_ai_weap.h
	//-----------------------------------
	ai->BotChooseBestFightWeapon = BotChooseBestFightWeapon;
	ai->BotGetWeaponInfo		 = BotGetWeaponInfo;
	ai->BotLoadWeaponWeights	 = BotLoadWeaponWeights;
	ai->BotAllocWeaponState		 = BotAllocWeaponState;
	ai->BotFreeWeaponState		 = BotFreeWeaponState;
	ai->BotResetWeaponState		 = BotResetWeaponState;
	//-----------------------------------
	// be_ai_gen.h
	//-----------------------------------
	ai->GeneticParentsAndChildSelection = GeneticParentsAndChildSelection;
}

botlib_export_t* GetBotLibAPI( int apiVersion, botlib_import_t* import )
{
	botimport = *import;

	memset( &be_botlib_export, 0, sizeof( be_botlib_export ) );

	if( apiVersion != BOTLIB_API_VERSION ) {
		botimport.Print( PRT_ERROR, "Mismatched BOTLIB_API_VERSION: expected %i, got %i\n", BOTLIB_API_VERSION, apiVersion );
		return NULL;
	}

	Init_AAS_Export( &be_botlib_export.aas );
	Init_EA_Export( &be_botlib_export.ea );
	Init_AI_Export( &be_botlib_export.ai );

	be_botlib_export.BotLibSetup		  = Export_BotLibSetup;
	be_botlib_export.BotLibShutdown		  = Export_BotLibShutdown;
	be_botlib_export.BotLibVarSet		  = Export_BotLibVarSet;
	be_botlib_export.BotLibVarGet		  = Export_BotLibVarGet;
	be_botlib_export.PC_AddGlobalDefine	  = PC_AddGlobalDefine;
	be_botlib_export.PC_LoadSourceHandle  = PC_LoadSourceHandle;
	be_botlib_export.PC_FreeSourceHandle  = PC_FreeSourceHandle;
	be_botlib_export.PC_ReadTokenHandle	  = PC_ReadTokenHandle;
	be_botlib_export.PC_SourceFileAndLine = PC_SourceFileAndLine;

	be_botlib_export.BotLibStartFrame	= Export_BotLibStartFrame;
	be_botlib_export.BotLibLoadMap		= Export_BotLibLoadMap;
	be_botlib_export.BotLibUpdateEntity = Export_BotLibUpdateEntity;
	be_botlib_export.Test				= BotExportTest;

	return &be_botlib_export;
}
