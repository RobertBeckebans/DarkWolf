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

// Copyright (C) 1999-2000 Id Software, Inc.
//
// q_shared.c -- stateless support routines that are included in each code dll
#include "q_shared.h"

/*!
	\brief Clamps a float value between a minimum and maximum bound.

	This function ensures that the provided value stays within the specified range. If the value is less than the minimum, it returns the minimum. If the value is greater than the maximum, it returns
   the maximum. Otherwise, it returns the original value. This is commonly used to constrain values such as brightness, handicap settings, or UI element properties.

	\param min The lower bound of the valid range.
	\param max The upper bound of the valid range.
	\param value The value to be clamped.
	\return The input value clamped to the range [min, max].
*/
float Com_Clamp( float min, float max, float value )
{
	if( value < min ) {
		return min;
	}

	if( value > max ) {
		return max;
	}

	return value;
}

char* COM_SkipPath( char* pathname )
{
	char* last;

	last = pathname;

	while( *pathname ) {
		if( *pathname == '/' ) {
			last = pathname + 1;
		}

		pathname++;
	}

	return last;
}

void COM_StripExtension( const char* in, char* out )
{
	while( *in && *in != '.' ) {
		*out++ = *in++;
	}

	*out = 0;
}

void COM_StripFilename( char* in, char* out )
{
	char* end;
	Q_strncpyz( out, in, strlen( in ) );
	end	 = COM_SkipPath( out );
	*end = 0;
}

void COM_DefaultExtension( char* path, int maxSize, const char* extension )
{
	char  oldPath[MAX_QPATH];
	char* src;

	//
	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	//
	src = path + strlen( path ) - 1;

	while( *src != '/' && src != path ) {
		if( *src == '.' ) {
			return; // it has an extension
		}

		src--;
	}

	Q_strncpyz( oldPath, path, sizeof( oldPath ) );
	Com_sprintf( path, maxSize, "%s%s", oldPath, extension );
}

qboolean COM_BitCheck( const int array[], int bitNum )
{
	int i;

	i = 0;

	while( bitNum > 31 ) {
		i++;
		bitNum -= 32;
	}

	return ( qboolean )( ( array[i] & ( 1 << bitNum ) ) != 0 ); // (SA) heh, whoops. :)
}

void COM_BitSet( int array[], int bitNum )
{
	int i;

	i = 0;

	while( bitNum > 31 ) {
		i++;
		bitNum -= 32;
	}

	array[i] |= ( 1 << bitNum );
}

void COM_BitClear( int array[], int bitNum )
{
	int i;

	i = 0;

	while( bitNum > 31 ) {
		i++;
		bitNum -= 32;
	}

	array[i] &= ~( 1 << bitNum );
}

//============================================================================

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

// can't just use function pointers, or dll linkage can
// mess up when qcommon is included in multiple places
static short ( *_BigShort )( short l );
static short ( *_LittleShort )( short l );
static int ( *_BigLong )( int l );
static int ( *_LittleLong )( int l );
static qint64 ( *_BigLong64 )( qint64 l );
static qint64 ( *_LittleLong64 )( qint64 l );
static float ( *_BigFloat )( float l );
static float ( *_LittleFloat )( float l );

/*!
	\brief Converts a short integer from host byte order to big endian byte order

	This function is used to ensure proper byte ordering when sending data over a network. It takes a short integer value and returns the same value with its bytes swapped if the host system uses
   little-endian byte order. This is necessary for network communication where data is typically transmitted in big-endian format. The function internally calls _BigShort to perform the actual byte
   swapping operation.

	\param l The short integer value to be converted to big endian byte order
	\return The short integer value with bytes swapped to big endian byte order
*/
short BigShort( short l )
{
	return _BigShort( l );
}

/*!
	\brief Converts a 16-bit integer value from big-endian to little-endian byte order.

	This function is used to ensure proper byte ordering when serializing or deserializing data structures that are written to or read from files. It takes a 16-bit integer input and returns the same
   value with its bytes swapped, which is necessary when the data needs to be compatible with systems using different byte orders. The function is typically used in conjunction with other similar
   functions like LittleLong for 32-bit values or LittleFloat for floating-point values.

	\param l The 16-bit integer value to be converted to little-endian byte order
	\return The input value with its byte order reversed to little-endian format
*/
short LittleShort( short l )
{
	return _LittleShort( l );
}

/*!
	\brief Converts a 32-bit integer from little-endian to big-endian byte order or vice versa

	This function performs byte order conversion for a 32-bit integer. It is typically used when transferring data between systems with different byte order conventions. The function internally calls
   _BigLong to perform the actual conversion operation. The conversion is symmetric, meaning calling the function twice with the same input will return the original value.

	\param l The 32-bit integer value to convert between byte orders
	\return The integer value with its byte order reversed
*/
int BigLong( int l )
{
	return _BigLong( l );
}

/*!
	\brief Converts a 32-bit integer from big-endian to little-endian byte order or vice versa

	This function is used to ensure proper byte order when reading or writing binary data across different architectures. It relies on the _LittleLong macro which performs the actual byte swapping
   operation. The function is typically used in conjunction with other similar functions like LittleShort and LittleFloat for consistent endianness handling. In the context of the Return to Castle
   Wolfenstein code base, this function is essential for loading AAS (Area Awareness System) data on systems with different endianness than the source system.

	\param l The 32-bit integer value to be converted between byte orders
	\return The integer value with its byte order reversed
*/
int LittleLong( int l )
{
	return _LittleLong( l );
}

qint64 BigLong64( qint64 l )
{
	return _BigLong64( l );
}

qint64 LittleLong64( qint64 l )
{
	return _LittleLong64( l );
}

/*!
	\brief Returns the big-endian float representation of the input float value

	This function converts a float value to its big-endian representation. It serves as a wrapper around the internal _BigFloat function which performs the actual conversion. The function is typically
   used when byte order needs to be explicitly controlled, such as when serializing data for network transmission or file I/O operations where consistent byte ordering is required.

	\param l input float value to convert to big-endian representation
	\return float value in big-endian byte order
*/
float BigFloat( float l )
{
	return _BigFloat( l );
}

/*!
	\brief Converts a float value from little-endian byte order to the native byte order.

	This function is used to ensure proper byte ordering when reading or writing binary data that may have been stored in little-endian format. It leverages an internal helper function _LittleFloat to
   perform the actual conversion. The function is commonly used in file I/O operations where data needs to be byte-swapped for cross-platform compatibility. Based on the usage examples, this function
   is particularly important in AAS (Area Awareness System) file processing, where various geometric data structures like bounding boxes, vertex coordinates, and plane equations need to be properly
   byte-ordered when loaded from files.

	\param l The float value in little-endian byte order to be converted
	\return The float value converted to the native byte order of the system
*/
float LittleFloat( float l )
{
	return _LittleFloat( l );
}

short ShortSwap( short l )
{
	byte b1, b2;

	b1 = l & 255;
	b2 = ( l >> 8 ) & 255;

	return ( b1 << 8 ) + b2;
}

short ShortNoSwap( short l )
{
	return l;
}

int LongSwap( int l )
{
	byte b1, b2, b3, b4;

	b1 = l & 255;
	b2 = ( l >> 8 ) & 255;
	b3 = ( l >> 16 ) & 255;
	b4 = ( l >> 24 ) & 255;

	return ( ( int )b1 << 24 ) + ( ( int )b2 << 16 ) + ( ( int )b3 << 8 ) + b4;
}

int LongNoSwap( int l )
{
	return l;
}

/*!
	\brief Swaps the byte order of a 64-bit integer.

	This function performs a byte swap on a 64-bit integer by reversing the order of its bytes. It takes the input integer and returns a new integer with the bytes in reverse order.

	\param ll The 64-bit integer whose byte order needs to be swapped
	\return The 64-bit integer with its byte order reversed
*/
qint64 Long64Swap( qint64 ll )
{
	qint64 result;

	result.b0 = ll.b7;
	result.b1 = ll.b6;
	result.b2 = ll.b5;
	result.b3 = ll.b4;
	result.b4 = ll.b3;
	result.b5 = ll.b2;
	result.b6 = ll.b1;
	result.b7 = ll.b0;

	return result;
}

/*!
	\brief Returns the input 64-bit integer without swapping its byte order.

	This function serves as a no-operation implementation for handling 64-bit integers, particularly in contexts where byte order swapping might normally occur. It simply returns the input value
   unchanged, which is useful when the system's endianness matches the expected byte order or when explicit swapping is not required.

	\param ll The 64-bit integer value to be returned unchanged
	\return The input 64-bit integer value without any modification to its byte order
*/
qint64 Long64NoSwap( qint64 ll )
{
	return ll;
}

float FloatSwap( float f )
{
	union {
		float f;
		byte  b[4];
	} dat1, dat2;

	dat1.f	  = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

float FloatNoSwap( float f )
{
	return f;
}

/*
================
Swap_Init
================
*/
void Swap_Init()
{
	byte swaptest[2] = { 1, 0 };

	// set the byte swapping variables in a portable manner
	if( *( short* )swaptest == 1 ) {
		_BigShort	  = ShortSwap;
		_LittleShort  = ShortNoSwap;
		_BigLong	  = LongSwap;
		_LittleLong	  = LongNoSwap;
		_BigLong64	  = Long64Swap;
		_LittleLong64 = Long64NoSwap;
		_BigFloat	  = FloatSwap;
		_LittleFloat  = FloatNoSwap;

	} else {
		_BigShort	  = ShortNoSwap;
		_LittleShort  = ShortSwap;
		_BigLong	  = LongNoSwap;
		_LittleLong	  = LongSwap;
		_BigLong64	  = Long64NoSwap;
		_LittleLong64 = Long64Swap;
		_BigFloat	  = FloatNoSwap;
		_LittleFloat  = FloatSwap;
	}
}

/*
============================================================================

PARSING

============================================================================
*/

static char	 com_token[MAX_TOKEN_CHARS];
static char	 com_parsename[MAX_TOKEN_CHARS];
static int	 com_lines;

static int	 backup_lines;
static char* backup_text;

void		 COM_BeginParseSession( const char* name )
{
	com_lines = 0;
	Com_sprintf( com_parsename, sizeof( com_parsename ), "%s", name );
}

void COM_RestoreParseSession( char** data_p )
{
	com_lines = backup_lines;
	*data_p	  = backup_text;
}

void COM_SetCurrentParseLine( int line )
{
	com_lines = line;
}

int COM_GetCurrentParseLine()
{
	return com_lines;
}

char* COM_Parse( char** data_p )
{
	return COM_ParseExt( data_p, qtrue );
}

void COM_ParseError( char* format, ... )
{
	va_list		argptr;
	static char string[4096];

	va_start( argptr, format );
	vsprintf( string, format, argptr );
	va_end( argptr );

	Com_Printf( "ERROR: %s, line %d: %s\n", com_parsename, com_lines, string );
}

void COM_ParseWarning( char* format, ... )
{
	va_list		argptr;
	static char string[4096];

	va_start( argptr, format );
	vsprintf( string, format, argptr );
	va_end( argptr );

	Com_Printf( "WARNING: %s, line %d: %s\n", com_parsename, com_lines, string );
}

/*!
	\brief Skips whitespace characters in a string and tracks if newline characters are encountered.

	This function advances through the input string, skipping all characters that are whitespace or control characters. It increments a global line counter when a newline character is encountered and
   sets the hasNewLines flag to true. If a null terminator is encountered, the function returns NULL to indicate end of input. The function is typically used as a helper in parsing routines to skip
   over irrelevant whitespace before extracting tokens or other meaningful data.

	\param data Pointer to the input string to process
	\param hasNewLines Pointer to a flag that will be set to true if a newline character is encountered while skipping whitespace
	\return Pointer to the first non-whitespace character in the input string, or NULL if end of input is reached
*/
char* SkipWhitespace( char* data, qboolean* hasNewLines )
{
	int c;

	while( ( c = *data ) <= ' ' ) {
		if( c == '\n' ) {
			com_lines++;
			*hasNewLines = qtrue;

		} else if( !c ) {
			return NULL;
		}

		data++;
	}

	return data;
}

int COM_Compress( char* data_p )
{
	char *	 datai, *datao;
	int		 c, pc, size;
	qboolean ws = qfalse;

	size  = 0;
	pc	  = 0;
	datai = datao = data_p;

	if( datai ) {
		while( ( c = *datai ) != 0 ) {
			if( c == 13 || c == 10 ) {
				*datao = c;
				datao++;
				ws = qfalse;
				pc = c;
				datai++;
				size++;
				// skip double slash comments

			} else if( c == '/' && datai[1] == '/' ) {
				while( *datai && *datai != '\n' ) {
					datai++;
				}

				ws = qfalse;
				// skip /* */ comments

			} else if( c == '/' && datai[1] == '*' ) {
				while( *datai && ( *datai != '*' || datai[1] != '/' ) ) {
					datai++;
				}

				if( *datai ) {
					datai += 2;
				}

				ws = qfalse;

			} else {
				if( ws ) {
					*datao = ' ';
					datao++;
				}

				*datao = c;
				datao++;
				datai++;
				ws = qfalse;
				pc = c;
				size++;
			}
		}
	}

	*datao = 0;
	return size;
}

/*!
	\brief Parses the next token from a string, handling comments and quoted strings, with optional line break support.

	This function extracts the next token from the provided string, skipping whitespace and comments. It supports both single-line and multi-line comments, as well as quoted strings. The function
   modifies the input string pointer to point to the next unparsed portion. Line break handling is controlled by the allowLineBreaks parameter.

	\param data_p Pointer to the input string to parse, updated to point to the next unparsed position
	\param allowLineBreak Flag indicating whether line breaks are allowed within tokens
	\return Pointer to the parsed token stored in a static buffer, or empty string if no token is found
*/
char* COM_ParseExt( char** data_p, qboolean allowLineBreaks )
{
	int		 c			 = 0, len;
	qboolean hasNewLines = qfalse;
	char*	 data;

	data		 = *data_p;
	len			 = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if( !data ) {
		*data_p = NULL;
		return com_token;
	}

	// RF, backup the session data so we can unget easily
	backup_lines = com_lines;
	backup_text	 = *data_p;

	while( 1 ) {
		// skip whitespace
		data = SkipWhitespace( data, &hasNewLines );

		if( !data ) {
			*data_p = NULL;
			return com_token;
		}

		if( hasNewLines && !allowLineBreaks ) {
			*data_p = data;
			return com_token;
		}

		c = *data;

		// skip double slash comments
		if( c == '/' && data[1] == '/' ) {
			data += 2;

			while( *data && *data != '\n' ) {
				data++;
			}
		}

		// skip /* */ comments
		else if( c == '/' && data[1] == '*' ) {
			data += 2;

			while( *data && ( *data != '*' || data[1] != '/' ) ) {
				data++;
			}

			if( *data ) {
				data += 2;
			}

		} else {
			break;
		}
	}

	// handle quoted strings
	if( c == '\"' ) {
		data++;

		while( 1 ) {
			c = *data++;

			if( c == '\"' || !c ) {
				com_token[len] = 0;
				*data_p		   = ( char* )data;
				return com_token;
			}

			if( len < MAX_TOKEN_CHARS ) {
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do {
		if( len < MAX_TOKEN_CHARS ) {
			com_token[len] = c;
			len++;
		}

		data++;
		c = *data;

		if( c == '\n' ) {
			com_lines++;
		}
	} while( c > ' ' );

	if( len == MAX_TOKEN_CHARS ) {
		//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}

	com_token[len] = '\0';

	*data_p = ( char* )data;
	return com_token;
}

/*
==================
COM_MatchToken
==================
*/
void COM_MatchToken( char** buf_p, char* match )
{
	char* token;

	token = COM_Parse( buf_p );

	if( strcmp( token, match ) ) {
		Com_Error( ERR_DROP, "MatchToken: %s != %s", token, match );
	}
}

void SkipBracedSection( char** program )
{
	char* token;
	int	  depth;

	depth = 0;

	do {
		token = COM_ParseExt( program, qtrue );

		if( token[1] == 0 ) {
			if( token[0] == '{' ) {
				depth++;

			} else if( token[0] == '}' ) {
				depth--;
			}
		}
	} while( depth && *program );
}

void SkipRestOfLine( char** data )
{
	char* p;
	int	  c;

	p = *data;

	while( ( c = *p++ ) != 0 ) {
		if( c == '\n' ) {
			com_lines++;
			break;
		}
	}

	*data = p;
}

void Parse1DMatrix( char** buf_p, int x, float* m )
{
	char* token;
	int	  i;

	COM_MatchToken( buf_p, "(" );

	for( i = 0; i < x; i++ ) {
		token = COM_Parse( buf_p );
		m[i]  = atof( token );
	}

	COM_MatchToken( buf_p, ")" );
}

void Parse2DMatrix( char** buf_p, int y, int x, float* m )
{
	int i;

	COM_MatchToken( buf_p, "(" );

	for( i = 0; i < y; i++ ) {
		Parse1DMatrix( buf_p, x, m + i * x );
	}

	COM_MatchToken( buf_p, ")" );
}

void Parse3DMatrix( char** buf_p, int z, int y, int x, float* m )
{
	int i;

	COM_MatchToken( buf_p, "(" );

	for( i = 0; i < z; i++ ) {
		Parse2DMatrix( buf_p, y, x, m + i * x * y );
	}

	COM_MatchToken( buf_p, ")" );
}

/*!
	\brief Checks if a character is a printable ASCII character.

	This function determines whether the provided character code falls within the range of printable ASCII characters, which spans from space (0x20) to tilde (0x7E). It returns a non-zero value if the
   character is printable, and zero otherwise. The function is useful for filtering or validating input in text processing scenarios.

	\param c The character code to check for printability
	\return Non-zero if the character is a printable ASCII character, zero otherwise.
*/
int Q_isprint( int c )
{
	if( c >= 0x20 && c <= 0x7E ) {
		return ( 1 );
	}

	return ( 0 );
}

/*!
	\brief Checks if a character is a lowercase letter.

	This function determines whether the provided character is a lowercase letter in the range 'a' to 'z'. It returns a non-zero value if the character is a lowercase letter, and zero otherwise. The
   function is commonly used for case-insensitive string comparisons and conversions.

	\param c The character to check
	\return Non-zero value if the character is a lowercase letter, zero otherwise
*/
int Q_islower( int c )
{
	if( c >= 'a' && c <= 'z' ) {
		return ( 1 );
	}

	return ( 0 );
}

/*!
	\brief Checks if a character is an uppercase letter.

	This function determines whether the provided character is an uppercase letter by comparing its ASCII value against the range of uppercase letters 'A' through 'Z'. It returns a non-zero value if
   the character is uppercase, and zero otherwise. The function is commonly used in text processing and string manipulation tasks where case sensitivity matters.

	\param c The character to check, passed as an integer representing its ASCII value
	\return Non-zero value if the character is an uppercase letter, zero otherwise
*/
int Q_isupper( int c )
{
	if( c >= 'A' && c <= 'Z' ) {
		return ( 1 );
	}

	return ( 0 );
}

/*!
	\brief Checks if a character is alphabetic

	This function determines whether the provided character is an alphabetic character, meaning it falls within the range of lowercase letters 'a' to 'z' or uppercase letters 'A' to 'Z'. It returns a
   non-zero value if the character is alphabetic, and zero otherwise. The function is commonly used for character validation and string processing tasks where it's necessary to distinguish between
   alphabetic and non-alphabetic characters.

	\param c The character to be checked for being alphabetic
	\return Non-zero value if the character is alphabetic, zero otherwise
*/
int Q_isalpha( int c )
{
	if( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ) {
		return ( 1 );
	}

	return ( 0 );
}

int Q_isnumeric( int c )
{
	if( c >= '0' && c <= '9' ) {
		return ( 1 );
	}

	return ( 0 );
}

int Q_isalphanumeric( int c )
{
	if( Q_isalpha( c ) || Q_isnumeric( c ) ) {
		return ( 1 );
	}

	return ( 0 );
}

int Q_isforfilename( int c )
{
	if( ( Q_isalphanumeric( c ) || c == '_' ) && c != ' ' ) { // space not allowed in filename
		return ( 1 );
	}

	return ( 0 );
}

/*!
	\brief Returns a pointer to the last occurrence of a character in a string, or NULL if not found

	This function searches for the last occurrence of a specified character within a null-terminated string. It traverses the entire string from beginning to end, keeping track of the last position
   where the character was found. If the character is not found, it returns NULL. If the character is a null terminator, it returns a pointer to the end of the string. This implementation handles the
   special case where the character to search for is the null terminator, ensuring proper behavior when searching for string terminators.

	\param string The null-terminated string to search within
	\param c The character to search for, passed as an int
	\return Pointer to the last occurrence of the character in the string, or NULL if not found
*/
char* Q_strrchr( const char* string, int c )
{
	char  cc = c;
	char* s;
	char* sp = ( char* )0;

	s = ( char* )string;

	while( *s ) {
		if( *s == cc ) {
			sp = s;
		}

		s++;
	}

	if( cc == 0 ) {
		sp = s;
	}

	return sp;
}

/*!
	\brief Copies a source string to a destination buffer with guaranteed null termination

	This function performs a safe string copy operation that ensures the destination buffer is always null terminated. It includes validation checks to ensure the source pointer is not null and the
   destination buffer size is valid. The function uses strncpy to copy the source string up to destsize - 1 characters, then explicitly sets the final character to null to guarantee termination. This
   prevents buffer overflows and ensures string safety in all cases.

	\param dest Destination buffer to copy string into
	\param src Source string to copy from
	\param destsize Size of the destination buffer in bytes
	\throws Fatal error if source pointer is null or destination buffer size is less than 1
*/
void Q_strncpyz( char* dest, const char* src, int destsize )
{
	if( !src ) {
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL src" );
	}

	if( destsize < 1 ) {
		Com_Error( ERR_FATAL, "Q_strncpyz: destsize < 1" );
	}

	strncpy( dest, src, destsize - 1 );
	dest[destsize - 1] = 0;
}

/*!
	\brief Performs a case-insensitive comparison of the first n characters of two strings and returns an integer indicating their lexicographical relationship.

	This function compares two null-terminated strings character by character, ignoring case differences, for up to n characters. It returns zero if the strings are equal up to the specified length, a
   negative value if the first string is lexicographically less than the second, and a positive value if the first string is greater. The comparison stops either when the end of either string is
   reached or when n characters have been compared, whichever comes first. The function handles mixed case letters by converting lowercase letters to uppercase for comparison purposes.

	\param s1 First null-terminated string to compare
	\param s2 Second null-terminated string to compare
	\param n Maximum number of characters to compare
	\return Zero if the strings are equal up to n characters, negative if s1 is less than s2, positive if s1 is greater than s2
*/
int Q_stricmpn( const char* s1, const char* s2, int n )
{
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if( !n-- ) {
			return 0; // strings are equal until end point
		}

		if( c1 != c2 ) {
			if( Q_islower( c1 ) ) {
				c1 -= ( 'a' - 'A' );
			}

			if( Q_islower( c2 ) ) {
				c2 -= ( 'a' - 'A' );
			}

			if( c1 != c2 ) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while( c1 );

	return 0; // strings are equal
}

/*!
	\brief Compares up to n characters of two strings lexicographically and returns an integer indicating their relationship.

	This function performs a case-sensitive comparison of two null-terminated strings, examining at most n characters. It returns zero if the strings are equal up to n characters, a negative value if
   the first string is lexicographically less than the second, and a positive value if the first string is greater. The comparison stops at the first difference or when n characters have been
   examined, whichever comes first. If the strings are equal up to n characters, it returns zero.

	\param s1 Pointer to the first string to compare
	\param s2 Pointer to the second string to compare
	\param n Maximum number of characters to compare
	\return Zero if strings are equal up to n characters, negative if s1 < s2, positive if s1 > s2
*/
int Q_strncmp( const char* s1, const char* s2, int n )
{
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if( !n-- ) {
			return 0; // strings are equal until end point
		}

		if( c1 != c2 ) {
			return c1 < c2 ? -1 : 1;
		}
	} while( c1 );

	return 0; // strings are equal
}

/*!
	\brief Performs a case-insensitive comparison of two null-terminated strings.

	This function compares two strings in a case-insensitive manner. It returns zero if the strings are equal, a negative value if the first string is lexicographically less than the second, and a
   positive value if the first string is greater. If either string pointer is null, the function returns -1. The comparison is limited to 99999 characters or until a null terminator is encountered,
   whichever comes first.

	\param s1 First null-terminated string to compare
	\param s2 Second null-terminated string to compare
	\return Zero if strings are equal, negative if s1 < s2, positive if s1 > s2, or -1 if either string is null
*/
int Q_stricmp( const char* s1, const char* s2 )
{
	return ( s1 && s2 ) ? Q_stricmpn( s1, s2, 99999 ) : -1;
}

/*!
	\brief Converts all characters in the input string to lowercase.

	The function takes a null-terminated string and converts each character to its lowercase equivalent using the tolower function. The conversion is done in place, modifying the original string. The
   function returns a pointer to the modified string.

	\param s1 Pointer to the null-terminated string to be converted to lowercase
	\return Pointer to the modified input string with all characters converted to lowercase
*/
char* Q_strlwr( char* s1 )
{
	char* s;

	s = s1;

	while( *s ) {
		*s = tolower( *s );
		s++;
	}

	return s1;
}

char* Q_strupr( char* s1 )
{
	char* s;

	s = s1;

	while( *s ) {
		*s = toupper( *s );
		s++;
	}

	return s1;
}

// never goes past bounds or leaves without a terminating 0
void Q_strcat( char* dest, int size, const char* src )
{
	int l1;

	l1 = strlen( dest );

	if( l1 >= size ) {
		Com_Error( ERR_FATAL, "Q_strcat: already overflowed" );
	}

	Q_strncpyz( dest + l1, src, size - l1 );
}

/*!
	\brief Calculates the length of a string while ignoring color string sequences.

	The function determines the display length of a string by iterating through each character and skipping over color string sequences. A color string is identified by the Q_IsColorString function
   and consists of two characters. The function returns the count of visible characters in the string, excluding color codes.

	\param string The input string to calculate the length of, can be NULL
	\return The length of the string excluding color string sequences, or 0 if the input string is NULL
*/
int Q_PrintStrlen( const char* string )
{
	int			len;
	const char* p;

	if( !string ) {
		return 0;
	}

	len = 0;
	p	= string;

	while( *p ) {
		if( Q_IsColorString( p ) ) {
			p += 2;
			continue;
		}

		p++;
		len++;
	}

	return len;
}

/*!
	\brief Removes non-printable characters from a string and color codes, returning a cleaned version of the input string.

	This function processes the input string by iterating through each character. It skips over color codes, which are identified using the Q_IsColorString function, and only copies characters that
   fall within the printable ASCII range of 0x20 to 0x7E. The cleaned string is then null-terminated and returned.

	\param string Input string to be cleaned of non-printable characters and color codes.
	\return A pointer to the cleaned input string with color codes and non-printable characters removed.
*/
char* Q_CleanStr( char* string )
{
	char* d;
	char* s;
	int	  c;

	s = string;
	d = string;

	while( ( c = *s ) != 0 ) {
		if( Q_IsColorString( s ) ) {
			s++;

		} else if( c >= 0x20 && c <= 0x7E ) {
			*d++ = c;
		}

		s++;
	}

	*d = '\0';

	return string;
}

void QDECL Com_sprintf( char* dest, int size, const char* fmt, ... )
{
	int		len;
	va_list argptr;
	char	bigbuffer[32000]; // big, but small enough to fit in PPC stack

	va_start( argptr, fmt );
	len = vsprintf( bigbuffer, fmt, argptr );
	va_end( argptr );

	if( len >= sizeof( bigbuffer ) ) {
		Com_Error( ERR_FATAL, "Com_sprintf: overflowed bigbuffer" );
	}

	if( len >= size ) {
		Com_Printf( "Com_sprintf: overflow of %i in %i\n", len, size );
	}

	Q_strncpyz( dest, bigbuffer, size );
}

/*!
	\brief Performs a case-insensitive comparison of two strings up to a specified number of characters.

	This function compares two strings in a case-insensitive manner, meaning that uppercase and lowercase letters are treated as equivalent. It stops comparing when either the end of one of the
   strings is reached or when the specified number of characters have been compared. If the strings are equal up to the specified length, it returns 0. If the strings differ, it returns -1. The
   function handles the conversion of lowercase letters to uppercase for comparison purposes.

	\param s1 First string to compare
	\param s2 Second string to compare
	\param n Maximum number of characters to compare
	\return 0 if the strings are equal up to the specified length, -1 if they differ
*/
int Q_strncasecmp( char* s1, char* s2, int n )
{
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if( !n-- ) {
			return 0; // strings are equal until end point
		}

		if( c1 != c2 ) {
			if( Q_islower( c1 ) ) {
				c1 -= ( 'a' - 'A' );
			}

			if( Q_islower( c2 ) ) {
				c2 -= ( 'a' - 'A' );
			}

			if( c1 != c2 ) {
				return -1; // strings not equal
			}
		}
	} while( c1 );

	return 0; // strings are equal
}

/*!
	\brief Performs a case-insensitive comparison of two null-terminated strings.

	This function compares two strings in a case-insensitive manner, returning zero if the strings are equal, a negative value if the first string is less than the second, and a positive value if the
   first string is greater than the second. It delegates the actual comparison to Q_strncasecmp with a large limit to ensure full string comparison.

	\param s1 First null-terminated string to compare
	\param s2 Second null-terminated string to compare
	\return Zero if the strings are equal, negative if s1 is less than s2, positive if s1 is greater than s2
*/
int Q_strcasecmp( char* s1, char* s2 )
{
	return Q_strncasecmp( s1, s2, 99999 );
}

// done.

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday

Ridah, modified this into a circular list, to further prevent stepping on
previous strings
============
*/
char* QDECL va( char* format, ... )
{
	va_list argptr;
#define MAX_VA_STRING 32000
	static char temp_buffer[MAX_VA_STRING];
	static char string[MAX_VA_STRING]; // in case va is called by nested functions
	static int	index = 0;
	char*		buf;
	int			len;

	va_start( argptr, format );
	vsprintf( temp_buffer, format, argptr );
	va_end( argptr );

	if( ( len = strlen( temp_buffer ) ) >= MAX_VA_STRING ) {
		Com_Error( ERR_DROP, "Attempted to overrun string in call to va()\n" );
	}

	if( len + index >= MAX_VA_STRING - 1 ) {
		index = 0;
	}

	buf = &string[index];
	memcpy( buf, temp_buffer, len + 1 );

	index += len + 1;

	return buf;
}

/*!
	\brief Returns a temporary vector with the specified coordinates, reusing previously allocated vectors to avoid memory allocation overhead.

	This function provides a convenience mechanism for creating temporary vectors used in function calls. It maintains an internal array of pre-allocated vectors and cycles through them to avoid
   repeated memory allocations. The function is designed to be safe for use in performance-critical code paths where frequent vector creation would otherwise cause overhead. The returned vector is
   valid until the next call to this function, as it reuses previously allocated memory.

	\param x The x component of the vector
	\param y The y component of the vector
	\param z The z component of the vector
	\return A pointer to a static vector containing the specified coordinates
*/
float* tv( float x, float y, float z )
{
	static int	  index;
	static vec3_t vecs[8];
	float*		  v;

	// use an array so that multiple tempvectors won't collide
	// for a while
	v	  = vecs[index];
	index = ( index + 1 ) & 7;

	v[0] = x;
	v[1] = y;
	v[2] = z;

	return v;
}

/*!
	\brief Extracts the value associated with a given key from an info string.

	This function parses an info string to find the value corresponding to the specified key. The info string is expected to be formatted as a series of key-value pairs separated by backslashes. The
   function uses a static buffer to store the result, which means it is not thread-safe and concurrent calls may overwrite each other's results. It returns an empty string if either the input string
   or key is null, or if the key is not found. The function also checks for oversize infostrings and triggers an error if the input exceeds the defined limit.

	\param s The info string to search through
	\param key The key to look for in the info string
	\return A pointer to the value associated with the key in the info string, or an empty string if the key is not found or if input is invalid.
	\throws ERROR_DROP if the info string exceeds the maximum allowed size.
*/
char* Info_ValueForKey( const char* s, const char* key )
{
	char		pkey[BIG_INFO_KEY];
	static char value[2][BIG_INFO_VALUE]; // use two buffers so compares
	// work without stomping on each other
	static int	valueindex = 0;
	char*		o;

	if( !s || !key ) {
		return "";
	}

	if( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_ValueForKey: oversize infostring" );
	}

	valueindex ^= 1;

	if( *s == '\\' ) {
		s++;
	}

	while( 1 ) {
		o = pkey;

		while( *s != '\\' ) {
			if( !*s ) {
				return "";
			}

			*o++ = *s++;
		}

		*o = 0;
		s++;

		o = value[valueindex];

		while( *s != '\\' && *s ) {
			*o++ = *s++;
		}

		*o = 0;

		if( !Q_stricmp( key, pkey ) ) {
			return value[valueindex];
		}

		if( !*s ) {
			break;
		}

		s++;
	}

	return "";
}

/*!
	\brief Parses the next key-value pair from a string buffer, advancing the head pointer.

	This function extracts a key-value pair from a string that is formatted as a sequence of escaped strings separated by backslashes. It advances the head pointer to point to the next pair in the
   string. The key and value are stored in the provided buffers, with null termination. The function handles the case where the string is exhausted or where the next character is a backslash, which is
   treated as the start of a new pair.

	\param s Pointer to the current position in the string buffer.
	\param key Buffer to store the extracted key.
	\param value Buffer to store the extracted value.
*/
void Info_NextPair( const char** head, char* key, char* value )
{
	char*		o;
	const char* s;

	s = *head;

	if( *s == '\\' ) {
		s++;
	}

	key[0]	 = 0;
	value[0] = 0;

	o = key;

	while( *s != '\\' ) {
		if( !*s ) {
			*o	  = 0;
			*head = s;
			return;
		}

		*o++ = *s++;
	}

	*o = 0;
	s++;

	o = value;

	while( *s != '\\' && *s ) {
		*o++ = *s++;
	}

	*o = 0;

	*head = s;
}

/*!
	\brief Removes a key-value pair from an info string if the key exists.

	This function processes an info string and removes the first occurrence of a specified key-value pair. The info string is expected to be in the format \key\value\key\value\... The function handles
   cases where the key does not exist by doing nothing. It also checks for oversize infostrings and invalid keys containing backslashes.

	\param s Pointer to the info string from which the key-value pair will be removed
	\param key The key whose associated value should be removed from the info string
	\throws This function may throw an error if the infostring is oversize as determined by MAX_INFO_STRING.
*/
void Info_RemoveKey( char* s, const char* key )
{
	char* start;
	char  pkey[MAX_INFO_KEY];
	char  value[MAX_INFO_VALUE];
	char* o;

	if( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_RemoveKey: oversize infostring" );
	}

	if( strchr( key, '\\' ) ) {
		return;
	}

	while( 1 ) {
		start = s;

		if( *s == '\\' ) {
			s++;
		}

		o = pkey;

		while( *s != '\\' ) {
			if( !*s ) {
				return;
			}

			*o++ = *s++;
		}

		*o = 0;
		s++;

		o = value;

		while( *s != '\\' && *s ) {
			if( !*s ) {
				return;
			}

			*o++ = *s++;
		}

		*o = 0;

		if( !strcmp( key, pkey ) ) {
			strcpy( start, s ); // remove this part
			return;
		}

		if( !*s ) {
			return;
		}
	}
}

/*!
	\brief Removes a key-value pair from a big info string if the key exists.

	This function processes a null-terminated string that contains key-value pairs separated by backslashes. It searches for the specified key and removes the corresponding key-value pair from the
   string. The function modifies the input string in place. If the input string exceeds the maximum allowed size for a big info string, an error is reported. The function returns early if the key
   contains a backslash character.

	\param s The info string from which the key-value pair will be removed
	\param key The key to be removed from the info string
	\throws Com_Error if the info string exceeds the maximum allowed size for a big info string
*/
void Info_RemoveKey_Big( char* s, const char* key )
{
	char* start;
	char  pkey[BIG_INFO_KEY];
	char  value[BIG_INFO_VALUE];
	char* o;

	if( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_RemoveKey_Big: oversize infostring" );
	}

	if( strchr( key, '\\' ) ) {
		return;
	}

	while( 1 ) {
		start = s;

		if( *s == '\\' ) {
			s++;
		}

		o = pkey;

		while( *s != '\\' ) {
			if( !*s ) {
				return;
			}

			*o++ = *s++;
		}

		*o = 0;
		s++;

		o = value;

		while( *s != '\\' && *s ) {
			if( !*s ) {
				return;
			}

			*o++ = *s++;
		}

		*o = 0;

		if( !strcmp( key, pkey ) ) {
			strcpy( start, s ); // remove this part
			return;
		}

		if( !*s ) {
			return;
		}
	}
}

/*!
	\brief Validates that an info string does not contain illegal characters

	Checks if the provided info string contains quotes or semicolons, which are illegal and could disrupt server parsing. Returns true if the string is valid, false otherwise.

	\param s The info string to validate
	\return qtrue if the info string is valid, qfalse if it contains illegal characters
*/
qboolean Info_Validate( const char* s )
{
	if( strchr( s, '\"' ) ) {
		return qfalse;
	}

	if( strchr( s, ';' ) ) {
		return qfalse;
	}

	return qtrue;
}

/*
==================
Info_SetValueForKey

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey( char* s, const char* key, const char* value )
{
	char newi[MAX_INFO_STRING];

	if( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_SetValueForKey: oversize infostring" );
	}

	if( strchr( key, '\\' ) || strchr( value, '\\' ) ) {
		Com_Printf( "Can't use keys or values with a \\\n" );
		return;
	}

	if( strchr( key, ';' ) || strchr( value, ';' ) ) {
		Com_Printf( "Can't use keys or values with a semicolon\n" );
		return;
	}

	if( strchr( key, '\"' ) || strchr( value, '\"' ) ) {
		Com_Printf( "Can't use keys or values with a \"\n" );
		return;
	}

	Info_RemoveKey( s, key );

	if( !value || !strlen( value ) ) {
		return;
	}

	Com_sprintf( newi, sizeof( newi ), "\\%s\\%s", key, value );

	if( strlen( newi ) + strlen( s ) > MAX_INFO_STRING ) {
		Com_Printf( "Info string length exceeded\n" );
		return;
	}

	strcat( s, newi );
}

/*
==================
Info_SetValueForKey_Big

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey_Big( char* s, const char* key, const char* value )
{
	char newi[BIG_INFO_STRING];

	if( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_SetValueForKey: oversize infostring" );
	}

	if( strchr( key, '\\' ) || strchr( value, '\\' ) ) {
		Com_Printf( "Can't use keys or values with a \\\n" );
		return;
	}

	if( strchr( key, ';' ) || strchr( value, ';' ) ) {
		Com_Printf( "Can't use keys or values with a semicolon\n" );
		return;
	}

	if( strchr( key, '\"' ) || strchr( value, '\"' ) ) {
		Com_Printf( "Can't use keys or values with a \"\n" );
		return;
	}

	Info_RemoveKey_Big( s, key );

	if( !value || !strlen( value ) ) {
		return;
	}

	Com_sprintf( newi, sizeof( newi ), "\\%s\\%s", key, value );

	if( strlen( newi ) + strlen( s ) > BIG_INFO_STRING ) {
		Com_Printf( "BIG Info string length exceeded\n" );
		return;
	}

	strcat( s, newi );
}

//====================================================================
