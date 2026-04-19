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
	\file engine/botlib/l_precomp.cpp
	\brief Implements the pre‑compiler that transforms raw script files into token streams for the DarkWolf engine.
	\note doxygenix: sha256=1bb24f6c737a311821b7858f4ac1fa37b68153779bfa877406640b0b07bea503

	\par File Purpose
	- Implements the pre‑compiler that transforms raw script files into token streams for the DarkWolf engine.
	- Handles all standard preprocessor directives: #include, #define, #undef, #ifdef, #ifndef, #else, #elif, #endif, #error, #pragma, $evalint, $evalfloat.
	- Manages macro tables, expansion logic, and built‑in macros such as __LINE__, __FILE__, __DATE__, and __TIME__.

	\par Core Responsibilities
	- Parse and preprocess script files, performing tokenization, directive handling, macro definition, expansion, and include processing.
	- Maintain a source_t context that tracks script stack, token queue, indent stack, skip count, defines, and include path.
	- Provide diagnostic reporting through SourceError and SourceWarning.
	- Expose a lightweight handle-based API for loading, reading, and freeing sources.
	- Support optional hashing of define tables to improve lookup speed.

	\par Key Types and Functions
	- source_t  — the parsing context storing script stack, token list, indent stack, skip count, includes, and define table.
	- token_t   — lexical token (type, subtype, string, numeric values).
	- define_t  — macro definition with name, parameters, replacement tokens, flags, builtin kind, and link fields.
	- indent_t  — indentation record used for conditional compilation tracking.
	- directive_t — associates a directive name with its handler function.
	- operator_t — represents an operator node with precedence and parentheses for expression evaluation.
	- value_t   — numeric value node used during PC_EvaluateTokens.
	- PC_ReadToken(source_t*) → int   — main token reader handling directives and macro expansion.
	- PC_Directive_define(source_t*) → int   — parses a #define and builds its define_t.
	- PC_ExpandDefine(source_t*, token_t*, define_t*, token_t**, token_t**) → int   — expands a macro into a token list.
	- PC_EvaluateTokens(source_t*, token_t*, signed long int*, double*, int) → int   — evaluates an expression and returns integer or floating result.
	- PC_ReadSourceToken(source_t*, token_t*) → int   — reads next token, loading new scripts as needed.
	- PC_ExpandBuiltinDefine(source_t*, token_t*, define_t*, token_t**, token_t**) → int   — expands built‑in macros like __LINE__.
	- PC_Directive_include(source_t*) → int   — loads an included script and pushes it onto the source stack.
	- PC_SetIncludePath(source_t*, char*)   — ensures the include path ends with the correct separator.
	- PC_LoadSourceHandle(const char*) → int   — allocates a source_t and returns a numeric handle.
	- PC_ReadTokenHandle(int, pc_token_t*) → int   — reads a token from a source handle, exposing a portable API.
	- PC_FreeSourceHandle(int)                → int   — releases a source associated with a handle.
	- PC_CheckOpenSourceHandles()                 → void   — diagnostics for leaked source handles.

	\par Control Flow
	- PC_ReadToken implements the main token read loop: it fetches tokens, dispatches # and $ directives, expands macros, concatenates strings, and handles whitespace.
	- Directive handlers (PC_ReadDirective, PC_ReadDollarDirective) invoke the appropriate # or $ directive functions such as PC_Directive_if, PC_Directive_define, PC_Directive_include, etc.
	- Token stream is constructed from script files via LoadScript*, with #include pushing a new script_t onto the source stack and the reader recursing as needed.
	- Conditional compilation uses an indentation stack: PC_PushIndent and PC_PopIndent are called to push or pop INDENT_* records; source->skip counts how many levels are suppressed.
	- Macro expansion is performed by PC_ExpandDefine and PC_ExpandDefineIntoSource, which replace a name token with a linked list of expansion tokens.
	- Expression evaluation for #if/#elif is carried out by PC_Evaluate, which gathers tokens, calls PC_EvaluateTokens, and writes the result back to the token stream.

	\par Dependencies
	- "q_shared.h"
	- "botshared/botlib.h"
	- "be_interface.h"
	- "l_memory.h"
	- "l_script.h"
	- "l_precomp.h"
	- "l_log.h"
	- "qbsp.h"
	- "l_mem.h"

	\par How It Fits
	- l_precomp.cpp sits between the file I/O layer (l_script) and the engine’s higher‑level parse/VM components.
	- It supplies token streams that the core script interpreter consumes.
	- Global defines allow scripts to query build/runtime information.
	- The component is used by both the main game engine and the bot library.
*/

// Copyright (C) 1999-2000 Id Software, Inc.
//

/*****************************************************************************
 * name:		l_precomp.c
 *
 * desc:		pre compiler
 *
 *
 *****************************************************************************/

// Notes:			fix: PC_StringizeTokens

// #define SCREWUP
// #define BOTLIB
// #define QUAKE
// #define QUAKEC
// #define MEQCC

#ifdef SCREWUP
	#include <stdio.h>
	#include <stdlib.h>
	#include <limits.h>
	#include <string.h>
	#include <stdarg.h>
	#include <time.h>
	#include "l_memory.h"
	#include "l_script.h"
	#include "l_precomp.h"

typedef enum { qfalse, qtrue } qboolean;
#endif // SCREWUP

#ifdef BOTLIB
	#include "q_shared.h"
	#include "botshared/botlib.h"
	#include "be_interface.h"
	#include "l_memory.h"
	#include "l_script.h"
	#include "l_precomp.h"
	#include "l_log.h"
#endif // BOTLIB

#ifdef MEQCC
	#include "qcc.h"
	#include "time.h" //time & ctime
	#include "math.h" //fabs
	#include "l_memory.h"
	#include "l_script.h"
	#include "l_precomp.h"
	#include "l_log.h"

	#define qtrue  true
	#define qfalse false
#endif // MEQCC

#ifdef BSPC
	// include files for usage in the BSP Converter
	#include "../bspc/qbsp.h"
	#include "../bspc/l_log.h"
	#include "../bspc/l_mem.h"
	#include "l_precomp.h"

	#define qtrue			true
	#define qfalse			false
	#define Q_stricmp		strcasecmp

	#define MAX_TOKENLENGTH 1024

#endif // BSPC

#if defined( QUAKE ) && !defined( BSPC )
	#include "l_utils.h"
#endif // QUAKE

// #define DEBUG_EVAL

#define MAX_DEFINEPARMS 128

#define DEFINEHASHING	1

// directive name with parse function
typedef struct directive_s {
	char* name;
	int ( *func )( source_t* source );
} directive_t;

#define DEFINEHASHSIZE	1024

#define TOKEN_HEAP_SIZE 4096

int		   numtokens;
/*
int tokenheapinitialized;				//true when the token heap is initialized
token_t token_heap[TOKEN_HEAP_SIZE];	//heap with tokens
token_t *freetokens;					//free tokens from the heap
*/

// list with global defines added to every source loaded
define_t*  globaldefines;

/*!
	\brief Reports an error with formatted message at the current source location.

	This function takes a source location and a variable argument list to format an error message. It retrieves the filename and line number from the source context and prints the error message using platform-specific output methods. It supports multiple compilation targets including BOTLIB, MEQCC, and BSPC, each using their respective print functions. The function is typically used in parsing and preprocessor code to report syntax or semantic errors.

	\param source Pointer to the source context containing filename and line information
	\param str Format string for the error message
	\param  Variable arguments for formatting the error message
	\return void
*/
void QDECL SourceError( source_t* source, char* str, ... )
{
	char	text[1024];
	va_list ap;

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );
#ifdef BOTLIB
	botimport.Print( PRT_ERROR, "file %s, line %d: %s\n", source->scriptstack->filename, source->scriptstack->line, text );
#endif // BOTLIB
#ifdef MEQCC
	printf( "error: file %s, line %d: %s\n", source->scriptstack->filename, source->scriptstack->line, text );
#endif // MEQCC
#ifdef BSPC
	Log_Print( "error: file %s, line %d: %s\n", source->scriptstack->filename, source->scriptstack->line, text );
#endif // BSPC
}

/*!
	\brief Reports a warning message with file and line number information from a source script.

	This function formats a warning message using variadic arguments and prints it to the appropriate output stream based on the compilation flags. It retrieves the filename and line number from the source script stack and includes this information in the warning message. The function is designed to be used for reporting warnings during compilation or processing of source files.

	\param source Pointer to the source structure containing script information
	\param str Format string for the warning message
*/
void QDECL SourceWarning( source_t* source, char* str, ... )
{
	char	text[1024];
	va_list ap;

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );
#ifdef BOTLIB
	botimport.Print( PRT_WARNING, "file %s, line %d: %s\n", source->scriptstack->filename, source->scriptstack->line, text );
#endif // BOTLIB
#ifdef MEQCC
	printf( "warning: file %s, line %d: %s\n", source->scriptstack->filename, source->scriptstack->line, text );
#endif // MEQCC
#ifdef BSPC
	Log_Print( "warning: file %s, line %d: %s\n", source->scriptstack->filename, source->scriptstack->line, text );
#endif // BSPC
}

/*!
	\brief Adds a new indentation context to the source’s stack.

	Creates an indent node containing the indentation type, a reference to the current script stack, and a flag indicating whether subsequent code should be skipped.
	The new node is inserted at the front of the source’s indentation list, and the source’s skip counter is increased when appropriate.

	\param source Source data structure containing the current parsing state, including script and indentation stacks.
	\param type Integer identifier for the indentation type.
	\param skip Nonzero value that signals the following block should be skipped.
*/
void PC_PushIndent( source_t* source, int type, int skip )
{
	indent_t* indent;

	indent		   = ( indent_t* )GetMemory( sizeof( indent_t ) );
	indent->type   = type;
	indent->script = source->scriptstack;
	indent->skip   = ( skip != 0 );
	source->skip += indent->skip;
	indent->next		= source->indentstack;
	source->indentstack = indent;
}

/*!
	\brief Removes the top indentation entry from a source stack and returns its type and skip values; if none is present or the entry does not belong to the current script, zero is returned.

	The function starts by resetting the output parameters to zero and then checks whether the source has an indentation stack to pop. It only proceeds if the top indentation belongs to the currently
   active script; otherwise the function exits without changing the outputs. When the correct indentation is found, its type and skip fields are copied out, the indentation node is removed from the
   stack, the source’s cumulative skip counter is adjusted, and the node’s memory is freed.

	\param source Pointer to a source_t structure that contains the indentation stack and related state.
	\param type Pointer to an int where the indentation type value will be written if a valid indentation is found.
	\param skip Pointer to an int where the indentation skip value will be written if a valid indentation is found.
*/
void PC_PopIndent( source_t* source, int* type, int* skip )
{
	indent_t* indent;

	*type = 0;
	*skip = 0;

	indent = source->indentstack;

	if( !indent ) {
		return;
	}

	// must be an indent from the current script
	if( source->indentstack->script != source->scriptstack ) {
		return;
	}

	*type				= indent->type;
	*skip				= indent->skip;
	source->indentstack = source->indentstack->next;
	source->skip -= indent->skip;
	FreeMemory( indent );
}

/*!
	\brief Pushes a script onto the source's stack after detecting recursive inclusion and reports an error if the script is already on the stack.

	The function iterates over the current script stack of the provided source, comparing the filename of each stack element with the filename of the script to be added in a case-insensitive manner.
   If a match is found, it calls SourceError to notify that the script would be recursively included, then exits without modifying the stack. If no recursive inclusion is detected, the new script is
   linked to the front of the stack, maintaining LIFO order. The comparison is performed using Q_stricmp for case-insensitive string comparison. No state is returned; the function simply updates the
   source structure in place.

	\param source Pointer to the source whose script stack is to be updated
	\param script Script to be added to the stack
*/
void PC_PushScript( source_t* source, script_t* script )
{
	script_t* s;

	for( s = source->scriptstack; s; s = s->next ) {
		if( !Q_stricmp( s->filename, script->filename ) ) {
			SourceError( source, "%s recursively included", script->filename );
			return;
		}
	}

	// push the script on the script stack
	script->next		= source->scriptstack;
	source->scriptstack = script;
}

/*!
	\brief Initializes the global token heap the first time it is called, setting up a linked list of free token structures.

	The function creates a singly linked list of all token objects stored in the static array token_heap.  It clears the freetokens pointer, then iterates over the array, linking each element’s next
   pointer to the current head of the free list and updating the head.  After all elements are linked, tokenheapinitialized is set to true so that subsequent calls return immediately without
   rebuilding the list.  This prepares the token heap for allocation by other parts of the parser.

*/
void PC_InitTokenHeap()
{
	/*
	int i;

	if (tokenheapinitialized) return;
	freetokens = NULL;
	for (i = 0; i < TOKEN_HEAP_SIZE; i++)
	{
		token_heap[i].next = freetokens;
		freetokens = &token_heap[i];
	}
	tokenheapinitialized = qtrue;
	*/
}

/*!
	\brief Creates a copy of the given token by allocating a new token and copying its contents.

	The function allocates memory for a new token structure using GetMemory, copies all fields from the supplied token via memcpy, resets the next pointer of the new token to NULL, increments the
   global count of tokens, and returns a pointer to the new token. If memory allocation fails, a fatal error is reported and NULL is returned.

	\param token The token to duplicate
	\return Pointer to the new token that is a copy of the original, or NULL if allocation failed.
*/
token_t* PC_CopyToken( token_t* token )
{
	token_t* t;

	//	t = (token_t *) malloc(sizeof(token_t));
	t = ( token_t* )GetMemory( sizeof( token_t ) );

	//	t = freetokens;
	if( !t ) {
#ifdef BSPC
		Error( "out of token space\n" );
#else
		Com_Error( ERR_FATAL, "out of token space\n" );
#endif
		return NULL;
	}

	//	freetokens = freetokens->next;
	memcpy( t, token, sizeof( token_t ) );
	t->next = NULL;
	numtokens++;
	return t;
}

/*!
	\brief Releases a token and updates the global token counter.

	The function takes a pointer to a token structure, frees the memory allocated for that token and decrements a global count of active tokens. It does not place the token back into any free list;
   the memory is simply released.

	\param token Pointer to the token to be freed.
*/
void PC_FreeToken( token_t* token )
{
	// free(token);
	FreeMemory( token );
	//	token->next = freetokens;
	//	freetokens = token;
	numtokens--;
}

/*!
	\brief Reads the next token from a source stack, loading from nested scripts as necessary

	The function consumes tokens from the source’s token queue. If the queue is empty it first attempts to read a new token from the current script using PS_ReadToken. When the current script
   finishes, it cleans up any pending indents associated with that script, then pops the script off the stack and frees its memory. If the stack is exhausted, the function signals end-of-input. When a
   token is available it copies it into the supplied token structure, removes it from the queue, and frees the temporary token. If a token was successfully retrieved, it returns qtrue; otherwise it
   returns qfalse.

	\param source Parsing context; may be advanced to the next script and have its indent stack updated
	\param token Output container supplied by the caller, which will receive the read token
	\return qtrue when a token was read, qfalse if no more tokens are available
*/
int PC_ReadSourceToken( source_t* source, token_t* token )
{
	token_t*  t;
	script_t* script;
	int		  type, skip;

	// if there's no token already available
	while( !source->tokens ) {
		// if there's a token to read from the script
		if( PS_ReadToken( source->scriptstack, token ) ) {
			return qtrue;
		}

		// if at the end of the script
		if( EndOfScript( source->scriptstack ) ) {
			// remove all indents of the script
			while( source->indentstack && source->indentstack->script == source->scriptstack ) {
				SourceWarning( source, "missing #endif" );
				PC_PopIndent( source, &type, &skip );
			}
		}

		// if this was the initial script
		if( !source->scriptstack->next ) {
			return qfalse;
		}

		// remove the script and return to the last one
		script				= source->scriptstack;
		source->scriptstack = source->scriptstack->next;
		FreeScript( script );
	}

	// copy the already available token
	memcpy( token, source->tokens, sizeof( token_t ) );
	// free the read token
	t			   = source->tokens;
	source->tokens = source->tokens->next;
	PC_FreeToken( t );
	return qtrue;
}

/*!
	\brief Adds a copy of the supplied token to the front of the source's token list.

	The function creates a duplicate of the token passed in by calling PC_CopyToken and then places this duplicate at the beginning of the source's token linked list. The next read operation on the
   source will retrieve this token first. The operation returns a boolean success value, qtrue, indicating that the token was successfully re‑inserted.

	\param source The source whose token list will receive the new token
	\param token The token to be un‑read; it is copied before insertion
	\return qtrue on success
*/
int PC_UnreadSourceToken( source_t* source, token_t* token )
{
	token_t* t;

	t			   = PC_CopyToken( token );
	t->next		   = source->tokens;
	source->tokens = t;
	return qtrue;
}

/*!
	\brief Reads and records the parameter list of a macro definition from the source stream.

	The routine first verifies that the macro declaration begins with an open parenthesis and that the declared number of parameters does not exceed the supplied limit. It then iterates through the
   token stream, handling nested parentheses and commas: each comma denotes the end of a parameter token list, while parentheses may be nested within a parameter. For each actual parameter token, a
   linked list of token structures is built and stored in the provided parms array. Mismatched parentheses, too many or too few parameters relative to the definition, or an incomplete definition
   trigger source‑level warnings or errors. On success, qtrue (non‑zero) is returned; on failure, qfalse is returned.

	\param source the context from which tokens are read
	\param define the macro definition being processed, providing its name and declared parameter count
	\param parms output array that will receive up to maxparms token chains, one per actual parameter
	\param maxparms the maximum number of parameter token chains that can be stored; used to detect excessive parameters
	\return qtrue on success, qfalse on failure
*/
int PC_ReadDefineParms( source_t* source, define_t* define, token_t** parms, int maxparms )
{
	token_t token, *t, *last;
	int		i, done, lastcomma, numparms, indent;

	if( !PC_ReadSourceToken( source, &token ) ) {
		SourceError( source, "define %s missing parms", define->name );
		return qfalse;
	}

	//
	if( define->numparms > maxparms ) {
		SourceError( source, "define with more than %d parameters", maxparms );
		return qfalse;
	}

	//
	for( i = 0; i < define->numparms; i++ ) {
		parms[i] = NULL;
	}

	// if no leading "("
	if( strcmp( token.string, "(" ) ) {
		PC_UnreadSourceToken( source, &token );
		SourceError( source, "define %s missing parms", define->name );
		return qfalse;
	}

	// read the define parameters
	for( done = 0, numparms = 0, indent = 0; !done; ) {
		if( numparms >= maxparms ) {
			SourceError( source, "define %s with too many parms", define->name );
			return qfalse;
		}

		if( numparms >= define->numparms ) {
			SourceWarning( source, "define %s has too many parms", define->name );
			return qfalse;
		}

		parms[numparms] = NULL;
		lastcomma		= 1;
		last			= NULL;

		while( !done ) {
			//
			if( !PC_ReadSourceToken( source, &token ) ) {
				SourceError( source, "define %s incomplete", define->name );
				return qfalse;
			}

			//
			if( !strcmp( token.string, "," ) ) {
				if( indent <= 0 ) {
					if( lastcomma ) {
						SourceWarning( source, "too many comma's" );
					}

					lastcomma = 1;
					break;
				}
			}

			lastcomma = 0;

			//
			if( !strcmp( token.string, "(" ) ) {
				indent++;
				continue;

			} else if( !strcmp( token.string, ")" ) ) {
				if( --indent <= 0 ) {
					if( !parms[define->numparms - 1] ) {
						SourceWarning( source, "too few define parms" );
					}

					done = 1;
					break;
				}
			}

			//
			if( numparms < define->numparms ) {
				//
				t		= PC_CopyToken( &token );
				t->next = NULL;

				if( last ) {
					last->next = t;

				} else {
					parms[numparms] = t;
				}

				last = t;
			}
		}

		numparms++;
	}

	return qtrue;
}

/*!
	\brief Creates a quoted string token by concatenating a list of tokens

	The function sets the token type to string, clears any existing whitespace information, and initializes the string content. It then surrounds the concatenated token strings with double quotes,
   ensuring that the buffer does not overflow by respecting the MAX_TOKEN limit. Finally, it returns a truth value indicating success.

	\param tokens list of tokens that are to be concatenated into the string
	\param token token structure that will receive the resulting quoted string
	\return an integer truth value (qtrue) indicating success
*/
int PC_StringizeTokens( token_t* tokens, token_t* token )
{
	token_t* t;

	token->type			   = TT_STRING;
	token->whitespace_p	   = NULL;
	token->endwhitespace_p = NULL;
	token->string[0]	   = '\0';
	strcat( token->string, "\"" );

	for( t = tokens; t; t = t->next ) {
		strncat( token->string, t->string, MAX_TOKEN - strlen( token->string ) );
	}

	strncat( token->string, "\"", MAX_TOKEN - strlen( token->string ) );
	return qtrue;
}

/*!
	\brief Merges two token objects by concatenating their string values when the combination is legal, returning a success status

	The function supports merging a name token with another name or a numeric token by appending the second token’s string to the first. It also handles concatenating two string tokens: the trailing
   quote of the first and the leading quote of the second are removed before concatenation. If the token types do not allow merging, the function returns false. The merge operation modifies the first
   token’s string buffer in place.

	\param t1 The token that will receive the merged result; its string buffer is modified.
	\param t2 The token whose string is appended or whose data is incorporated into the first token.
	\return An integer where a non‑zero value indicates successful merge (qtrue) and zero indicates failure (qfalse).
*/
int PC_MergeTokens( token_t* t1, token_t* t2 )
{
	// merging of a name with a name or number
	if( t1->type == TT_NAME && ( t2->type == TT_NAME || t2->type == TT_NUMBER ) ) {
		strcat( t1->string, t2->string );
		return qtrue;
	}

	// merging of two strings
	if( t1->type == TT_STRING && t2->type == TT_STRING ) {
		// remove trailing double quote
		t1->string[strlen( t1->string ) - 1] = '\0';
		// concat without leading double quote
		strcat( t1->string, &t2->string[1] );
		return qtrue;
	}

	// FIXME: merging of two number of the same sub type
	return qfalse;
}

/*
void PC_PrintDefine(define_t *define)
{
	printf("define->name = %s\n", define->name);
	printf("define->flags = %d\n", define->flags);
	printf("define->builtin = %d\n", define->builtin);
	printf("define->numparms = %d\n", define->numparms);
//	token_t *parms;					//define parameters
//	token_t *tokens;					//macro tokens (possibly containing parm tokens)
//	struct define_s *next;			//next defined macro in a list
}
*/
#if DEFINEHASHING
/*!
	\brief Prints the contents of the define hash table for debugging purposes.

	The function iterates over every bucket of a hash table that stores define_t structures. For each bucket it writes the bucket index followed by the names of all entries stored in that bucket. The
   output is generated using Log_Write, with each bucket’s data ending on a new line. This is typically used during development to inspect the state of the define table.

	\param definehash an array of pointers to define_t structures, representing the hash table buckets.
*/
void PC_PrintDefineHashTable( define_t** definehash )
{
	int		  i;
	define_t* d;

	for( i = 0; i < DEFINEHASHSIZE; i++ ) {
		Log_Write( "%4d:", i );

		for( d = definehash[i]; d; d = d->hashnext ) {
			Log_Write( " %s", d->name );
		}

		Log_Write( "\n" );
	}
}

/*!
	\brief Computes a hash value for a null-terminated string.

	The function processes each character in the input name by adding the product of the character's ASCII value and a weight that increases with the character position. After all characters are
   processed, it mixes the accumulated value using XOR operations with right‑shifted versions of itself and then constrains the result to the range defined by the global DEFINEHASHSIZE constant. The
   final integer is returned as the hash index suitable for hash tables sized according to DEFINEHASHSIZE.

	\param name A null-terminated C string to be hashed.
	\return An integer hash index in the range [0, DEFINEHASHSIZE-1].
*/
int PC_NameHash( char* name )
{
	int register hash, i;

	hash = 0;

	for( i = 0; name[i] != '\0'; i++ ) {
		hash += name[i] * ( 119 + i );
		// hash += (name[i] << 7) + i;
		// hash += (name[i] << (i&15));
	}

	hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) ) & ( DEFINEHASHSIZE - 1 );
	return hash;
}

/*!
	\brief Adds a define entry to the hashtable bucket list according to its name hash

	The function calculates the hash value from the define's name using PC_NameHash. It then inserts the entry at the beginning of the bucket list in the definehash array, updating the hashnext link
   to maintain the chain. This operation does not allocate memory or perform any other side effects beyond pointer updates.

	\param define The define_t structure representing the entry to insert
	\param definehash An array of define_t pointers used as hash table buckets
*/
void PC_AddDefineToHash( define_t* define, define_t** definehash )
{
	int hash;

	hash			 = PC_NameHash( define->name );
	define->hashnext = definehash[hash];
	definehash[hash] = define;
}

/*!
	\brief Searches a hashed define table for a definition with the given name.

	Computes a hash value for the name, then traverses the linked list at that hash bucket in the table. Each node’s name field is compared to the target using string comparison. If a match is found,
   the pointer to that node is returned; otherwise, the function returns null.

	If the name field is null or the table contains empty buckets, the function simply returns null, as there is no matching definition.

	\param definehash array of buckets, each bucket is a chained list of define_t structures hashed by name
	\param name name of the define to locate
	\return pointer to the matching define_t, or null if no such definition exists
*/
define_t* PC_FindHashedDefine( define_t** definehash, char* name )
{
	define_t* d;
	int		  hash;

	hash = PC_NameHash( name );

	for( d = definehash[hash]; d; d = d->hashnext ) {
		if( !strcmp( d->name, name ) ) {
			return d;
		}
	}

	return NULL;
}

#endif // DEFINEHASHING

/*!
	\brief Looks up a define by name in a linked list and returns a pointer to it or NULL if not found.

	The function iterates through the linked list pointed to by 'defines', comparing each node's 'name' field against the supplied 'name' argument using strcmp. When a match is found, the
   corresponding node pointer is returned. If the entire list is traversed without finding a match, the function returns NULL.

	\param defines head of a singly linked list of define_t structures to search
	\param name name of the define to find
	\return pointer to the matching define_t node, or NULL if no match
*/
define_t* PC_FindDefine( define_t* defines, char* name )
{
	define_t* d;

	for( d = defines; d; d = d->next ) {
		if( !strcmp( d->name, name ) ) {
			return d;
		}
	}

	return NULL;
}

/*!
	\brief Returns the index of a define parameter matching the given name or -1 if it does not exist.

	Iterates over the linked list of parameter tokens stored in the provided define structure. For each token, it compares the token's string with the supplied name. When a match is found, the current
   position in the list, expressed as a zero‑based index, is returned. If the entire list is traversed without finding a match, the function returns –1 to indicate the absence of such a parameter.

	\param define pointer to a define structure that contains the list of parameter tokens
	\param name name of the parameter to search for
	\return index of the matching parameter, or –1 when not found
*/
int PC_FindDefineParm( define_t* define, char* name )
{
	token_t* p;
	int		 i;

	i = 0;

	for( p = define->parms; p; p = p->next ) {
		if( !strcmp( p->string, name ) ) {
			return i;
		}

		i++;
	}

	return -1;
}

/*!
	\brief Frees a define structure and all tokens it owns.

	The function walks through the list of parameter tokens attached to the define, frees each token, then does the same for the list of tokens that make up the body of the define, and finally
   releases the memory for the define structure itself. Both token lists are singly linked and can be empty.

	\param define Pointer to the define to deallocate; may not be null.
*/
void PC_FreeDefine( define_t* define )
{
	token_t *t, *next;

	// free the define parameters
	for( t = define->parms; t; t = next ) {
		next = t->next;
		PC_FreeToken( t );
	}

	// free the define tokens
	for( t = define->tokens; t; t = next ) {
		next = t->next;
		PC_FreeToken( t );
	}

	// free the define
	FreeMemory( define );
}

/*!
	\brief Adds built-in preprocessor defines to the specified source

	This function initializes and adds several built-in preprocessor definitions to the given source. It creates definitions for __LINE__, __FILE__, __DATE__, and __TIME__ that are automatically available during preprocessing. Each built-in define is marked as fixed and has a specific builtin identifier. The function handles the memory allocation for each define and properly links them to the source's define list, using either hashed or sequential linking based on the DEFINEHASHING compile flag

	\param source Pointer to the source structure where the built-in defines will be added
*/
void PC_AddBuiltinDefines( source_t* source )
{
	int		  i;
	define_t* define;
	struct builtin {
		char* string;
		int	  builtin;
	} builtin[] = { { "__LINE__", BUILTIN_LINE },
		{ "__FILE__", BUILTIN_FILE },
		{ "__DATE__", BUILTIN_DATE },
		{ "__TIME__", BUILTIN_TIME },
		//		"__STDC__", BUILTIN_STDC,
		{ NULL, 0 } };

	for( i = 0; builtin[i].string; i++ ) {
		define = ( define_t* )GetMemory( sizeof( define_t ) + strlen( builtin[i].string ) + 1 );
		memset( define, 0, sizeof( define_t ) );
		define->name = ( char* )define + sizeof( define_t );
		strcpy( define->name, builtin[i].string );
		define->flags |= DEFINE_FIXED;
		define->builtin = builtin[i].builtin;
		// add the define to the source
#if DEFINEHASHING
		PC_AddDefineToHash( define, source->definehash );
#else
		define->next	= source->defines;
		source->defines = define;
#endif // DEFINEHASHING
	}
}

/*!
	\brief Expands built‑in preprocessor macros such as __LINE__, __FILE__, __DATE__, and __TIME__ into a token list and reports success.

	The function creates a copy of the macro invocation token and, depending on the builtin type specified in the definition, replaces its contents with the appropriate value. For __LINE__ a numeric
   token containing the source line number is generated; for __FILE__ the current script filename is inserted; for __DATE__ an 11‑character string containing the month, day, and year is produced; for
   __TIME__ an 8‑character string containing the hour, minute, and second is produced. The token type and subtype are set to reflect the generated value. The created token is returned via the
   firsttoken and lasttoken output parameters. If the builtin is unknown or a special case such as __STDC__, the output pointers are set to NULL. The function always returns a non‑zero value to
   indicate successful expansion.


	\param source Context of the source token stream; used for diagnostics
	\param deftoken Token representing the original macro invocation
	\param define Macro definition containing builtin information
	\param firsttoken Output pointer receiving the first token of the expanded list
	\param lasttoken Output pointer receiving the last token of the expanded list
	\return Non‑zero on success, zero on failure
*/
int PC_ExpandBuiltinDefine( source_t* source, token_t* deftoken, define_t* define, token_t** firsttoken, token_t** lasttoken )
{
	token_t*	  token;
	unsigned long t; //	time_t t; //to prevent LCC warning
	char*		  curtime;

	token = PC_CopyToken( deftoken );

	switch( define->builtin ) {
		case BUILTIN_LINE: {
			sprintf( token->string, "%d", deftoken->line );
#ifdef NUMBERVALUE
			token->intvalue	  = deftoken->line;
			token->floatvalue = deftoken->line;
#endif // NUMBERVALUE
			token->type	   = TT_NUMBER;
			token->subtype = TT_DECIMAL | TT_INTEGER;
			*firsttoken	   = token;
			*lasttoken	   = token;
			break;
		} // end case

		case BUILTIN_FILE: {
			strcpy( token->string, source->scriptstack->filename );
			token->type	   = TT_NAME;
			token->subtype = strlen( token->string );
			*firsttoken	   = token;
			*lasttoken	   = token;
			break;
		} // end case

		case BUILTIN_DATE: {
			t		= time( NULL );
			curtime = ctime( ( const time_t* const )&t );
			strcpy( token->string, "\"" );
			strncat( token->string, curtime + 4, 7 );
			strncat( token->string + 7, curtime + 20, 4 );
			strcat( token->string, "\"" );
			free( curtime );
			token->type	   = TT_NAME;
			token->subtype = strlen( token->string );
			*firsttoken	   = token;
			*lasttoken	   = token;
			break;
		} // end case

		case BUILTIN_TIME: {
			t		= time( NULL );
			curtime = ctime( ( const time_t* const )&t );
			strcpy( token->string, "\"" );
			strncat( token->string, curtime + 11, 8 );
			strcat( token->string, "\"" );
			free( curtime );
			token->type	   = TT_NAME;
			token->subtype = strlen( token->string );
			*firsttoken	   = token;
			*lasttoken	   = token;
			break;
		} // end case

		case BUILTIN_STDC:
		default: {
			*firsttoken = NULL;
			*lasttoken	= NULL;
			break;
		} // end case
	}

	return qtrue;
}

/*!
	\brief Expands a preprocessor define into a linked list of tokens and returns the first and last token of the expansion.

	The function handles both builtin and user‑defined macros. For user macros it first reads any argument lists. It then walks through the macro replacement text, substituting argument tokens and
   handling the stringizing ("#") and token‑pasting ("##") operators. Tokens are copied into a new linked list, which is returned via the firsttoken and lasttoken pointers. If an error occurs, any
   allocated tokens are freed and qfalse is returned. Built‑in macros are delegated to a separate routine. The function does not modify the original source token stream – it only produces the expanded
   token list for further insertion.

	\param source A context describing the source token stream and error handling facilities
	\param deftoken The token where the macro invocation occurs, used mainly for error reporting
	\param define The definition object containing the replacement text and metadata
	\param firsttoken Output pointer set to the first token of the expanded list
	\param lasttoken Output pointer set to the last token of the expanded list
	\return qtrue to indicate successful expansion and list allocation, qfalse if an error was detected or a builtin macro failed to expand.
*/
int PC_ExpandDefine( source_t* source, token_t* deftoken, define_t* define, token_t** firsttoken, token_t** lasttoken )
{
	token_t *parms[MAX_DEFINEPARMS], *dt, *pt, *t;
	token_t *t1, *t2, *first, *last, *nextpt, token;
	int		 parmnum, i;

	// if it is a builtin define
	if( define->builtin ) {
		return PC_ExpandBuiltinDefine( source, deftoken, define, firsttoken, lasttoken );
	}

	// if the define has parameters
	if( define->numparms ) {
		if( !PC_ReadDefineParms( source, define, parms, MAX_DEFINEPARMS ) ) {
			return qfalse;
		}

#ifdef DEBUG_EVAL

		for( i = 0; i < define->numparms; i++ ) {
			Log_Write( "define parms %d:", i );

			for( pt = parms[i]; pt; pt = pt->next ) {
				Log_Write( "%s", pt->string );
			}
		}

#endif // DEBUG_EVAL
	}

	// empty list at first
	first = NULL;
	last  = NULL;

	// create a list with tokens of the expanded define
	for( dt = define->tokens; dt; dt = dt->next ) {
		parmnum = -1;

		// if the token is a name, it could be a define parameter
		if( dt->type == TT_NAME ) {
			parmnum = PC_FindDefineParm( define, dt->string );
		}

		// if it is a define parameter
		if( parmnum >= 0 ) {
			for( pt = parms[parmnum]; pt; pt = pt->next ) {
				t = PC_CopyToken( pt );
				// add the token to the list
				t->next = NULL;

				if( last ) {
					last->next = t;

				} else {
					first = t;
				}

				last = t;
			}

		} else {
			// if stringizing _operator
			if( dt->string[0] == '#' && dt->string[1] == '\0' ) {
				// the stringizing _operator must be followed by a define parameter
				if( dt->next ) {
					parmnum = PC_FindDefineParm( define, dt->next->string );

				} else {
					parmnum = -1;
				}

				//
				if( parmnum >= 0 ) {
					// step over the stringizing _operator
					dt = dt->next;

					// stringize the define parameter tokens
					if( !PC_StringizeTokens( parms[parmnum], &token ) ) {
						SourceError( source, "can't stringize tokens" );
						return qfalse;
					}

					t = PC_CopyToken( &token );

				} else {
					SourceWarning( source, "stringizing _operator without define parameter" );
					continue;
				}

			} else {
				t = PC_CopyToken( dt );
			}

			// add the token to the list
			t->next = NULL;

			if( last ) {
				last->next = t;

			} else {
				first = t;
			}

			last = t;
		}
	}

	// check for the merging _operator
	for( t = first; t; ) {
		if( t->next ) {
			// if the merging _operator
			if( t->next->string[0] == '#' && t->next->string[1] == '#' ) {
				t1 = t;
				t2 = t->next->next;

				if( t2 ) {
					if( !PC_MergeTokens( t1, t2 ) ) {
						SourceError( source, "can't merge %s with %s", t1->string, t2->string );
						return qfalse;
					}

					PC_FreeToken( t1->next );
					t1->next = t2->next;

					if( t2 == last ) {
						last = t1;
					}

					PC_FreeToken( t2 );
					continue;
				}
			}
		}

		t = t->next;
	}

	// store the first and last token of the list
	*firsttoken = first;
	*lasttoken	= last;

	// free all the parameter tokens
	for( i = 0; i < define->numparms; i++ ) {
		for( pt = parms[i]; pt; pt = nextpt ) {
			nextpt = pt->next;
			PC_FreeToken( pt );
		}
	}

	//
	return qtrue;
}

/*!
	\brief Expands a define macro into the source token stream and inserts the resulting tokens at the front of the source's token list.

	The function first calls PC_ExpandDefine to perform the macro expansion, obtaining the first and last tokens of the expanded sequence.  If the expansion succeeds and returns a non‑empty token
   list, the newly created tokens are linked before the current source->tokens list, effectively inserting the macro expansion at the head of the token stream.  If PC_ExpandDefine fails or produces no
   tokens, the function returns false, leaving the source token list unchanged.  The return value is the integer constant qtrue on success and qfalse on failure.

	\param source The source object whose token list will be modified to include the expanded macro tokens.
	\param deftoken The token that triggered the macro expansion; it is passed to PC_ExpandDefine to allow context‑specific handling of the expansion.
	\param define Descriptor of the macro definition that is to be expanded; contains the definition’s body and associated metadata.
	\return qtrue if the macro expansion produced tokens and they were inserted into the source, otherwise qfalse.
*/
int PC_ExpandDefineIntoSource( source_t* source, token_t* deftoken, define_t* define )
{
	token_t *firsttoken, *lasttoken;

	if( !PC_ExpandDefine( source, deftoken, define, &firsttoken, &lasttoken ) ) {
		return qfalse;
	}

	if( firsttoken && lasttoken ) {
		lasttoken->next = source->tokens;
		source->tokens	= firsttoken;
		return qtrue;
	}

	return qfalse;
}

/*!
	\brief Normalizes a file path string by collapsing duplicate separators and converting all slashes to the OS‑specific separator.

	The function iterates over the supplied null‑terminated string, first removing any consecutive slash or backslash characters by moving the remainder of the string one position to the left. After
   all duplicates are eliminated, it replaces every slash or backslash with the value defined by the macro PATHSEPERATOR_CHAR, ensuring the path uses the native separator for the current platform. The
   input is modified in place.

	This routine is intended for use throughout the Wolfenstein source tree wherever user‑supplied or configuration paths need to be rendered compatible with the running operating system.

	\param path Pointer to a mutable null‑terminated C string that will be transformed in place.
*/
void PC_ConvertPath( char* path )
{
	char* ptr;

	// remove double path seperators
	for( ptr = path; *ptr; ) {
		if( ( *ptr == '\\' || *ptr == '/' ) && ( *( ptr + 1 ) == '\\' || *( ptr + 1 ) == '/' ) ) {
			strcpy( ptr, ptr + 1 );

		} else {
			ptr++;
		}
	}

	// set OS dependent path seperators
	for( ptr = path; *ptr; ) {
		if( *ptr == '/' || *ptr == '\\' ) {
			*ptr = PATHSEPERATOR_CHAR;
		}

		ptr++;
	}
}

/*!
	\brief Processes an \#include directive by loading the specified file and adding it to the script stack.

	The function reads the next token after the #include keyword, determining whether the file name is given as a string literal or within angle brackets.  It strips quotes, converts the path, and
   attempts to load the file.  If the file is not found, it reports an error (unless a special build flag allows a warning).  On success the loaded script is pushed onto the source stack so that its
   contents will be parsed next.  The function also honors a skip flag on the source to skip includes when required.

	\param source The current source object from which tokens are read and to which the new script will be added.
	\return Returns qtrue (1) on success or when the source skip flag is set; returns qfalse (0) if the include file cannot be located or another error occurs.
*/
int PC_Directive_include( source_t* source )
{
	script_t* script;
	token_t	  token;
	char	  path[_MAX_PATH];
#ifdef QUAKE
	foundfile_t file;
#endif // QUAKE

	if( source->skip > 0 ) {
		return qtrue;
	}

	//
	if( !PC_ReadSourceToken( source, &token ) ) {
		SourceError( source, "#include without file name" );
		return qfalse;
	}

	if( token.linescrossed > 0 ) {
		SourceError( source, "#include without file name" );
		return qfalse;
	}

	if( token.type == TT_STRING ) {
		StripDoubleQuotes( token.string );
		PC_ConvertPath( token.string );
		script = LoadScriptFile( token.string );

		if( !script ) {
			strcpy( path, source->includepath );
			strcat( path, token.string );
			script = LoadScriptFile( path );
		}

	} else if( token.type == TT_PUNCTUATION && *token.string == '<' ) {
		strcpy( path, source->includepath );

		while( PC_ReadSourceToken( source, &token ) ) {
			if( token.linescrossed > 0 ) {
				PC_UnreadSourceToken( source, &token );
				break;
			}

			if( token.type == TT_PUNCTUATION && *token.string == '>' ) {
				break;
			}

			strncat( path, token.string, _MAX_PATH );
		}

		if( *token.string != '>' ) {
			SourceWarning( source, "#include missing trailing >" );
		}

		if( !strlen( path ) ) {
			SourceError( source, "#include without file name between < >" );
			return qfalse;
		}

		PC_ConvertPath( path );
		script = LoadScriptFile( path );

	} else {
		SourceError( source, "#include without file name" );
		return qfalse;
	}

#ifdef QUAKE

	if( !script ) {
		memset( &file, 0, sizeof( foundfile_t ) );
		script = LoadScriptFile( path );

		if( script ) {
			strncpy( script->filename, path, _MAX_PATH );
		}
	}

#endif // QUAKE

	if( !script ) {
#ifdef SCREWUP
		SourceWarning( source, "file %s not found", path );
		return qtrue;
#else
		SourceError( source, "file %s not found", path );
		return qfalse;
#endif // SCREWUP
	}

	PC_PushScript( source, script );
	return qtrue;
}

/*!
	\brief Reads a line of tokens from the source and returns true if successful.

	This function reads tokens from the provided source until it reaches the end of a line or encounters an error. It handles token crossing across lines and ensures that only tokens from the same line are read. The function returns false if an error occurs or if a token crosses multiple lines, otherwise it returns true.

	\param source Pointer to the source structure containing the token stream to read from.
	\param token Pointer to the token structure where the read token will be stored.
	\return Returns qtrue if the line was read successfully, or qfalse if an error occurred or if a token crossed multiple lines.
*/
int PC_ReadLine( source_t* source, token_t* token )
{
	int crossline;

	crossline = 0;

	do {
		if( !PC_ReadSourceToken( source, token ) ) {
			return qfalse;
		}

		if( token->linescrossed > crossline ) {
			PC_UnreadSourceToken( source, token );
			return qfalse;
		}

		crossline = 1;
	} while( !strcmp( token->string, "\\" ) );

	return qtrue;
}

/*!
	\brief Checks if whitespace exists before a token

	This function determines whether there is whitespace preceding a given token by comparing the positions of the token's whitespace and end whitespace pointers. It returns a non-zero value if there is whitespace before the token, and zero if there is not. This is used in preprocessing to handle token separation and define parameter parsing.

	\param token Pointer to the token structure to check for preceding whitespace
	\return Integer value indicating whether whitespace exists before the token (non-zero if whitespace exists, zero otherwise)
*/
int PC_WhiteSpaceBeforeToken( token_t* token )
{
	return token->endwhitespace_p - token->whitespace_p > 0;
}

/*!
	\brief Reset the whitespace information stored in a token

	The function clears the whitespace metadata of a token by setting both the start and end whitespace pointers to null and resetting the line count cross counter to zero. This prepares the token for
   fresh processing without any residual whitespace state.

	\param token Pointer to the token whose whitespace data should be cleared.
*/
void PC_ClearTokenWhiteSpace( token_t* token )
{
	token->whitespace_p	   = NULL;
	token->endwhitespace_p = NULL;
	token->linescrossed	   = 0;
}

/*!
	\brief Processes a #undef directive by removing the specified definition from the source's define table.

	When invoked, the function first checks if the source is currently skipping content; if so it reports success immediately. It then reads the next token and expects it to be a name; otherwise it
   signals an error. Using either a hashed or linear search depending on compile time configuration, it looks for a define entry with that name. If found and marked as a fixed (i.e., cannot be
   altered) the function issues a warning; otherwise the entry is unlinked from the list and its memory deallocated. If no matching define exists the function simply returns a success code. The
   routine always signals success unless it encounters a read or validation failure.

	\param source The parsing context containing the define list and related flags.
	\return Non‑zero if the undef operation succeeded; zero if an error such as a missing name was encountered.
*/
int PC_Directive_undef( source_t* source )
{
	token_t	  token;
	define_t *define, *lastdefine;
	int		  hash;

	if( source->skip > 0 ) {
		return qtrue;
	}

	//
	if( !PC_ReadLine( source, &token ) ) {
		SourceError( source, "undef without name" );
		return qfalse;
	}

	if( token.type != TT_NAME ) {
		PC_UnreadSourceToken( source, &token );
		SourceError( source, "expected name, found %s", token.string );
		return qfalse;
	}

#if DEFINEHASHING

	hash = PC_NameHash( token.string );

	for( lastdefine = NULL, define = source->definehash[hash]; define; define = define->hashnext ) {
		if( !strcmp( define->name, token.string ) ) {
			if( define->flags & DEFINE_FIXED ) {
				SourceWarning( source, "can't undef %s", token.string );

			} else {
				if( lastdefine ) {
					lastdefine->hashnext = define->hashnext;

				} else {
					source->definehash[hash] = define->hashnext;
				}

				PC_FreeDefine( define );
			}

			break;
		}

		lastdefine = define;
	}

#else // DEFINEHASHING

	for( lastdefine = NULL, define = source->defines; define; define = define->next ) {
		if( !strcmp( define->name, token.string ) ) {
			if( define->flags & DEFINE_FIXED ) {
				SourceWarning( source, "can't undef %s", token.string );

			} else {
				if( lastdefine ) {
					lastdefine->next = define->next;

				} else {
					source->defines = define->next;
				}

				PC_FreeDefine( define );
			}

			break;
		}

		lastdefine = define;
	}

#endif // DEFINEHASHING
	return qtrue;
}

/*!
	\brief Parses a #define directive from a script source, creating or replacing a definition in the source's define table.

	If the source's skip counter is positive the directive is ignored and the function immediately reports success. The function reads the token following #define, expecting a token of type TT_NAME;
   failure to provide a proper name results in a source error and failure. It checks for an existing definition with the same name; if the existing definition is not fixed the function warns, unmasks
   the old definition by invoking PC_Directive_undef, and then proceeds. A new define_t structure is allocated, its name field populated, and it is inserted into the source's define list or hash. The
   function then reads the remaining line to see whether the directive contains parameters, indicated by an opening parenthesis. If parameters are present it parses a comma‑separated name list,
   rejecting duplicates, and stores each as a token linked to the define's parms list. After the parameter list, the body tokens are read until the end of the line, copying each token into the
   define's tokens list while checking for recursive definitions and misplaced token‑pasting operators (##). Any parsing error causes a source error and a failure result. On success, the newly defined
   structure is fully populated and the function returns success.

	\param source Pointer to a source_t structure representing the script being parsed, whose define table will be modified.
	\return Returns non‑zero (true) on success and zero (false) on failure.
*/
int PC_Directive_define( source_t* source )
{
	token_t	  token, *t, *last;
	define_t* define;

	if( source->skip > 0 ) {
		return qtrue;
	}

	//
	if( !PC_ReadLine( source, &token ) ) {
		SourceError( source, "#define without name" );
		return qfalse;
	}

	if( token.type != TT_NAME ) {
		PC_UnreadSourceToken( source, &token );
		SourceError( source, "expected name after #define, found %s", token.string );
		return qfalse;
	}

	// check if the define already exists
#if DEFINEHASHING
	define = PC_FindHashedDefine( source->definehash, token.string );
#else
	define = PC_FindDefine( source->defines, token.string );
#endif // DEFINEHASHING

	if( define ) {
		if( define->flags & DEFINE_FIXED ) {
			SourceError( source, "can't redefine %s", token.string );
			return qfalse;
		}

		SourceWarning( source, "redefinition of %s", token.string );
		// unread the define name before executing the #undef directive
		PC_UnreadSourceToken( source, &token );

		if( !PC_Directive_undef( source ) ) {
			return qfalse;
		}

		// if the define was not removed (define->flags & DEFINE_FIXED)
#if DEFINEHASHING
		define = PC_FindHashedDefine( source->definehash, token.string );
#else
		define = PC_FindDefine( source->defines, token.string );
#endif // DEFINEHASHING
	}

	// allocate define
	define = ( define_t* )GetMemory( sizeof( define_t ) + strlen( token.string ) + 1 );
	memset( define, 0, sizeof( define_t ) );
	define->name = ( char* )define + sizeof( define_t );
	strcpy( define->name, token.string );
	// add the define to the source
#if DEFINEHASHING
	PC_AddDefineToHash( define, source->definehash );
#else  // DEFINEHASHING
	define->next	= source->defines;
	source->defines = define;
#endif // DEFINEHASHING

	// if nothing is defined, just return
	if( !PC_ReadLine( source, &token ) ) {
		return qtrue;
	}

	// if it is a define with parameters
	if( !PC_WhiteSpaceBeforeToken( &token ) && !strcmp( token.string, "(" ) ) {
		// read the define parameters
		last = NULL;

		if( !PC_CheckTokenString( source, ")" ) ) {
			while( 1 ) {
				if( !PC_ReadLine( source, &token ) ) {
					SourceError( source, "expected define parameter" );
					return qfalse;
				}

				// if it isn't a name
				if( token.type != TT_NAME ) {
					SourceError( source, "invalid define parameter" );
					return qfalse;
				}

				//
				if( PC_FindDefineParm( define, token.string ) >= 0 ) {
					SourceError( source, "two the same define parameters" );
					return qfalse;
				}

				// add the define parm
				t = PC_CopyToken( &token );
				PC_ClearTokenWhiteSpace( t );
				t->next = NULL;

				if( last ) {
					last->next = t;

				} else {
					define->parms = t;
				}

				last = t;
				define->numparms++;

				// read next token
				if( !PC_ReadLine( source, &token ) ) {
					SourceError( source, "define parameters not terminated" );
					return qfalse;
				}

				//
				if( !strcmp( token.string, ")" ) ) {
					break;
				}

				// then it must be a comma
				if( strcmp( token.string, "," ) ) {
					SourceError( source, "define not terminated" );
					return qfalse;
				}
			}
		}

		if( !PC_ReadLine( source, &token ) ) {
			return qtrue;
		}
	}

	// read the defined stuff
	last = NULL;

	do {
		t = PC_CopyToken( &token );

		if( t->type == TT_NAME && !strcmp( t->string, define->name ) ) {
			SourceError( source, "recursive define (removed recursion)" );
			continue;
		}

		PC_ClearTokenWhiteSpace( t );
		t->next = NULL;

		if( last ) {
			last->next = t;

		} else {
			define->tokens = t;
		}

		last = t;
	} while( PC_ReadLine( source, &token ) );

	//
	if( last ) {
		// check for merge operators at the beginning or end
		if( !strcmp( define->tokens->string, "##" ) || !strcmp( last->string, "##" ) ) {
			SourceError( source, "define with misplaced ##" );
			return qfalse;
		}
	}

	return qtrue;
}

/*!
	\brief Parse a null-terminated string that contains a preprocessor directive and return the define created from it

	The function initializes the token heap, loads the supplied source text into a temporary script buffer with a placeholder filename, and then calls PC_Directive_define to interpret the directive.
   After parsing, any tokens that were not consumed are freed and the resulting define is extracted from the temporary source structure or from a hash table if hashing is enabled. The temporary script
   buffer and associated memory are then released. If the parse was successful the newly created define is returned; otherwise any partially created define is released and a null pointer is returned.

	\param string the source text containing a #define directive to parse
	\return a pointer to the created define_t structure, or NULL if parsing failed
*/
define_t* PC_DefineFromString( char* string )
{
	script_t* script;
	source_t  src;
	token_t*  t;
	int		  res, i;
	define_t* def;

	PC_InitTokenHeap();

	script = LoadScriptMemory( string, strlen( string ), "*extern" );
	// create a new source
	memset( &src, 0, sizeof( source_t ) );
	strncpy( src.filename, "*extern", _MAX_PATH );
	src.scriptstack = script;
#if DEFINEHASHING
	src.definehash = ( define_t** )GetClearedMemory( DEFINEHASHSIZE * sizeof( define_t* ) );
#endif // DEFINEHASHING
	// create a define from the source
	res = PC_Directive_define( &src );

	// free any tokens if left
	for( t = src.tokens; t; t = src.tokens ) {
		src.tokens = src.tokens->next;
		PC_FreeToken( t );
	}

#ifdef DEFINEHASHING
	def = NULL;

	for( i = 0; i < DEFINEHASHSIZE; i++ ) {
		if( src.definehash[i] ) {
			def = src.definehash[i];
			break;
		}
	}

#else
	def = src.defines;
#endif // DEFINEHASHING
	   //
#if DEFINEHASHING
	FreeMemory( src.definehash );
#endif // DEFINEHASHING
	//
	FreeScript( script );

	// if the define was created succesfully
	if( res > 0 ) {
		return def;
	}

	// free the define if created
	if( src.defines ) {
		PC_FreeDefine( def );
	}

	//
	return NULL;
}

/*!
	\brief Adds a preprocessor define from a string to the given source

	This function parses a string to create a preprocessor define and adds it to the specified source. The define is added to either a hash table or a linked list depending on the DEFINEHASHING compile-time flag. If the string cannot be parsed into a valid define, the function returns qfalse. Otherwise, it returns qtrue to indicate successful addition.

	\param source Pointer to the source structure where the define will be added
	\param string String representation of the define to be added
	\return qtrue if the define was successfully added, qfalse if the string could not be parsed into a valid define
*/
int PC_AddDefine( source_t* source, char* string )
{
	define_t* define;

	define = PC_DefineFromString( string );

	if( !define ) {
		return qfalse;
	}

#if DEFINEHASHING
	PC_AddDefineToHash( define, source->definehash );
#else  // DEFINEHASHING
	define->next	= source->defines;
	source->defines = define;
#endif // DEFINEHASHING
	return qtrue;
}

/*!
	\brief Adds a global define from a string to the parser's global define list

	This function parses a string to create a define object and adds it to the beginning of the global defines list. The string is expected to contain a valid define definition. If the string cannot be parsed into a valid define, the function returns false. Otherwise, it successfully adds the define to the global list and returns true.

	\param string A null-terminated string containing the define definition to be added
	\return 1 if the define was successfully added, 0 if the string could not be parsed into a valid define
*/
int PC_AddGlobalDefine( char* string )
{
	define_t* define;

	define = PC_DefineFromString( string );

	if( !define ) {
		return qfalse;
	}

	define->next  = globaldefines;
	globaldefines = define;
	return qtrue;
}

/*!
	\brief Removes a global define with the specified name from the global defines list

	This function searches for a global define with the given name and removes it if found. It returns true if the define was successfully removed, or false if no define with the specified name exists

	\param name the name of the global define to remove
	\return 1 if the global define was successfully removed, 0 if no define with the specified name was found
*/
int PC_RemoveGlobalDefine( char* name )
{
	define_t* define;

	define = PC_FindDefine( globaldefines, name );

	if( define ) {
		PC_FreeDefine( define );
		return qtrue;
	}

	return qfalse;
}

/*!
	\brief Removes and deallocates all entries in the global defines linked list.

	Iterates through the globaldefines list, unlinking each node and calling PC_FreeDefine to release the memory for each entry.

*/
void PC_RemoveAllGlobalDefines()
{
	define_t* define;

	for( define = globaldefines; define; define = globaldefines ) {
		globaldefines = globaldefines->next;
		PC_FreeDefine( define );
	}
}

/*!
	\brief Creates a deep copy of a define structure, including its name, flags, tokens, and parameters.

	The function allocates a new define_t object with enough space to store the define name. It copies the name string, flag values, builtin flag, and number of parameters from the original define.
   The new define is left unlinked and its hashnext field is cleared. It then iterates through the token list of the original define, using PC_CopyToken to duplicate each token and links them into a
   new list. The same process is performed for the define’s parameter list.

	The source parameter is present for context compatibility but is not used by the current implementation. The resulting define_t structure is returned to the caller.

	\param source Pointer to the current source context; currently unused.
	\param define The define structure to be duplicated.
	\return Pointer to the newly allocated and fully duplicated define_t structure.
*/
define_t* PC_CopyDefine( source_t* source, define_t* define )
{
	define_t* newdefine;
	token_t * token, *newtoken, *lasttoken;

	newdefine = ( define_t* )GetMemory( sizeof( define_t ) + strlen( define->name ) + 1 );
	// copy the define name
	newdefine->name = ( char* )newdefine + sizeof( define_t );
	strcpy( newdefine->name, define->name );
	newdefine->flags	= define->flags;
	newdefine->builtin	= define->builtin;
	newdefine->numparms = define->numparms;
	// the define is not linked
	newdefine->next		= NULL;
	newdefine->hashnext = NULL;
	// copy the define tokens
	newdefine->tokens = NULL;

	for( lasttoken = NULL, token = define->tokens; token; token = token->next ) {
		newtoken	   = PC_CopyToken( token );
		newtoken->next = NULL;

		if( lasttoken ) {
			lasttoken->next = newtoken;

		} else {
			newdefine->tokens = newtoken;
		}

		lasttoken = newtoken;
	}

	// copy the define parameters
	newdefine->parms = NULL;

	for( lasttoken = NULL, token = define->parms; token; token = token->next ) {
		newtoken	   = PC_CopyToken( token );
		newtoken->next = NULL;

		if( lasttoken ) {
			lasttoken->next = newtoken;

		} else {
			newdefine->parms = newtoken;
		}

		lasttoken = newtoken;
	}

	return newdefine;
}

/*!
	\brief Copies every global preprocessor define into the specified source’s list of defines.

	The function iterates over the global list of preprocessor definitions, duplicates each definition for the target source using PC_CopyDefine, and then inserts the copy into the source’s define
   list or hash table, depending on whether DEFINEHASHING is enabled. This ensures that all globally defined macros are available during preprocessing of the individual source file.

	\param source Pointer to the source_t structure to which the global defines will be attached.
*/
void PC_AddGlobalDefinesToSource( source_t* source )
{
	define_t *define, *newdefine;

	for( define = globaldefines; define; define = define->next ) {
		newdefine = PC_CopyDefine( source, define );
#if DEFINEHASHING
		PC_AddDefineToHash( newdefine, source->definehash );
#else  // DEFINEHASHING
		newdefine->next = source->defines;
		source->defines = newdefine;
#endif // DEFINEHASHING
	}
}

/*!
	\brief Handles an #ifdef or #ifndef directive by parsing the following name token, checking if the token names a defined symbol, and updating the source's indentation state accordingly.

	The function first reads a line from the source and expects a name token; if the token is missing or not a name, an error is recorded. It then looks up the name in the source's define table,
   optionally using a hash. Based on whether the directive type is INDENT_IFDEF or INDENT_IFNDEF and whether the define was found, it computes a skip flag and pushes an indentation record onto the
   source's stack. The function returns true (qtrue) when the directive is processed successfully, otherwise false (qfalse).

	\param source Pointer to the current source context being parsed
	\param type An integer constant indicating the directive type, either INDENT_IFDEF or INDENT_IFNDEF
	\return Integer status: true (qtrue) if the directive was processed successfully, false (qfalse) on error.
*/
int PC_Directive_if_def( source_t* source, int type )
{
	token_t	  token;
	define_t* d;
	int		  skip;

	if( !PC_ReadLine( source, &token ) ) {
		SourceError( source, "#ifdef without name" );
		return qfalse;
	}

	if( token.type != TT_NAME ) {
		PC_UnreadSourceToken( source, &token );
		SourceError( source, "expected name after #ifdef, found %s", token.string );
		return qfalse;
	}

#if DEFINEHASHING
	d = PC_FindHashedDefine( source->definehash, token.string );
#else
	d = PC_FindDefine( source->defines, token.string );
#endif // DEFINEHASHING
	skip = ( type == INDENT_IFDEF ) == ( d == NULL );
	PC_PushIndent( source, type, skip );
	return qtrue;
}

/*!
	\brief Processes a conditional #ifdef directive in the source

	This function forwards the handling to PC_Directive_if_def, passing an indentation flag that denotes an #ifdef construct. The outcome returned reflects whether the directive was processed
   successfully.

	\param source Pointer to the source context being parsed
	\return Integer result code from PC_Directive_if_def; typically 0 indicates success, non‑zero signals an error.
*/
int PC_Directive_ifdef( source_t* source )
{
	return PC_Directive_if_def( source, INDENT_IFDEF );
}

/*!
	\brief Handles an #ifndef directive by delegating to a generic if/definition handler.

	Invokes PC_Directive_if_def with the current source and the INDENT_IFNDEF flag, returning its result.

	\param source Pointer to the source structure representing the current parser state.
	\return Integer status code provided by PC_Directive_if_def.
*/
int PC_Directive_ifndef( source_t* source )
{
	return PC_Directive_if_def( source, INDENT_IFNDEF );
}

/*!
	\brief Handles the preprocessing #else directive by adjusting the indentation stack and reporting errors for misplaced or duplicate #else directives.

	The function first pops the most recent indentation entry from the source stack. If no entry was present, it reports a misplaced #else error. If the entry was already an #else indentation, it
   reports a duplicate #else error. Otherwise, it pushes a new indentation entry of type INDENT_ELSE, with the skip flag inverted from the popped entry. It returns qtrue on success or qfalse when an
   error occurs.

	\param source The source context for the preprocessor operation.
	\return qtrue if the #else was processed successfully, qfalse otherwise.
*/
int PC_Directive_else( source_t* source )
{
	int type, skip;

	PC_PopIndent( source, &type, &skip );

	if( !type ) {
		SourceError( source, "misplaced #else" );
		return qfalse;
	}

	if( type == INDENT_ELSE ) {
		SourceError( source, "#else after #else" );
		return qfalse;
	}

	PC_PushIndent( source, INDENT_ELSE, !skip );
	return qtrue;
}

/*!
	\brief Ends an #if block by popping the indentation state and reports an error if there is no matching start.

	Invokes the stack-pop routine to remove the most recent indentation level. If the pop returns an empty type, a misplaced #endif is reported via SourceError. The function returns a boolean value,
   true if a matching start was found, false otherwise.

	\param source pointer to the source context being processed
	\return indicates whether an #endif was valid (true) or misplaced (false)
*/
int PC_Directive_endif( source_t* source )
{
	int type, skip;

	PC_PopIndent( source, &type, &skip );

	if( !type ) {
		SourceError( source, "misplaced #endif" );
		return qfalse;
	}

	return qtrue;
}

typedef struct operator_s {
	int				   _operator;
	int				   priority;
	int				   parentheses;
	struct operator_s *prev, *next;
} operator_t;

typedef struct value_s {
	signed long int intvalue;
	double			floatvalue;
	int				parentheses;
	struct value_s *prev, *next;
} value_t;

/*!
	\brief Returns the precedence priority for a given operator code.

	The function maps operator subtype constants to integer precedence levels used when parsing expressions. For example, multiplication and division have a priority of 15, addition and subtraction
   14, logical and bitwise operators have varying priorities, and conditional operators return 5.

	If an operator code does not match any known case the function returns zero (qfalse).

	\param op operator subtype code
	\return integer priority value of the operator, or zero if unknown
*/
int PC_OperatorPriority( int op )
{
	switch( op ) {
		case P_MUL:
			return 15;

		case P_DIV:
			return 15;

		case P_MOD:
			return 15;

		case P_ADD:
			return 14;

		case P_SUB:
			return 14;

		case P_LOGIC_AND:
			return 7;

		case P_LOGIC_OR:
			return 6;

		case P_LOGIC_GEQ:
			return 12;

		case P_LOGIC_LEQ:
			return 12;

		case P_LOGIC_EQ:
			return 11;

		case P_LOGIC_UNEQ:
			return 11;

		case P_LOGIC_NOT:
			return 16;

		case P_LOGIC_GREATER:
			return 12;

		case P_LOGIC_LESS:
			return 12;

		case P_RSHIFT:
			return 13;

		case P_LSHIFT:
			return 13;

		case P_BIN_AND:
			return 10;

		case P_BIN_OR:
			return 8;

		case P_BIN_XOR:
			return 9;

		case P_BIN_NOT:
			return 16;

		case P_COLON:
			return 5;

		case P_QUESTIONMARK:
			return 5;
	}

	return qfalse;
}

// #define AllocValue()			GetClearedMemory(sizeof(value_t));
// #define FreeValue(val)		FreeMemory(val)
// #define AllocOperator(op)		op = (operator_t *) GetClearedMemory(sizeof(operator_t));
// #define FreeOperator(op)		FreeMemory(op);

#define MAX_VALUES	  64
#define MAX_OPERATORS 64
#define AllocValue( val )                              \
	if( numvalues >= MAX_VALUES ) {                    \
		SourceError( source, "out of value space\n" ); \
		error = 1;                                     \
		break;                                         \
	} else {                                           \
		val = &value_heap[numvalues++];                \
	}
#define FreeValue( val )
//
#define AllocOperator( op )                                \
	if( numoperators >= MAX_OPERATORS ) {                  \
		SourceError( source, "out of _operator space\n" ); \
		error = 1;                                         \
		break;                                             \
	} else {                                               \
		op = &operator_heap[numoperators++];               \
	}
#define FreeOperator( op )

/*!
	\brief Evaluates a list of preprocessor tokens as an expression, storing the integer or floating result.

	The function walks a linked list of tokens that represent a #if or #elif expression. It builds a value and operator chain while performing syntax checks such as matching parentheses, correct
   placement of operators, and preventing disallowed operations on floating‑point operands. The special token "defined" is handled by looking up symbols in the source’s define table. After the token
   list is processed, the expression is evaluated using standard operator precedence and the result is written to the supplied intvalue or floatvalue pointer. When "integer" is non‑zero the expression
   is interpreted as an integer expression; otherwise floating‑point operators are permitted. The function returns 1 on successful evaluation and 0 if a syntax error was detected or evaluation could
   not be completed.

	\param source pointer to the source context used for error reporting
	\param tokens head of a linked list of token_t describing the expression to evaluate
	\param intvalue pointer to store the integer result, set to 0 if NULL
	\param floatvalue pointer to store the floating‑point result, set to 0.0 if NULL
	\param integer flag indicating whether to evaluate as an integer expression (non‑zero) or allow floating point operations
	\return 1 if the expression was evaluated successfully; 0 on error.
*/
int PC_EvaluateTokens( source_t* source, token_t* tokens, signed long int* intvalue, double* floatvalue, int integer )
{
	operator_t *o, *firstoperator, *lastoperator;
	value_t *	v, *firstvalue, *lastvalue, *v1, *v2;
	token_t*	t;
	int			brace				= 0;
	int			parentheses			= 0;
	int			error				= 0;
	int			lastwasvalue		= 0;
	int			negativevalue		= 0;
	int			questmarkintvalue	= 0;
	double		questmarkfloatvalue = 0;
	int			gotquestmarkvalue	= qfalse;
	int			lastoperatortype	= 0;
	//
	operator_t	operator_heap[MAX_OPERATORS];
	int			numoperators = 0;
	value_t		value_heap[MAX_VALUES];
	int			numvalues = 0;

	firstoperator = lastoperator = NULL;
	firstvalue = lastvalue = NULL;

	if( intvalue ) {
		*intvalue = 0;
	}

	if( floatvalue ) {
		*floatvalue = 0;
	}

	for( t = tokens; t; t = t->next ) {
		switch( t->type ) {
			case TT_NAME: {
				if( lastwasvalue || negativevalue ) {
					SourceError( source, "syntax error in #if/#elif" );
					error = 1;
					break;
				}

				if( strcmp( t->string, "defined" ) ) {
					SourceError( source, "undefined name %s in #if/#elif", t->string );
					error = 1;
					break;
				}

				t = t->next;

				if( !strcmp( t->string, "(" ) ) {
					brace = qtrue;
					t	  = t->next;
				}

				if( !t || t->type != TT_NAME ) {
					SourceError( source, "defined without name in #if/#elif" );
					error = 1;
					break;
				}

				// v = (value_t *) GetClearedMemory(sizeof(value_t));
				AllocValue( v );
#if DEFINEHASHING

				if( PC_FindHashedDefine( source->definehash, t->string ) )
#else
				if( PC_FindDefine( source->defines, t->string ) )
#endif // DEFINEHASHING
				{
					v->intvalue	  = 1;
					v->floatvalue = 1;

				} else {
					v->intvalue	  = 0;
					v->floatvalue = 0;
				}

				v->parentheses = parentheses;
				v->next		   = NULL;
				v->prev		   = lastvalue;

				if( lastvalue ) {
					lastvalue->next = v;

				} else {
					firstvalue = v;
				}

				lastvalue = v;

				if( brace ) {
					t = t->next;

					if( !t || strcmp( t->string, ")" ) ) {
						SourceError( source, "defined without ) in #if/#elif" );
						error = 1;
						break;
					}
				}

				brace = qfalse;
				// defined() creates a value
				lastwasvalue = 1;
				break;
			} // end case

			case TT_NUMBER: {
				if( lastwasvalue ) {
					SourceError( source, "syntax error in #if/#elif" );
					error = 1;
					break;
				}

				// v = (value_t *) GetClearedMemory(sizeof(value_t));
				AllocValue( v );

				if( negativevalue ) {
					v->intvalue	  = -( signed int )t->intvalue;
					v->floatvalue = -t->floatvalue;

				} else {
					v->intvalue	  = t->intvalue;
					v->floatvalue = t->floatvalue;
				}

				v->parentheses = parentheses;
				v->next		   = NULL;
				v->prev		   = lastvalue;

				if( lastvalue ) {
					lastvalue->next = v;

				} else {
					firstvalue = v;
				}

				lastvalue = v;
				// last token was a value
				lastwasvalue = 1;
				//
				negativevalue = 0;
				break;
			} // end case

			case TT_PUNCTUATION: {
				if( negativevalue ) {
					SourceError( source, "misplaced minus sign in #if/#elif" );
					error = 1;
					break;
				}

				if( t->subtype == P_PARENTHESESOPEN ) {
					parentheses++;
					break;

				} else if( t->subtype == P_PARENTHESESCLOSE ) {
					parentheses--;

					if( parentheses < 0 ) {
						SourceError( source, "too many ) in #if/#elsif" );
						error = 1;
					}

					break;
				}

				// check for invalid operators on floating point values
				if( !integer ) {
					if( t->subtype == P_BIN_NOT || t->subtype == P_MOD || t->subtype == P_RSHIFT || t->subtype == P_LSHIFT || t->subtype == P_BIN_AND || t->subtype == P_BIN_OR ||
						t->subtype == P_BIN_XOR ) {
						SourceError( source, "illigal _operator %s on floating point operands\n", t->string );
						error = 1;
						break;
					}
				}

				switch( t->subtype ) {
					case P_LOGIC_NOT:
					case P_BIN_NOT: {
						if( lastwasvalue ) {
							SourceError( source, "! or ~ after value in #if/#elif" );
							error = 1;
							break;
						}

						break;
					} // end case

					case P_INC:
					case P_DEC: {
						SourceError( source, "++ or -- used in #if/#elif" );
						break;
					} // end case

					case P_SUB: {
						if( !lastwasvalue ) {
							negativevalue = 1;
							break;
						}
					} // end case

					case P_MUL:
					case P_DIV:
					case P_MOD:
					case P_ADD:

					case P_LOGIC_AND:
					case P_LOGIC_OR:
					case P_LOGIC_GEQ:
					case P_LOGIC_LEQ:
					case P_LOGIC_EQ:
					case P_LOGIC_UNEQ:

					case P_LOGIC_GREATER:
					case P_LOGIC_LESS:

					case P_RSHIFT:
					case P_LSHIFT:

					case P_BIN_AND:
					case P_BIN_OR:
					case P_BIN_XOR:

					case P_COLON:
					case P_QUESTIONMARK: {
						if( !lastwasvalue ) {
							SourceError( source, "_operator %s after _operator in #if/#elif", t->string );
							error = 1;
							break;
						}

						break;
					} // end case

					default: {
						SourceError( source, "invalid _operator %s in #if/#elif", t->string );
						error = 1;
						break;
					} // end default
				}

				if( !error && !negativevalue ) {
					// o = (operator_t *) GetClearedMemory(sizeof(operator_t));
					AllocOperator( o );
					o->_operator   = t->subtype;
					o->priority	   = PC_OperatorPriority( t->subtype );
					o->parentheses = parentheses;
					o->next		   = NULL;
					o->prev		   = lastoperator;

					if( lastoperator ) {
						lastoperator->next = o;

					} else {
						firstoperator = o;
					}

					lastoperator = o;
					lastwasvalue = 0;
				}

				break;
			} // end case

			default: {
				SourceError( source, "unknown %s in #if/#elif", t->string );
				error = 1;
				break;
			} // end default
		}

		if( error ) {
			break;
		}
	}

	if( !error ) {
		if( !lastwasvalue ) {
			SourceError( source, "trailing _operator in #if/#elif" );
			error = 1;

		} else if( parentheses ) {
			SourceError( source, "too many ( in #if/#elif" );
			error = 1;
		}
	}

	//
	gotquestmarkvalue	= qfalse;
	questmarkintvalue	= 0;
	questmarkfloatvalue = 0;

	// while there are operators
	while( !error && firstoperator ) {
		v = firstvalue;

		for( o = firstoperator; o->next; o = o->next ) {
			// if the current _operator is nested deeper in parentheses
			// than the next _operator
			if( o->parentheses > o->next->parentheses ) {
				break;
			}

			// if the current and next _operator are nested equally deep in parentheses
			if( o->parentheses == o->next->parentheses ) {
				// if the priority of the current _operator is equal or higher
				// than the priority of the next _operator
				if( o->priority >= o->next->priority ) {
					break;
				}
			}

			// if the arity of the _operator isn't equal to 1
			if( o->_operator != P_LOGIC_NOT && o->_operator != P_BIN_NOT ) {
				v = v->next;
			}

			// if there's no value or no next value
			if( !v ) {
				SourceError( source, "mising values in #if/#elif" );
				error = 1;
				break;
			}
		}

		if( error ) {
			break;
		}

		v1 = v;
		v2 = v->next;
#ifdef DEBUG_EVAL

		if( integer ) {
			Log_Write( "_operator %s, value1 = %d", PunctuationFromNum( source->scriptstack, o->_operator ), v1->intvalue );

			if( v2 ) {
				Log_Write( "value2 = %d", v2->intvalue );
			}

		} else {
			Log_Write( "_operator %s, value1 = %f", PunctuationFromNum( source->scriptstack, o->_operator ), v1->floatvalue );

			if( v2 ) {
				Log_Write( "value2 = %f", v2->floatvalue );
			}
		}

#endif // DEBUG_EVAL

		switch( o->_operator ) {
			case P_LOGIC_NOT:
				v1->intvalue   = !v1->intvalue;
				v1->floatvalue = !v1->floatvalue;
				break;

			case P_BIN_NOT:
				v1->intvalue = ~v1->intvalue;
				break;

			case P_MUL:
				v1->intvalue *= v2->intvalue;
				v1->floatvalue *= v2->floatvalue;
				break;

			case P_DIV:
				if( !v2->intvalue || !v2->floatvalue ) {
					SourceError( source, "divide by zero in #if/#elif\n" );
					error = 1;
					break;
				}

				v1->intvalue /= v2->intvalue;
				v1->floatvalue /= v2->floatvalue;
				break;

			case P_MOD:
				if( !v2->intvalue ) {
					SourceError( source, "divide by zero in #if/#elif\n" );
					error = 1;
					break;
				}

				v1->intvalue %= v2->intvalue;
				break;

			case P_ADD:
				v1->intvalue += v2->intvalue;
				v1->floatvalue += v2->floatvalue;
				break;

			case P_SUB:
				v1->intvalue -= v2->intvalue;
				v1->floatvalue -= v2->floatvalue;
				break;

			case P_LOGIC_AND:
				v1->intvalue   = v1->intvalue && v2->intvalue;
				v1->floatvalue = v1->floatvalue && v2->floatvalue;
				break;

			case P_LOGIC_OR:
				v1->intvalue   = v1->intvalue || v2->intvalue;
				v1->floatvalue = v1->floatvalue || v2->floatvalue;
				break;

			case P_LOGIC_GEQ:
				v1->intvalue   = v1->intvalue >= v2->intvalue;
				v1->floatvalue = v1->floatvalue >= v2->floatvalue;
				break;

			case P_LOGIC_LEQ:
				v1->intvalue   = v1->intvalue <= v2->intvalue;
				v1->floatvalue = v1->floatvalue <= v2->floatvalue;
				break;

			case P_LOGIC_EQ:
				v1->intvalue   = v1->intvalue == v2->intvalue;
				v1->floatvalue = v1->floatvalue == v2->floatvalue;
				break;

			case P_LOGIC_UNEQ:
				v1->intvalue   = v1->intvalue != v2->intvalue;
				v1->floatvalue = v1->floatvalue != v2->floatvalue;
				break;

			case P_LOGIC_GREATER:
				v1->intvalue   = v1->intvalue > v2->intvalue;
				v1->floatvalue = v1->floatvalue > v2->floatvalue;
				break;

			case P_LOGIC_LESS:
				v1->intvalue   = v1->intvalue < v2->intvalue;
				v1->floatvalue = v1->floatvalue < v2->floatvalue;
				break;

			case P_RSHIFT:
				v1->intvalue >>= v2->intvalue;
				break;

			case P_LSHIFT:
				v1->intvalue <<= v2->intvalue;
				break;

			case P_BIN_AND:
				v1->intvalue &= v2->intvalue;
				break;

			case P_BIN_OR:
				v1->intvalue |= v2->intvalue;
				break;

			case P_BIN_XOR:
				v1->intvalue ^= v2->intvalue;
				break;

			case P_COLON: {
				if( !gotquestmarkvalue ) {
					SourceError( source, ": without ? in #if/#elif" );
					error = 1;
					break;
				}

				if( integer ) {
					if( !questmarkintvalue ) {
						v1->intvalue = v2->intvalue;
					}

				} else {
					if( !questmarkfloatvalue ) {
						v1->floatvalue = v2->floatvalue;
					}
				}

				gotquestmarkvalue = qfalse;
				break;
			} // end case

			case P_QUESTIONMARK: {
				if( gotquestmarkvalue ) {
					SourceError( source, "? after ? in #if/#elif" );
					error = 1;
					break;
				}

				questmarkintvalue	= v1->intvalue;
				questmarkfloatvalue = v1->floatvalue;
				gotquestmarkvalue	= qtrue;
				break;
			}
		}

#ifdef DEBUG_EVAL

		if( integer ) {
			Log_Write( "result value = %d", v1->intvalue );

		} else {
			Log_Write( "result value = %f", v1->floatvalue );
		}

#endif // DEBUG_EVAL

		if( error ) {
			break;
		}

		lastoperatortype = o->_operator;

		// if not an _operator with arity 1
		if( o->_operator != P_LOGIC_NOT && o->_operator != P_BIN_NOT ) {
			// remove the second value if not question mark _operator
			if( o->_operator != P_QUESTIONMARK ) {
				v = v->next;
			}

			//
			if( v->prev ) {
				v->prev->next = v->next;

			} else {
				firstvalue = v->next;
			}

			if( v->next ) {
				v->next->prev = v->prev;

			} else {
				lastvalue = v->prev;
			}

			// FreeMemory(v);
			FreeValue( v );
		}

		// remove the _operator
		if( o->prev ) {
			o->prev->next = o->next;

		} else {
			firstoperator = o->next;
		}

		if( o->next ) {
			o->next->prev = o->prev;

		} else {
			lastoperator = o->prev;
		}

		// FreeMemory(o);
		FreeOperator( o );
	}

	if( firstvalue ) {
		if( intvalue ) {
			*intvalue = firstvalue->intvalue;
		}

		if( floatvalue ) {
			*floatvalue = firstvalue->floatvalue;
		}
	}

	for( o = firstoperator; o; o = lastoperator ) {
		lastoperator = o->next;
		// FreeMemory(o);
		FreeOperator( o );
	}

	for( v = firstvalue; v; v = lastvalue ) {
		lastvalue = v->next;
		// FreeMemory(v);
		FreeValue( v );
	}

	if( !error ) {
		return qtrue;
	}

	if( intvalue ) {
		*intvalue = 0;
	}

	if( floatvalue ) {
		*floatvalue = 0;
	}

	return qfalse;
}

/*!
	\brief Parse and evaluate the expression following a preprocessor directive, storing the result as an integer or float if requested.

	The routine begins by resetting any provided integer or float output pointers to zero. It then expects at least one token after the directive; otherwise an error is reported. Tokens are read one
   at a time, expanding identifiers, handling the special "defined" construct, and converting numbers and punctuation directly into a linked list. If a token is an undefined name, an error is emitted.
   Once all tokens are gathered, the token list is handed to PC_EvaluateTokens, which performs the actual arithmetic evaluation. On success, optional output pointers are updated and the temporary
   token list is freed. Debug traces can be emitted if DEBUG_EVAL is enabled. The function returns a non-zero value to indicate success and zero to indicate failure.

	The integer parameter selects whether the evaluation should produce an integer result (qtrue) or a floating‐point result. If both intvalue and floatvalue are NULL the function still performs the
   parse step but discards the value.


	\param source the preprocessor source context containing definitions and the current script position
	\param intvalue pointer to receive the evaluated integer result; may be NULL if the caller does not need an integer value
	\param floatvalue pointer to receive the evaluated floating‑point result; may be NULL if the caller does not need a float value
	\param integer flag indicating that the expression should be evaluated as an integer (non‑zero) or as a floating‑point number (zero)
	\return non‑zero if evaluation succeeded; zero if an error occurred
*/
int PC_Evaluate( source_t* source, signed long int* intvalue, double* floatvalue, int integer )
{
	token_t	  token, *firsttoken, *lasttoken;
	token_t * t, *nexttoken;
	define_t* define;
	int		  defined = qfalse;

	if( intvalue ) {
		*intvalue = 0;
	}

	if( floatvalue ) {
		*floatvalue = 0;
	}

	//
	if( !PC_ReadLine( source, &token ) ) {
		SourceError( source, "no value after #if/#elif" );
		return qfalse;
	}

	firsttoken = NULL;
	lasttoken  = NULL;

	do {
		// if the token is a name
		if( token.type == TT_NAME ) {
			if( defined ) {
				defined = qfalse;
				t		= PC_CopyToken( &token );
				t->next = NULL;

				if( lasttoken ) {
					lasttoken->next = t;

				} else {
					firsttoken = t;
				}

				lasttoken = t;

			} else if( !strcmp( token.string, "defined" ) ) {
				defined = qtrue;
				t		= PC_CopyToken( &token );
				t->next = NULL;

				if( lasttoken ) {
					lasttoken->next = t;

				} else {
					firsttoken = t;
				}

				lasttoken = t;

			} else {
				// then it must be a define
#if DEFINEHASHING
				define = PC_FindHashedDefine( source->definehash, token.string );
#else
				define = PC_FindDefine( source->defines, token.string );
#endif // DEFINEHASHING

				if( !define ) {
					SourceError( source, "can't evaluate %s, not defined", token.string );
					return qfalse;
				}

				if( !PC_ExpandDefineIntoSource( source, &token, define ) ) {
					return qfalse;
				}
			}
		}

		// if the token is a number or a punctuation
		else if( token.type == TT_NUMBER || token.type == TT_PUNCTUATION ) {
			t		= PC_CopyToken( &token );
			t->next = NULL;

			if( lasttoken ) {
				lasttoken->next = t;

			} else {
				firsttoken = t;
			}

			lasttoken = t;

		} else { // can't evaluate the token
			SourceError( source, "can't evaluate %s", token.string );
			return qfalse;
		}
	} while( PC_ReadLine( source, &token ) );

	//
	if( !PC_EvaluateTokens( source, firsttoken, intvalue, floatvalue, integer ) ) {
		return qfalse;
	}

	//
#ifdef DEBUG_EVAL
	Log_Write( "eval:" );
#endif // DEBUG_EVAL

	for( t = firsttoken; t; t = nexttoken ) {
#ifdef DEBUG_EVAL
		Log_Write( " %s", t->string );
#endif // DEBUG_EVAL
		nexttoken = t->next;
		PC_FreeToken( t );
	}

#ifdef DEBUG_EVAL

	if( integer ) {
		Log_Write( "eval result: %d", *intvalue );

	} else {
		Log_Write( "eval result: %f", *floatvalue );
	}

#endif // DEBUG_EVAL
	//
	return qtrue;
}

/*!
	\brief Evaluates a dollar expression directive and stores the resulting integer or float value.

	The function is used for parsing $evalint and $evalfloat pre‑compiler directives. It first expects a leading '(' token following the directive keyword, then reads and expands tokens until the
   matching ')' is found, handling macro definitions and the "defined" keyword. A linked list of the parsed tokens is built and passed to PC_EvaluateTokens, which performs the arithmetic evaluation.
   If the evaluation succeeds, the result is written to either *intvalue or *floatvalue depending on the integer flag; otherwise the error is reported via SourceError and the function returns false.
   The supplied output pointers may be NULL if the caller is interested only in one type of result.

	\param source Context and stream from which the directive is read; used to fetch and report tokens.
	\param intvalue Pointer to a signed long int where the integer result will be stored, or NULL if not needed.
	\param floatvalue Pointer to a double where the floating‑point result will be stored, or NULL if not needed.
	\param integer Flag indicating whether to evaluate the expression as an integer (true) or as a float (false).
	\return Non‑zero if the expression was successfully parsed and evaluated, zero if an error occurred. The result is written to the provided output pointers.
*/
int PC_DollarEvaluate( source_t* source, signed long int* intvalue, double* floatvalue, int integer )
{
	int		  indent, defined = qfalse;
	token_t	  token, *firsttoken, *lasttoken;
	token_t * t, *nexttoken;
	define_t* define;

	if( intvalue ) {
		*intvalue = 0;
	}

	if( floatvalue ) {
		*floatvalue = 0;
	}

	//
	if( !PC_ReadSourceToken( source, &token ) ) {
		SourceError( source, "no leading ( after $evalint/$evalfloat" );
		return qfalse;
	}

	if( !PC_ReadSourceToken( source, &token ) ) {
		SourceError( source, "nothing to evaluate" );
		return qfalse;
	}

	indent	   = 1;
	firsttoken = NULL;
	lasttoken  = NULL;

	do {
		// if the token is a name
		if( token.type == TT_NAME ) {
			if( defined ) {
				defined = qfalse;
				t		= PC_CopyToken( &token );
				t->next = NULL;

				if( lasttoken ) {
					lasttoken->next = t;

				} else {
					firsttoken = t;
				}

				lasttoken = t;

			} else if( !strcmp( token.string, "defined" ) ) {
				defined = qtrue;
				t		= PC_CopyToken( &token );
				t->next = NULL;

				if( lasttoken ) {
					lasttoken->next = t;

				} else {
					firsttoken = t;
				}

				lasttoken = t;

			} else {
				// then it must be a define
#if DEFINEHASHING
				define = PC_FindHashedDefine( source->definehash, token.string );
#else
				define = PC_FindDefine( source->defines, token.string );
#endif // DEFINEHASHING

				if( !define ) {
					SourceError( source, "can't evaluate %s, not defined", token.string );
					return qfalse;
				}

				if( !PC_ExpandDefineIntoSource( source, &token, define ) ) {
					return qfalse;
				}
			}
		}

		// if the token is a number or a punctuation
		else if( token.type == TT_NUMBER || token.type == TT_PUNCTUATION ) {
			if( *token.string == '(' ) {
				indent++;

			} else if( *token.string == ')' ) {
				indent--;
			}

			if( indent <= 0 ) {
				break;
			}

			t		= PC_CopyToken( &token );
			t->next = NULL;

			if( lasttoken ) {
				lasttoken->next = t;

			} else {
				firsttoken = t;
			}

			lasttoken = t;

		} else { // can't evaluate the token
			SourceError( source, "can't evaluate %s", token.string );
			return qfalse;
		}
	} while( PC_ReadSourceToken( source, &token ) );

	//
	if( !PC_EvaluateTokens( source, firsttoken, intvalue, floatvalue, integer ) ) {
		return qfalse;
	}

	//
#ifdef DEBUG_EVAL
	Log_Write( "$eval:" );
#endif // DEBUG_EVAL

	for( t = firsttoken; t; t = nexttoken ) {
#ifdef DEBUG_EVAL
		Log_Write( " %s", t->string );
#endif // DEBUG_EVAL
		nexttoken = t->next;
		PC_FreeToken( t );
	}

#ifdef DEBUG_EVAL

	if( integer ) {
		Log_Write( "$eval result: %d", *intvalue );

	} else {
		Log_Write( "$eval result: %f", *floatvalue );
	}

#endif // DEBUG_EVAL
	//
	return qtrue;
}

/*!
	\brief Processes a preprocessor #elif directive by evaluating its expression and updating the indentation stack.

	The function first removes the current indentation state. If there is no current state or the current state is an #else, a mis‑placed #elif error is reported and the function returns failure. It
   then attempts to evaluate the expression that follows the #elif. If the evaluation fails, the function returns failure. The result of the evaluation determines whether the code block following the
   #elif should be skipped (zero means skip). A new indentation record of type INDENT_ELIF is then pushed onto the stack with the computed skip flag. The function returns a success value if all steps
   succeed.

	\param source pointer to the current source context used for parsing and error reporting.
	\return qtrue on success, qfalse on failure such as a mis‑placed #elif or a failed expression evaluation.
*/
int PC_Directive_elif( source_t* source )
{
	signed long int value;
	int				type, skip;

	PC_PopIndent( source, &type, &skip );

	if( !type || type == INDENT_ELSE ) {
		SourceError( source, "misplaced #elif" );
		return qfalse;
	}

	if( !PC_Evaluate( source, &value, NULL, qtrue ) ) {
		return qfalse;
	}

	skip = ( value == 0 );
	PC_PushIndent( source, INDENT_ELIF, skip );
	return qtrue;
}

/*!
	\brief Evaluates a conditional "if" directive from the source and records whether the following block should be executed.

	The function first evaluates the expression supplied by the source using PC_Evaluate.  If evaluation fails it returns a failure code.  Otherwise it checks the resulting value: if the value is zero
   the block is marked as skipped.  It then pushes a new indentation level with the skip flag using PC_PushIndent and returns a success code.  The push stores state needed for nested directives and to
   control parsing of the body.

	The function uses a signed long integer to hold the evaluated value, and a Boolean-like int for the skip flag.  The return type follows the convention used elsewhere in the parser, where qtrue (1)
   indicates success and qfalse (0) indicates failure.

	\param source pointer to the source file data structure being parsed
	\return 1 if the directive was evaluated successfully, 0 otherwise
*/
int PC_Directive_if( source_t* source )
{
	signed long int value;
	int				skip;

	if( !PC_Evaluate( source, &value, NULL, qtrue ) ) {
		return qfalse;
	}

	skip = ( value == 0 );
	PC_PushIndent( source, INDENT_IF, skip );
	return qtrue;
}

/*!
	\brief Reports an unsupported #line directive from a source file.

	The function is called when a #line directive is encountered while parsing source files. It logs an error message indicating the directive is not supported and signals failure by returning false.
   No further processing of the directive occurs.

	\param source Pointer to the source structure from which the directive was read.
	\return False, indicating the directive is unsupported; qfalse is typically defined as 0.
*/
int PC_Directive_line( source_t* source )
{
	SourceError( source, "#line directive not supported" );
	return qfalse;
}

/*!
	\brief Reads the token following an #error directive and reports a source error.

	The function retrieves the following source token, then calls SourceError with the token string prefixed by "#error directive:". It finally returns a false indicator.

	Because it performs a source error report, callers should treat the return value as a failure status.

	Note: The token string is cleared before reading, ensuring that only the newly read token is used.


	\param source Source context from which to read the next token and to report errors.
	\return An integer value of zero indicating failure.
*/
int PC_Directive_error( source_t* source )
{
	token_t token;

	strcpy( token.string, "" );
	PC_ReadSourceToken( source, &token );
	SourceError( source, "#error directive: %s", token.string );
	return qfalse;
}

/*!
	\brief Issues a warning for an unsupported #pragma directive and skips its line contents.

	The function uses SourceWarning to inform the user that the #pragma directive is not supported. It then calls PC_ReadLine repeatedly until the end of the line is reached, effectively ignoring the
   rest of the directive. Finally, it returns qtrue to indicate successful handling.

	\param source pointer to the current source context parsing the directive
	\return int value qtrue (1) indicating the directive was processed
*/
int PC_Directive_pragma( source_t* source )
{
	token_t token;

	SourceWarning( source, "#pragma directive not supported" );

	while( PC_ReadLine( source, &token ) )
		;

	return qtrue;
}

/*!
	\brief Reinserts a minus sign token into the source stream for parsing.

	Creates a token_t reflecting the current script position, sets its string to "-", designates it as a punctuation token, and then pushes it back into the source queue via PC_UnreadSourceToken. This
   allows the parser to later process the sign as a separate token. The token inherits line number and whitespace information from the source's scriptstack.

	\param source Pointer to a source_t structure, providing context such as the script stack for token creation and unread operations.
*/
void UnreadSignToken( source_t* source )
{
	token_t token;

	token.line			  = source->scriptstack->line;
	token.whitespace_p	  = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed	  = 0;
	strcpy( token.string, "-" );
	token.type	  = TT_PUNCTUATION;
	token.subtype = P_SUB;
	PC_UnreadSourceToken( source, &token );
}

/*!
	\brief Evaluates a script expression and inserts the numeric result into the source token stream, providing a sign token when the result is negative

	The function calls PC_Evaluate to compute the value of the current expression. If the evaluation fails it returns an error flag. On success it creates a token representing the absolute value of
   the result, sets its type to an integer token, and pushes it back onto the source token stream using PC_UnreadSourceToken. If the original value was negative, a separate sign token is also unread.
   The function returns a success indicator.

	\param source the source context containing the script stack and token stream
	\return qtrue if the expression was successfully evaluated and the result token was inserted; otherwise qfalse
*/
int PC_Directive_eval( source_t* source )
{
	signed long int value;
	token_t			token;

	if( !PC_Evaluate( source, &value, NULL, qtrue ) ) {
		return qfalse;
	}

	//
	token.line			  = source->scriptstack->line;
	token.whitespace_p	  = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed	  = 0;
	sprintf( token.string, "%d", abs( value ) );
	token.type	  = TT_NUMBER;
	token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL;
	PC_UnreadSourceToken( source, &token );

	if( value < 0 ) {
		UnreadSignToken( source );
	}

	return qtrue;
}

/*!
	\brief Evaluate a floating‑point value from the script and unread a number token representing its absolute value, restoring a preceding minus sign if necessary.

	This routine first evaluates the next expression from the script source using PC_Evaluate, storing the result in a double. If the evaluation fails it returns false.

	It then creates a token with the absolute value formatted to two decimal digits, records the current script position, and pushes the token back onto the source stack with PC_UnreadSourceToken. If
   the original value was negative, UnreadSignToken is used to place a minus sign token back. The function returns true on success and false when any failure occurs.

	\param source script source structure containing the script stack and current parsing state
	\return nonzero if evaluation succeeded and token was unread, zero otherwise.
*/
int PC_Directive_evalfloat( source_t* source )
{
	double	value;
	token_t token;

	if( !PC_Evaluate( source, NULL, &value, qfalse ) ) {
		return qfalse;
	}

	token.line			  = source->scriptstack->line;
	token.whitespace_p	  = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed	  = 0;
	sprintf( token.string, "%1.2f", fabs( value ) );
	token.type	  = TT_NUMBER;
	token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;
	PC_UnreadSourceToken( source, &token );

	if( value < 0 ) {
		UnreadSignToken( source );
	}

	return qtrue;
}

directive_t directives[20] = { { "if", PC_Directive_if },
	{ "ifdef", PC_Directive_ifdef },
	{ "ifndef", PC_Directive_ifndef },
	{ "elif", PC_Directive_elif },
	{ "else", PC_Directive_else },
	{ "endif", PC_Directive_endif },
	{ "include", PC_Directive_include },
	{ "define", PC_Directive_define },
	{ "undef", PC_Directive_undef },
	{ "line", PC_Directive_line },
	{ "error", PC_Directive_error },
	{ "pragma", PC_Directive_pragma },
	{ "eval", PC_Directive_eval },
	{ "evalfloat", PC_Directive_evalfloat },
	{ NULL, NULL } };

/*!
	\brief Reads and processes a preprocessor directive from the source input.

	The function consumes the next token from the source and expects it to be the name of a recognized directive. It verifies that the name token does not span multiple lines, then looks up the
   corresponding directive in a global table. If found, it invokes the associated handler function, passing the same source, and returns the handler's success flag. If the token is missing, spans a
   line break, or does not match any known directive, an error is reported via SourceError and the function returns a failure flag.

	\param source Pointer to the source stream from which tokens are read and directives are parsed.
	\return Non‑zero (true) if the directive was successfully handled; zero (false) otherwise.
*/
int			PC_ReadDirective( source_t* source )
{
	token_t token;
	int		i;

	// read the directive name
	if( !PC_ReadSourceToken( source, &token ) ) {
		SourceError( source, "found # without name" );
		return qfalse;
	}

	// directive name must be on the same line
	if( token.linescrossed > 0 ) {
		PC_UnreadSourceToken( source, &token );
		SourceError( source, "found # at end of line" );
		return qfalse;
	}

	// if if is a name
	if( token.type == TT_NAME ) {
		// find the precompiler directive
		for( i = 0; directives[i].name; i++ ) {
			if( !strcmp( directives[i].name, token.string ) ) {
				return directives[i].func( source );
			}
		}
	}

	SourceError( source, "unknown precompiler directive %s", token.string );
	return qfalse;
}

/*!
	\brief Evaluates a $ directive expression and pushes its integer result onto the token stream

	This routine interprets a $ directive that should produce a numeric result. It uses PC_DollarEvaluate to compute a signed long integer. If the evaluation fails, the function returns a zero flag.
   When successful, it constructs a token containing the absolute value of the result, marking it as an integer token and storing the original numeric value when NUMBERVALUE is defined. The token is
   then placed back on the source's token stack. If the evaluated value was negative, a separate sign token is also unread. The function signals success by returning a non‑zero value.

	\param source pointer to the source structure used for reading and writing tokens
	\return non‑zero if the expression was evaluated successfully and the token was pushed; otherwise 0
*/
int PC_DollarDirective_evalint( source_t* source )
{
	signed long int value;
	token_t			token;

	if( !PC_DollarEvaluate( source, &value, NULL, qtrue ) ) {
		return qfalse;
	}

	//
	token.line			  = source->scriptstack->line;
	token.whitespace_p	  = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed	  = 0;
	sprintf( token.string, "%d", abs( value ) );
	token.type	  = TT_NUMBER;
	token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL;
#ifdef NUMBERVALUE
	token.intvalue	 = value;
	token.floatvalue = value;
#endif // NUMBERVALUE
	PC_UnreadSourceToken( source, &token );

	if( value < 0 ) {
		UnreadSignToken( source );
	}

	return qtrue;
}

/*!
	\brief Evaluates the current script dollar directive and produces a floating‑point token.

	The function first invokes PC_DollarEvaluate to compute a numeric value. If the evaluation fails, it reports failure. When successful it creates a token containing the absolute value of that
   number, formatted to two decimal places, and pushes the token back onto the source stream. If the computed value was negative, it also inserts a minus sign token. An optional NUMBERVALUE section
   may store the numeric value in the token’s integer and float fields. The function then returns an integer flag indicating success (qtrue) or failure (qfalse).

	\param source Pointer to the source_t structure that provides the current script context and token stream.
	\return A non‑zero value indicates success (the token was created and placed back on the stream); zero indicates failure.
*/
int PC_DollarDirective_evalfloat( source_t* source )
{
	double	value;
	token_t token;

	if( !PC_DollarEvaluate( source, NULL, &value, qfalse ) ) {
		return qfalse;
	}

	token.line			  = source->scriptstack->line;
	token.whitespace_p	  = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed	  = 0;
	sprintf( token.string, "%1.2f", fabs( value ) );
	token.type	  = TT_NUMBER;
	token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;
#ifdef NUMBERVALUE
	token.intvalue	 = ( unsigned long )value;
	token.floatvalue = value;
#endif // NUMBERVALUE
	PC_UnreadSourceToken( source, &token );

	if( value < 0 ) {
		UnreadSignToken( source );
	}

	return qtrue;
}

directive_t dollardirectives[20] = { { "evalint", PC_DollarDirective_evalint }, { "evalfloat", PC_DollarDirective_evalfloat }, { NULL, NULL } };

/*!
	\brief Processes a precompiler directive that starts with $ from the provided source input.

	The function first reads the next token from the source. If the token cannot be read, an error indicating a $ without a name is reported and the function returns false. It then checks that the
   directive name is on the same line; if the token spans multiple lines, the token is unread, an error is reported, and false is returned. When the token is a name, the function searches a global
   table of known dollar directives for a matching entry and invokes its associated handler function, returning the handler's result. If no matching handler is found or the token is not a valid name,
   the directive is considered unknown, the token is returned to the source stream, an appropriate error is emitted, and the function returns false. A true value indicates successful handling, while
   false signifies an error or unknown directive.

	\param source pointer to the source context from which tokens are read
	\return true if the directive was recognized and handled; false otherwise
*/
int			PC_ReadDollarDirective( source_t* source )
{
	token_t token;
	int		i;

	// read the directive name
	if( !PC_ReadSourceToken( source, &token ) ) {
		SourceError( source, "found $ without name" );
		return qfalse;
	}

	// directive name must be on the same line
	if( token.linescrossed > 0 ) {
		PC_UnreadSourceToken( source, &token );
		SourceError( source, "found $ at end of line" );
		return qfalse;
	}

	// if if is a name
	if( token.type == TT_NAME ) {
		// find the precompiler directive
		for( i = 0; dollardirectives[i].name; i++ ) {
			if( !strcmp( dollardirectives[i].name, token.string ) ) {
				return dollardirectives[i].func( source );
			}
		}
	}

	PC_UnreadSourceToken( source, &token );
	SourceError( source, "unknown precompiler directive %s", token.string );
	return qfalse;
}

#ifdef QUAKEC
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int BuiltinFunction( source_t* source )
{
	token_t token;

	if( !PC_ReadSourceToken( source, &token ) ) {
		return qfalse;
	}

	if( token.type == TT_NUMBER ) {
		PC_UnreadSourceToken( source, &token );
		return qtrue;

	} else {
		PC_UnreadSourceToken( source, &token );
		return qfalse;
	}
}

int QuakeCMacro( source_t* source )
{
	int		i;
	token_t token;

	if( !PC_ReadSourceToken( source, &token ) ) {
		return qtrue;
	}

	if( token.type != TT_NAME ) {
		PC_UnreadSourceToken( source, &token );
		return qtrue;
	}

	// find the precompiler directive
	for( i = 0; dollardirectives[i].name; i++ ) {
		if( !strcmp( dollardirectives[i].name, token.string ) ) {
			PC_UnreadSourceToken( source, &token );
			return qfalse;
		}
	}

	PC_UnreadSourceToken( source, &token );
	return qtrue;
}

#endif // QUAKEC

/*!
	\brief Reads the next token from the source and handles preprocessor directives, string concatenation, and macro expansion.

	This function reads tokens from a source stream, processing preprocessor directives such as # directives and $ directives. It also handles string concatenation by merging consecutive string tokens and expands macro definitions found in the source. The function returns true when a valid token is read, and false when the end of the source is reached or an error occurs.

	\param source Pointer to the source stream from which to read tokens.
	\param token Pointer to the token structure where the read token will be stored.
	\return Returns qtrue if a token was successfully read and processed, or qfalse if the end of the source is reached or an error occurs.
*/
int PC_ReadToken( source_t* source, token_t* token )
{
	define_t* define;

	while( 1 ) {
		if( !PC_ReadSourceToken( source, token ) ) {
			return qfalse;
		}

		// check for precompiler directives
		if( token->type == TT_PUNCTUATION && *token->string == '#' ) {
#ifdef QUAKEC

			if( !BuiltinFunction( source ) )
#endif // QUAKC
			{
				// read the precompiler directive
				if( !PC_ReadDirective( source ) ) {
					return qfalse;
				}

				continue;
			}
		}

		if( token->type == TT_PUNCTUATION && *token->string == '$' ) {
#ifdef QUAKEC

			if( !QuakeCMacro( source ) )
#endif // QUAKEC
			{
				// read the precompiler directive
				if( !PC_ReadDollarDirective( source ) ) {
					return qfalse;
				}

				continue;
			}
		}

		// recursively concatenate strings that are behind each other still resolving defines
		if( token->type == TT_STRING ) {
			token_t newtoken;

			if( PC_ReadToken( source, &newtoken ) ) {
				if( newtoken.type == TT_STRING ) {
					token->string[strlen( token->string ) - 1] = '\0';

					if( strlen( token->string ) + strlen( newtoken.string + 1 ) + 1 >= MAX_TOKEN ) {
						SourceError( source, "string longer than MAX_TOKEN %d\n", MAX_TOKEN );
						return qfalse;
					}

					strcat( token->string, newtoken.string + 1 );

				} else {
					PC_UnreadToken( source, &newtoken );
				}
			}
		}

		// if skipping source because of conditional compilation
		if( source->skip ) {
			continue;
		}

		// if the token is a name
		if( token->type == TT_NAME ) {
			// check if the name is a define macro
#if DEFINEHASHING
			define = PC_FindHashedDefine( source->definehash, token->string );
#else
			define = PC_FindDefine( source->defines, token->string );
#endif // DEFINEHASHING

			// if it is a define macro
			if( define ) {
				// expand the defined macro
				if( !PC_ExpandDefineIntoSource( source, token, define ) ) {
					return qfalse;
				}

				continue;
			}
		}

		// copy token for unreading
		memcpy( &source->token, token, sizeof( token_t ) );
		// found a token
		return qtrue;
	}
}

/*!
	\brief Checks if the next token in the source matches the expected string, returning true if it matches or false if it doesn't.

	This function reads the next token from the provided source and compares its string value with the expected string. If the token does not match or if there is no token available, an error is reported and the function returns false. It is commonly used in parsing operations where specific token sequences are expected.

	\param source Pointer to the source from which the token is read
	\param string The expected string to match against the next token
	\return True if the next token matches the expected string, false otherwise.
*/
int PC_ExpectTokenString( source_t* source, char* string )
{
	token_t token;

	if( !PC_ReadToken( source, &token ) ) {
		SourceError( source, "couldn't find expected %s", string );
		return qfalse;
	}

	if( strcmp( token.string, string ) ) {
		SourceError( source, "expected %s, found %s", string, token.string );
		return qfalse;
	}

	return qtrue;
}

/*!
	\brief Attempts to read and validate a token of the specified type and subtype from the source, returning true if successful.

	This function reads a token from the provided source and checks if its type matches the expected type. If the type matches, it further validates the subtype for number and punctuation tokens. For number tokens, it ensures the token's subtype matches the specified subtype. For punctuation tokens, it verifies the token's subtype equals the specified subtype. If any validation fails, an error message is generated and the function returns false. This function is typically used in parsing to ensure the correct token types are encountered in the expected sequence.

	\param source Pointer to the source from which to read the token
	\param type Expected type of the token to read
	\param subtype Expected subtype of the token to read, used for number and punctuation tokens
	\param token Pointer to store the read token
	\return True if the token was successfully read and validated, false otherwise.
*/
int PC_ExpectTokenType( source_t* source, int type, int subtype, token_t* token )
{
	char str[MAX_TOKEN];

	if( !PC_ReadToken( source, token ) ) {
		SourceError( source, "couldn't read expected token" );
		return qfalse;
	}

	if( token->type != type ) {
		strcpy( str, "" );

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

		SourceError( source, "expected a %s, found %s", str, token->string );
		return qfalse;
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

			SourceError( source, "expected %s, found %s", str, token->string );
			return qfalse;
		}

	} else if( token->type == TT_PUNCTUATION ) {
		if( token->subtype != subtype ) {
			SourceError( source, "found %s", token->string );
			return qfalse;
		}
	}

	return qtrue;
}

/*!
	\brief Reads the next token from the source and returns true if successful, false otherwise

	This function attempts to read the next token from the provided source. It returns true if a token is successfully read, and false if no token is available. If the token reading fails, an error message is generated using SourceError to indicate that a expected token could not be read. The function is commonly used in parsing operations where the presence of a specific token is required for continued processing.

	\param source Pointer to the source from which to read the token
	\param token Pointer to the token structure to store the read token
	\return Returns qtrue if a token is successfully read, qfalse otherwise
*/
int PC_ExpectAnyToken( source_t* source, token_t* token )
{
	if( !PC_ReadToken( source, token ) ) {
		SourceError( source, "couldn't read expected token" );
		return qfalse;

	} else {
		return qtrue;
	}
}

/*!
	\brief Checks if the next token in the source matches the specified string and advances the token pointer if it matches.

	This function reads the next token from the provided source and compares its string value with the given string. If they match, the function returns true and advances the source's token pointer. If they do not match, the function unread the token and returns false. This function is typically used for parsing structured input where specific token sequences need to be validated.

	\param source The source from which to read the next token
	\param string The string to compare against the next token in the source
	\return True if the next token in the source matches the given string, false otherwise
*/
int PC_CheckTokenString( source_t* source, char* string )
{
	token_t tok;

	if( !PC_ReadToken( source, &tok ) ) {
		return qfalse;
	}

	// if the token is available
	if( !strcmp( tok.string, string ) ) {
		return qtrue;
	}

	//
	PC_UnreadSourceToken( source, &tok );
	return qfalse;
}

/*!
	\brief Checks if the next token in the source matches the specified type and subtype, and returns it if it does.

	This function reads the next token from the provided source and verifies if its type and subtype match the given parameters. If they match, the token is copied to the output parameter and the function returns true. Otherwise, the token is pushed back to the source and the function returns false. The subtype check uses a bitwise AND operation to ensure all specified bits are set in the token's subtype.

	\param source Pointer to the source from which to read the token
	\param type Expected type of the token to match
	\param subtype Expected subtype of the token to match, using bitwise operations
	\param token Pointer to the token structure where the matched token will be stored
	\return True if the next token matches the specified type and subtype, false otherwise.
*/
int PC_CheckTokenType( source_t* source, int type, int subtype, token_t* token )
{
	token_t tok;

	if( !PC_ReadToken( source, &tok ) ) {
		return qfalse;
	}

	// if the type matches
	if( tok.type == type && ( tok.subtype & subtype ) == subtype ) {
		memcpy( token, &tok, sizeof( token_t ) );
		return qtrue;
	}

	//
	PC_UnreadSourceToken( source, &tok );
	return qfalse;
}

/*!
	\brief Skips tokens in the source until the specified string is found.

	This function reads tokens from the provided source until it encounters a token that matches the specified string. It returns true if the string is found and false if the end of the source is reached without finding the string.

	\param source Pointer to the source from which tokens are read.
	\param string The string to search for in the source.
	\return Non-zero value if the specified string is found, zero if the end of the source is reached.
*/
int PC_SkipUntilString( source_t* source, char* string )
{
	token_t token;

	while( PC_ReadToken( source, &token ) ) {
		if( !strcmp( token.string, string ) ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*!
	\brief Unreads the last token from the source, making it available for subsequent reads.

	This function undoes the reading of the most recently consumed token from the provided source. It places the token back into the source's buffer, allowing it to be read again in the future. The function operates on the token stored in the source's token member.

	\param source Pointer to the source object from which the last token will be unread.
*/
void PC_UnreadLastToken( source_t* source )
{
	PC_UnreadSourceToken( source, &source->token );
}

/*!
	\brief Unreads a token back into the source stream for the given source.

	This function pushes a token back onto the source stream, effectively undoing a previous token read operation. It is used to implement look-ahead or backtracking in the parsing process.

	\param source Pointer to the source structure from which the token was read
	\param token Pointer to the token structure to be pushed back into the source
*/
void PC_UnreadToken( source_t* source, token_t* token )
{
	PC_UnreadSourceToken( source, token );
}

/*!
	\brief Sets the include path for the given source and ensures it ends with a path separator.

	This function copies the provided path into the source's include path buffer and appends the appropriate path separator if it is not already present. The function ensures that the path is properly formatted for subsequent file inclusion operations. The maximum path length is limited by the _MAX_PATH constant.

	\param source Pointer to the source structure whose include path is to be set.
	\param path Character array containing the path to be set as the include path.
*/
void PC_SetIncludePath( source_t* source, char* path )
{
	strncpy( source->includepath, path, _MAX_PATH );

	// add trailing path seperator
	if( source->includepath[strlen( source->includepath ) - 1] != '\\' && source->includepath[strlen( source->includepath ) - 1] != '/' ) {
		strcat( source->includepath, PATHSEPERATOR_STR );
	}
}

/*!
	\brief Sets the punctuation table for the given source.

	This function assigns a punctuation table to the specified source object, which will be used for tokenizing and parsing operations. The punctuation table defines characters that have special meaning during parsing, such as delimiters or operators.

	\param source Pointer to the source object that will use the punctuation table
	\param p Pointer to the punctuation table to be assigned to the source
*/
void PC_SetPunctuations( source_t* source, punctuation_t* p )
{
	source->punctuations = p;
}

source_t* LoadSourceFile( const char* filename )
{
	source_t* source;
	script_t* script;

	PC_InitTokenHeap();

	script = LoadScriptFile( filename );

	if( !script ) {
		return NULL;
	}

	script->next = NULL;

	source = ( source_t* )GetMemory( sizeof( source_t ) );
	memset( source, 0, sizeof( source_t ) );

	strncpy( source->filename, filename, _MAX_PATH );
	source->scriptstack = script;
	source->tokens		= NULL;
	source->defines		= NULL;
	source->indentstack = NULL;
	source->skip		= 0;

#if DEFINEHASHING
	source->definehash = ( define_t** )GetClearedMemory( DEFINEHASHSIZE * sizeof( define_t* ) );
#endif // DEFINEHASHING
	PC_AddGlobalDefinesToSource( source );
	return source;
}

/*!
	\brief Initializes and returns a source_t structure from memory-loaded script data

	This function creates a source_t structure by loading script data from memory. It first initializes the token heap, then loads the script using the provided memory buffer and length. If the script loading fails, it returns NULL. Otherwise, it allocates memory for the source structure, initializes it, and sets up the filename, script stack, and various other fields. The function also handles define hashing if enabled and adds global defines to the source

	\param ptr Pointer to the memory buffer containing the script data
	\param length Length of the script data in bytes
	\param name Name of the source file
	\return Pointer to the initialized source_t structure, or NULL if script loading fails
*/
source_t* LoadSourceMemory( char* ptr, int length, char* name )
{
	source_t* source;
	script_t* script;

	PC_InitTokenHeap();

	script = LoadScriptMemory( ptr, length, name );

	if( !script ) {
		return NULL;
	}

	script->next = NULL;

	source = ( source_t* )GetMemory( sizeof( source_t ) );
	memset( source, 0, sizeof( source_t ) );

	strncpy( source->filename, name, _MAX_PATH );
	source->scriptstack = script;
	source->tokens		= NULL;
	source->defines		= NULL;
	source->indentstack = NULL;
	source->skip		= 0;

#if DEFINEHASHING
	source->definehash = ( define_t** )GetClearedMemory( DEFINEHASHSIZE * sizeof( define_t* ) );
#endif // DEFINEHASHING
	PC_AddGlobalDefinesToSource( source );
	return source;
}

/*!
	\brief Frees all memory allocated for a source including its scripts, tokens, defines, and indents.

	This function cleans up all resources associated with a source_t structure. It iterates through and frees the linked lists of scripts, tokens, defines, and indents. It also handles hash table cleanup for defines when DEFINEHASHING is enabled. Finally, it frees the source structure itself.

	\param source Pointer to the source structure to be freed
*/
void FreeSource( source_t* source )
{
	script_t* script;
	token_t*  token;
	define_t* define;
	indent_t* indent;
	int		  i;

	// PC_PrintDefineHashTable(source->definehash);
	// free all the scripts
	while( source->scriptstack ) {
		script				= source->scriptstack;
		source->scriptstack = source->scriptstack->next;
		FreeScript( script );
	}

	// free all the tokens
	while( source->tokens ) {
		token		   = source->tokens;
		source->tokens = source->tokens->next;
		PC_FreeToken( token );
	}

#if DEFINEHASHING

	for( i = 0; i < DEFINEHASHSIZE; i++ ) {
		while( source->definehash[i] ) {
			define				  = source->definehash[i];
			source->definehash[i] = source->definehash[i]->hashnext;
			PC_FreeDefine( define );
		}
	}

#else // DEFINEHASHING

	// free all defines
	while( source->defines ) {
		define			= source->defines;
		source->defines = source->defines->next;
		PC_FreeDefine( define );
	}

#endif // DEFINEHASHING

	// free all indents
	while( source->indentstack ) {
		indent				= source->indentstack;
		source->indentstack = source->indentstack->next;
		FreeMemory( indent );
	}

#if DEFINEHASHING

	//
	if( source->definehash ) {
		FreeMemory( source->definehash );
	}

#endif // DEFINEHASHING
	// free the source itself
	FreeMemory( source );
}

#define MAX_SOURCEFILES 64

source_t* sourceFiles[MAX_SOURCEFILES];

int		  PC_LoadSourceHandle( const char* filename )
{
	source_t* source;
	int		  i;

	for( i = 1; i < MAX_SOURCEFILES; i++ ) {
		if( !sourceFiles[i] ) {
			break;
		}
	}

	if( i >= MAX_SOURCEFILES ) {
		return 0;
	}

	PS_SetBaseFolder( "" );
	source = LoadSourceFile( filename );

	if( !source ) {
		return 0;
	}

	sourceFiles[i] = source;
	return i;
}

int PC_FreeSourceHandle( int handle )
{
	if( handle < 1 || handle >= MAX_SOURCEFILES ) {
		return qfalse;
	}

	if( !sourceFiles[handle] ) {
		return qfalse;
	}

	FreeSource( sourceFiles[handle] );
	sourceFiles[handle] = NULL;
	return qtrue;
}

/*!
	\brief Reads the next token from the source file identified by the given handle and stores it in the supplied token structure.

	The function first checks that the supplied handle is within the valid range and that a source file is open for that handle; otherwise it returns zero. It then calls PC_ReadToken on the underlying
   source file, storing the result in a temporary token_t structure. The token’s fields are copied into the pc_token argument. If the token type indicates a string, the surrounding double quotes are
   removed to provide the raw string value. The function returns the integer from PC_ReadToken, where a non‑zero result signifies a successful read and zero indicates failure or end of file.

	\param handle Integer identifier for the source file; must be between 1 and MAX_SOURCEFILES‑1 and correspond to an open file.
	\param pc_token Pointer to a pc_token_t structure that will receive the token’s data.
	\return Non‑zero if a token was successfully read; zero if the handle is invalid, no file is open for the handle, or no token could be read.
*/
int PC_ReadTokenHandle( int handle, pc_token_t* pc_token )
{
	token_t token;
	int		ret;

	if( handle < 1 || handle >= MAX_SOURCEFILES ) {
		return 0;
	}

	if( !sourceFiles[handle] ) {
		return 0;
	}

	ret = PC_ReadToken( sourceFiles[handle], &token );
	strcpy( pc_token->string, token.string );
	pc_token->type		 = token.type;
	pc_token->subtype	 = token.subtype;
	pc_token->intvalue	 = token.intvalue;
	pc_token->floatvalue = token.floatvalue;

	if( pc_token->type == TT_STRING ) {
		StripDoubleQuotes( pc_token->string );
	}

	return ret;
}

int PC_SourceFileAndLine( int handle, char* filename, int* line )
{
	if( handle < 1 || handle >= MAX_SOURCEFILES ) {
		return qfalse;
	}

	if( !sourceFiles[handle] ) {
		return qfalse;
	}

	strcpy( filename, sourceFiles[handle]->filename );

	if( sourceFiles[handle]->scriptstack ) {
		*line = sourceFiles[handle]->scriptstack->line;

	} else {
		*line = 0;
	}

	return qtrue;
}

void PC_SetBaseFolder( char* path )
{
	PS_SetBaseFolder( path );
}

void PC_CheckOpenSourceHandles()
{
	int i;

	for( i = 1; i < MAX_SOURCEFILES; i++ ) {
		if( sourceFiles[i] ) {
#ifdef BOTLIB
			botimport.Print( PRT_ERROR, "file %s still open in precompiler\n", sourceFiles[i]->scriptstack->filename );
#endif // BOTLIB
		}
	}
}
