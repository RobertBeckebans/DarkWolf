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
 * name:		be_aas_main.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#ifdef AASINTERN

extern aas_t( *aasworld );

// AAS error message
void QDECL AAS_Error( char* fmt, ... );
// set AAS initialized
void	   AAS_SetInitialized();
// setup AAS with the given number of entities and clients
int		   AAS_Setup();
// shutdown AAS
void	   AAS_Shutdown();
// start a new map
int		   AAS_LoadMap( const char* mapname );
// start a new time frame
int		   AAS_StartFrame( float time );
#endif // AASINTERN

/*!
	\brief Return a nonzero value when the AAS system has been initialized.

	The function accesses the global AAS world structure and returns its initialized flag. An initialized value of zero indicates that the AAS data has not been loaded, while any non‑zero value
   signals that the AAS is ready for use. This check is performed by the bot libraries before performing AAS queries.

	\return An integer that is zero when the AAS is not initialized; otherwise a non‑zero value indicates initialization.
*/
int	  AAS_Initialized();

/*!
	\brief Returns a value indicating whether the AAS file is loaded.

	The function checks the global aasworld structure's loaded flag and returns its value. A non-zero return signifies that the AAS data has been successfully loaded, while zero indicates it is not
   loaded.

	\return int; non-zero if the AAS file is loaded, otherwise zero.
*/
int	  AAS_Loaded();

/*!
	\brief retrieves the model name for a given index

	Historically this function returned a pointer to the model name string from the configuration strings using a lookup helper. The CS_MODELS defines were removed, so the function currently returns a
   null pointer and acts as a placeholder for a future implementation. It should return the model name or null if the index is out of bounds or not implemented.

	\param index the index into the model configuration strings
	\return a pointer to the model name string for the given index, or null if the index is invalid or the function is not yet implemented
*/
char* AAS_ModelFromIndex( int index );

/*!
	\brief Obtains the AAS numeric index for a specified model name

	Historically this function searched the AI navigation system's model configuration strings for the given filename and returned its corresponding index. Within this code base the lookup logic has
   been removed, leaving the function as a placeholder that currently returns zero. The intended behavior is to map a model string such as \"models/weapons/grapple/hook/tris.md2\" to the internal
   model index used by the AAS system, returning 0 when the model is not present. TODO: clarify whether this placeholder should be replaced with working logic or deprecated.

	\param modelname The model file name, including path and extension, to look up in the AAS configuration
	\return The numeric configuration index for the specified model, or 0 if the model is not found or the function is in its placeholder form.
*/
int	  AAS_IndexFromModel( char* modelname );

/*!
	\brief Returns the current AAS world time.

	This function accesses the global AAS world structure and retrieves the value stored in its time member. The returned value is a floating point representation of the current simulation time within
   the AAS system.

	\return The current time value from the AAS world time field.
*/
float AAS_Time();

/*!
	\brief Sets the current AAS world to the world at the given index.

	The function verifies that the supplied index lies within the valid range [0, MAX_AAS_WORLDS-1]. If the index is outside this range an error message is emitted via AAS_Error and the function exits
   without changing the current world. On success it sets the global pointer to the corresponding world entry.

	\param index zero‑based index of the world to activate
*/
void  AAS_SetCurrentWorld( int index );
// done.
