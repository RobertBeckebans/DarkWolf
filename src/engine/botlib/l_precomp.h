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
	\file engine/botlib/l_precomp.h
	\brief Defines the core pre‑processor and tokenizer used by the Return to Castle Wolfenstein bot compiler, enabling bots to compile simple scripting languages that support macros, conditional compilation, and file inclusion.
	\note doxygenix: sha256=9d84e86429bb65d2f856a2da135867068587d4b8501942efa7f31bf04c49dfe2

	\par File Purpose
	- Defines the core pre‑processor and tokenizer used by the Return to Castle Wolfenstein bot compiler, enabling bots to compile simple scripting languages that support macros, conditional compilation, and file inclusion.

	\par Core Responsibilities
	- Provide a tokenizing engine for bot scripts with a source_t abstraction that abstracts file layout and include-path resolution.
	- Implement preprocessor directives: #include, #define, #undef, #if, #ifdef, #else, #elif, #endif.
	- Expand macros, supporting parameters and adjacent string concatenation.
	- Offer non‑consuming token queries (PC_CheckTokenString, PC_CheckTokenType) and typed token reads (PC_ExpectTokenType, PC_ExpectTokenString).
	- Handle errors and warnings with file and line context via SourceError and SourceWarning.
	- Expose a handle table (PC_LoadSourceHandle, PC_FreeSourceHandle, PC_ReadTokenHandle, PC_SourceFileAndLine) so external tools can refer to scripts by index.

	\par Key Types and Functions
	- define_t — structure that holds a macro definition, its name, parameters, token list, and hash chain linkage.
	- define_t::name — null‑terminated string of the macro identifier.
	- define_t::flags — bitfield controlling compile‑time behaviour (e.g., DEFINE_FIXED).
	- define_t::builtin — flag indicating a built‑in macro (e.g., __LINE__).
	- define_t::numparms — number of macro parameters.
	- define_t::parms — list of parameter names (token_t*).
	- define_t::tokens — tokenized macro body (token_t*).
	- indent_t — represents a nested pre‑processor conditional; tracks type, skip flag, script, and next indent.
	- source_t — driver structure for a script file; holds filename, include path, punctuation table, script stack, token buffer, macro table, indentation stack, skip flag, and last read token.
	- PC_ReadToken(source_t*, token_t*) — obtains the next logical token, performing macro expansion and skip handling.
	- PC_ExpectTokenString(source_t*, char*) — reads a token and asserts that its string matches the supplied value.
	- PC_ExpectTokenType(source_t*, int type, int subtype, token_t*) — reads a token and verifies its type and subtype bitmask, reporting detailed errors.
	- PC_ExpectAnyToken(source_t*, token_t*) — reads a token without type checks, returning failure on EOF.
	- PC_CheckTokenString(source_t*, char*) — peeks at the next token, returning true if it matches the string; otherwise pushes the token back.
	- PC_CheckTokenType(source_t*, int, int, token_t*) — peeks at the next token, returning true if both type and subtype match, else restores it.
	- PC_SkipUntilString(source_t*, char*) — advances through tokens until a token with the specified string appears, discarding intervening tokens.
	- PC_UnreadLastToken(source_t*) — restores the most recently read token so that it will be returned again.
	- PC_UnreadToken(source_t*, token_t*) — pushes an arbitrary token back onto the source’s token stream.
	- PC_ReadLine(source_t*, token_t*) — consumes tokens until a non‑backslash arrives, ensuring the token does not cross into a new logical line.
	- PC_WhiteSpaceBeforeToken(token_t*) — indicates whether a token has preceding whitespace in its source buffer.
	- PC_AddDefine(source_t*, char*) — parses a #define string and inserts the resulting define_t into the source’s macro table.
	- PC_AddGlobalDefine(char*) — adds a define that is visible to all subsequently loaded sources.
	- PC_RemoveGlobalDefine(char*) — removes a previously added global macro by name.
	- PC_RemoveAllGlobalDefines() — clears the global define list.
	- PC_AddBuiltinDefines(source_t*) — injects built‑in macros like __LINE__, __FILE__, __DATE__, __TIME__ into a source.
	- PC_SetIncludePath(source_t*, char*) — assigns a base path for #include directives, ensuring it terminates with a platform separator.
	- PC_SetPunctuations(source_t*, punctuation_t*) — assigns the punctuation table used by the tokenizer.
	- PC_SetBaseFolder(char*) — updates the global base directory used for file loading.
	- LoadSourceFile(const char*) — constructs a source_t from a file on disk; initializes internal structures and applies global defines.
	- LoadSourceMemory(char*, int, char*) — constructs a source_t from a memory buffer.
	- FreeSource(source_t*) — deallocates all data structures owned by a source_t.
	- SourceError(source_t*, char*, ...) — logs a formatted error with file and line context.
	- SourceWarning(source_t*, char*, ...) — logs a formatted warning similarly.
	- PC_LoadSourceHandle(const char*) — stores a new source_t into the global handle table and returns its index.
	- PC_FreeSourceHandle(int) — frees a source referenced by a handle, returning success/failure.
	- PC_ReadTokenHandle(int, pc_token_s*) — reads a token from a handle, copying it into the provided BSPC pc_token_s structure.
	- PC_SourceFileAndLine(int, char*, int*) — returns file name and current line number for a handle.
	- PC_CheckOpenSourceHandles() — emits diagnostics for any handles still open at shutdown.

	\par Control Flow
	- The pre‑compiler constructs a `source_t` that holds the file name, include path, punctuation table, script stack, token buffer, macro table, indentation stack, and a skip flag.
	- PC_ReadToken() loops: it calls PC_ReadSourceToken() to get the next raw token; if the token is a preprocessor directive (# or $) it delegates to the appropriate handler such as PC_AddDefine or PC_SetIncludePath.
	- If a NAME token matches an entry in the define table, the macro is expanded by inserting its token list into the current token stream via PC_UnreadToken/PC_ReadLine, allowing nested expansions.
	- Conditional directives push an indent_t onto the stack; the skip flag indicates whether a branch should be ignored until the matching #endif.
	- PC_ReadLine() consumes backslash line‑continuation tokens and rejects tokens that span a new logical line.
	- When a source is closed (FreeSource or PC_FreeSourceHandle) the source’s script, token, define, and indent linked lists are traversed and freed.

	\par Dependencies
	- relies on token_t, script_t, and punctuation_t types defined elsewhere in botlib;
	- uses the token‑heap allocator provided by LoadScriptFile, LoadSourceFile, and LoadSourceMemory;
	- depends on build macros such as MAX_QPATH and MAX_TOKENLENGTH, usually defined in q_shared.h;
	- provides a stub pc_token_t for BSPC when q_shared.h is not included.

	\par How It Fits
	- engine/botlib/l_precomp.h is the foundation of the bot scripting subsystem; higher‑level parser and compiler modules include this header to obtain token streams and macro facilities.
	- Its API is used by the bot compilation pipeline to load scripts, read tokens sequentially, and report errors, while the global source handle table allows multiple scripts to be open concurrently.
	- By encapsulating tokenisation, macro expansion, and file inclusion, it keeps the rest of the engine modular and isolates bot script syntax from core game logic.
*/

/*****************************************************************************
 * name:		l_precomp.h
 *
 * desc:		pre compiler
 *
 *
 *****************************************************************************/

#ifndef _MAX_PATH
	#define MAX_PATH MAX_QPATH
#endif

#ifndef PATH_SEPERATORSTR
	#if defined( WIN32 ) | defined( _WIN32 ) | defined( __NT__ ) | defined( __WINDOWS__ ) | defined( __WINDOWS_386__ )
		#define PATHSEPERATOR_STR "\\"
	#else
		#define PATHSEPERATOR_STR "/"
	#endif
#endif
#ifndef PATH_SEPERATORCHAR
	#if defined( WIN32 ) | defined( _WIN32 ) | defined( __NT__ ) | defined( __WINDOWS__ ) | defined( __WINDOWS_386__ )
		#define PATHSEPERATOR_CHAR '\\'
	#else
		#define PATHSEPERATOR_CHAR '/'
	#endif
#endif

#define DEFINE_FIXED  0x0001

#define BUILTIN_LINE  1
#define BUILTIN_FILE  2
#define BUILTIN_DATE  3
#define BUILTIN_TIME  4
#define BUILTIN_STDC  5

#define INDENT_IF	  0x0001
#define INDENT_ELSE	  0x0002
#define INDENT_ELIF	  0x0004
#define INDENT_IFDEF  0x0008
#define INDENT_IFNDEF 0x0010

// macro definitions
typedef struct define_s {
	char*			 name;	   // define name
	int				 flags;	   // define flags
	int				 builtin;  // > 0 if builtin define
	int				 numparms; // number of define parameters
	token_t*		 parms;	   // define parameters
	token_t*		 tokens;   // macro tokens (possibly containing parm tokens)
	struct define_s* next;	   // next defined macro in a list
	struct define_s* hashnext; // next define in the hash chain
} define_t;

// indents
// used for conditional compilation directives:
// #if, #else, #elif, #ifdef, #ifndef
typedef struct indent_s {
	int				 type;	 // indent type
	int				 skip;	 // true if skipping current indent
	script_t*		 script; // script the indent was in
	struct indent_s* next;	 // next indent on the indent stack
} indent_t;

// source file
typedef struct source_s {
	char		   filename[_MAX_PATH];	   // file name of the script
	char		   includepath[_MAX_PATH]; // path to include files
	punctuation_t* punctuations;		   // punctuations to use
	script_t*	   scriptstack;			   // stack with scripts of the source
	token_t*	   tokens;				   // tokens to read first
	define_t*	   defines;				   // list with macro definitions
	define_t**	   definehash;			   // hash chain with defines
	indent_t*	   indentstack;			   // stack with indents
	int			   skip;				   // > 0 if skipping conditional code
	token_t		   token;				   // last read token
} source_t;

/*!
	\brief Reads the next token from a source stream, handling preprocessor directives, string concatenation, and macro expansion

	The function first obtains a source token using PC_ReadSourceToken. If the token is a preprocessor directive (# or $), it delegates to the appropriate handler and restarts the loop. Adjacent
   string tokens are concatenated recursively, ensuring the resultant string does not exceed MAX_TOKEN. When a name token is detected it checks for a macro definition and expands it into the source
   stream before continuing. Tokens read while the source's skip flag is set are ignored, supporting conditional compilation. The final token is stored in the supplied token_t structure and also
   copied into source->token for potential unreading. Success is indicated by returning an integer true value; failure returns false.

	\param source pointer to the source structure from which to read tokens
	\param token pointer to a token_t structure that will receive the read token
	\return an integer true value (1) if a token was successfully read; otherwise an integer false value (0)
*/
int		   PC_ReadToken( source_t* source, token_t* token );

/*!
	\brief Checks that the next token from the source matches the specified string.

	The function reads the next token from the given source. If no token can be read, it reports an error indicating the expected string could not be found and returns false. If a token is read but
   its string differs from the expected one, it reports an error showing the expected and actual tokens, then returns false. On a successful match, it returns true.

	\param source pointer to the source from which to read the next token
	\param string the token string that the next token is expected to equal
	\return non-zero value (true) if the next token equals the expected string; zero (false) otherwise
*/
int		   PC_ExpectTokenString( source_t* source, char* string );

/*!
	\brief Reads the next token from the source and verifies it matches a specified type and subtype.

	The function attempts to read a token from the supplied source. If a token cannot be read, it reports an error and returns false. It then verifies that the token's type matches the expected type,
   reporting an informative message if it does not. When the expected type is TT_NUMBER, the function additionally checks that the token's subtype bits include all requested subtype flags, and builds
   a detailed description of the expected numeric format (decimal, hex, octal, binary, long, unsigned, float, integer). For TT_PUNCTUATION, it matches the exact subtype value.  Any mismatch results in
   an error message and a false return value. If the token satisfies all conditions, the function returns true.

	\param source source to read the next token from
	\param type expected token type constant (e.g., TT_NUMBER, TT_STRING)
	\param subtype mask of expected subtype flags to enforce
	\param token token structure that will receive the read token and provide details
	\return non‑zero if the token matches the expected type and subtype, zero otherwise
*/
int		   PC_ExpectTokenType( source_t* source, int type, int subtype, token_t* token );

/*!
	\brief Reads the next token from the source and reports an error if the read fails.

	The function calls PC_ReadToken to fetch the next token. If the read is unsuccessful, a source error is reported with the message "couldn't read expected token" and the function returns zero. On
   success the token is stored in the supplied token_t structure and the function returns non‑zero.

	This routine is used by higher‑level parsing code that expects a token to be present but does not enforce any specific type.


	\param source pointer to the source from which to read the next token
	\param token pointer to the token structure to be filled with the read token
	\return Non‑zero value indicating success, or zero indicating failure to read a token.
*/
int		   PC_ExpectAnyToken( source_t* source, token_t* token );

/*!
	\brief Checks if the next token from the source matches the given string, returning true if it does.

	The function reads a token from the provided source stream. If that token equals the specified string, it returns true. If it does not match, the token is pushed back onto the source stream and
   false is returned. No output is produced except the boolean result.

	No side effects other than potentially unread the token.

	\param source The source stream to read a token from.
	\param string The string to compare against the read token.
	\return true (qtrue) if the next token equals string; false (qfalse) otherwise.
*/
int		   PC_CheckTokenString( source_t* source, char* string );

/*!
	\brief Checks the next token from the source for a matching type and subtype, returning the token if found.

	The function reads a token from the source stream. If the token's type matches the supplied type and its subtype matches the supplied subtype bitmask, it copies the token into the caller's token
   structure and returns true. If the token does not match, it is returned to the source stream and the function returns false.

	\param source Pointer to the source from which tokens are read.
	\param type Expected token type to match.
	\param subtype Bitmask of expected token subtypes to match against.
	\param token Output parameter that receives the matched token when the function returns true.
	\return True if a token matching the specified type and subtype is read; false otherwise.
*/
int		   PC_CheckTokenType( source_t* source, int type, int subtype, token_t* token );

/*!
	\brief Skips through tokens in the source until a token matching the specified string is found or the input ends.

	The function repeatedly reads the next token from the provided source. After each token is read, it compares the token's string value to the supplied target string. If they match, the function
   returns a success indicator. If the end of the source is reached without finding the target, the function returns a failure indicator.

	\param source A pointer to the source from which tokens are read. The function consumes tokens from this source until the target is found or the source is exhausted.
	\param string The string value that the function is searching for among the tokens read from the source.
	\return qtrue (1) if the specified string is encountered; qfalse (0) otherwise.
*/
int		   PC_SkipUntilString( source_t* source, char* string );

/*!
	\brief Reverts the last token read from the given script source.

	The function invokes PC_UnreadSourceToken on the source’s current token, effectively pushing that token back onto the input stream so it will be returned again on the next read. It assumes that
   source->token holds the most recently read token; if the token field is empty or uninitialized the effect is unspecified. TODO: clarify error handling and state changes when the token stream is at
   its beginning.

	\param source Pointer to the script source structure whose last token should be unread.
*/
void	   PC_UnreadLastToken( source_t* source );

/*!
	\brief Makes the given token available for the next fetch from the source.

	This convenience wrapper forwards its arguments to PC_UnreadSourceToken, placing the token at the front of the source's token stream so that subsequent read operations will return it again.

	\param source Pointer to the source from which the token was read.
	\param token Pointer to the token to place back into the source.
*/
void	   PC_UnreadToken( source_t* source, token_t* token );

/*!
	\brief Reads a token from the source, ignoring line continuations and reporting failure if the token is on a new line

	The function repeatedly reads tokens from the source until it encounters a token whose string is not a backslash ("\\"). A backslash token is treated as a line‑continuation marker and is skipped.
   For each token read, the function checks whether the token spans multiple lines by comparing its “linescrossed” count with a running flag. If a token comes from a line beyond the current logical
   line, it is unread and the function returns false. If the read succeeds and the token lies on the same logical line (after discarding any line‑continuation backslashes), the function returns true
   and the passed token structure is populated with the next non‑backslash token.

	\param source the source structure from which tokens are read
	\param token the token_t structure to receive the extracted token
	\return true (1) on success; false (0) if end‑of‑file is reached or a token from a new logical line is encountered
*/
int		   PC_ReadLine( source_t* source, token_t* token );

/*!
	\brief Determines whether a token is preceded by whitespace characters.

	The function compares the token’s ending whitespace pointer to its starting whitespace pointer. If the difference is greater than zero, whitespace was found directly before the token, and the
   function returns a non‑zero value. Otherwise it returns zero.

	\param token Token to inspect. The token structure contains pointers indicating where any leading whitespace ends (endwhitespace_p) and where it starts (whitespace_p).
	\return A non‑zero integer if the token has whitespace in front; zero if not.
*/
int		   PC_WhiteSpaceBeforeToken( token_t* token );

/*!
	\brief Inserts a new define parsed from the supplied string into the source's collection, returning a success flag.

	The function first parses the string into a define_t structure via PC_DefineFromString. If parsing fails, it signals failure by returning qfalse. Depending on whether DEFINEHASHING is enabled, the
   parsed define is added either to the source's hash table or to the front of a linked list. On successful insertion, it returns qtrue. This function does not throw exceptions.

	\param source The source_t object whose define collection is updated.
	\param string A C string containing the define declaration to be parsed and added.
	\return qtrue if the define was successfully created and stored; qfalse otherwise.
*/
int		   PC_AddDefine( source_t* source, char* string );

/*!
	\brief Adds a preprocessor define that will be applied to every opened source file

	The function parses the supplied C string into a define object using PC_DefineFromString. If the parse fails, the function returns false. On success it inserts the new define at the head of the
   global defines linked list and returns true. The added define becomes effective for all subsequently loaded sources.

	\param string a C string containing the preprocessor definition to add globally
	\return non‑zero if the define was successfully added, zero otherwise
*/
int		   PC_AddGlobalDefine( char* string );

/*!
	\brief removes a global define by its name

	The function looks up a define in the global defines table. If the define is found it is freed and the function returns a true status value. If the name is not present no action is taken and a
   false status is returned.

	\param name the name of the define to remove
	\return qtrue if a define was removed; otherwise qfalse
*/
int		   PC_RemoveGlobalDefine( char* name );

/*!
	\brief Removes every global preprocessor define from the list.

	Iterates through the linked list of globaldefines, freeing each node’s resources via PC_FreeDefine. After this call the globaldefines list is empty and no defines remain.

*/
void	   PC_RemoveAllGlobalDefines();

/*!
	\brief Adds predefined macro definitions to the provided source.

	The function iterates over a static array of builtin macro names such as __LINE__, __FILE__, __DATE__, and __TIME__. For each entry it allocates memory for a define_t structure, copies the macro
   name into that structure, sets it as fixed, records the builtin type, and then inserts the define into the source's definition collection. Depending on whether DEFINEHASHING is enabled the define
   is added to a hash table or linked directly to the source's list.

	\param source pointer to a source structure that will receive the builtin defines
*/
void	   PC_AddBuiltinDefines( source_t* source );

/*!
	\brief Sets the include path for a source and ensures it ends with a path separator.

	The function copies the supplied string into the includepath array of the source up to a maximum length specified by _MAX_PATH. Afterwards it checks the last character of the resulting string; if
   that character is not a backslash or forward slash, a platform‑specific path separator is appended. Because strncpy is used, the destination may not be null‑terminated if the input exceeds
   _MAX_PATH, so callers must ensure the path length is appropriate. The function does not throw exceptions.

	\param source Pointer to a source_t whose includepath member will be updated.
	\param path String containing the new include path to store in includepath.
*/
void	   PC_SetIncludePath( source_t* source, char* path );

/*!
	\brief Sets the punctuation set for the given source.

	Assigns the specified punctuation table to the source structure, replacing any previously used set.

	\param source pointer to the source structure to receive the set
	\param p pointer to the punctuation table to apply
*/
void	   PC_SetPunctuations( source_t* source, punctuation_t* p );

/*!
	\brief Sets the base folder to use when loading files.

	The function forwards the provided path to the underlying persistence system, updating the directory that subsequent file load operations will reference.

	\param path Path of the directory that should become the new base folder.
*/
void	   PC_SetBaseFolder( char* path );

/*!
	\brief Creates and initializes a source_t structure from a given file path.

	The function initializes the token heap and loads the script file using LoadScriptFile. It then constructs a new source_t object, copying the provided filename, and setting up pointers for tokens,
   defines, and indentation stacks. The skip counter is zeroed, and if define hashing is enabled, a hash table for defines is allocated. Global defines are added to the source before the structure is
   returned. If the script file fails to load, NULL is returned.

	\param filename path to the source file to be loaded.
	\return Pointer to the newly allocated source_t on success, or NULL on failure.
*/
source_t*  LoadSourceFile( const char* filename );

/*!
	\brief Creates a source_t object from a memory buffer containing script data.

	The function first initializes the token heap, then attempts to load a script from the provided memory block. If the script load fails, it returns NULL. Otherwise it allocates and zero-initializes
   a source_t, copies the supplied name into the filename field, assigns the loaded script to the source's script stack, clears token and define lists, and allocates a define hash table if hashing is
   enabled. Global defines are added before the source is returned.

	\param ptr Pointer to the memory buffer containing the source script text.
	\param length Length of the memory buffer, in bytes.
	\param name Name or filename to associate with the source.
	\return Pointer to the initialized source_t structure, or NULL on failure.
*/
source_t*  LoadSourceMemory( char* ptr, int length, char* name );

/*!
	\brief Frees all memory allocated for a source and its scripts, tokens, defines, and indents.

	This routine walks through the source's script stack, token list, define table (or list), and indentation stack, releasing each element with the appropriate free helper. If define hashing is
   enabled, it iterates over the hash buckets and frees the linked define lists; otherwise it uses the defines linked list. After all substructures have been deallocated, the source itself is freed.

	\param source pointer to the source structure to free
*/
void	   FreeSource( source_t* source );

/*!
	\brief Formats and reports an error from a source script file.

	The function builds a 1024 character buffer from the format string and any additional arguments, then prints it together with the current source script's filename and line number. Depending on
   compilation flags, the message is sent either to the bot library printer, standard output, or the BSPC logger. The function does not return any value and does not throw exceptions.

	\param source Pointer to a source_t containing the script stack used to retrieve the filename and line number.
	\param str Format string for the error message, similar to printf.
	\param ... Variable arguments corresponding to placeholders in the format string.
*/
void QDECL SourceError( source_t* source, char* str, ... );

/*!
	\brief Print a warning message that includes file and line information from a script source.

	This function formats the supplied variable argument list into a local buffer and outputs a warning string together with the source file name and line number obtained from the source structure.
   Depending on build configuration, the warning is sent to the bot import system, standard output, or a log file. The format used is "%s, line %d: %s".

	\param source pointer to the source_t structure containing script context
	\param str format string for the warning message
*/
void QDECL SourceWarning( source_t* source, char* str, ... );

#ifdef BSPC
	// some of BSPC source does include game/q_shared.h and some does not
	// we define pc_token_s pc_token_t if needed (yes, it's ugly)
	#ifndef __Q_SHARED_H
		#define MAX_TOKENLENGTH 1024
typedef struct pc_token_s {
	int	  type;
	int	  subtype;
	int	  intvalue;
	float floatvalue;
	char  string[MAX_TOKENLENGTH];
} pc_token_t;
	#endif //!_Q_SHARED_H
#endif	   // BSPC

/*!
	\brief Loads a source file and returns a handle index, or zero if allocation fails.

	The function searches the global source file table for an unused slot, starting with index 1. If no free slot is found or the file cannot be loaded, it returns 0. Otherwise it resets the base
   folder, loads the file via LoadSourceFile, stores the resulting source pointer in the table, and returns the slot number. Handle 0 is considered invalid and is returned only on failure.

	\param filename Path to the source file to load.
	\return The index of the allocated source file, or zero if the load failed or no free slot was available.
*/
int	 PC_LoadSourceHandle( const char* filename );

/*!
	\brief Releases a loaded source file identified by its handle.

	The function validates the supplied handle against the bounds of the sourceFiles array, ensuring it falls within the range [1, MAX_SOURCEFILES). If the handle is invalid or no source file is
   associated with it, the function returns 0 (qfalse). Otherwise, it calls the internal FreeSource routine to deallocate the source, clears the array entry, and returns 1 (qtrue) to indicate success.

	The handle refers to an index in the global sourceFiles array, representing a script file that has been loaded earlier via PC_LoadSourceHandle. The function does not throw exceptions and uses
   simple return values to convey the status of the operation.

	\param handle Index of the source file to be freed, must be between 1 and MAX_SOURCEFILES-1.
	\return 1 if the source file was successfully freed, 0 if the handle was invalid or not active.
*/
int	 PC_FreeSourceHandle( int handle );

/*!
	\brief Reads the next token from the source file identified by handle and copies it into the supplied pc_token structure.

	The function first checks that the handle refers to a valid source file within the allocated range and that the source file is open. It then retrieves the next token using PC_ReadToken, copies the
   token's string, type, subtype, integer and float values into the provided pc_token structure, strips surrounding double quotes for string tokens, and returns the result from PC_ReadToken. A return
   value of 0 indicates an invalid handle, an unopened source file, or no remaining tokens; any non‑zero value indicates a successfully read token.

	\param handle index of the open source file to read from
	\param pc_token pointer to a pc_token_s structure that will receive the read token data
	\return non‑zero if a token was successfully read, otherwise 0 for invalid handle, closed file, or end of file
*/
int	 PC_ReadTokenHandle( int handle, struct pc_token_s* pc_token );

/*!
	\brief Retrieves the file name and current line number for a source handle.

	The function checks that the source handle is within range and points to a valid source file. It copies the associated file name into the provided buffer and, if a script stack exists for the
   source, writes the current line number to the provided line pointer; otherwise the line is set to zero. It returns a non‑zero value on success and zero on error.

	\param handle Handle of a source file returned by PC_LoadSourceHandle, must be in [1, MAX_SOURCEFILES) and not null.
	\param filename Buffer that receives the null‑terminated file name; must be large enough to hold it.
	\param line Output pointer to receive the current line number; set to 0 if no script stack is present.
	\return Non‑zero on success (a valid handle and file available); zero if the handle was out of range or invalid.
*/
int	 PC_SourceFileAndLine( int handle, char* filename, int* line );

/*!
	\brief Reports any source files that remain open after precompilation.

	The function scans the array of source file references, starting at index 1 up to MAX_SOURCEFILES-1, and, for each non‑null entry, outputs an error message containing the filename of the file that
   was not closed. This diagnostic is emitted only when BOTLIB is defined.

*/
void PC_CheckOpenSourceHandles();
