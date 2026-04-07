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
 * name:		l_script.h
 *
 * desc:		lexicographical parser
 *
 *
 *****************************************************************************/

// Ridah, can't get it to compile without this
#ifndef QDECL

	// for windows fastcall option
	#define QDECL
	//======================= WIN32 DEFINES =================================
	#ifdef WIN32
		#undef QDECL
		#define QDECL __cdecl
	#endif
#endif
// done.

// undef if binary numbers of the form 0b... or 0B... are not allowed
#define BINARYNUMBERS
// undef if not using the token.intvalue and token.floatvalue
#define NUMBERVALUE
// use dollar sign also as punctuation
#define DOLLAR

// maximum token length
#define MAX_TOKEN 1024
// maximum path length
#ifndef _MAX_PATH
	// TTimo: used to be MAX_QPATH, which is the game filesystem max len, and not the OS max len
	#define _MAX_PATH 1024
#endif

// script flags
#define SCFL_NOERRORS			 0x0001
#define SCFL_NOWARNINGS			 0x0002
#define SCFL_NOSTRINGWHITESPACES 0x0004
#define SCFL_NOSTRINGESCAPECHARS 0x0008
#define SCFL_PRIMITIVE			 0x0010
#define SCFL_NOBINARYNUMBERS	 0x0020
#define SCFL_NONUMBERVALUES		 0x0040

// token types
#define TT_STRING				 1 // string
#define TT_LITERAL				 2 // literal
#define TT_NUMBER				 3 // number
#define TT_NAME					 4 // name
#define TT_PUNCTUATION			 5 // punctuation

// string sub type
//---------------
//		the length of the string
// literal sub type
//----------------
//		the ASCII code of the literal
// number sub type
//---------------
#define TT_DECIMAL				 0x0008 // decimal number
#define TT_HEX					 0x0100 // hexadecimal number
#define TT_OCTAL				 0x0200 // octal number
#ifdef BINARYNUMBERS
	#define TT_BINARY 0x0400	  // binary number
#endif							  // BINARYNUMBERS
#define TT_FLOAT		   0x0800 // floating point number
#define TT_INTEGER		   0x1000 // integer number
#define TT_LONG			   0x2000 // long number
#define TT_UNSIGNED		   0x4000 // unsigned number
// punctuation sub type
//--------------------
#define P_RSHIFT_ASSIGN	   1
#define P_LSHIFT_ASSIGN	   2
#define P_PARMS			   3
#define P_PRECOMPMERGE	   4

#define P_LOGIC_AND		   5
#define P_LOGIC_OR		   6
#define P_LOGIC_GEQ		   7
#define P_LOGIC_LEQ		   8
#define P_LOGIC_EQ		   9
#define P_LOGIC_UNEQ	   10

#define P_MUL_ASSIGN	   11
#define P_DIV_ASSIGN	   12
#define P_MOD_ASSIGN	   13
#define P_ADD_ASSIGN	   14
#define P_SUB_ASSIGN	   15
#define P_INC			   16
#define P_DEC			   17

#define P_BIN_AND_ASSIGN   18
#define P_BIN_OR_ASSIGN	   19
#define P_BIN_XOR_ASSIGN   20
#define P_RSHIFT		   21
#define P_LSHIFT		   22

#define P_POINTERREF	   23
#define P_CPP1			   24
#define P_CPP2			   25
#define P_MUL			   26
#define P_DIV			   27
#define P_MOD			   28
#define P_ADD			   29
#define P_SUB			   30
#define P_ASSIGN		   31

#define P_BIN_AND		   32
#define P_BIN_OR		   33
#define P_BIN_XOR		   34
#define P_BIN_NOT		   35

#define P_LOGIC_NOT		   36
#define P_LOGIC_GREATER	   37
#define P_LOGIC_LESS	   38

#define P_REF			   39
#define P_COMMA			   40
#define P_SEMICOLON		   41
#define P_COLON			   42
#define P_QUESTIONMARK	   43

#define P_PARENTHESESOPEN  44
#define P_PARENTHESESCLOSE 45
#define P_BRACEOPEN		   46
#define P_BRACECLOSE	   47
#define P_SQBRACKETOPEN	   48
#define P_SQBRACKETCLOSE   49
#define P_BACKSLASH		   50

#define P_PRECOMP		   51
#define P_DOLLAR		   52
// name sub type
//-------------
//		the length of the name

// punctuation
typedef struct punctuation_s {
	char*				  p;	// punctuation character(s)
	int					  n;	// punctuation indication
	struct punctuation_s* next; // next punctuation
} punctuation_t;

// token
typedef struct token_s {
	char string[MAX_TOKEN]; // available token
	int	 type;				// last read token type
	int	 subtype;			// last read token sub type
#ifdef NUMBERVALUE
	unsigned long int intvalue;		 // integer value
	long double		  floatvalue;	 // floating point value
#endif								 // NUMBERVALUE
	char*			whitespace_p;	 // start of white space before token
	char*			endwhitespace_p; // start of white space before token
	int				line;			 // line the token was on
	int				linescrossed;	 // lines crossed in white space
	struct token_s* next;			 // next token in chain
} token_t;

// script file
typedef struct script_s {
	char			 filename[_MAX_PATH]; // file name of the script
	char*			 buffer;			  // buffer containing the script
	char*			 script_p;			  // current pointer in the script
	char*			 end_p;				  // pointer to the end of the script
	char*			 lastscript_p;		  // script pointer before reading token
	char*			 whitespace_p;		  // begin of the white space
	char*			 endwhitespace_p;	  // end of the white space
	int				 length;			  // length of the script in bytes
	int				 line;				  // current line in script
	int				 lastline;			  // line before reading token
	int				 tokenavailable;	  // set by UnreadLastToken
	int				 flags;				  // several script flags
	punctuation_t*	 punctuations;		  // the punctuations used in the script
	punctuation_t**	 punctuationtable;
	token_t			 token; // available token
	struct script_s* next;	// next script in a chain
} script_t;

/*!
	\brief Reads the next token from a script and fills the provided token structure.

	The routine first checks if a token has been pushed back via UnreadToken; if so it restores that token and returns success.  It then records the current script position and line number, clears the
   token buffer and records whitespace positions.  The function consumes leading whitespace, records the line information for the new token, and then determines the token kind.  It supports quoted
   strings, literal strings within single quotes, numeric constants, primitive scripts (when SCFL_PRIMITIVE is set), identifier names, and punctuation.  When a primitive or name token is read, it
   calls the appropriate helper.  If the token cannot be parsed, an error may be reported via ScriptError and the function returns failure.  On success the freshly read token is copied into the
   script's token slot and the function returns a positive value.

	The function uses several helpers: PS_ReadWhiteSpace, PS_ReadString, PS_ReadNumber, PS_ReadPrimitive, PS_ReadName, PS_ReadPunctuation.  The token structure receives fields such as whitespace
   start/end positions, line number, and lines crossed.  The script structure tracks last script and line for later token re‑reads.

	It never throws exceptions – any fatal condition is reported through ScriptError, which typically terminates parsing.


	\param script the script from which a token should be read
	\param token the token structure to populate with the parsed token
	\return 1 if a token was successfully read; 0 if end of script or if parsing fails
*/
int				PS_ReadToken( script_t* script, token_t* token );

/*!
	\brief Verifies that the next token in the script matches a specified string.

	The function attempts to read the next token from the provided script. If a token cannot be read, an error message is issued and the function returns 0. If a token is read but its string value
   does not match the expected string, another error message is emitted and the function returns 0. When the token string matches the expected value, the function returns 1.

	\param script Pointer to the script being parsed
	\param string String that the next token must equal
	\return 1 if the expected token was found, 0 otherwise
*/
int				PS_ExpectTokenString( script_t* script, char* string );

/*!
	\brief Reads a token from the script and verifies it matches a specified type and subtype, reporting an error if it does not.

	The function first attempts to read the next token from the script. If reading fails, an error is reported. It then checks whether the token’s type matches the expected type; if it does not, a
   descriptive error message is generated using human‑readable names for the types. For number tokens the function additionally checks a subtype mask, constructing a string that describes the expected
   numeric format (decimal, hex, octal, binary, long, unsigned, float, or integer). For punctuation tokens the subtype must match exactly; a negative subtype triggers an error. In all error cases the
   function reports via ScriptError and returns 0. When the token satisfies all expectations, the function returns 1.

	\param script pointer to the script structure from which to read the token
	\param type integer constant indicating the expected main token type (e.g., TT_STRING, TT_LITERAL, TT_NUMBER, etc.)
	\param subtype integer bitmask or value specifying expected subtype details (used for number formats or punctuation)
	\param token pointer to a token_t object that will receive the read token data or be examined for type/subtype match
	\return 1 if the read token matches the expected type and subtype, 0 otherwise
*/
int				PS_ExpectTokenType( script_t* script, int type, int subtype, token_t* token );

/*!
	\brief Reads the next token from the script and reports an error if none can be read.

	The function attempts to read the next token from the provided script into the supplied token structure. If the read fails, a script error is generated and the function returns 0, indicating
   failure; otherwise it returns 1, indicating success.

	\param script the script from which to read a token
	\param token structure to receive the read token
	\return 1 when a token was successfully read, 0 otherwise
*/
int				PS_ExpectAnyToken( script_t* script, token_t* token );

/*!
	\brief Verify that the next token in the script matches a given string and advance the script pointer only if it does.

	This function attempts to read the next token from the script. If a token is available and its string field equals the supplied "string", the function returns 1 and the script pointer remains
   advanced. If no token is available or the token does not match, the script pointer is reset to its previous position stored in script->lastscript_p and the function returns 0.

	\param script pointer to a script_t structure that contains the current parsing context and positions
	\param string C string value that the next token is compared against
	\return 1 when the next token exactly matches the supplied string; 0 otherwise
*/
int				PS_CheckTokenString( script_t* script, char* string );

/*!
	\brief Checks the next script token against a given type and subtype, optionally consuming it when matching.

	The function attempts to read the next token from the supplied script. If the token’s type equals the expected type and all bits in the expected subtype mask match those of the token’s subtype
   mask, the token is copied into the caller-provided token structure and the function returns a non‑zero value.  If the read fails or the token does not match, the script reading position is restored
   to the previous location (so the token is not consumed) and zero is returned.  The caller should provide a valid token_t pointer for the output.

	\param script Handle to the script parser context from which to read the token.
	\param type The token type the function should accept.
	\param subtype A bitmask of subtype flags that the token must contain; all bits must be present in the token’s subtype.
	\param token Pointer to a token_t structure to receive the matched token if the function succeeds.
	\return 1 when a token matching the requested type and subtype is found and returned; 0 otherwise.
*/
int				PS_CheckTokenType( script_t* script, int type, int subtype, token_t* token );

/*!
	\brief Searches a script for a specific token string and indicates whether it was found.

	The function repeatedly calls PS_ReadToken to extract tokens sequentially from the supplied script. Each extracted token's string is compared with the target string. If a match is encountered the
   function returns 1 immediately. If the end of the script is reached without encountering the target, it returns 0. The tokens read during the search are consumed and not available for subsequent
   parsing.

	This helper is typically used when a script reader needs to skip over a section of a script until a particular keyword appears.

	TODO: clarify behavior if script or string pointers are null.

	\param script Pointer to a script_t structure from which tokens are sequentially read.
	\param string C-string that the function searches for among token strings.
	\return 1 if the target string is encountered before the script ends; 0 otherwise.
*/
int				PS_SkipUntilString( script_t* script, char* string );

/*!
	\brief Marks the last token of a script as unread so it can be read again.

	Sets the script's tokenavailable flag to true, indicating that the most recent token read is still available and should be returned on the next parse request.

	\param script Script whose last read token will be marked as available again.
*/
void			PS_UnreadLastToken( script_t* script );

/*!
	\brief Pushes a token back into the script so it will be read again next time.

	Copies the supplied token into the script's internal token storage and marks that a token is available. The next call to get a token from the script will return this stored token rather than
   reading from the source.

	\param script script structure that provides a token buffer and availability flag.
	\param token token to be unread into the script.
*/
void			PS_UnreadToken( script_t* script, token_t* token );

/*!
	\brief Retrieves the next character from a script's whitespace buffer, returning zero when no characters remain.

	The function inspects the script structure's whitespace pointers. If the current pointer has not yet reached the end marker, it returns the character at the current position and advances the
   pointer. Once all whitespace characters are consumed, it returns the null character (0). This helps callers iterate over whitespace while processing a script.

	\param script Pointer to a script_t structure containing whitespace pointers and boundaries.
	\return The next whitespace character, or zero if the buffer is exhausted.
*/
char			PS_NextWhiteSpaceChar( script_t* script );

/*!
	\brief Removes leading and trailing double quotes from a string in place.

	The function examines the first character of the supplied character array; if it is a double quote, the string is shifted left by one position by copying from the second character onwards. It then
   checks the last character after the potential shift; if this character is a double quote, it replaces it with a null terminator to truncate the string. The operation modifies the original array
   directly and does not allocate additional memory.

	\param string Pointer to a NUL‑terminated character array that will have any leading and trailing double quotes removed; the array is modified in place.
*/
void			StripDoubleQuotes( char* string );

/*!
	\brief Removes leading and trailing single quotes from a string.

	The function examines the first character of the string and, if it is a single quote, shifts the entire string left by one position. It then checks the last character after this possible shift,
   and if that character is a single quote it terminates the string there. The input must be a mutable, null‑terminated C string; behavior is undefined if the string is empty or null.

	\param string The character array from which single quotes should be stripped.
*/
void			StripSingleQuotes( char* string );

/*!
	\brief Read a signed integer value from a script token stream

	This function peeks at the next token in the script. If the token string is a minus sign, it interprets the following token as a positive number and multiplies it by –1. For all other tokens, it
   verifies that the token represents an integer and rejects floating point numbers. The signed integer value is then returned.

	If the token is not a valid integer representation, a script error is reported.

	\param script pointer to the script token stream from which to read
	\return the signed long integer value extracted from the script
*/
signed long int ReadSignedInt( script_t* script );

/*!
	\brief Reads a signed floating point number from the script stream.

	It first consumes any token from the script. If the token string equals "-", it records a negative sign and consumes the next token, which must be a numeric value. If the token is not a numeric
   value when expected, it reports an error via ScriptError. Finally, it returns the numeric value multiplied by the sign.

	\param script pointer to the current script context from which tokens are extracted
	\return a long double representing the signed floating point value read from the script
*/
long double		ReadSignedFloat( script_t* script );

/*!
	\brief Stores a punctuation lookup table for a script and optionally creates it when compile‑time support is enabled.

	When the optional compile‑time symbol PUNCTABLE is defined, the function creates a punctuation lookup table using the given pointer or a default array via PS_CreatePunctuationTable. In all cases
   it saves the pointer—either the supplied table or a default one—to the script structure’s punctuations member. Passing a null pointer causes the default C/C++ punctuation set to be used.

	\param p Pointer to a punctuation lookup table; if null, the default table is used.
	\param script The script data structure that will receive the punctuation table.
*/
void			SetScriptPunctuations( script_t* script, punctuation_t* p );

/*!
	\brief Set the flag bits on a script object.

	Writes the provided flag value into the script's flags field, replacing any previous value.

	\param script pointer to the script whose flags are to be set
	\param flags integer specifying the new flag bits
*/
void			SetScriptFlags( script_t* script, int flags );

/*!
	\brief Retrieves the flags associated with a script.

	The function returns the integer value stored in the script's flags field, which represents a bitmask of script configuration options or states.

	\param script Pointer to the script object from which the flags value is fetched.
	\return An integer bitmask containing the script flags.
*/
int				GetScriptFlags( script_t* script );

/*!
	\brief Resets a script to its initial state, clearing all pointers, token information, and line counters.

	The function restores a script_t instance to a clean start. It sets the current reading pointer to the beginning of the buffer and resets the previous script pointer to the same position.
   White‑space pointers are cleared, the token‑available flag is reset, and the line numbers are set to one. Finally the token structure is zeroed out to ensure no stale data remains.

	\param script The script structure to reset, whose buffer must already point to the script data.
*/
void			ResetScript( script_t* script );

/*!
	\brief Determine if a script has reached its end position.

	The function compares the script's current pointer to its end pointer. If the current pointer is equal to or beyond the end pointer, a non‑zero value is returned, indicating that the script has
   been fully processed. Otherwise zero is returned. The return value is typically used as a boolean in calling code.

	\param script the script to test
	\return non‑zero if the current position is at or beyond the script's end, zero otherwise
*/
int				EndOfScript( script_t* script );

/*!
	\brief Retrieves the punctuation string associated with the given number from a script

	The function iterates over the script's punctuation table, checking each entry's numeric identifier for a match with the supplied number. When a match is found, it returns the pointer to the
   corresponding string. If no matching entry exists, it returns the literal string "unkown punctuation".

	\param script script containing the punctuation array
	\param num numeric identifier of the desired punctuation
	\return pointer to the punctuation string, or a fallback string if not found
*/
char*			PunctuationFromNum( script_t* script, int num );

/*!
	\brief Loads a script file into a script_t structure and returns a pointer to it.

	The function opens the specified file, reads its contents into a newly allocated buffer, builds a script_t structure that points to this buffer, compresses the script data, and initializes various
   bookkeeping fields. The implementation differs for a bot library build where a platform‑specific file system API is used; otherwise standard C FILE handling is used. If the file cannot be opened or
   read successfully, the function returns NULL and frees any allocated memory.

	\param filename the path to the script file to read
	\return a non‑null pointer to the initialized script_t object, or NULL if the file could not be opened or read
*/
script_t*		LoadScriptFile( const char* filename );

/*!
	\brief Create a script object from raw script data in memory

	The function allocates a single block large enough for a script_t structure, the supplied data, and an extra null terminator. It then initializes all fields of the script structure—including file
   name, buffer pointers, parsing state, and line counters—before copying the source bytes into the buffer. The initialized script pointer is returned.

	\param ptr pointer to the raw script data
	\param length number of bytes to copy from ptr
	\param name file name to assign to the script object
	\return pointer to the newly allocated and initialized script_t instance
*/
script_t*		LoadScriptMemory( char* ptr, int length, char* name );

/*!
	\brief Releases the memory owned by a script object, including its optional punctuation table if compiled with the PUNCTABLE flag.

	When invoked, the function first checks whether the script has an allocated punctuation table. If so, it deallocates that table using FreeMemory. It then deallocates the script structure itself.
   The caller must ensure that the script pointer is valid and not NULL, and must not use the pointer after freeing.

	\param script pointer to the script_t object to be released
*/
void			FreeScript( script_t* script );

/*!
	\brief Sets the base folder path used for loading files

	Updates the global base folder string to the specified path, which is then used by file loading functions.  On normal builds the global buffer is filled with a standard string formatter that
   limits writing to the buffer size.  When compiled for the BSPC tool the function falls back to the standard C library formatter.

	\param path The path string to use as the new base folder
*/
void			PS_SetBaseFolder( char* path );

/*!
	\brief Reports a formatted script error that includes the script's filename and line number.

	If the SCFL_NOERRORS flag is set on the script, the error is suppressed.
	The function formats the message using the supplied printf‑style format string and any additional arguments into a 1024‑byte buffer.
	The resulting text is then emitted through the appropriate logging mechanism, which may differ depending on compile‑time flags such as BOTLIB, MEQCC, or BSPC.

	\param script a pointer to the script context, providing filename, line, and flags information
	\param str the printf‑style format string
	\param … variable arguments supplied to the format string
*/
void QDECL		ScriptError( script_t* script, char* str, ... );

/*!
	\brief Prints a formatted warning message for a script, including its filename and line number, unless warnings are suppressed.

	If script->flags indicates SCFL_NOWARNINGS, the warning is ignored. Otherwise, the function formats the supplied message using the variadic arguments into a 1024 character buffer. Depending on
   compile‑time options, the warning is written to the bot import logger, stdout, or a BSP log. The function never throws and simply returns void.

	\param ... additional arguments matched to the format string
	\param script the script context containing file name, current line, and flags
	\param str the printf‑style format string
*/
void QDECL		ScriptWarning( script_t* script, char* str, ... );
