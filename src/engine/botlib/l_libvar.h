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
 * name:		l_libvar.h
 *
 * desc:		botlib vars
 *
 *
 *****************************************************************************/

// library variable
typedef struct libvar_s {
	char*			 name;
	char*			 string;
	int				 flags;
	qboolean		 modified; // set each time the cvar is changed
	float			 value;
	struct libvar_s* next;
} libvar_t;

/*!
	\brief Frees all library variable structures and clears the list.

	The function iterates over the linked list of library variables, deallocating each node using LibVarDeAlloc and then setting the list head to NULL to indicate an empty list.

*/
void	  LibVarDeAllocAll();

/*!
	\brief Retrieves a library variable with the specified name or returns null if it does not exist.

	The function walks the global linked list of library variables. For each entry it performs a case‑insensitive comparison of the variable's name with the supplied var_name using Q_stricmp. If a
   match is found it immediately returns the associated libvar_t pointer. If the list is exhausted without a match, the function returns NULL. The operation is read‑only and does not alter any data.

	\param var_name Name of the variable to look up.
	\return Pointer to the matching libvar_t or NULL if not found.
*/
libvar_t* LibVarGet( char* var_name );

/*!
	\brief Returns the value string for a named library variable.

	If the variable exists, a pointer to its stored string is returned; otherwise a pointer to an empty string literal is returned. The caller should not attempt to modify the returned string.

	\param var_name the name of the library variable to look up
	\return A null‑terminated string containing the variable's value or an empty string if the name is not found.
*/
char*	  LibVarGetString( char* var_name );

/*!
	\brief Retrieves the value of a library variable by name, returning 0 if the variable does not exist.

	The function queries the library's variable table via LibVarGet using the provided name. If the variable exists, its stored float value is returned. If the lookup fails, the function returns zero
   to indicate that the variable is undefined or not present.

	\param var_name the name of the variable to retrieve
	\return the value associated with the variable, or 0 if the variable was not found
	\throws null
*/
float	  LibVarGetValue( char* var_name );

/*!
	\brief Retrieves an existing library variable or creates a new one and returns its pointer.

	The function first attempts to obtain an existing variable with the given name using LibVarGet. If found, it returns that pointer immediately. When the variable does not exist, LibVarAlloc is
   called to create a new libvar_t structure. The provided value string is then copied into freshly allocated memory and assigned to the variable’s string field. The numeric representation of that
   string is derived via LibVarStringValue and stored in the value field. The variable is marked as modified by setting modified to true and the new pointer is returned. All allocations use GetMemory
   for the string, and no explicit error handling or throwing is performed.

	\param var_name Name of the variable to retrieve or create
	\param value String value to assign when creating a new variable
	\return Pointer to the libvar_t structure representing the requested variable; if already present, the existing pointer is returned; otherwise a new instance is allocated and initialized.
*/
libvar_t* LibVar( char* var_name, char* value );

/*!
	\brief creates a library variable if it does not already exist and returns its numeric value

	Calls LibVar to locate or create a variable named by var_name, initializing it with the string supplied in value if it is new. The string is converted to a floating point number inside LibVar; the
   resulting value is then returned from LibVarValue. This function provides a convenient way to read configuration values with a default fallback.

	\param var_name the name of the variable to obtain or create
	\param value a string representation of the default numeric value to use if the variable is not yet defined
	\return the numeric value stored in the library variable
*/
float	  LibVarValue( char* var_name, char* value );

/*!
	\brief Return the string value of the specified library variable, creating it if it does not exist.

	Calls LibVar with the supplied name and value. LibVar ensures the variable exists, assigning the given value only when creation is required. The function returns the string field of the resulting
   libvar_t instance. The returned pointer is owned by the library variable and should remain valid until the variable is destroyed or its value is modified.


	\param var_name name of the variable to look up or create
	\param value initial value used when the variable is newly created.
	\return pointer to the stored string value associated with the variable name
*/
char*	  LibVarString( char* var_name, char* value );

/*!
	\brief Sets the value of a library variable, creating or updating its string representation and numeric value.

	The function retrieves the library variable structure by name. If it exists, its old string memory is freed; otherwise a new variable entry is allocated. The new string is copied into fresh
   memory, and its numeric value is parsed and stored. Finally, the variable is marked as modified.

	\param var_name name of the variable to set
	\param value new string value for the variable
*/
void	  LibVarSet( char* var_name, char* value );

/*!
	\brief Checks whether a library variable has been modified.

	The function locates the global library variable by name using LibVarGet. If the variable is present, it returns the value of the internal modified flag, indicating whether LibVarSet or similar
   functions have changed its value since creation. If no matching variable exists, the function returns false.

	\param var_name Name of the library variable to query
	\return qtrue if the variable exists and its modified flag is set; otherwise qfalse
*/
qboolean  LibVarChanged( char* var_name );

/*!
	\brief Marks the specified library variable as unmodified.

	The function searches for the variable by name; if found, its modified flag is set to false.

	\param var_name The name of the library variable to modify the flag of.
*/
void	  LibVarSetNotModified( char* var_name );
