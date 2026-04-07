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
 * name:		l_libvar.c
 *
 * desc:		bot library variables
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_memory.h"
#include "l_libvar.h"

// list with library variables
libvar_t* libvarlist;

/*!
	\brief Parses a decimal numeric string into a float, returning 0 if the string is invalid.

	The function scans the input string character by character, accumulating the numeric value. An optional decimal point is allowed and is handled by a ‘dotfound’ multiplier that initially is 0 and
   becomes 10 when a dot is encountered. If any non‑digit character is found after the dot or a second dot appears, the function returns 0 to indicate failure. The result is returned as a float.

	The function is tolerant only to simple decimal representations and treats any deviation (e.g., embedded letters, multiple decimal points, leading or trailing non‑digits) as an error.

	The implementation performs its parsing in a single pass and does not modify the input string.

	\param string Null‑terminated C‑style string containing the numeric value to interpret.
	\return The floating‑point value represented by the string, or 0 if the string is not a valid decimal number.
*/
float	  LibVarStringValue( char* string )
{
	int	  dotfound = 0;
	float value	   = 0;

	while( *string ) {
		if( *string < '0' || *string > '9' ) {
			if( dotfound || *string != '.' ) {
				return 0;

			} else {
				dotfound = 10;
				string++;
			}
		}

		if( dotfound ) {
			value = value + ( float )( *string - '0' ) / ( float )dotfound;
			dotfound *= 10;

		} else {
			value = value * 10.0 + ( float )( *string - '0' );
		}

		string++;
	}

	return value;
}

/*!
	\brief Creates and registers a new library variable with the given name.

	Allocates a block of memory large enough to hold a libvar_t structure followed by a copy of the variable name.
	The newly allocated structure is zeroed and its name pointer is set to the string portion of the allocation.
	The supplied name is copied into that memory.
	The variable is inserted at the head of the global libvarlist and the function returns a pointer to it.

	\param var_name The C string containing the variable name to allocate; the function copies this string into the new struct.
	\return A pointer to the newly allocated libvar_t structure, or null if allocation fails (though the code does not explicitly handle failure).
*/
libvar_t* LibVarAlloc( char* var_name )
{
	libvar_t* v;

	v = ( libvar_t* )GetMemory( sizeof( libvar_t ) + strlen( var_name ) + 1 );
	memset( v, 0, sizeof( libvar_t ) );
	v->name = ( char* )v + sizeof( libvar_t );
	strcpy( v->name, var_name );
	// add the variable in the list
	v->next	   = libvarlist;
	libvarlist = v;
	return v;
}

/*!
	\brief Releases the memory allocated for a libvar_t structure, including its string field if present.

	The function expects a valid libvar_t pointer. If the string member is non-null, FreeMemory is invoked to free that memory, followed by freeing the libvar_t structure itself. No value is returned.
   Passing a null pointer results in undefined behavior due to dereference.

	\param v pointer to the libvar_t instance to deallocate
*/
void LibVarDeAlloc( libvar_t* v )
{
	if( v->string ) {
		FreeMemory( v->string );
	}

	FreeMemory( v );
}

void LibVarDeAllocAll()
{
	libvar_t* v;

	for( v = libvarlist; v; v = libvarlist ) {
		libvarlist = libvarlist->next;
		LibVarDeAlloc( v );
	}

	libvarlist = NULL;
}

libvar_t* LibVarGet( char* var_name )
{
	libvar_t* v;

	for( v = libvarlist; v; v = v->next ) {
		if( !Q_stricmp( v->name, var_name ) ) {
			return v;
		}
	}

	return NULL;
}

char* LibVarGetString( char* var_name )
{
	libvar_t* v;

	v = LibVarGet( var_name );

	if( v ) {
		return v->string;

	} else {
		return "";
	}
}

float LibVarGetValue( char* var_name )
{
	libvar_t* v;

	v = LibVarGet( var_name );

	if( v ) {
		return v->value;

	} else {
		return 0;
	}
}

libvar_t* LibVar( char* var_name, char* value )
{
	libvar_t* v;
	v = LibVarGet( var_name );

	if( v ) {
		return v;
	}

	// create new variable
	v = LibVarAlloc( var_name );
	// variable string
	v->string = ( char* )GetMemory( strlen( value ) + 1 );
	strcpy( v->string, value );
	// the value
	v->value = LibVarStringValue( v->string );
	// variable is modified
	v->modified = qtrue;
	//
	return v;
}

char* LibVarString( char* var_name, char* value )
{
	libvar_t* v;

	v = LibVar( var_name, value );
	return v->string;
}

float LibVarValue( char* var_name, char* value )
{
	libvar_t* v;

	v = LibVar( var_name, value );
	return v->value;
}

void LibVarSet( char* var_name, char* value )
{
	libvar_t* v;

	v = LibVarGet( var_name );

	if( v ) {
		FreeMemory( v->string );

	} else {
		v = LibVarAlloc( var_name );
	}

	// variable string
	v->string = ( char* )GetMemory( strlen( value ) + 1 );
	strcpy( v->string, value );
	// the value
	v->value = LibVarStringValue( v->string );
	// variable is modified
	v->modified = qtrue;
}

qboolean LibVarChanged( char* var_name )
{
	libvar_t* v;

	v = LibVarGet( var_name );

	if( v ) {
		return v->modified;

	} else {
		return qfalse;
	}
}

void LibVarSetNotModified( char* var_name )
{
	libvar_t* v;

	v = LibVarGet( var_name );

	if( v ) {
		v->modified = qfalse;
	}
}
