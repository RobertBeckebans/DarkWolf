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
	\file engine/botlib/l_struct.h
	\brief Act as the core parser and serializer for bot AI script data structures, exposing a simple API to read/write arbitrary C structures described by a runtime schema.
	\note doxygenix: sha256=056d66978a989f0ee0e6185abda3e4f7019977a5b345b3ed67d26af6d9788014

	\par File Purpose
	- Act as the core parser and serializer for bot AI script data structures, exposing a simple API to read/write arbitrary C structures described by a runtime schema.
	- Serve as the foundation for configuration and state files that drive AI behavior (e.g., weapon choices, mission objectives, AI personality traits).

	\par Core Responsibilities
	- Define constants and masks for script field types and sub‑types, allowing flexible description of primitive and composite data structures.
	- Represent individual struct fields and whole structure layouts with fielddef_s and structdef_s, including offset, type, bounds, and optional sub‑structure linkage.
	- Parse script input into a populated C structure according to a supplied definition, handling primitives, arrays, bounded values, strings, and nested structs.
	- Serialize a structure definition into a readable file representation, supporting indentation and compact float formatting.

	\par Key Types and Functions
	- fielddef_t — structure describing a single field: name, memory offset, type, array size, bounds, and optional sub‑structure pointer.
	- structdef_t — structure describing an entire composite type: total size and pointer to a fielddef_t array.
	- ReadStructure(source_t* source, structdef_t* def, char* structure) — parses script source according to def and fills the provided memory buffer.
	- WriteStructure(FILE* fp, structdef_t* def, char* structure) — outputs a textual representation of the structure definition to a file.
	- WriteIndent(FILE* fp, int indent) — emits a specified number of tab characters to the file stream.
	- WriteFloat(FILE* fp, float value) — writes a float to the file without trailing zeros.

	\par Control Flow
	- ReadStructure begins by consuming an opening brace, then enters a token‑driven loop where each field name token is matched against the provided struct definition; array fields trigger nested brace parsing and per‑element recursive or helper reads; upon encountering a closing brace the loop exits and the populated memory buffer is returned as a success flag.
	- WriteStructure calls WriteStructWithIndent with zero indent to serialise the struct name and fields into a file, recursively handling nested sub‑structures and arrays via WriteStructure calls.
	- WriteIndent outputs the requested number of tab characters to the given FILE stream, looping until the indent count reaches zero, aborting on any fprintf failure.
	- WriteFloat formats a float to a string, trims trailing zeros and a trailing decimal point, then writes the compact representation to the FILE stream.

	\par Dependencies
	- stdlib/stdio.h for FILE handling (implicit assumption).
	- Definition of source_t and Token handling must be included by the compilation unit that uses these prototypes.
	- Requires a parsing subsystem that supplies tokens such as opening/closing braces, field names, values, and error reporting via SourceError.

	\par How It Fits
	- Part of the botlib module, this header collaborates with other scripting utilities to provide a lightweight, flexible data definition system.
	- Facilitates decoupling AI behavior parameters from hard‑coded C structs, enabling designers to edit scripts without recompiling.
	- Provides recursive support for nested structs, which aligns with the hierarchical design of many AI scripts (e.g., missions containing sub‑missions).
*/

/*****************************************************************************
 * name:		l_struct.h
 *
 * desc:		structure reading/writing
 *
 *
 *****************************************************************************/

#define MAX_STRINGFIELD 80
// field types
#define FT_CHAR			1 // char
#define FT_INT			2 // int
#define FT_FLOAT		3 // float
#define FT_STRING		4 // char [MAX_STRINGFIELD]
#define FT_STRUCT		6 // struct (sub structure)
// type only mask
#define FT_TYPE			0x00FF // only type, clear subtype
// sub types
#define FT_ARRAY		0x0100 // array of type
#define FT_BOUNDED		0x0200 // bounded value
#define FT_UNSIGNED		0x0400

// structure field definition
typedef struct fielddef_s {
	char*				name;	// name of the field
	int					offset; // offset in the structure
	int					type;	// type of the field
	// type specific fields
	int					maxarray;			// maximum array size
	float				floatmin, floatmax; // float min and max
	struct structdef_s* substruct;			// sub structure
} fielddef_t;

// structure definition
typedef struct structdef_s {
	int			size;
	fielddef_t* fields;
} structdef_t;

/*!
	\brief Parses a script source into a C structure according to a provided definition.

	The function begins by expecting an opening brace token to indicate the start of a structured block. It then enters a loop reading tokens sequentially; each token should name a field in the
   structure. The field name is matched against the definition, and if it is an array the function expects a nested brace to contain elements. For each field or array element, the appropriate helper
   routine (ReadChar, ReadNumber, ReadString, or a recursive call for sub‑structures) is invoked to read the value from the source and store it into the correct offset within the target structure.
   Commas separate array elements, and the loop terminates when a closing brace token is encountered. On any parsing error or unexpected token, a message is sent via SourceError and the function
   returns false; otherwise it returns true after successfully filling the structure.

	\param source pointer to the script source being read
	\param def definition of the structure’s fields and layout
	\param structure pointer to the memory buffer to populate with parsed values
	\return non‑zero on success (qtrue), zero on failure (qfalse)
*/
int ReadStructure( source_t* source, structdef_t* def, char* structure );

/*!
	\brief Writes the structure definition to the specified file.

	The function forwards the request to WriteStructWithIndent, passing an indentation level of zero so that the structure is written without any preceding indentation.

	\param fp File pointer to write to.
	\param def Pointer to the structure definition data.
	\param structure String containing the structure's source code representation.
	\return Result of WriteStructWithIndent; typically indicates success or an error code.
*/
int WriteStructure( FILE* fp, structdef_t* def, char* structure );

/*!
	\brief Writes a given number of tab columns to a file.

	This function outputs tab characters to the supplied file stream until the specified indentation level has been written. It counts down from the provided indent value, writing a single tab
   character for each iteration. If any call to fprintf fails, the function aborts and signals an error by returning qfalse. On successful completion it returns qtrue.

	Note that the function's return type is int while the returned constants qtrue and qfalse represent boolean success or failure, the latter being zero.

	\param fp pointer to the FILE stream to write the indents to
	\param indent number of tab characters to output
	\return qtrue (non‑zero) if all indents were written successfully; qfalse (zero) if any write operation failed
*/
int WriteIndent( FILE* fp, int indent );

/*!
	\brief Writes a float to a file without trailing zeros.

	The function formats the supplied float into a decimal string using sprintf, then removes any trailing zero characters and an optional decimal point so the output is as concise as possible. The
   resulting string is written to the specified FILE stream with fprintf. It returns 1 when the write succeeds and 0 if an error occurs during output.

	\param fp File stream to which the value will be written
	\param value Floating point number to output
	\return 1 if the value was successfully written, 0 otherwise
*/
int WriteFloat( FILE* fp, float value );
