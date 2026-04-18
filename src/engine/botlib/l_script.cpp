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
 * name:		l_script.c
 *
 * desc:		lexicographical parser
 *
 *
 *****************************************************************************/

// #define SCREWUP
// #define BOTLIB
// #define MEQCC
// #define BSPC

#ifdef SCREWUP
	#include <stdio.h>
	#include <stdlib.h>
	#include <limits.h>
	#include <string.h>
	#include <stdarg.h>
	#include "../botlib/l_memory.h"
	#include "../botlib/l_script.h"

typedef enum { qfalse, qtrue } qboolean;

#endif // SCREWUP

#ifdef BOTLIB
	// include files for usage in the bot library
	#include "q_shared.h"
	#include "botshared/botlib.h"
	#include "be_interface.h"
	#include "l_script.h"
	#include "l_memory.h"
	#include "l_log.h"
	#include "l_libvar.h"
#endif // BOTLIB

#ifdef MEQCC
	// include files for usage in MrElusive's QuakeC Compiler
	#include "qcc.h"
	#include "l_script.h"
	#include "l_memory.h"
	#include "l_log.h"

	#define qtrue  true
	#define qfalse false
#endif // MEQCC

#ifdef BSPC
	// include files for usage in the BSP Converter
	#include "../bspc/qbsp.h"
	#include "../bspc/l_log.h"
	#include "../bspc/l_mem.h"
int COM_Compress( char* data_p );

	#define qtrue  true
	#define qfalse false
#endif // BSPC

#define PUNCTABLE

// longer punctuations first
punctuation_t default_punctuations[] = {
	// binary operators
	{ ">>=", P_RSHIFT_ASSIGN, NULL },
	{ "<<=", P_LSHIFT_ASSIGN, NULL },
	//
	{ "...", P_PARMS, NULL },
	// define merge _operator
	{ "##", P_PRECOMPMERGE, NULL },
	// logic operators
	{ "&&", P_LOGIC_AND, NULL },
	{ "||", P_LOGIC_OR, NULL },
	{ ">=", P_LOGIC_GEQ, NULL },
	{ "<=", P_LOGIC_LEQ, NULL },
	{ "==", P_LOGIC_EQ, NULL },
	{ "!=", P_LOGIC_UNEQ, NULL },
	// arithmatic operators
	{ "*=", P_MUL_ASSIGN, NULL },
	{ "/=", P_DIV_ASSIGN, NULL },
	{ "%=", P_MOD_ASSIGN, NULL },
	{ "+=", P_ADD_ASSIGN, NULL },
	{ "-=", P_SUB_ASSIGN, NULL },
	{ "++", P_INC, NULL },
	{ "--", P_DEC, NULL },
	// binary operators
	{ "&=", P_BIN_AND_ASSIGN, NULL },
	{ "|=", P_BIN_OR_ASSIGN, NULL },
	{ "^=", P_BIN_XOR_ASSIGN, NULL },
	{ ">>", P_RSHIFT, NULL },
	{ "<<", P_LSHIFT, NULL },
	// reference operators
	{ "->", P_POINTERREF, NULL },
	// C++
	{ "::", P_CPP1, NULL },
	{ ".*", P_CPP2, NULL },
	// arithmatic operators
	{ "*", P_MUL, NULL },
	{ "/", P_DIV, NULL },
	{ "%", P_MOD, NULL },
	{ "+", P_ADD, NULL },
	{ "-", P_SUB, NULL },
	{ "=", P_ASSIGN, NULL },
	// binary operators
	{ "&", P_BIN_AND, NULL },
	{ "|", P_BIN_OR, NULL },
	{ "^", P_BIN_XOR, NULL },
	{ "~", P_BIN_NOT, NULL },
	// logic operators
	{ "!", P_LOGIC_NOT, NULL },
	{ ">", P_LOGIC_GREATER, NULL },
	{ "<", P_LOGIC_LESS, NULL },
	// reference _operator
	{ ".", P_REF, NULL },
	// seperators
	{ ",", P_COMMA, NULL },
	{ ";", P_SEMICOLON, NULL },
	// label indication
	{ ":", P_COLON, NULL },
	// if statement
	{ "?", P_QUESTIONMARK, NULL },
	// embracements
	{ "(", P_PARENTHESESOPEN, NULL },
	{ ")", P_PARENTHESESCLOSE, NULL },
	{ "{", P_BRACEOPEN, NULL },
	{ "}", P_BRACECLOSE, NULL },
	{ "[", P_SQBRACKETOPEN, NULL },
	{ "]", P_SQBRACKETCLOSE, NULL },
	//
	{ "\\", P_BACKSLASH, NULL },
	// precompiler _operator
	{ "#", P_PRECOMP, NULL },
#ifdef DOLLAR
	{ "$", P_DOLLAR, NULL },
#endif // DOLLAR
	{ NULL, 0 }
};

#ifdef BSPC
char basefolder[MAX_PATH];
#else
char basefolder[MAX_QPATH];
#endif

/*!
	\brief Creates a sorted punctuation lookup table for a script from a list of punctuation entries.

	The function first ensures that the script has a pre‑allocated table of 256 entries, each entry pointing to a linked list of punctuation structures. It then iterates through the supplied array of
   punctuations, inserting each one into the appropriate list based on the first character of the punctuation string. Within each list the punctuations are ordered by length, with longer strings
   inserted before shorter ones to guarantee correct matching during script parsing. The table is initialized to zero before use, and each new entry is linked either at the front or after the correct
   predecessor to maintain the order.

	\param script pointer to the script structure that will receive the punctuation table
	\param punctuations array of punctuation_t objects terminated by an entry with a null string field
*/
void PS_CreatePunctuationTable( script_t* script, punctuation_t* punctuations )
{
	int			   i;
	punctuation_t *p, *lastp, *newp;

	// get memory for the table
	if( !script->punctuationtable ) {
		script->punctuationtable = ( punctuation_t** )GetMemory( 256 * sizeof( punctuation_t* ) );
	}

	memset( script->punctuationtable, 0, 256 * sizeof( punctuation_t* ) );

	// add the punctuations in the list to the punctuation table
	for( i = 0; punctuations[i].p; i++ ) {
		newp  = &punctuations[i];
		lastp = NULL;

		// sort the punctuations in this table entry on length (longer punctuations first)
		for( p = script->punctuationtable[( unsigned int )newp->p[0]]; p; p = p->next ) {
			if( strlen( p->p ) < strlen( newp->p ) ) {
				newp->next = p;

				if( lastp ) {
					lastp->next = newp;

				} else {
					script->punctuationtable[( unsigned int )newp->p[0]] = newp;
				}

				break;
			}

			lastp = p;
		}

		if( !p ) {
			newp->next = NULL;

			if( lastp ) {
				lastp->next = newp;

			} else {
				script->punctuationtable[( unsigned int )newp->p[0]] = newp;
			}
		}
	}
}

/*!
	\brief Returns the punctuation string corresponding to a given punctuation number from the script's punctuation table

	This function searches through the punctuation table of a script to find the string representation of a given punctuation number. It iterates through the punctuation entries until it finds a match for the specified number, returning the corresponding punctuation string. If no match is found, it returns a default string "unkown punctuation".

	\param script Pointer to the script structure containing the punctuation table
	\param num The punctuation number to look up
	\return Pointer to the punctuation string corresponding to the given number, or a default error string if not found
*/
char* PunctuationFromNum( script_t* script, int num )
{
	int i;

	for( i = 0; script->punctuations[i].p; i++ ) {
		if( script->punctuations[i].n == num ) {
			return script->punctuations[i].p;
		}
	}

	return "unkown punctuation";
}

/*!
	\brief Reports an error with formatted message at the current script location

	This function reports an error message at the current location in a script file. It formats the error message using a printf-style format string and variable arguments, then outputs the error with file name, line number, and the formatted message. The function checks if error reporting is disabled for the script before proceeding. It supports multiple build configurations including bot library, MEQCC, and BSPC for different output methods.

	\param script pointer to the script structure containing the file information and current line number
	\param str printf-style format string for the error message
*/
void QDECL ScriptError( script_t* script, char* str, ... )
{
	char	text[1024];
	va_list ap;

	if( script->flags & SCFL_NOERRORS ) {
		return;
	}

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );
#ifdef BOTLIB
	botimport.Print( PRT_ERROR, "file %s, line %d: %s\n", script->filename, script->line, text );
#endif // BOTLIB
#ifdef MEQCC
	printf( "error: file %s, line %d: %s\n", script->filename, script->line, text );
#endif // MEQCC
#ifdef BSPC
	Log_Print( "error: file %s, line %d: %s\n", script->filename, script->line, text );
#endif // BSPC
}

/*!
	\brief Reports a warning message with file and line number information from a script

	This function outputs a formatted warning message to different destinations depending on the compilation flags. The warning includes the script filename, current line number, and the formatted warning text. It checks if warnings are disabled for the script before proceeding. The function uses variadic arguments to allow for flexible warning message formatting.

	\param script Pointer to the script context containing filename and line information
	\param str Format string for the warning message
*/
void QDECL ScriptWarning( script_t* script, char* str, ... )
{
	char	text[1024];
	va_list ap;

	if( script->flags & SCFL_NOWARNINGS ) {
		return;
	}

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );
#ifdef BOTLIB
	botimport.Print( PRT_WARNING, "file %s, line %d: %s\n", script->filename, script->line, text );
#endif // BOTLIB
#ifdef MEQCC
	printf( "warning: file %s, line %d: %s\n", script->filename, script->line, text );
#endif // MEQCC
#ifdef BSPC
	Log_Print( "warning: file %s, line %d: %s\n", script->filename, script->line, text );
#endif // BSPC
}

/*!
	\brief Sets the punctuation table for a script, using either a provided table or a default table if none is provided.

	This function configures the punctuation table that a script will use for parsing. If a custom punctuation table is provided, it will be used; otherwise, a default punctuation table is applied. The function ensures that the script's punctuation table is properly updated regardless of whether the provided table is NULL.

	\param script Pointer to the script structure that will have its punctuation table set
	\param p Pointer to the punctuation table to use, or NULL to use the default table
*/
void SetScriptPunctuations( script_t* script, punctuation_t* p )
{
#ifdef PUNCTABLE

	if( p ) {
		PS_CreatePunctuationTable( script, p );

	} else {
		PS_CreatePunctuationTable( script, default_punctuations );
	}

#endif // PUNCTABLE

	if( p ) {
		script->punctuations = p;

	} else {
		script->punctuations = default_punctuations;
	}
}

/*!
	\brief Consumes whitespace and comments from a script, advancing the script pointer and line counter.
*/
int PS_ReadWhiteSpace( script_t* script )
{
	while( 1 ) {
		// skip white space
		while( *script->script_p <= ' ' ) {
			if( !*script->script_p ) {
				return 0;
			}

			if( *script->script_p == '\n' ) {
				script->line++;
			}

			script->script_p++;
		}

		// skip comments
		if( *script->script_p == '/' ) {
			// comments //
			if( *( script->script_p + 1 ) == '/' ) {
				script->script_p++;

				do {
					script->script_p++;

					if( !*script->script_p ) {
						return 0;
					}
				} // end do

				while( *script->script_p != '\n' );

				script->line++;
				script->script_p++;

				if( !*script->script_p ) {
					return 0;
				}

				continue;
			}

			// comments /* */
			else if( *( script->script_p + 1 ) == '*' ) {
				script->script_p++;

				do {
					script->script_p++;

					if( !*script->script_p ) {
						return 0;
					}

					if( *script->script_p == '\n' ) {
						script->line++;
					}
				} // end do

				while( !( *script->script_p == '*' && *( script->script_p + 1 ) == '/' ) );

				script->script_p++;

				if( !*script->script_p ) {
					return 0;
				}

				script->script_p++;

				if( !*script->script_p ) {
					return 0;
				}

				continue;
			}
		}

		break;
	}

	return 1;
}

/*!
	\brief Reads and interprets the next escape sequence in a script and stores the resulting character.

	The function assumes that the script pointer is positioned at the backslash that begins an escape sequence. It then advances past this backslash, examines the following character(s), and converts
   them into the corresponding single character. Supported escapes include standard C literals such as \\n, \\r, \\t, \\v, \\b, \\f, \\a, as well as escaped quotes and the question mark. Hexadecimal
   escapes of the form \\xNN are processed by reading subsequent hexadecimal digits; decimal escapes of the form \d+ are also accepted. Values larger than 0xFF trigger a warning and are truncated to
   0xFF. Any unknown escape character causes an error. After determining the character value, the function stores it through the provided char pointer, moves the script pointer past the processed
   escape code, and returns 1 to indicate success.

	\param script the script object to read the escape from
	\param ch pointer to store the resolved character
	\return 1 to signal that a character was read successfully; callers typically treat a zero return as failure
*/
int PS_ReadEscapeCharacter( script_t* script, char* ch )
{
	int c, val, i;

	// step over the leading '\\'
	script->script_p++;

	// determine the escape character
	switch( *script->script_p ) {
		case '\\':
			c = '\\';
			break;

		case 'n':
			c = '\n';
			break;

		case 'r':
			c = '\r';
			break;

		case 't':
			c = '\t';
			break;

		case 'v':
			c = '\v';
			break;

		case 'b':
			c = '\b';
			break;

		case 'f':
			c = '\f';
			break;

		case 'a':
			c = '\a';
			break;

		case '\'':
			c = '\'';
			break;

		case '\"':
			c = '\"';
			break;

		case '\?':
			c = '\?';
			break;

		case 'x': {
			script->script_p++;

			for( i = 0, val = 0;; i++, script->script_p++ ) {
				c = *script->script_p;

				if( c >= '0' && c <= '9' ) {
					c = c - '0';

				} else if( c >= 'A' && c <= 'Z' ) {
					c = c - 'A' + 10;

				} else if( c >= 'a' && c <= 'z' ) {
					c = c - 'a' + 10;

				} else {
					break;
				}

				val = ( val << 4 ) + c;
			}

			script->script_p--;

			if( val > 0xFF ) {
				ScriptWarning( script, "too large value in escape character" );
				val = 0xFF;
			}

			c = val;
			break;
		} // end case

		default: { // NOTE: decimal ASCII code, NOT octal
			if( *script->script_p < '0' || *script->script_p > '9' ) {
				ScriptError( script, "unknown escape char" );
			}

			for( i = 0, val = 0;; i++, script->script_p++ ) {
				c = *script->script_p;

				if( c >= '0' && c <= '9' ) {
					c = c - '0';

				} else {
					break;
				}

				val = val * 10 + c;
			}

			script->script_p--;

			if( val > 0xFF ) {
				ScriptWarning( script, "too large value in escape character" );
				val = 0xFF;
			}

			c = val;
			break;
		} // end default
	}

	// step over the escape character or the last digit of the number
	script->script_p++;
	// store the escape character
	*ch = c;
	// succesfully read escape character
	return 1;
}

/*!
	\brief Reads a string literal or quoted text from the script while handling escape characters, optional whitespace, and concatenating adjacent strings

	The function identifies the type of the token based on the quote character, then copies characters from the script buffer into the token's string field. It supports escaping characters unless
   disabled by the script flags, enforces a maximum token length, and allows two strings separated by whitespace to be merged into one token. When a terminating quote is found it appends the quote,
   null‑terminates the token, sets the subtype to the token length, and returns success. Errors such as missing trailing quote or newline inside a string cause a script error and result in a return of
   0.

	\param quote the quotation character that marks the start and end of the string
	\param script pointer to the script state used for reading input
	\param token buffer where the extracted string and its metadata are stored
	\return Non‑zero on success, zero on failure
*/
int PS_ReadString( script_t* script, token_t* token, int quote )
{
	int	  len, tmpline;
	char* tmpscript_p;

	if( quote == '\"' ) {
		token->type = TT_STRING;

	} else {
		token->type = TT_LITERAL;
	}

	len = 0;
	// leading quote
	token->string[len++] = *script->script_p++;

	//
	while( 1 ) {
		// minus 2 because trailing double quote and zero have to be appended
		if( len >= MAX_TOKEN - 2 ) {
			ScriptError( script, "string longer than MAX_TOKEN = %d", MAX_TOKEN );
			return 0;
		}

		// if there is an escape character and
		// if escape characters inside a string are allowed
		if( *script->script_p == '\\' && !( script->flags & SCFL_NOSTRINGESCAPECHARS ) ) {
			if( !PS_ReadEscapeCharacter( script, &token->string[len] ) ) {
				token->string[len] = 0;
				return 0;
			}

			len++;
		}

		// if a trailing quote
		else if( *script->script_p == quote ) {
			// step over the double quote
			script->script_p++;

			// if white spaces in a string are not allowed
			if( script->flags & SCFL_NOSTRINGWHITESPACES ) {
				break;
			}

			//
			tmpscript_p = script->script_p;
			tmpline		= script->line;

			// read unusefull stuff between possible two following strings
			if( !PS_ReadWhiteSpace( script ) ) {
				script->script_p = tmpscript_p;
				script->line	 = tmpline;
				break;
			}

			// if there's no leading double qoute
			if( *script->script_p != quote ) {
				script->script_p = tmpscript_p;
				script->line	 = tmpline;
				break;
			}

			// step over the new leading double quote
			script->script_p++;

		} else {
			if( *script->script_p == '\0' ) {
				token->string[len] = 0;
				ScriptError( script, "missing trailing quote" );
				return 0;
			}

			if( *script->script_p == '\n' ) {
				token->string[len] = 0;
				ScriptError( script, "newline inside string %s", token->string );
				return 0;
			}

			token->string[len++] = *script->script_p++;
		}
	}

	// trailing quote
	token->string[len++] = quote;
	// end string with a zero
	token->string[len] = '\0';
	// the sub type is the length of the string
	token->subtype = len;
	return 1;
}

/*!
	\brief Reads a name token from a script into the given token structure

	The function marks the token as a name and then consumes characters from the script pointer that are alphabetic, numeric, or underscore. Each consumed character is appended to the token's string
   buffer, and the function monitors the length to not exceed MAX_TOKEN. If the token would be too long, ScriptError is invoked with an appropriate message and the function returns 0 to indicate
   failure. Upon successful completion, the token string is null‑terminated, its subtype is set to the name length, and the function returns 1.

	\param script pointer to the script currently being parsed
	\param token token structure to populate with the name token
	\return 1 if a name token was successfully read; 0 if the token exceeded MAX_TOKEN and an error was reported
*/
int PS_ReadName( script_t* script, token_t* token )
{
	int	 len = 0;
	char c;

	token->type = TT_NAME;

	do {
		token->string[len++] = *script->script_p++;

		if( len >= MAX_TOKEN ) {
			ScriptError( script, "name longer than MAX_TOKEN = %d", MAX_TOKEN );
			return 0;
		}

		c = *script->script_p;
	} while( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) || c == '_' );

	token->string[len] = '\0';
	// the sub type is the length of the name
	token->subtype = len;
	return 1;
}

/*!
	\brief Parses a numeric string according to a subtype mask and assigns both integer and floating‑point representations.

	The function interprets the character sequence pointed to by &String based on the bitmask in &subtype, which may indicate a floating‑point number (TT_FLOAT), an ordinary decimal integer
   (TT_DECIMAL), a hexadecimal integer (TT_HEX), an octal integer (TT_OCTAL), or a binary integer (TT_BINARY). For TT_FLOAT it processes digits and an optional single decimal point, accumulating a
   long double value; the integer output is then set to the truncation of that value. For the other subtypes it directly converts the digits into an unsigned long integer, skipping the appropriate
   leading prefixes (0x, 0, 0b). In all cases the floating‑point output is set to the exact same numeric value as the integer output. If a second decimal point is encountered while parsing TT_FLOAT,
   the function exits early without modifying the outputs beyond the values computed so far.

	\param string String containing the number literal to parse
	\param subtype Bitmask specifying the numeric format (TT_FLOAT, TT_DECIMAL, TT_HEX, TT_OCTAL, TT_BINARY)
	\param intvalue Pointer to store the computed integer value
	\param floatvalue Pointer to store the computed long double value
*/
void NumberValue( char* string, int subtype, unsigned long int* intvalue, long double* floatvalue )
{
	unsigned long int dotfound = 0;

	*intvalue	= 0;
	*floatvalue = 0;

	// floating point number
	if( subtype & TT_FLOAT ) {
		while( *string ) {
			if( *string == '.' ) {
				if( dotfound ) {
					return;
				}

				dotfound = 10;
				string++;
			}

			if( dotfound ) {
				*floatvalue = *floatvalue + ( long double )( *string - '0' ) / ( long double )dotfound;
				dotfound *= 10;

			} else {
				*floatvalue = *floatvalue * 10.0 + ( long double )( *string - '0' );
			}

			string++;
		}

		*intvalue = ( unsigned long )*floatvalue;

	} else if( subtype & TT_DECIMAL ) {
		while( *string ) {
			*intvalue = *intvalue * 10 + ( *string++ - '0' );
		}

		*floatvalue = *intvalue;

	} else if( subtype & TT_HEX ) {
		// step over the leading 0x or 0X
		string += 2;

		while( *string ) {
			*intvalue <<= 4;

			if( *string >= 'a' && *string <= 'f' ) {
				*intvalue += *string - 'a' + 10;

			} else if( *string >= 'A' && *string <= 'F' ) {
				*intvalue += *string - 'A' + 10;

			} else {
				*intvalue += *string - '0';
			}

			string++;
		}

		*floatvalue = *intvalue;

	} else if( subtype & TT_OCTAL ) {
		// step over the first zero
		string += 1;

		while( *string ) {
			*intvalue = ( *intvalue << 3 ) + ( *string++ - '0' );
		}

		*floatvalue = *intvalue;

	} else if( subtype & TT_BINARY ) {
		// step over the leading 0b or 0B
		string += 2;

		while( *string ) {
			*intvalue = ( *intvalue << 1 ) + ( *string++ - '0' );
		}

		*floatvalue = *intvalue;
	}
}

/*!
	\brief Reads the next numeric literal from a script and fills a token with its value and type information.

	The routine begins by marking the token as a number. It first checks for hexadecimal prefixed with 0x or 0X and collects the digits until a non‑digit character is found, flagging the token subtype
   as hex.  If BINARYNUMBERS is defined it also accepts 0b or 0B followed by binary digits and flags the subtype accordingly.  For all other numbers it parses decimal, octal and floating point forms:
   an initial 0 sets an octal flag, dots enable a floating point flag, and any digits 8 or 9 clear the octal flag.  The implementation ensures the token string does not exceed MAX_TOKEN characters,
   emitting ScriptError on overflow and returning 0.  After the main number digits, it scans an optional long "l"/"L" and unsigned "u"/"U" suffix, updating the corresponding subtype flags.  The final
   character terminator is inserted, and if NUMBERVALUE is defined the numeric string is converted to integer and float values stored in the token.  The token subtype is marked as an integer if no
   float flag is present.  On success the function returns 1, otherwise 0.

	\param script pointer to the script context being parsed
	\param token token structure to receive the parsed number and its metadata
	\return non‑zero on success, zero on failure (e.g., token too long or a script parsing error)
*/
int PS_ReadNumber( script_t* script, token_t* token )
{
	int	 len = 0, i;
	int	 octal, dot;
	char c;
	//	unsigned long int intvalue = 0;
	//	long double floatvalue = 0;

	token->type = TT_NUMBER;

	// check for a hexadecimal number
	if( *script->script_p == '0' && ( *( script->script_p + 1 ) == 'x' || *( script->script_p + 1 ) == 'X' ) ) {
		token->string[len++] = *script->script_p++;
		token->string[len++] = *script->script_p++;
		c					 = *script->script_p;

		// hexadecimal
		while( ( c >= '0' && c <= '9' ) || ( c >= 'a' && c <= 'f' ) || ( c >= 'A' && c <= 'A' ) ) {
			token->string[len++] = *script->script_p++;

			if( len >= MAX_TOKEN ) {
				ScriptError( script, "hexadecimal number longer than MAX_TOKEN = %d", MAX_TOKEN );
				return 0;
			}

			c = *script->script_p;
		}

		token->subtype |= TT_HEX;
	}
#ifdef BINARYNUMBERS
	// check for a binary number
	else if( *script->script_p == '0' && ( *( script->script_p + 1 ) == 'b' || *( script->script_p + 1 ) == 'B' ) ) {
		token->string[len++] = *script->script_p++;
		token->string[len++] = *script->script_p++;
		c					 = *script->script_p;

		// hexadecimal
		while( c == '0' || c == '1' ) {
			token->string[len++] = *script->script_p++;

			if( len >= MAX_TOKEN ) {
				ScriptError( script, "binary number longer than MAX_TOKEN = %d", MAX_TOKEN );
				return 0;
			}

			c = *script->script_p;
		}

		token->subtype |= TT_BINARY;
	}

#endif // BINARYNUMBERS

	else { // decimal or octal integer or floating point number
		octal = qfalse;
		dot	  = qfalse;

		if( *script->script_p == '0' ) {
			octal = qtrue;
		}

		while( 1 ) {
			c = *script->script_p;

			if( c == '.' ) {
				dot = qtrue;

			} else if( c == '8' || c == '9' ) {
				octal = qfalse;

			} else if( c < '0' || c > '9' ) {
				break;
			}

			token->string[len++] = *script->script_p++;

			if( len >= MAX_TOKEN - 1 ) {
				ScriptError( script, "number longer than MAX_TOKEN = %d", MAX_TOKEN );
				return 0;
			}
		}

		if( octal ) {
			token->subtype |= TT_OCTAL;

		} else {
			token->subtype |= TT_DECIMAL;
		}

		if( dot ) {
			token->subtype |= TT_FLOAT;
		}
	}

	for( i = 0; i < 2; i++ ) {
		c = *script->script_p;

		// check for a LONG number
		//  TTimo: gcc: suggest parentheses around && within ||
		//  initial line:
		//  if (c == 'l' || c == 'L' && !(token->subtype & TT_LONG))
		//  changed behaviour
		if( ( c == 'l' || c == 'L' ) && !( token->subtype & TT_LONG ) ) {
			script->script_p++;
			token->subtype |= TT_LONG;
		}

		// check for an UNSIGNED number
		//  TTimo: gcc: suggest parentheses around && within ||
		//  initial line:
		//  else if (c == 'u' || c == 'U' && !(token->subtype & (TT_UNSIGNED | TT_FLOAT)))
		//  changed behaviour
		else if( ( c == 'u' || c == 'U' ) && !( token->subtype & ( TT_UNSIGNED | TT_FLOAT ) ) ) {
			script->script_p++;
			token->subtype |= TT_UNSIGNED;
		}
	}

	token->string[len] = '\0';
#ifdef NUMBERVALUE
	NumberValue( token->string, token->subtype, &token->intvalue, &token->floatvalue );
#endif // NUMBERVALUE

	if( !( token->subtype & TT_FLOAT ) ) {
		token->subtype |= TT_INTEGER;
	}

	return 1;
}

/*!
	\brief Parses a character literal from the script and stores it in the token.

	The function expects the current script position to be at a single quote beginning of a literal. It reads the first quote, checks for end of file, handles an escape sequence if the next character
   is a backslash, otherwise reads the next character. If the closing quote is missing it emits a warning and skips until the closing quote or a line break. It then stores the trailing quote and a
   null terminator in the token’s string array and sets the subtype to the literal character value. The function returns 1 on success and 0 if an error such as unexpected end of file or escape read
   failure occurs.

	\param script script context from which to read characters
	\param token token to populate with parsed literal
	\return nonzero on success, zero on failure
*/
int PS_ReadLiteral( script_t* script, token_t* token )
{
	token->type = TT_LITERAL;
	// first quote
	token->string[0] = *script->script_p++;

	// check for end of file
	if( !*script->script_p ) {
		ScriptError( script, "end of file before trailing \'" );
		return 0;
	}

	// if it is an escape character
	if( *script->script_p == '\\' ) {
		if( !PS_ReadEscapeCharacter( script, &token->string[1] ) ) {
			return 0;
		}

	} else {
		token->string[1] = *script->script_p++;
	}

	// check for trailing quote
	if( *script->script_p != '\'' ) {
		ScriptWarning( script, "too many characters in literal, ignored" );

		while( *script->script_p && *script->script_p != '\'' && *script->script_p != '\n' ) {
			script->script_p++;
		}

		if( *script->script_p == '\'' ) {
			script->script_p++;
		}
	}

	// store the trailing quote
	token->string[2] = *script->script_p++;
	// store trailing zero to end the string
	token->string[3] = '\0';
	// the sub type is the integer literal value
	token->subtype = token->string[1];
	//
	return 1;
}

/*!
	\brief Attempts to read the next punctuation token from the script and stores it in the provided token structure.

	It searches the script’s punctuation table (or array if the table is not used) for a string that matches the text at the current script pointer. If a match is found, the token’s string field is
   filled up to MAX_TOKEN, the script pointer is advanced by the length of the punctuation, the token type is set to TT_PUNCTUATION, and the token subtype is set to the punctuation’s index number. The
   function returns 1 on success and 0 if no matching punctuation can be read or the script does not contain enough characters.

	\param script The script context from which to read the next token
	\param token The token structure to populate when a punctuation is detected
	\return 1 if a punctuation token was found and copied, otherwise 0
*/
int PS_ReadPunctuation( script_t* script, token_t* token )
{
	int			   len;
	char*		   p;
	punctuation_t* punc;

#ifdef PUNCTABLE

	for( punc = script->punctuationtable[( unsigned int )*script->script_p]; punc; punc = punc->next ) {
#else
	int i;

	for( i = 0; script->punctuations[i].p; i++ ) {
		punc = &script->punctuations[i];
#endif // PUNCTABLE
		p	= punc->p;
		len = strlen( p );

		// if the script contains at least as much characters as the punctuation
		if( script->script_p + len <= script->end_p ) {
			// if the script contains the punctuation
			if( !strncmp( script->script_p, p, len ) ) {
				strncpy( token->string, p, MAX_TOKEN );
				script->script_p += len;
				token->type = TT_PUNCTUATION;
				// sub type is the number of the punctuation
				token->subtype = punc->n;
				return 1;
			}
		}
	}

	return 0;
}

/*!
	\brief Reads a primitive token from a script buffer into the provided token structure

	The function scans the script data starting at script->script_p and collects characters until a space or a semicolon is encountered. Each character is appended to token->string until the MAX_TOKEN
   limit is reached. If the token would exceed this limit, ScriptError is invoked and the function returns 0, indicating failure. Upon successful reading, the token string is null‑terminated, the
   token structure is copied into script->token for quick access by the caller, and a value of 1 is returned.

	\param script Pointer to the script structure used to track the current reading position and state
	\param token Pointer to a token_t structure that will be filled with the read token string
	\return 1 if the primitive token was read successfully, 0 if the token exceeded the maximum length and an error was reported
*/
int PS_ReadPrimitive( script_t* script, token_t* token )
{
	int len;

	len = 0;

	while( *script->script_p > ' ' && *script->script_p != ';' ) {
		if( len >= MAX_TOKEN ) {
			ScriptError( script, "primitive token longer than MAX_TOKEN = %d", MAX_TOKEN );
			return 0;
		}

		token->string[len++] = *script->script_p++;
	}

	token->string[len] = 0;
	// copy the token into the script structure
	memcpy( &script->token, token, sizeof( token_t ) );
	// primitive reading successfull
	return 1;
}

/*!
	\brief Reads the next token from the script input and stores it in the provided token structure

	This function is responsible for parsing the next token from a script input stream. It first checks if there is a previously unread token available, and if so, returns that token. Otherwise, it reads whitespace, identifies the type of token (string, number, name, or punctuation), and populates the token structure accordingly. The function handles various token types including quoted strings, numeric literals, and identifiers. If a primitive script flag is set, it delegates to a specialized primitive reading function. The function returns 1 on success and 0 on failure, typically indicating end-of-input or a parsing error

	\param script Pointer to the script structure containing the input stream and parsing state
	\param token Pointer to the token structure where the parsed token will be stored
	\return Integer value indicating success (1) or failure (0) of the token reading operation
*/
int PS_ReadToken( script_t* script, token_t* token )
{
	// if there is a token available (from UnreadToken)
	if( script->tokenavailable ) {
		script->tokenavailable = 0;
		memcpy( token, &script->token, sizeof( token_t ) );
		return 1;
	}

	// save script pointer
	script->lastscript_p = script->script_p;
	// save line counter
	script->lastline = script->line;
	// clear the token stuff
	memset( token, 0, sizeof( token_t ) );
	// start of the white space
	script->whitespace_p = script->script_p;
	token->whitespace_p	 = script->script_p;

	// read unusefull stuff
	if( !PS_ReadWhiteSpace( script ) ) {
		return 0;
	}

	// end of the white space
	script->endwhitespace_p = script->script_p;
	token->endwhitespace_p	= script->script_p;
	// line the token is on
	token->line = script->line;
	// number of lines crossed before token
	token->linescrossed = script->line - script->lastline;

	// if there is a leading double quote
	if( *script->script_p == '\"' ) {
		if( !PS_ReadString( script, token, '\"' ) ) {
			return 0;
		}
	}

	// if an literal
	else if( *script->script_p == '\'' ) {
		// if (!PS_ReadLiteral(script, token)) return 0;
		if( !PS_ReadString( script, token, '\'' ) ) {
			return 0;
		}
	}

	// if there is a number
	else if( ( *script->script_p >= '0' && *script->script_p <= '9' ) || ( *script->script_p == '.' && ( *( script->script_p + 1 ) >= '0' && *( script->script_p + 1 ) <= '9' ) ) ) {
		if( !PS_ReadNumber( script, token ) ) {
			return 0;
		}
	}

	// if this is a primitive script
	else if( script->flags & SCFL_PRIMITIVE ) {
		return PS_ReadPrimitive( script, token );
	}

	// if there is a name
	else if( ( *script->script_p >= 'a' && *script->script_p <= 'z' ) || ( *script->script_p >= 'A' && *script->script_p <= 'Z' ) || *script->script_p == '_' ) {
		if( !PS_ReadName( script, token ) ) {
			return 0;
		}
	}

	// check for punctuations
	else if( !PS_ReadPunctuation( script, token ) ) {
		ScriptError( script, "can't read token" );
		return 0;
	}

	// copy the token into the script structure
	memcpy( &script->token, token, sizeof( token_t ) );
	// succesfully read a token
	return 1;
}

/*!
	\brief Checks if the next token in the script matches the expected string, returning 1 if successful or 0 if not.

	This function reads the next token from the provided script and compares it to the expected string. If the token does not exist or does not match the expected string, it reports an error and returns 0. If the token matches the expected string, it returns 1. The function is typically used for parsing and validation in script processing.

	\param script Pointer to the script structure containing the token stream
	\param string The expected string to match against the next token
	\return Returns 1 if the next token in the script matches the expected string, or 0 if it does not match or if there is no token available.
*/
int PS_ExpectTokenString( script_t* script, char* string )
{
	token_t token;

	if( !PS_ReadToken( script, &token ) ) {
		ScriptError( script, "couldn't find expected %s", string );
		return 0;
	}

	if( strcmp( token.string, string ) ) {
		ScriptError( script, "expected %s, found %s", string, token.string );
		return 0;
	}

	return 1;
}

/*!
	\brief Reads and validates the next token from a script, ensuring it matches the expected type and subtype

	This function reads the next token from the provided script and verifies that its type matches the expected type. If the token type does not match, it reports an error and returns zero. For number tokens, it also validates that the subtype matches the expected one. For punctuation tokens, it ensures the subtype matches exactly. If any validation fails, the function reports an error and returns zero. Otherwise, it returns one to indicate success.

	\param script Pointer to the script from which to read the token
	\param type Expected type of the token to read
	\param subtype Expected subtype of the token, used for number and punctuation types
	\param token Pointer to the token structure to fill with the read token
	\return Non-zero on success, zero if the token cannot be read or fails validation
	\throws ScriptError is called when token validation fails
*/
int PS_ExpectTokenType( script_t* script, int type, int subtype, token_t* token )
{
	char str[MAX_TOKEN];

	if( !PS_ReadToken( script, token ) ) {
		ScriptError( script, "couldn't read expected token" );
		return 0;
	}

	if( token->type != type ) {
		if( type == TT_STRING ) {
			strcpy( str, "string" );
		}

		if( type == TT_LITERAL ) {
			strcpy( str, "literal" );
		}

		if( type == TT_NUMBER ) {
			strcpy( str, "number" );
		}

		if( type == TT_NAME ) {
			strcpy( str, "name" );
		}

		if( type == TT_PUNCTUATION ) {
			strcpy( str, "punctuation" );
		}

		ScriptError( script, "expected a %s, found %s", str, token->string );
		return 0;
	}

	if( token->type == TT_NUMBER ) {
		if( ( token->subtype & subtype ) != subtype ) {
			if( subtype & TT_DECIMAL ) {
				strcpy( str, "decimal" );
			}

			if( subtype & TT_HEX ) {
				strcpy( str, "hex" );
			}

			if( subtype & TT_OCTAL ) {
				strcpy( str, "octal" );
			}

			if( subtype & TT_BINARY ) {
				strcpy( str, "binary" );
			}

			if( subtype & TT_LONG ) {
				strcat( str, " long" );
			}

			if( subtype & TT_UNSIGNED ) {
				strcat( str, " unsigned" );
			}

			if( subtype & TT_FLOAT ) {
				strcat( str, " float" );
			}

			if( subtype & TT_INTEGER ) {
				strcat( str, " integer" );
			}

			ScriptError( script, "expected %s, found %s", str, token->string );
			return 0;
		}

	} else if( token->type == TT_PUNCTUATION ) {
		if( subtype < 0 ) {
			ScriptError( script, "BUG: wrong punctuation subtype" );
			return 0;
		}

		if( token->subtype != subtype ) {
			ScriptError( script, "expected %s, found %s", script->punctuations[subtype], token->string );
			return 0;
		}
	}

	return 1;
}

/*!
	\brief Reads the next token from the script and returns whether the operation was successful.

	This function attempts to read the next token from the provided script and stores it in the given token structure. If the token reading fails, it triggers a script error and returns zero. Otherwise, it returns a non-zero value indicating success. The function is typically used when the exact type of the expected token is not known or not important.

	\param script Pointer to the script structure to read from.
	\param token Pointer to the token structure where the read token will be stored.
	\return Non-zero value if the token was successfully read, zero if the operation failed.
*/
int PS_ExpectAnyToken( script_t* script, token_t* token )
{
	if( !PS_ReadToken( script, token ) ) {
		ScriptError( script, "couldn't read expected token" );
		return 0;

	} else {
		return 1;
	}
}

/*!
	\brief Checks if the next token in the script matches the specified string and advances the script position if it does.

	This function attempts to read the next token from the provided script and compares it with the given string.
	If the token matches the string, the function returns 1 and advances the script's position.
	If the token does not match or if there are no more tokens, the function returns 0 and resets the script position to the previous location.

	\param script Pointer to the script structure containing the script data and current position.
	\param string The string to compare against the next token in the script.
	\return 1 if the next token in the script matches the specified string, 0 otherwise.
*/
int PS_CheckTokenString( script_t* script, char* string )
{
	token_t tok;

	if( !PS_ReadToken( script, &tok ) ) {
		return 0;
	}

	// if the token is available
	if( !strcmp( tok.string, string ) ) {
		return 1;
	}

	// token not available
	script->script_p = script->lastscript_p;
	return 0;
}

/*!
	\brief Checks if the next token in the script matches the specified type and subtype, and returns it if it does.

	This function attempts to read the next token from the provided script and compares its type and subtype against the given parameters. If the token matches the specified criteria, the token data is copied to the output parameter and the function returns true. If the token does not match, the script's position is restored and the function returns false. This function is typically used for parsing structured input where specific token sequences need to be validated and extracted.

	\param script Pointer to the script structure to read from
	\param type Expected token type to match against
	\param subtype Expected token subtype to match against
	\param token Pointer to store the matching token if found
	\return Non-zero value if a matching token was found and copied, zero otherwise
*/
int PS_CheckTokenType( script_t* script, int type, int subtype, token_t* token )
{
	token_t tok;

	if( !PS_ReadToken( script, &tok ) ) {
		return 0;
	}

	// if the type matches
	if( tok.type == type && ( tok.subtype & subtype ) == subtype ) {
		memcpy( token, &tok, sizeof( token_t ) );
		return 1;
	}

	// token is not available
	script->script_p = script->lastscript_p;
	return 0;
}

/*!
	\brief Skips tokens in the script until the specified string is found.

	This function reads tokens from the provided script until it encounters a token that matches the specified string. It returns 1 if the string is found and 0 if the end of the script is reached without finding the string.

	\param script Pointer to the script structure to read from
	\param string The string to search for in the script
	\return 1 if the specified string is found in the script, 0 if the end of the script is reached without finding the string
*/
int PS_SkipUntilString( script_t* script, char* string )
{
	token_t token;

	while( PS_ReadToken( script, &token ) ) {
		if( !strcmp( token.string, string ) ) {
			return 1;
		}
	}

	return 0;
}

/*!
	\brief Marks the last parsed token in the script as unread so it can be read again.

	This function sets a flag in the script structure indicating that the last token parsed is available for reading again. It is typically used to implement backtracking functionality in the parsing process.

	\param script Pointer to the script structure containing the token state
*/
void PS_UnreadLastToken( script_t* script )
{
	script->tokenavailable = 1;
}

/*!
	\brief Unreads a token back into the script's token buffer

	This function pushes a token back onto the script's token stream, making it available for subsequent reads. It copies the provided token into the script's internal token storage and marks the token as available for reading.

	\param script Pointer to the script structure containing the token stream
	\param token Pointer to the token to be pushed back onto the stream
*/
void PS_UnreadToken( script_t* script, token_t* token )
{
	memcpy( &script->token, token, sizeof( token_t ) );
	script->tokenavailable = 1;
}

/*!
	\brief Returns the next whitespace character from the script's whitespace buffer or zero if none remain.

	This function retrieves the next character from the whitespace buffer of a script. It advances the internal pointer to the next position in the buffer. If the end of the whitespace buffer is reached, it returns zero to indicate no more whitespace characters are available. This function is typically used to handle whitespace separation during token parsing or to preserve formatting when writing source code.

	\param script Pointer to the script structure containing the whitespace buffer to read from.
	\return The next whitespace character from the script's buffer, or zero if the end has been reached.
*/
char PS_NextWhiteSpaceChar( script_t* script )
{
	if( script->whitespace_p != script->endwhitespace_p ) {
		return *script->whitespace_p++;

	} else {
		return 0;
	}
}

/*!
	\brief Removes surrounding double quotes from a string in place.

	This function modifies the input string by removing double quotes if they exist at the beginning and/or end of the string. It first checks if the first character is a double quote and removes it if present. Then it checks if the last character is a double quote and removes it if present. The function operates in place, modifying the original string buffer.

	\param string Input string to process, modified in place
*/
void StripDoubleQuotes( char* string )
{
	if( *string == '\"' ) {
		strcpy( string, string + 1 );
	}

	if( string[strlen( string ) - 1] == '\"' ) {
		string[strlen( string ) - 1] = '\0';
	}
}

/*!
	\brief Removes single quotes from the beginning and end of a character string if they exist.

	This function checks if the first character of the input string is a single quote and removes it by copying the rest of the string over the beginning. It then checks if the last character of the string is a single quote and removes it by null terminating the string one position earlier. The function modifies the input string in place.

	\param string Character string to process, may be modified in place
*/
void StripSingleQuotes( char* string )
{
	if( *string == '\'' ) {
		strcpy( string, string + 1 );
	}

	if( string[strlen( string ) - 1] == '\'' ) {
		string[strlen( string ) - 1] = '\0';
	}
}

/*!
	\brief Reads a signed floating-point number from a script token stream.

	This function reads a token from the provided script and interprets it as a floating-point number. If the token is a minus sign, it sets the sign to negative and then reads the next token as the number. If the first token is not a number, it throws a script error. The function supports parsing both positive and negative floating-point values.

	\param script Pointer to the script object to read from.
	\return The parsed floating-point number with the correct sign applied.
	\throws ScriptError is thrown if the expected number token is not found.
*/
long double ReadSignedFloat( script_t* script )
{
	token_t		token;
	long double sign = 1;

	PS_ExpectAnyToken( script, &token );

	if( !strcmp( token.string, "-" ) ) {
		sign = -1;
		PS_ExpectTokenType( script, TT_NUMBER, 0, &token );

	} else if( token.type != TT_NUMBER ) {
		ScriptError( script, "expected float value, found %s\n", token.string );
	}

	return sign * token.floatvalue;
}

/*!
	\brief Reads a signed integer from a script, handling optional negative sign.

	This function reads a token from the provided script and interprets it as a signed integer. It supports optional negative sign handling, where a minus symbol is consumed and the subsequent number is treated as negative. If the token is not a valid integer, the function will trigger a script error.

	\param script Pointer to the script object to read from.
	\return The signed integer value read from the script, with the appropriate sign applied.
	\throws ScriptError is thrown if the expected integer value is not found or is invalid.
*/
signed long int ReadSignedInt( script_t* script )
{
	token_t			token;
	signed long int sign = 1;

	PS_ExpectAnyToken( script, &token );

	if( !strcmp( token.string, "-" ) ) {
		sign = -1;
		PS_ExpectTokenType( script, TT_NUMBER, TT_INTEGER, &token );

	} else if( token.type != TT_NUMBER || token.subtype == TT_FLOAT ) {
		ScriptError( script, "expected integer value, found %s\n", token.string );
	}

	return sign * token.intvalue;
}

/*!
	\brief Sets the flags for the specified script.

	This function assigns the provided flags value to the flags member of the script structure. It is used to configure the behavior or state of a script based on the flag values passed to it.

	\param script Pointer to the script structure whose flags are to be set.
	\param flags The flag values to be assigned to the script.
*/
void SetScriptFlags( script_t* script, int flags )
{
	script->flags = flags;
}

/*!
	\brief Returns the flags associated with the given script.

	This function retrieves the flag value stored in the script structure. The flags typically represent various attributes or states of the script, such as whether it is active, has been processed, or requires special handling. The returned integer value contains bit-flags that can be checked using bitwise operations.

	\param script Pointer to the script structure from which to retrieve the flags
	\return The integer value containing the flags set for the specified script
*/
int GetScriptFlags( script_t* script )
{
	return script->flags;
}

/*!
	\brief Resets the script parser state to its initial condition.

	This function resets the internal state of a script parser identified by the provided script pointer. It resets the parsing pointers to the beginning of the script buffer, clears the whitespace tracking pointers, resets the line number counters, and clears the currently stored token. This allows the script to be parsed from the beginning without any residual state from previous parses.

	\param script Pointer to the script structure to be reset
*/
void ResetScript( script_t* script )
{
	// pointer in script buffer
	script->script_p = script->buffer;
	// pointer in script buffer before reading token
	script->lastscript_p = script->buffer;
	// begin of white space
	script->whitespace_p = NULL;
	// end of white space
	script->endwhitespace_p = NULL;
	// set if there's a token available in script->token
	script->tokenavailable = 0;
	//
	script->line	 = 1;
	script->lastline = 1;
	// clear the saved token
	memset( &script->token, 0, sizeof( token_t ) );
}

/*!
	\brief Checks if the end of the script has been reached

	Returns true if the current position in the script has reached or exceeded the end position, indicating that there are no more tokens to read from the script. This function is commonly used in parsing operations to determine when a script stream has been fully consumed.

	\param script Pointer to the script structure to check
	\return Integer value indicating whether the end of the script has been reached, where a non-zero value indicates the end has been reached
*/
int EndOfScript( script_t* script )
{
	return script->script_p >= script->end_p;
}

/*!
	\brief Returns the numeric difference between a script's current line and its last line position.

	This function subtracts the script's last stored line number from the current line number to determine how many lines have been traversed since the last update. The result can be positive if
   script processing moves forward, negative if it returns to a previous line, or zero if no movement has occurred.

	\param script Pointer to a script structure whose line and lastline members are accessed.
	\return An integer representing the change in line count.
*/
int NumLinesCrossed( script_t* script )
{
	return script->line - script->lastline;
}

/*!
	\brief Advances the script pointer until a given string is found, indicating success if located.

	The function repeatedly calls a whitespace‑skipping routine and compares the next characters of the script buffer to the target string.
	If the target matches the current position it returns 1. If end of the script is reached or the whitespace function fails, it returns 0.
	The script pointer is advanced step by step until a match or termination condition.


	\param script script_t structure representing the current position in the script
	\param value null‑terminated C string to search for
	\return 1 when the string is matched, 0 when end of script is reached before finding it
*/
int ScriptSkipTo( script_t* script, char* value )
{
	int	 len;
	char firstchar;

	firstchar = *value;
	len		  = strlen( value );

	do {
		if( !PS_ReadWhiteSpace( script ) ) {
			return 0;
		}

		if( *script->script_p == firstchar ) {
			if( !strncmp( script->script_p, value, len ) ) {
				return 1;
			}
		}

		script->script_p++;
	} while( 1 );
}

#ifndef BOTLIB

/*!
	\brief Determines the size of an open file without modifying the current file position.

	The routine records the current offset of the supplied file pointer, seeks to the end to obtain the file&#39;s total length, restores the original position, and then returns the length.  The
   size is returned in bytes and is based on the file’s entire contents, regardless of the current read position.  If any of the standard I/O calls fail, the function returns the error code
   reported by ftell, typically –1.

	\param fp Pointer to an already opened FILE object
	\return Non‑negative integer representing the file length in bytes, or –1 if an error occurs
*/
int FileLength( FILE* fp )
{
	int pos;
	int end;

	pos = ftell( fp );
	fseek( fp, 0, SEEK_END );
	end = ftell( fp );
	fseek( fp, pos, SEEK_SET );

	return end;
}

#endif

script_t* LoadScriptFile( const char* filename )
{
#ifdef BOTLIB
	fileHandle_t fp;
	char		 pathname[MAX_QPATH];
#else
	FILE* fp;
#endif
	int		  length;
	void*	  buffer;
	script_t* script;

#ifdef BOTLIB

	if( strlen( basefolder ) ) {
		Com_sprintf( pathname, sizeof( pathname ), "%s/%s", basefolder, filename );

	} else {
		Com_sprintf( pathname, sizeof( pathname ), "%s", filename );
	}

	length = botimport.FS_FOpenFile( pathname, &fp, FS_READ );

	if( !fp ) {
		return NULL;
	}

#else
	fp = fopen( filename, "rb" );

	if( !fp ) {
		return NULL;
	}

	length = FileLength( fp );
#endif

	buffer = GetClearedMemory( sizeof( script_t ) + length + 1 );
	script = ( script_t* )buffer;
	memset( script, 0, sizeof( script_t ) );
	strcpy( script->filename, filename );
	script->buffer		   = ( char* )buffer + sizeof( script_t );
	script->buffer[length] = 0;
	script->length		   = length;
	// pointer in script buffer
	script->script_p = script->buffer;
	// pointer in script buffer before reading token
	script->lastscript_p = script->buffer;
	// pointer to end of script buffer
	script->end_p = &script->buffer[length];
	// set if there's a token available in script->token
	script->tokenavailable = 0;
	//
	script->line	 = 1;
	script->lastline = 1;
	//
	SetScriptPunctuations( script, NULL );
	//
#ifdef BOTLIB
	botimport.FS_Read( script->buffer, length, fp );
	botimport.FS_FCloseFile( fp );
#else

	if( fread( script->buffer, length, 1, fp ) != 1 ) {
		FreeMemory( buffer );
		script = NULL;
	}

	fclose( fp );
#endif
	//
	script->length = COM_Compress( script->buffer );

	return script;
}

/*!
	\brief Loads a script from memory into a script_t structure for parsing

	This function allocates memory for a script_t structure and copies the provided script data into it. The function sets up all necessary pointers and metadata for the script parser to work with the memory buffer. The script data is copied from the input pointer into the allocated buffer, and the script structure is initialized with appropriate values for line numbers, token tracking, and buffer management. The function is commonly used when parsing entity data or other text-based formats where the input is already in memory.

	\param ptr pointer to the script data to load
	\param length number of bytes of script data to load
	\param name name of the script, used for error reporting and debugging
	\return pointer to the newly created script_t structure containing the loaded script data
*/
script_t* LoadScriptMemory( char* ptr, int length, char* name )
{
	void*	  buffer;
	script_t* script;

	buffer = GetClearedMemory( sizeof( script_t ) + length + 1 );
	script = ( script_t* )buffer;
	memset( script, 0, sizeof( script_t ) );
	strcpy( script->filename, name );
	script->buffer		   = ( char* )buffer + sizeof( script_t );
	script->buffer[length] = 0;
	script->length		   = length;
	// pointer in script buffer
	script->script_p = script->buffer;
	// pointer in script buffer before reading token
	script->lastscript_p = script->buffer;
	// pointer to end of script buffer
	script->end_p = &script->buffer[length];
	// set if there's a token available in script->token
	script->tokenavailable = 0;
	//
	script->line	 = 1;
	script->lastline = 1;
	//
	SetScriptPunctuations( script, NULL );
	//
	memcpy( script->buffer, ptr, length );
	//
	return script;
}

/*!
	\brief Frees the memory allocated for a script object and its punctuation table if applicable

	This function releases the memory occupied by a script_t structure. If the PUNCTABLE compiler flag is enabled, it first frees the punctuation table associated with the script before freeing the script structure itself. The function assumes the script pointer is valid and has been previously allocated with memory allocation functions

	\param script Pointer to the script structure to be freed
*/
void FreeScript( script_t* script )
{
#ifdef PUNCTABLE

	if( script->punctuationtable ) {
		FreeMemory( script->punctuationtable );
	}

#endif // PUNCTABLE
	FreeMemory( script );
}

void PS_SetBaseFolder( char* path )
{
#ifdef BSPC
	sprintf( basefolder, path );
#else
	Com_sprintf( basefolder, sizeof( basefolder ), path );
#endif
}
