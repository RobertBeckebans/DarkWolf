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
 * name:		be_ai_char.c
 *
 * desc:		bot characters
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_log.h"
#include "l_memory.h"
#include "l_utils.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_libvar.h"
#include "aasfile.h"
#include "botshared/botlib.h"
#include "botshared/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "botshared/be_ai_char.h"

#define MAX_CHARACTERISTICS 80

#define CT_INTEGER			1
#define CT_FLOAT			2
#define CT_STRING			3

#define DEFAULT_CHARACTER	"bots/default_c.c"

// characteristic value
union cvalue {
	int	  integer;
	float _float;
	char* string;
};

// a characteristic
typedef struct bot_characteristic_s {
	char		 type;	// characteristic type
	union cvalue value; // characteristic value
} bot_characteristic_t;

// a bot character
typedef struct bot_character_s {
	char				 filename[MAX_QPATH];
	int					 skill;
	bot_characteristic_t c[1]; // variable sized
} bot_character_t;

bot_character_t* botcharacters[MAX_CLIENTS + 1];

/*!
	\brief Retrieves a bot character pointer from a global array given its handle.

	This function looks up the global botcharacters array using the provided handle, which must be within the range 1 to MAX_CLIENTS inclusive. If the handle is outside this range or if the array
   entry is NULL, the function prints a fatal error using botimport.Print and returns NULL. Otherwise it returns the pointer to the requested bot_character_t.

	\param handle the numeric identifier for a bot character; must be in the valid range 1..MAX_CLIENTS.
	\return a pointer to the bot_character_t at the specified handle, or NULL if the handle is out of range, invalid, or the slot is unused.
*/
bot_character_t* BotCharacterFromHandle( int handle )
{
	if( handle <= 0 || handle > MAX_CLIENTS ) {
		botimport.Print( PRT_FATAL, "character handle %d out of range\n", handle );
		return NULL;
	}

	if( !botcharacters[handle] ) {
		botimport.Print( PRT_FATAL, "invalid character %d\n", handle );
		return NULL;
	}

	return botcharacters[handle];
}

/*!
	\brief Logs the filename, skill level, and characteristics of a bot character.

	The function writes the character's filename and skill to the log. It then opens a bracket, iterates over all defined characteristics, and logs each one according to its type: integers, floats, or
   strings. After the list, it writes a closing bracket.

	\param ch pointer to a bot_character_t structure whose information will be written to the log
*/
void BotDumpCharacter( bot_character_t* ch )
{
	int i;

	Log_Write( "%s", ch->filename );
	Log_Write( "skill %d\n", ch->skill );
	Log_Write( "{\n" );

	for( i = 0; i < MAX_CHARACTERISTICS; i++ ) {
		switch( ch->c[i].type ) {
			case CT_INTEGER:
				Log_Write( " %4d %d\n", i, ch->c[i].value.integer );
				break;

			case CT_FLOAT:
				Log_Write( " %4d %f\n", i, ch->c[i].value._float );
				break;

			case CT_STRING:
				Log_Write( " %4d %s\n", i, ch->c[i].value.string );
				break;
		} // end case
	}

	Log_Write( "}\n" );
}

/*!
	\brief Frees all dynamically allocated string values stored in a bot character's characteristics.

	Iterates over every characteristic in the character structure and releases the memory for those of type CT_STRING by calling FreeMemory on the stored string pointer. This cleans up allocated
   resources prior to reusing or destroying the character.

	\param ch pointer to the bot_character_t whose characteristic strings should be freed.
*/
void BotFreeCharacterStrings( bot_character_t* ch )
{
	int i;

	for( i = 0; i < MAX_CHARACTERISTICS; i++ ) {
		if( ch->c[i].type == CT_STRING ) {
			FreeMemory( ch->c[i].value.string );
		}
	}
}

/*!
	\brief Releases the memory and string resources associated with a bot character identified by its handle.

	First verifies that the provided handle falls within the allowed range [1, MAX_CLIENTS]; if not, it logs a fatal message and exits. Second checks that a bot character exists at that index; if
   missing, logs a fatal message. Upon successful validation, it calls BotFreeCharacterStrings to release any allocated strings, frees the character structure, and clears the global reference to
   prevent dangling pointers. The function has no return value and only performs cleanup.

	\param handle Index of the bot character to free; valid values are 1 through MAX_CLIENTS inclusive.
*/
void BotFreeCharacter2( int handle )
{
	if( handle <= 0 || handle > MAX_CLIENTS ) {
		botimport.Print( PRT_FATAL, "character handle %d out of range\n", handle );
		return;
	}

	if( !botcharacters[handle] ) {
		botimport.Print( PRT_FATAL, "invalid character %d\n", handle );
		return;
	}

	BotFreeCharacterStrings( botcharacters[handle] );
	FreeMemory( botcharacters[handle] );
	botcharacters[handle] = NULL;
}

/*!
	\brief Frees a bot character when reloading of characters is enabled.

	The function checks the value of the configuration variable "bot_reloadcharacters".  If the value is false, the function exits immediately and the character remains allocated.  When the variable
   is true, the internal routine BotFreeCharacter2 is called to perform the actual release of resources associated with the character handle.

	\param handle identifier of the character to free
*/
void BotFreeCharacter( int handle )
{
	if( !LibVarGetValue( "bot_reloadcharacters" ) ) {
		return;
	}

	BotFreeCharacter2( handle );
}

/*!
	\brief Copies default characteristics into a bot character when its values are unset.

	Iterates over all characteristic indices. When the destination characteristic type is zero, it copies the type and value from the provided default structure. For float and integer types, it copies
   directly. For string types, it allocates memory via GetMemory, then duplicates the source string into the allocated space.

	\param ch Destination bot_character_t to be filled with default values when unset.
	\param defaultch Reference bot_character_t providing default characteristic values to copy from.
*/
void BotDefaultCharacteristics( bot_character_t* ch, bot_character_t* defaultch )
{
	int i;

	for( i = 0; i < MAX_CHARACTERISTICS; i++ ) {
		if( ch->c[i].type ) {
			continue;
		}

		//
		if( defaultch->c[i].type == CT_FLOAT ) {
			ch->c[i].type		  = CT_FLOAT;
			ch->c[i].value._float = defaultch->c[i].value._float;

		} else if( defaultch->c[i].type == CT_INTEGER ) {
			ch->c[i].type		   = CT_INTEGER;
			ch->c[i].value.integer = defaultch->c[i].value.integer;

		} else if( defaultch->c[i].type == CT_STRING ) {
			ch->c[i].type		  = CT_STRING;
			ch->c[i].value.string = ( char* )GetMemory( strlen( defaultch->c[i].value.string ) + 1 );
			strcpy( ch->c[i].value.string, defaultch->c[i].value.string );
		}
	}
}

/*!
	\brief Loads a bot character from a file for a specified skill level

	The function reads the character definition file and searches for a block beginning with a "skill" token matching the requested skill value (or any skill when a negative value is passed). It then
   parses an array of characteristics, each consisting of an integer index and a value that may be an integer, float or string. Values are stored in a newly allocated bot_character_t structure, string
   values being duplicated with GetMemory. If the file cannot be loaded, the requested skill block is absent, or any parsing error occurs, the function cleans up allocated resources and returns NULL.
   Diagnostic messages are emitted through botimport.Print.

	\param charfile Path to the character file to load
	\param skill Desired skill level to look for, or a negative value to accept any skill
	\return Pointer to the constructed bot_character_t, or NULL on error
*/
bot_character_t* BotLoadCharacterFromFile( char* charfile, int skill )
{
	int				 indent, index, foundcharacter;
	bot_character_t* ch;
	source_t*		 source;
	token_t			 token;

	foundcharacter = qfalse;
	// a bot character is parsed in two phases
	source = LoadSourceFile( charfile );

	if( !source ) {
		botimport.Print( PRT_ERROR, "counldn't load %s\n", charfile );
		return NULL;
	}

	ch = ( bot_character_t* )GetClearedMemory( sizeof( bot_character_t ) + MAX_CHARACTERISTICS * sizeof( bot_characteristic_t ) );
	strcpy( ch->filename, charfile );

	while( PC_ReadToken( source, &token ) ) {
		if( !strcmp( token.string, "skill" ) ) {
			if( !PC_ExpectTokenType( source, TT_NUMBER, 0, &token ) ) {
				FreeSource( source );
				BotFreeCharacterStrings( ch );
				FreeMemory( ch );
				return NULL;
			}

			if( !PC_ExpectTokenString( source, "{" ) ) {
				FreeSource( source );
				BotFreeCharacterStrings( ch );
				FreeMemory( ch );
				return NULL;
			}

			// if it's the correct skill
			if( skill < 0 || token.intvalue == skill ) {
				foundcharacter = qtrue;
				ch->skill	   = token.intvalue;

				while( PC_ExpectAnyToken( source, &token ) ) {
					if( !strcmp( token.string, "}" ) ) {
						break;
					}

					if( token.type != TT_NUMBER || !( token.subtype & TT_INTEGER ) ) {
						SourceError( source, "expected integer index, found %s\n", token.string );
						FreeSource( source );
						BotFreeCharacterStrings( ch );
						FreeMemory( ch );
						return NULL;
					}

					index = token.intvalue;

					if( index < 0 || index > MAX_CHARACTERISTICS ) {
						SourceError( source, "characteristic index out of range [0, %d]\n", MAX_CHARACTERISTICS );
						FreeSource( source );
						BotFreeCharacterStrings( ch );
						FreeMemory( ch );
						return NULL;
					}

					if( ch->c[index].type ) {
						SourceError( source, "characteristic %d already initialized\n", index );
						FreeSource( source );
						BotFreeCharacterStrings( ch );
						FreeMemory( ch );
						return NULL;
					}

					if( !PC_ExpectAnyToken( source, &token ) ) {
						FreeSource( source );
						BotFreeCharacterStrings( ch );
						FreeMemory( ch );
						return NULL;
					}

					if( token.type == TT_NUMBER ) {
						if( token.subtype & TT_FLOAT ) {
							ch->c[index].value._float = token.floatvalue;
							ch->c[index].type		  = CT_FLOAT;

						} else {
							ch->c[index].value.integer = token.intvalue;
							ch->c[index].type		   = CT_INTEGER;
						}

					} else if( token.type == TT_STRING ) {
						StripDoubleQuotes( token.string );
						ch->c[index].value.string = ( char* )GetMemory( strlen( token.string ) + 1 );
						strcpy( ch->c[index].value.string, token.string );
						ch->c[index].type = CT_STRING;

					} else {
						SourceError( source, "expected integer, float or string, found %s\n", token.string );
						FreeSource( source );
						BotFreeCharacterStrings( ch );
						FreeMemory( ch );
						return NULL;
					}
				}

				break;

			} else {
				indent = 1;

				while( indent ) {
					if( !PC_ExpectAnyToken( source, &token ) ) {
						FreeSource( source );
						BotFreeCharacterStrings( ch );
						FreeMemory( ch );
						return NULL;
					}

					if( !strcmp( token.string, "{" ) ) {
						indent++;

					} else if( !strcmp( token.string, "}" ) ) {
						indent--;
					}
				}
			}

		} else {
			SourceError( source, "unknown definition %s\n", token.string );
			FreeSource( source );
			BotFreeCharacterStrings( ch );
			FreeMemory( ch );
			return NULL;
		}
	}

	FreeSource( source );

	//
	if( !foundcharacter ) {
		BotFreeCharacterStrings( ch );
		FreeMemory( ch );
		return NULL;
	}

	return ch;
}

/*!
	\brief Finds the handle of a cached character file with a specified skill level, or any skill if none is indicated.

	The function iterates over the botcharacter array indexed by client handles. For each existing entry, it compares the stored filename with the supplied charfile. If the filenames match and the
   requested skill is non‑negative and equals the entry’s skill, or the requested skill is negative then any skill matches, the corresponding handle is returned. If no match is found the function
   returns 0.

	\param charfile filename of the character file to locate
	\param skill desired skill level; if negative, matches any cached skill
	\return Handle number of the cached character, or 0 if not found
*/
int BotFindCachedCharacter( char* charfile, int skill )
{
	int handle;

	for( handle = 1; handle <= MAX_CLIENTS; handle++ ) {
		if( !botcharacters[handle] ) {
			continue;
		}

		if( strcmp( botcharacters[handle]->filename, charfile ) == 0 && ( skill < 0 || botcharacters[handle]->skill == skill ) ) {
			return handle;
		}
	}

	return 0;
}

/*!
	\brief Loads a bot character from a file, optionally using cached data and a specified skill level.

	The function first locates a free slot in the global character array.
	If a fresh load is not requested (reload==0), it checks for an existing cached entry matching both the file and skill.
	When a cache hit occurs, the cached character handle is returned immediately.
	If no cached candidate is found or a reload is requested, the function attempts to create a new character by reading the file at the requested skill level.
	Failing that, it falls back to the default character file with the same skill.
	Should the default also fail, the routine relaxes the skill constraint, looking first for a cached entry with any skill, then loading from the original file again with any skill.
	Finally, it tries the default character file with any skill.
	If all of these attempts fail, the function logs a warning and returns 0.
	Successful loads are stored in the global array and a message is printed.
	The return value is the slot index of the loaded character or 0 if the process fails.

	\param charfile Path to the character file to load
	\param skill Desired skill level, 0‑n for specific or –1 for any
	\param reload Non‑zero forces a fresh load, zero allows use of cached characters
	\return The index of the loaded character, or 0 on failure
*/
int BotLoadCachedCharacter( char* charfile, int skill, int reload )
{
	int				 handle, cachedhandle;
	bot_character_t* ch = NULL;
#ifdef DEBUG
	int starttime;

	starttime = Sys_MilliSeconds();
#endif // DEBUG

	// find a free spot for a character
	for( handle = 1; handle <= MAX_CLIENTS; handle++ ) {
		if( !botcharacters[handle] ) {
			break;
		}
	}

	if( handle > MAX_CLIENTS ) {
		return 0;
	}

	// try to load a cached character with the given skill
	if( !reload ) {
		cachedhandle = BotFindCachedCharacter( charfile, skill );

		if( cachedhandle ) {
			botimport.Print( PRT_MESSAGE, "loaded cached skill %d from %s\n", skill, charfile );
			return cachedhandle;
		}
	}

	// try to load the character with the given skill
	ch = BotLoadCharacterFromFile( charfile, skill );

	if( ch ) {
		botcharacters[handle] = ch;
		//
		botimport.Print( PRT_MESSAGE, "loaded skill %d from %s\n", skill, charfile );
#ifdef DEBUG

		if( bot_developer ) {
			botimport.Print( PRT_MESSAGE, "skill %d loaded in %d msec from %s\n", skill, Sys_MilliSeconds() - starttime, charfile );
		}

#endif // DEBUG
		return handle;
	}

	//
	botimport.Print( PRT_WARNING, "couldn't find skill %d in %s\n", skill, charfile );

	//
	if( !reload ) {
		// try to load a cached default character with the given skill
		cachedhandle = BotFindCachedCharacter( "bots/default_c.c", skill );

		if( cachedhandle ) {
			botimport.Print( PRT_MESSAGE, "loaded cached default skill %d from %s\n", skill, charfile );
			return cachedhandle;
		}
	}

	// try to load the default character with the given skill
	ch = BotLoadCharacterFromFile( DEFAULT_CHARACTER, skill );

	if( ch ) {
		botcharacters[handle] = ch;
		botimport.Print( PRT_MESSAGE, "loaded default skill %d from %s\n", skill, charfile );
		return handle;
	}

	//
	if( !reload ) {
		// try to load a cached character with any skill
		cachedhandle = BotFindCachedCharacter( charfile, -1 );

		if( cachedhandle ) {
			botimport.Print( PRT_MESSAGE, "loaded cached skill %d from %s\n", botcharacters[cachedhandle]->skill, charfile );
			return cachedhandle;
		}
	}

	// try to load a character with any skill
	ch = BotLoadCharacterFromFile( charfile, -1 );

	if( ch ) {
		botcharacters[handle] = ch;
		botimport.Print( PRT_MESSAGE, "loaded skill %d from %s\n", ch->skill, charfile );
		return handle;
	}

	//
	if( !reload ) {
		// try to load a cached character with any skill
		cachedhandle = BotFindCachedCharacter( DEFAULT_CHARACTER, -1 );

		if( cachedhandle ) {
			botimport.Print( PRT_MESSAGE, "loaded cached default skill %d from %s\n", botcharacters[cachedhandle]->skill, charfile );
			return cachedhandle;
		}
	}

	// try to load a character with any skill
	ch = BotLoadCharacterFromFile( DEFAULT_CHARACTER, -1 );

	if( ch ) {
		botcharacters[handle] = ch;
		botimport.Print( PRT_MESSAGE, "loaded default skill %d from %s\n", ch->skill, charfile );
		return handle;
	}

	//
	botimport.Print( PRT_WARNING, "couldn't load any skill from %s\n", charfile );
	// couldn't load any character
	return 0;
}

/*!
	\brief Loads a character file for a specified skill, merging in default characteristics if both are available.

	The function first loads the default character (using the predefined DEFAULT_CHARACTER) for the requested skill without forcing a reload. It then loads the character from the specified file,
   optionally reloading if the global variable bot_reloadcharacters is true. If both the default and requested characters load successfully, it copies the default characteristics into the loaded
   character state. Finally, it returns the handle to the loaded character, or 0 if loading failed.

	\param charfile path to the character file to load
	\param skill desired skill level (typically 1–5)
	\return handle to the loaded character, or 0 on failure
*/
int BotLoadCharacterSkill( char* charfile, int skill )
{
	int ch, defaultch;

	defaultch = BotLoadCachedCharacter( DEFAULT_CHARACTER, skill, qfalse );
	ch		  = BotLoadCachedCharacter( charfile, skill, LibVarGetValue( "bot_reloadcharacters" ) );

	if( defaultch && ch ) {
		BotDefaultCharacteristics( botcharacters[ch], botcharacters[defaultch] );
	}

	return ch;
}

/*!
	\brief Interpolates parameters between two bot characters to create a new character at a desired skill level.

	This routine takes two existing bot character handles and a target skill level, then produces a new character whose characteristics are linearly interpolated between the two inputs. It looks up
   the source characters by handle; if either is missing, or if no free slot in the global array is available, it returns 0. A new character structure is allocated, its skill field set to the desired
   level, and its filename copied from the first source. For each characteristic slot up to MAX_CHARACTERISTICS, floating‑point values are interpolated using a scale factor derived from the skill
   difference, integer values are copied wholesale, and string values are duplicated into newly allocated memory. The new handle is stored in the global array and returned.

	\param handle1 Handle of the first source character
	\param handle2 Handle of the second source character
	\param desiredskill Target skill level for the new interpolated character
	\return Handle of the newly created interpolated character or 0 if the operation failed.
*/
int BotInterpolateCharacters( int handle1, int handle2, int desiredskill )
{
	bot_character_t *ch1, *ch2, *out;
	int				 i, handle;
	float			 scale;

	ch1 = BotCharacterFromHandle( handle1 );
	ch2 = BotCharacterFromHandle( handle2 );

	if( !ch1 || !ch2 ) {
		return 0;
	}

	// find a free spot for a character
	for( handle = 1; handle <= MAX_CLIENTS; handle++ ) {
		if( !botcharacters[handle] ) {
			break;
		}
	}

	if( handle > MAX_CLIENTS ) {
		return 0;
	}

	out		   = ( bot_character_t* )GetClearedMemory( sizeof( bot_character_t ) + MAX_CHARACTERISTICS * sizeof( bot_characteristic_t ) );
	out->skill = desiredskill;
	strcpy( out->filename, ch1->filename );
	botcharacters[handle] = out;

	scale = ( float )( desiredskill - 1 ) / ( ch2->skill - ch1->skill );

	for( i = 0; i < MAX_CHARACTERISTICS; i++ ) {
		//
		if( ch1->c[i].type == CT_FLOAT && ch2->c[i].type == CT_FLOAT ) {
			out->c[i].type		   = CT_FLOAT;
			out->c[i].value._float = ch1->c[i].value._float + ( ch2->c[i].value._float - ch1->c[i].value._float ) * scale;

		} else if( ch1->c[i].type == CT_INTEGER ) {
			out->c[i].type			= CT_INTEGER;
			out->c[i].value.integer = ch1->c[i].value.integer;

		} else if( ch1->c[i].type == CT_STRING ) {
			out->c[i].type		   = CT_STRING;
			out->c[i].value.string = ( char* )GetMemory( strlen( ch1->c[i].value.string ) + 1 );
			strcpy( out->c[i].value.string, ch1->c[i].value.string );
		}
	}

	return handle;
}

int BotLoadCharacter( char* charfile, int skill )
{
	int skill1, skill4, handle;

	// make sure the skill is in the valid range
	if( skill < 1 ) {
		skill = 1;

	} else if( skill > 5 ) {
		skill = 5;
	}

	// skill 1, 4 and 5 should be available in the character files
	if( skill == 1 || skill == 4 || skill == 5 ) {
		return BotLoadCharacterSkill( charfile, skill );
	}

	// check if there's a cached skill 2 or 3
	handle = BotFindCachedCharacter( charfile, skill );

	if( handle ) {
		botimport.Print( PRT_MESSAGE, "loaded cached skill %d from %s\n", skill, charfile );
		return handle;
	}

	// load skill 1 and 4
	skill1 = BotLoadCharacterSkill( charfile, 1 );

	if( !skill1 ) {
		return 0;
	}

	skill4 = BotLoadCharacterSkill( charfile, 4 );

	if( !skill4 ) {
		return skill1;
	}

	// interpolate between 1 and 4 to create skill 2 or 3
	handle = BotInterpolateCharacters( skill1, skill4, skill );

	if( !handle ) {
		return 0;
	}

	// write the character to the log file
	BotDumpCharacter( botcharacters[handle] );
	//
	return handle;
}

/*!
	\brief Validates a bot character handle and characteristic index, ensuring the index exists and is initialized.

	The function retrieves the bot character structure from a handle. It first checks whether the handle is valid; if not, it immediately reports failure. It then verifies the supplied index is within
   the bounds of the maximum characteristics limit. If the index is out of range or the characteristic at that index has not been initialized (its type is zero), it prints an error message to the bot
   import system. When all checks succeed, the function returns a true value indicating the index is valid for use.

	\param character Handle or identifier used to look up the bot character structure; should correspond to a valid character loaded in the system.
	\param index The numeric index into the character's characteristic array to be validated.
	\return An integer representing a boolean: qtrue (non‑zero) if the character and index are valid, otherwise qfalse (zero).
*/
int CheckCharacteristicIndex( int character, int index )
{
	bot_character_t* ch;

	ch = BotCharacterFromHandle( character );

	if( !ch ) {
		return qfalse;
	}

	if( index < 0 || index >= MAX_CHARACTERISTICS ) {
		botimport.Print( PRT_ERROR, "characteristic %d does not exist\n", index );
		return qfalse;
	}

	if( !ch->c[index].type ) {
		botimport.Print( PRT_ERROR, "characteristic %d is not initialized\n", index );
		return qfalse;
	}

	return qtrue;
}

float Characteristic_Float( int character, int index )
{
	bot_character_t* ch;

	ch = BotCharacterFromHandle( character );

	if( !ch ) {
		return 0;
	}

	// check if the index is in range
	if( !CheckCharacteristicIndex( character, index ) ) {
		return 0;
	}

	// an integer will be converted to a float
	if( ch->c[index].type == CT_INTEGER ) {
		return ( float )ch->c[index].value.integer;
	}

	// floats are just returned
	else if( ch->c[index].type == CT_FLOAT ) {
		return ch->c[index].value._float;
	}

	// cannot convert a string pointer to a float
	else {
		botimport.Print( PRT_ERROR, "characteristic %d is not a float\n", index );
		return 0;
	}

	//	return 0;
}

float Characteristic_BFloat( int character, int index, float min, float max )
{
	float			 value;
	bot_character_t* ch;

	ch = BotCharacterFromHandle( character );

	if( !ch ) {
		return 0;
	}

	if( min > max ) {
		botimport.Print( PRT_ERROR, "cannot bound characteristic %d between %f and %f\n", index, min, max );
		return 0;
	}

	value = Characteristic_Float( character, index );

	if( value < min ) {
		return min;
	}

	if( value > max ) {
		return max;
	}

	return value;
}

int Characteristic_Integer( int character, int index )
{
	bot_character_t* ch;

	ch = BotCharacterFromHandle( character );

	if( !ch ) {
		return 0;
	}

	// check if the index is in range
	if( !CheckCharacteristicIndex( character, index ) ) {
		return 0;
	}

	// an integer will just be returned
	if( ch->c[index].type == CT_INTEGER ) {
		return ch->c[index].value.integer;
	}

	// floats are casted to integers
	else if( ch->c[index].type == CT_FLOAT ) {
		return ( int )ch->c[index].value._float;

	} else {
		botimport.Print( PRT_ERROR, "characteristic %d is not a integer\n", index );
		return 0;
	}

	//	return 0;
}

int Characteristic_BInteger( int character, int index, int min, int max )
{
	int				 value;
	bot_character_t* ch;

	ch = BotCharacterFromHandle( character );

	if( !ch ) {
		return 0;
	}

	if( min > max ) {
		botimport.Print( PRT_ERROR, "cannot bound characteristic %d between %d and %d\n", index, min, max );
		return 0;
	}

	value = Characteristic_Integer( character, index );

	if( value < min ) {
		return min;
	}

	if( value > max ) {
		return max;
	}

	return value;
}

void Characteristic_String( int character, int index, char* buf, int size )
{
	bot_character_t* ch;

	ch = BotCharacterFromHandle( character );

	if( !ch ) {
		return;
	}

	// check if the index is in range
	if( !CheckCharacteristicIndex( character, index ) ) {
		return;
	}

	// an integer will be converted to a float
	if( ch->c[index].type == CT_STRING ) {
		strncpy( buf, ch->c[index].value.string, size - 1 );
		buf[size - 1] = '\0';
		return;

	} else {
		botimport.Print( PRT_ERROR, "characteristic %d is not a string\n", index );
		return;
	}

	return;
}

void BotShutdownCharacters()
{
	int handle;

	for( handle = 1; handle <= MAX_CLIENTS; handle++ ) {
		if( botcharacters[handle] ) {
			BotFreeCharacter2( handle );
		}
	}
}
