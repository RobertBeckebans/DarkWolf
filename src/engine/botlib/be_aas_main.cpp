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
 * name:		be_aas_main.c
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_memory.h"
#include "l_libvar.h"
#include "l_utils.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_log.h"
#include "aasfile.h"
#include "botshared/botlib.h"
#include "botshared/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"

aas_t	   aasworlds[MAX_AAS_WORLDS];

aas_t*	   aasworld;

/*!
	\brief Formats a fatal error message and logs it via botimport.Print.

	The function takes a printf‑style format string followed by variable arguments. It uses a 1024‑byte buffer to format the message with vsprintf. The resulting string is passed to botimport.Print
   with a fatal error level. No value is returned by the function.

	\param ... Variable arguments to fill the format string
	\param fmt Printf‑style format string specifying the error message
*/
void QDECL AAS_Error( char* fmt, ... )
{
	char	str[1024];
	va_list arglist;

	va_start( arglist, fmt );
	vsprintf( str, fmt, arglist );
	va_end( arglist );
	botimport.Print( PRT_FATAL, str );
}

void AAS_SetCurrentWorld( int index )
{
	if( index >= MAX_AAS_WORLDS || index < 0 ) {
		AAS_Error( "AAS_SetCurrentWorld: index out of range\n" );
		return;
	}

	// set the current world pointer
	aasworld = &aasworlds[index];
}

/*!
	\brief Retrieves a string from an indexed array after validating index and setup status.

	The function checks whether the AAS world has been initialized and if the requested index is within bounds. If the index is invalid or the array entry is null, an error message is printed using
   botimport.Print and an empty string is returned. When the index is valid, the function returns the string at that position. The indexname parameter is only used to identify the array in error
   messages.

	\param indexname a descriptive name of the index array, used only for error reporting
	\param stringindex array of null‑terminated strings indexed by integer
	\param numindexes maximum number of valid indices in the array
	\param index the specific index from which to retrieve the string
	\return a pointer to the string at the requested index, or a pointer to the empty string literal if an error occurred or the entry is unused
*/
char* AAS_StringFromIndex( char* indexname, char* stringindex[], int numindexes, int index )
{
	if( !( *aasworld ).indexessetup ) {
		botimport.Print( PRT_ERROR, "%s: index %d not setup\n", indexname, index );
		return "";
	}

	if( index < 0 || index >= numindexes ) {
		botimport.Print( PRT_ERROR, "%s: index %d out of range\n", indexname, index );
		return "";
	}

	if( !stringindex[index] ) {
		if( index ) {
			botimport.Print( PRT_ERROR, "%s: reference to unused index %d\n", indexname, index );
		}

		return "";
	}

	return stringindex[index];
}

/*!
	\brief Returns the index of a specified string in an array of configuration strings or zero when not found or indices are not initialized.

	The function first checks whether the AAS world has completed its string index setup; if not, it logs an error message that includes the supplied index name and the string being searched for, then
   returns zero. It then iterates over the array of string pointers, skipping any null entries. For each non‑null entry, it performs a case‑insensitive comparison with the target string. If a match is
   found, the function immediately returns the current array index. If the loop finishes without finding a match, the function returns zero, which serves as the default value signalling that the
   requested string was not present.

	\param indexname a name used only in the error message when indices are not set up
	\param stringindex an array of pointers to configuration strings to search
	\param numindexes the number of elements in the stringindex array
	\param string the string whose index is to be found
	\return The array index of the string if a match is found; otherwise zero.
*/
int AAS_IndexFromString( char* indexname, char* stringindex[], int numindexes, char* string )
{
	int i;

	if( !( *aasworld ).indexessetup ) {
		botimport.Print( PRT_ERROR, "%s: index not setup \"%s\"\n", indexname, string );
		return 0;
	}

	for( i = 0; i < numindexes; i++ ) {
		if( !stringindex[i] ) {
			continue;
		}

		if( !Q_stricmp( stringindex[i], string ) ) {
			return i;
		}
	}

	return 0;
}

char* AAS_ModelFromIndex( int index )
{
	//	return AAS_StringFromIndex("ModelFromIndex", &(*aasworld).configstrings[CS_MODELS], MAX_MODELS, index);
	return 0; // removed so the CS_ defines could be removed from be_aas_def.h
}

int AAS_IndexFromModel( char* modelname )
{
	//	return AAS_IndexFromString("IndexFromModel", &(*aasworld).configstrings[CS_MODELS], MAX_MODELS, modelname);
	return 0; // removed so the CS_ defines could be removed from be_aas_def.h
}

/*!
	\brief Copies the supplied configuration strings into the AAS world's string table and marks the indexes as set up.

	The routine loops over the provided array of strings. For each non‑null entry it allocates a buffer large enough to hold the string plus a terminating null, copies the source into that buffer, and
   stores the pointer in the world structure. After all strings have been processed the flag indexessetup is set to true, indicating that the index data is ready for use. Any previously allocated
   string buffers are not freed – the code that would do that is commented out.

	\param numconfigstrings Number of strings in the array that should be processed
	\param configstrings Array of pointers to null‑terminated configurations; each element is copied if it is non‑null
*/
void AAS_UpdateStringIndexes( int numconfigstrings, char* configstrings[] )
{
	int i;

	// set string pointers and copy the strings
	for( i = 0; i < numconfigstrings; i++ ) {
		if( configstrings[i] ) {
			// if ((*aasworld).configstrings[i]) FreeMemory((*aasworld).configstrings[i]);
			( *aasworld ).configstrings[i] = ( char* )GetMemory( strlen( configstrings[i] ) + 1 );
			strcpy( ( *aasworld ).configstrings[i], configstrings[i] );
		}
	}

	( *aasworld ).indexessetup = qtrue;
}

int AAS_Loaded()
{
	return ( *aasworld ).loaded;
}

int AAS_Initialized()
{
	return ( *aasworld ).initialized;
}

/*!
	\brief Sets the global AAS world as initialized, prints a message, and builds the routing table.

	The function marks the AAS data structure as initialized and informs the user via a console message.
	In non-debug builds, it then constructs the route table by invoking AAS_RT_BuildRouteTable.
	In debug builds, commented code suggests optional creation of routing caches or printing routing information, but these are currently disabled.

*/
void AAS_SetInitialized()
{
	( *aasworld ).initialized = qtrue;
	botimport.Print( PRT_MESSAGE, "AAS initialized.\n" );
#ifdef DEBUG
	// create all the routing cache
	// AAS_CreateAllRoutingCache();
	//
	// AAS_RoutingInfo();
#endif

	// Ridah, build/load the route-table
	AAS_RT_BuildRouteTable();
	// done.
}

/*!
	\brief Continues AAS world initialization within the given time slice

	The routine first checks whether an AAS world has been loaded and whether it has already been fully initialized; if either condition is false it returns immediately. It then calls
   AAS_ContinueInitReachability with the supplied time budget; if the reachability phase has not yet finished, the function exits to be called again later. Once reachability is complete, the function
   proceeds to initialize clustering, and if a save file is requested or a force write flag is set, it may optimize the data (unless optimization is disabled) and attempts to write the AAS file to
   disk, reporting success or failure. Finally it initializes routing and marks the world as initialized.

	Calls to botimport.Print are used to surface the outcome of the write operation.

	\param time The maximum time (in seconds) allotted to finalizing reachability during this invocation.
*/
void AAS_ContinueInit( float time )
{
	// if no AAS file loaded
	if( !( *aasworld ).loaded ) {
		return;
	}

	// if AAS is already initialized
	if( ( *aasworld ).initialized ) {
		return;
	}

	// calculate reachability, if not finished return
	if( AAS_ContinueInitReachability( time ) ) {
		return;
	}

	// initialize clustering for the new map
	AAS_InitClustering();

	// if reachability has been calculated and an AAS file should be written
	// or there is a forced data optimization
	if( ( *aasworld ).savefile || ( ( int )LibVarGetValue( "forcewrite" ) ) ) {
		// optimize the AAS data
		if( !( ( int )LibVarValue( "nooptimize", "1" ) ) ) {
			AAS_Optimize();
		}

		// save the AAS file
		if( AAS_WriteAASFile( ( *aasworld ).filename ) ) {
			botimport.Print( PRT_MESSAGE, "%s written succesfully\n", ( *aasworld ).filename );

		} else {
			botimport.Print( PRT_ERROR, "couldn't write %s\n", ( *aasworld ).filename );
		}
	}

	// initialize the routing
	AAS_InitRouting();
	// at this point AAS is initialized
	AAS_SetInitialized();
}

/*!
	\brief Initializes data for a new frame across all AAS worlds and returns a success code.

	The function iterates over every AAS world defined by MAX_AAS_WORLDS, sets the current world, assigns the supplied frame time, invalidates all entities, continues initialization for that world,
   and resets the routing update counter for the frame. After processing all worlds, the global frame counter is incremented.

	All operations are performed through global state manipulation; no dynamic memory is allocated or deallocated in this function. The routine always ends by returning the constant BLERR_NOERROR,
   indicating a successful start of the frame.

	\param time The time value for the frame, typically the global frame counter or system time.
	\return BLERR_NOERROR
*/
int AAS_StartFrame( float time )
{
	// Ridah, do each of the aasworlds
	int i;

	for( i = 0; i < MAX_AAS_WORLDS; i++ ) {
		AAS_SetCurrentWorld( i );

		( *aasworld ).time = time;
		// invalidate the entities
		AAS_InvalidateEntities();
		// initialize AAS
		AAS_ContinueInit( time );
		//
		( *aasworld ).frameroutingupdates = 0;
		//
		/* Ridah, disabled for speed
		if (LibVarGetValue("showcacheupdates"))
		{
			AAS_RoutingInfo();
			LibVarSet("showcacheupdates", "0");
		}
		if (LibVarGetValue("showmemoryusage"))
		{
			PrintUsedMemorySize();
			LibVarSet("showmemoryusage", "0");
		}
		if (LibVarGetValue("memorydump"))
		{
			PrintMemoryLabels();
			LibVarSet("memorydump", "0");
		}
		*/
	}

	( *aasworld ).numframes++;
	return BLERR_NOERROR;
}

float AAS_Time()
{
	return ( *aasworld ).time;
}

/*!
	\brief AAS_LoadFiles loads the BSP and AAS files for the specified map and initializes global state

	The function first copies the supplied map name into the global world structure, resets entity links, loads the BSP file and then loads the corresponding .aas file. It prints a message on success,
   copies the full filename into the global structure and returns a status code.

	\param mapname name of the map without extension
	\return BLERR_NOERROR if both files are loaded successfully, otherwise the error code returned by AAS_LoadAASFile
*/
int AAS_LoadFiles( const char* mapname )
{
	int	 errnum;
	char aasfile[MAX_PATH];
	//	char bspfile[MAX_PATH];

	strcpy( ( *aasworld ).mapname, mapname );
	// NOTE: first reset the entity links into the AAS areas and BSP leaves
	//  the AAS link heap and BSP link heap are reset after respectively the
	//  AAS file and BSP file are loaded
	AAS_ResetEntityLinks();
	//

	// load bsp info
	AAS_LoadBSPFile();

	// load the aas file
	Com_sprintf( aasfile, MAX_PATH, "maps/%s.aas", mapname );
	errnum = AAS_LoadAASFile( aasfile );

	if( errnum != BLERR_NOERROR ) {
		return errnum;
	}

	botimport.Print( PRT_MESSAGE, "loaded %s\n", aasfile );
	strncpy( ( *aasworld ).filename, aasfile, MAX_PATH );
	return BLERR_NOERROR;
}

/*!
	\brief Attempts to load AAS map data for all supported world indices from a base map name and returns a status code.

	The function iterates over all possible AAS world indices. For each index it constructs a filename derived from the supplied map name by appending the suffix "_b" and the world number. The current
   world is set, any previously allocated routing caches are freed, and then the map file is loaded. If a file fails to load, the function records the error but continues trying the remaining worlds.
   After successfully loading a map it initializes several AAS subsystems, such as settings, link heap, linked entities, reachability, and alternative routing.

	If no mapname is supplied the function terminates immediately and reports success. If none of the attempts succeeds, the function returns the last error code encountered; otherwise it returns 0 to
   indicate success.

	The return value is one of the BLERR_* error codes: BLERR_NOERROR for success or a specific error code for failure.

	\param mapname the base name of the map to load; used to construct world‑specific filenames
	\return Zero indicates all relevant files were loaded; non‑zero denotes a failure and points to a BLERR_* error value.
*/
int AAS_LoadMap( const char* mapname )
{
	int		 errnum;
	int		 i;
	char	 this_mapname[256], intstr[4];
	qboolean loaded		   = qfalse;
	int		 missingErrNum = 0; // TTimo: init

	for( i = 0; i < MAX_AAS_WORLDS; i++ ) {
		AAS_SetCurrentWorld( i );

		strncpy( this_mapname, mapname, 256 );
		strncat( this_mapname, "_b", 256 );
		sprintf( intstr, "%i", i );
		strncat( this_mapname, intstr, 256 );

		// if no mapname is provided then the string indexes are updated
		if( !mapname ) {
			return 0;
		}

		//
		( *aasworld ).initialized = qfalse;
		// NOTE: free the routing caches before loading a new map because
		//  to free the caches the old number of areas, number of clusters
		//  and number of areas in a clusters must be available
		AAS_FreeRoutingCaches();

		// load the map
		errnum = AAS_LoadFiles( this_mapname );

		if( errnum != BLERR_NOERROR ) {
			( *aasworld ).loaded = qfalse;
			// RF, we are allowed to skip one of the files, but not both
			// return errnum;
			missingErrNum = errnum;
			continue;
		}

		//
		loaded = qtrue;
		//
		AAS_InitSettings();

		// initialize the AAS link heap for the new map
		AAS_InitAASLinkHeap();

		// initialize the AAS linked entities for the new map
		AAS_InitAASLinkedEntities();

		// initialize reachability for the new map
		AAS_InitReachability();

		// initialize the alternative routing
		AAS_InitAlternativeRouting();
	}

	if( !loaded ) {
		return missingErrNum;
	}

	// everything went ok
	return 0;
}

/*!
	\brief Initializes the AAS subsystem by setting up default world parameters and allocating entity memory.

	The function resets the AAS world to its default state, loads configuration values for maximum clients and entities, frees any previously allocated entity memory if it exists, then allocates new
   cleared memory for all entities. It invalidates all current entities and resets the frame counter. The code also contains commented hints for forcing clustering and reachability calculations. It
   returns a standard error code from the library, normally indicating success.

	\return Error code, usually BLERR_NOERROR on success.
*/
int AAS_Setup()
{
	// Ridah, just use the default world for entities
	AAS_SetCurrentWorld( 0 );

	( *aasworlds ).maxclients  = ( int )LibVarValue( "maxclients", "128" );
	( *aasworlds ).maxentities = ( int )LibVarValue( "maxentities", "1024" );

	// allocate memory for the entities
	if( ( *aasworld ).entities ) {
		FreeMemory( ( *aasworld ).entities );
	}

	( *aasworld ).entities = ( aas_entity_t* )GetClearedHunkMemory( ( *aasworld ).maxentities * sizeof( aas_entity_t ) );

	// invalidate all the entities
	AAS_InvalidateEntities();

	// force some recalculations
	// LibVarSet("forceclustering", "1");			//force clustering calculation
	// LibVarSet("forcereachability", "1");		//force reachability calculation
	( *aasworld ).numframes = 0;
	return BLERR_NOERROR;
}

/*!
	\brief Shuts down the AAS system by freeing routing tables, caches, heaps, and linked data for every world.

	The function iterates through all AAS worlds. For each one it sets the current world, shuts down the route table, clears alternative routing information, outputs BSP and AAS data dumps, frees
   routing caches, the AAS link heap, and linked entities. For the first world it additionally releases entity data if present. After processing, it clears the world structure, marks it as
   uninitialized, and prints a shutdown confirmation. Memory for the BSP file itself is handled elsewhere and is not freed here.

*/
void AAS_Shutdown()
{
	// Ridah, do each of the worlds
	int i;

	for( i = 0; i < MAX_AAS_WORLDS; i++ ) {
		AAS_SetCurrentWorld( i );

		// Ridah, kill the route-table data
		AAS_RT_ShutdownRouteTable();

		AAS_ShutdownAlternativeRouting();
		AAS_DumpBSPData();

		// free routing caches
		AAS_FreeRoutingCaches();

		// free aas link heap
		AAS_FreeAASLinkHeap();

		// free aas linked entities
		AAS_FreeAASLinkedEntities();

		// free the aas data
		AAS_DumpAASData();

		if( i == 0 ) {
			// free the entities
			if( ( *aasworld ).entities ) {
				FreeMemory( ( *aasworld ).entities );
			}
		}

		// clear the (*aasworld) structure
		memset( &( *aasworld ), 0, sizeof( aas_t ) );
		// aas has not been initialized
		( *aasworld ).initialized = qfalse;
	}

	// NOTE: as soon as a new .bsp file is loaded the .bsp file memory is
	//  freed an reallocated, so there's no need to free that memory here
	// print shutdown
	botimport.Print( PRT_MESSAGE, "AAS shutdown.\n" );
}
