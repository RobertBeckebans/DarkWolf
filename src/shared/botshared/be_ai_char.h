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
 * name:		be_ai_char.h
 *
 * desc:		bot characters
 *
 *
 *****************************************************************************/

/*!
	\brief Loads a bot character from a file with the specified skill level, interpolating between skill levels when necessary.

	This function loads a bot character from the specified file and skill level. It ensures the skill is within the valid range of 1 to 5. For skill levels 1, 4, and 5, it directly loads the character
   skill. For skill levels 2 and 3, it first checks for a cached character, and if not found, interpolates between skill 1 and 4 to create the character for the desired skill level. The function
   returns a handle to the loaded character or zero if the loading fails.

	\param charfile Path to the character file to load
	\param skill Skill level for the character, must be between 1 and 5
	\return Handle to the loaded bot character, or zero if the loading fails
*/
int	  BotLoadCharacter( char* charfile, int skill );

/*!
	\brief Frees a bot character resource referenced by the given handle

	This function releases the memory and resources associated with a bot character that was previously loaded using BotLoadCharacter. It only performs the actual freeing operation if the
   bot_reloadcharacters library variable is set to true. The function delegates the actual cleanup work to BotFreeCharacter2.

	\param character Handle to the bot character to be freed
*/
void  BotFreeCharacter( int character );

/*!
	\brief Retrieves a float value from a character's characteristic at the specified index.

	This function retrieves a float value from a character's characteristic array. It first validates the character handle and checks if the characteristic index is valid. If the characteristic is
   stored as an integer, it will be converted to a float before returning. If the characteristic is already a float, it is returned directly. If the characteristic is of a different type, an error is
   printed and zero is returned. The function is used to access numeric characteristics of bot characters in the game.

	\param character Handle to the character whose characteristic is being retrieved
	\param index Index of the characteristic to retrieve
	\return The float value of the characteristic at the specified index, or 0 if the character handle is invalid or the index is out of range.
*/
float Characteristic_Float( int character, int index );

/*!
	\brief Returns a bounded float characteristic value for a bot character within specified minimum and maximum bounds.

	This function retrieves a float characteristic value for a specified bot character and ensures it falls within the provided minimum and maximum bounds. If the character handle is invalid or the
   minimum value exceeds the maximum value, the function returns 0. Otherwise, it clamps the characteristic value between the specified bounds before returning it.

	\param character Handle to the bot character
	\param index Index of the characteristic to retrieve
	\param min Minimum allowed value for the characteristic
	\param max Maximum allowed value for the characteristic
	\return The bounded characteristic value, clamped between min and max, or 0 if the character handle is invalid or min > max.
*/
float Characteristic_BFloat( int character, int index, float min, float max );

/*!
	\brief Retrieves an integer characteristic value from a bot character by index

	This function retrieves an integer characteristic value from a bot character identified by the character handle. It first validates the character handle and checks if the characteristic index is
   within valid range. If the characteristic exists and is of integer type, it returns the integer value directly. If the characteristic is of float type, it casts the float value to an integer. If
   the characteristic is of another type or invalid, it logs an error and returns zero. The function is part of the bot character management system in the Return to Castle Wolfenstein game engine.

	\param character Handle identifier for the bot character
	\param index Index of the characteristic to retrieve
	\return Integer characteristic value if valid, or zero if the character handle is invalid, index is out of range, or the characteristic is not an integer type
*/
int	  Characteristic_Integer( int character, int index );

/*!
	\brief Returns a bounded integer characteristic value for a bot character within specified bounds

	This function retrieves an integer characteristic value for a specified bot character and ensures it falls within the provided minimum and maximum bounds. If the character handle is invalid or the
   minimum bound exceeds the maximum bound, the function returns 0. The function first obtains the character data structure using the provided character handle, then fetches the unbounded integer
   characteristic value and clamps it between the specified minimum and maximum values.

	\param character Handle to the bot character
	\param index Index of the characteristic to retrieve
	\param min Minimum allowed value for the characteristic
	\param max Maximum allowed value for the characteristic
	\return The bounded integer characteristic value, clamped between min and max, or 0 if invalid parameters are provided
*/
int	  Characteristic_BInteger( int character, int index, int min, int max );

/*!
	\brief Retrieves a string characteristic value for a bot character and stores it in the provided buffer.

	This function retrieves a string characteristic value associated with a specific index from a bot character's characteristic table. It first validates the character handle and characteristic index
   before performing the retrieval. The function ensures the retrieved string is properly null-terminated and stored in the provided buffer with the specified size limit. If the characteristic index
   is invalid or the characteristic is not of string type, an error message is printed and the function returns without modifying the buffer.

	\param character Handle to the bot character
	\param index Index of the characteristic to retrieve
	\param buf Output buffer to store the retrieved string
	\param size Size of the output buffer
*/
void  Characteristic_String( int character, int index, char* buf, int size );

/*!
	\brief Frees all cached bot characters

	This function iterates through all possible client handles from 1 to MAX_CLIENTS and frees any bot character that is currently allocated. It is used to clean up bot character data when shutting
   down the AI subsystems.

*/
void  BotShutdownCharacters();
