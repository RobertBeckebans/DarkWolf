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
 * name:		l_struct.c
 *
 * desc:		structure reading / writing
 *
 *
 *****************************************************************************/

#ifdef BOTLIB
	#include "q_shared.h"
	#include "botshared/botlib.h" //for the include of be_interface.h
	#include "l_script.h"
	#include "l_precomp.h"
	#include "l_struct.h"
	#include "l_utils.h"
	#include "be_interface.h"
#endif // BOTLIB

#ifdef BSPC
	// include files for usage in the BSP Converter
	#include "../bspc/qbsp.h"
	#include "../bspc/l_log.h"
	#include "../bspc/l_mem.h"
	#include "l_precomp.h"
	#include "l_struct.h"

	#define qtrue  true
	#define qfalse false
#endif // BSPC

/*!
	\brief Finds a field definition by name in a null-terminated array of fielddef_t structures.

	The function performs a linear scan over the array pointed to by defs. Each element is examined until its name member is null, marking the end of the array. If an element’s name matches the
   provided name using a case-sensitive string comparison, a pointer to that element is returned. If no element matches, the function returns NULL.

	\param defs Array of field definitions terminated by an element whose name field is NULL.
	\param name C string containing the name to search for.
	\return Pointer to the matching fielddef_t structure or NULL when the name is not present.
*/
fielddef_t* FindField( fielddef_t* defs, char* name )
{
	int i;

	for( i = 0; defs[i].name; i++ ) {
		if( !strcmp( defs[i].name, name ) ) {
			return &defs[i];
		}
	}

	return NULL;
}

/*!
	\brief Reads a numeric token from the source, converts it according to the field definition, and stores the result at the supplied location.

	The function first retrieves any token. If the token is a minus sign it is consumed and a negative flag is set – this is rejected for unsigned fields. The next token must be a numeric token. For
   floating point tokens the value is validated against the field’s type and optional bounds. Integer tokens are converted, signedness is applied, and the appropriate bounds are chosen based on field
   type (char, int) and unsigned flag. Bounded fields enforce min/max limits derived from the field definition or the token’s floatmin/floatmax if present. Out-of-range values trigger a source error
   and cause the function to return false. On success the numeric value is written to the destination pointer using the proper type cast.

	The parser signals errors by calling SourceError; it does not throw exceptions. The function returns true (non‑zero) on success and false (zero) on failure.

	\param source pointer to the token source used by the parser
	\param fd field definition describing expected type, signedness, boundedness, and limits
	\param p pointer to the destination where the parsed value is written; concrete type depends on the field definition
	\return Non‑zero if the number was successfully read and stored; zero otherwise.
*/
qboolean ReadNumber( source_t* source, fielddef_t* fd, void* p )
{
	token_t	 token;
	int		 negative = qfalse;
	long int intval, intmin = 0, intmax = 0;
	double	 floatval;

	if( !PC_ExpectAnyToken( source, &token ) ) {
		return 0;
	}

	// check for minus sign
	if( token.type == TT_PUNCTUATION ) {
		if( fd->type & FT_UNSIGNED ) {
			SourceError( source, "expected unsigned value, found %s", token.string );
			return 0;
		}

		// if not a minus sign
		if( strcmp( token.string, "-" ) ) {
			SourceError( source, "unexpected punctuation %s", token.string );
			return 0;
		}

		negative = qtrue;

		// read the number
		if( !PC_ExpectAnyToken( source, &token ) ) {
			return 0;
		}
	}

	// check if it is a number
	if( token.type != TT_NUMBER ) {
		SourceError( source, "expected number, found %s", token.string );
		return 0;
	}

	// check for a float value
	if( token.subtype & TT_FLOAT ) {
		if( ( fd->type & FT_TYPE ) != FT_FLOAT ) {
			SourceError( source, "unexpected float" );
			return 0;
		}

		floatval = token.floatvalue;

		if( negative ) {
			floatval = -floatval;
		}

		if( fd->type & FT_BOUNDED ) {
			if( floatval < fd->floatmin || floatval > fd->floatmax ) {
				SourceError( source, "float out of range [%f, %f]", fd->floatmin, fd->floatmax );
				return 0;
			}
		}

		*( float* )p = ( float )floatval;
		return 1;
	}

	//
	intval = token.intvalue;

	if( negative ) {
		intval = -intval;
	}

	// check bounds
	if( ( fd->type & FT_TYPE ) == FT_CHAR ) {
		if( fd->type & FT_UNSIGNED ) {
			intmin = 0;
			intmax = 255;

		} else {
			intmin = -128;
			intmax = 127;
		}
	}

	if( ( fd->type & FT_TYPE ) == FT_INT ) {
		if( fd->type & FT_UNSIGNED ) {
			intmin = 0;
			intmax = 65535;

		} else {
			intmin = -32768;
			intmax = 32767;
		}
	}

	if( ( fd->type & FT_TYPE ) == FT_CHAR || ( fd->type & FT_TYPE ) == FT_INT ) {
		if( fd->type & FT_BOUNDED ) {
			intmin = Maximum( intmin, fd->floatmin );
			intmax = Minimum( intmax, fd->floatmax );
		}

		if( intval < intmin || intval > intmax ) {
			SourceError( source, "value %d out of range [%d, %d]", intval, intmin, intmax );
			return 0;
		}

	} else if( ( fd->type & FT_TYPE ) == FT_FLOAT ) {
		if( fd->type & FT_BOUNDED ) {
			if( intval < fd->floatmin || intval > fd->floatmax ) {
				SourceError( source, "value %d out of range [%f, %f]", intval, fd->floatmin, fd->floatmax );
				return 0;
			}
		}
	}

	// store the value
	if( ( fd->type & FT_TYPE ) == FT_CHAR ) {
		if( fd->type & FT_UNSIGNED ) {
			*( unsigned char* )p = ( unsigned char )intval;

		} else {
			*( char* )p = ( char )intval;
		}

	} else if( ( fd->type & FT_TYPE ) == FT_INT ) {
		if( fd->type & FT_UNSIGNED ) {
			*( unsigned int* )p = ( unsigned int )intval;

		} else {
			*( int* )p = ( int )intval;
		}

	} else if( ( fd->type & FT_TYPE ) == FT_FLOAT ) {
		*( float* )p = ( float )intval;
	}

	return 1;
}

/*!
	\brief Reads a single character from the source token stream and stores it in the supplied location.

	The function consumes a token from the source stream. If the token is a literal, surrounding single quotes are stripped and the first character of the resulting string is copied to the destination
   pointer. If the token is not a literal, the token is put back onto the stream and ReadNumber is called to interpret the token as a numeric constant; the numeric value is stored as a character.
   Return a non‑zero value when the read succeeds and zero otherwise.

	\param source pointer to the source from which to read tokens
	\param fd field definition used when interpreting non‑literal values
	\param p pointer to memory where the resulting character should be stored; the caller should provide a pointer to a char
	\return non‑zero if the character was read successfully, zero on failure
*/
qboolean ReadChar( source_t* source, fielddef_t* fd, void* p )
{
	token_t token;

	if( !PC_ExpectAnyToken( source, &token ) ) {
		return 0;
	}

	// take literals into account
	if( token.type == TT_LITERAL ) {
		StripSingleQuotes( token.string );
		*( char* )p = token.string[0];

	} else {
		PC_UnreadLastToken( source );

		if( !ReadNumber( source, fd, p ) ) {
			return 0;
		}
	}

	return 1;
}

/*!
	\brief Reads a quoted string token from the source and stores it in the provided buffer.

	The function first attempts to read the next token from the source, expecting it to be of string type. On success, the surrounding quotation marks are stripped and the resulting string is copied
   into the buffer passed via p, with a maximum of MAX_STRINGFIELD bytes. The final byte is always set to the null character to guarantee a properly terminated C string. The function returns 1 if the
   string was successfully read and copied, otherwise 0.

	This routine is used by the structure parsing logic to fill string fields within data structures during a source file load.

	\param source Pointer to the source context from which tokens are read
	\param fd Descriptor of the field being read (unused in this implementation but kept for API consistency)
	\param p Pointer to the destination buffer that should be at least MAX_STRINGFIELD bytes long
	\return 1 on successful read; 0 if the expected string token was not found.
*/
int ReadString( source_t* source, fielddef_t* fd, void* p )
{
	token_t token;

	if( !PC_ExpectTokenType( source, TT_STRING, 0, &token ) ) {
		return 0;
	}

	// remove the double quotes
	StripDoubleQuotes( token.string );
	// copy the string
	strncpy( ( char* )p, token.string, MAX_STRINGFIELD );
	// make sure the string is closed with a zero
	( ( char* )p )[MAX_STRINGFIELD - 1] = '\0';
	//
	return 1;
}

int ReadStructure( source_t* source, structdef_t* def, char* structure )
{
	token_t		token;
	fielddef_t* fd;
	void*		p;
	int			num;

	if( !PC_ExpectTokenString( source, "{" ) ) {
		return 0;
	}

	while( 1 ) {
		if( !PC_ExpectAnyToken( source, &token ) ) {
			return qfalse;
		}

		// if end of structure
		if( !strcmp( token.string, "}" ) ) {
			break;
		}

		// find the field with the name
		fd = FindField( def->fields, token.string );

		if( !fd ) {
			SourceError( source, "unknown structure field %s", token.string );
			return qfalse;
		}

		if( fd->type & FT_ARRAY ) {
			num = fd->maxarray;

			if( !PC_ExpectTokenString( source, "{" ) ) {
				return qfalse;
			}

		} else {
			num = 1;
		}

		p = ( void* )( structure + fd->offset );

		while( num-- > 0 ) {
			if( fd->type & FT_ARRAY ) {
				if( PC_CheckTokenString( source, "}" ) ) {
					break;
				}
			}

			switch( fd->type & FT_TYPE ) {
				case FT_CHAR: {
					if( !ReadChar( source, fd, p ) ) {
						return qfalse;
					}

					p = ( char* )p + sizeof( char );
					break;
				} // end case

				case FT_INT: {
					if( !ReadNumber( source, fd, p ) ) {
						return qfalse;
					}

					p = ( char* )p + sizeof( int );
					break;
				} // end case

				case FT_FLOAT: {
					if( !ReadNumber( source, fd, p ) ) {
						return qfalse;
					}

					p = ( char* )p + sizeof( float );
					break;
				} // end case

				case FT_STRING: {
					if( !ReadString( source, fd, p ) ) {
						return qfalse;
					}

					p = ( char* )p + MAX_STRINGFIELD;
					break;
				} // end case

				case FT_STRUCT: {
					if( !fd->substruct ) {
						SourceError( source, "BUG: no sub structure defined" );
						return qfalse;
					}

					ReadStructure( source, fd->substruct, ( char* )p );
					p = ( char* )p + fd->substruct->size;
					break;
				} // end case
			}

			if( fd->type & FT_ARRAY ) {
				if( !PC_ExpectAnyToken( source, &token ) ) {
					return qfalse;
				}

				if( !strcmp( token.string, "}" ) ) {
					break;
				}

				if( strcmp( token.string, "," ) ) {
					SourceError( source, "expected a comma, found %s", token.string );
					return qfalse;
				}
			}
		}
	}

	return qtrue;
}

int WriteIndent( FILE* fp, int indent )
{
	while( indent-- > 0 ) {
		if( fprintf( fp, "\t" ) < 0 ) {
			return qfalse;
		}
	}

	return qtrue;
}

int WriteFloat( FILE* fp, float value )
{
	char buf[128];
	int	 l;

	sprintf( buf, "%f", value );
	l = strlen( buf );

	// strip any trailing zeros
	while( l-- > 1 ) {
		if( buf[l] != '0' && buf[l] != '.' ) {
			break;
		}

		if( buf[l] == '.' ) {
			buf[l] = 0;
			break;
		}

		buf[l] = 0;
	}

	// write the float to file
	if( fprintf( fp, "%s", buf ) < 0 ) {
		return 0;
	}

	return 1;
}

/*!
	\brief Writes the formatted contents of a structure to a file with indentation

	This function outputs the data of a structure, as described by a struct definition, to a FILE stream. It starts by writing the opening brace and then iterates over each field in the definition.
   For each field it prints the field name followed by its value: integers, floats, characters, and strings are printed directly, array fields are enclosed in braces and comma‑separated, and nested
   struct fields are emitted recursively with increased indentation. After all fields are processed the function writes a closing brace. All newlines and indentation are handled by the helper
   WriteIndent. The function returns qtrue on success and qfalse if any I/O operation fails.

	\param fp pointer to the file where the struct is written
	\param def metadata describing the structure's fields
	\param structure pointer to the actual data instance to serialize
	\param indent current indentation depth (0 for top level)
	\return qtrue (non‑zero) on successful write, qfalse (zero) on failure
*/
int WriteStructWithIndent( FILE* fp, structdef_t* def, char* structure, int indent )
{
	int			i, num;
	void*		p;
	fielddef_t* fd;

	if( !WriteIndent( fp, indent ) ) {
		return qfalse;
	}

	if( fprintf( fp, "{\r\n" ) < 0 ) {
		return qfalse;
	}

	indent++;

	for( i = 0; def->fields[i].name; i++ ) {
		fd = &def->fields[i];

		if( !WriteIndent( fp, indent ) ) {
			return qfalse;
		}

		if( fprintf( fp, "%s\t", fd->name ) < 0 ) {
			return qfalse;
		}

		p = ( void* )( structure + fd->offset );

		if( fd->type & FT_ARRAY ) {
			num = fd->maxarray;

			if( fprintf( fp, "{" ) < 0 ) {
				return qfalse;
			}

		} else {
			num = 1;
		}

		while( num-- > 0 ) {
			switch( fd->type & FT_TYPE ) {
				case FT_CHAR: {
					if( fprintf( fp, "%d", *( char* )p ) < 0 ) {
						return qfalse;
					}

					p = ( char* )p + sizeof( char );
					break;
				} // end case

				case FT_INT: {
					if( fprintf( fp, "%d", *( int* )p ) < 0 ) {
						return qfalse;
					}

					p = ( char* )p + sizeof( int );
					break;
				} // end case

				case FT_FLOAT: {
					if( !WriteFloat( fp, *( float* )p ) ) {
						return qfalse;
					}

					p = ( char* )p + sizeof( float );
					break;
				} // end case

				case FT_STRING: {
					if( fprintf( fp, "\"%s\"", ( char* )p ) < 0 ) {
						return qfalse;
					}

					p = ( char* )p + MAX_STRINGFIELD;
					break;
				} // end case

				case FT_STRUCT: {
					if( !WriteStructWithIndent( fp, fd->substruct, structure, indent ) ) {
						return qfalse;
					}

					p = ( char* )p + fd->substruct->size;
					break;
				} // end case
			}

			if( fd->type & FT_ARRAY ) {
				if( num > 0 ) {
					if( fprintf( fp, "," ) < 0 ) {
						return qfalse;
					}

				} else {
					if( fprintf( fp, "}" ) < 0 ) {
						return qfalse;
					}
				}
			}
		}

		if( fprintf( fp, "\r\n" ) < 0 ) {
			return qfalse;
		}
	}

	indent--;

	if( !WriteIndent( fp, indent ) ) {
		return qfalse;
	}

	if( fprintf( fp, "}\r\n" ) < 0 ) {
		return qfalse;
	}

	return qtrue;
}

int WriteStructure( FILE* fp, structdef_t* def, char* structure )
{
	return WriteStructWithIndent( fp, def, structure, 0 );
}
