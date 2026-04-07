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
 * name:		be_ai_weight.c
 *
 * desc:		fuzzy logic
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_memory.h"
#include "l_log.h"
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
#include "be_ai_weight.h"

#define MAX_INVENTORYVALUE 999999
#define EVALUATERECURSIVELY

#define MAX_WEIGHT_FILES 128
weightconfig_t* weightFileList[MAX_WEIGHT_FILES];

/*!
	\brief Receives a numeric token from the source, warns on leading minus signs, and writes the parsed float value to the output pointer, returning success or failure as an integer.

	The function first consumes any token from the provided source. If the token read is a minus sign, it emits a warning and consumes the next token which must be a number; this number is then used
   as the value (the sign is discarded). If the resulting token is not a numerical token, an error is reported and the function signals failure. On success, the numeric value stored in the token is
   assigned to the supplied float pointer and the function returns true, otherwise it returns false.

	\param source pointer to a token source from which to read the next token
	\param value output pointer to store the parsed float value
	\return returns true (qtrue) if a number was successfully parsed and stored, otherwise returns false (qfalse).
*/
int				ReadValue( source_t* source, float* value )
{
	token_t token;

	if( !PC_ExpectAnyToken( source, &token ) ) {
		return qfalse;
	}

	if( !strcmp( token.string, "-" ) ) {
		SourceWarning( source, "negative value set to zero\n" );

		if( !PC_ExpectTokenType( source, TT_NUMBER, 0, &token ) ) {
			return qfalse;
		}
	}

	if( token.type != TT_NUMBER ) {
		SourceError( source, "invalid return value %s\n", token.string );
		return qfalse;
	}

	*value = token.floatvalue;
	return qtrue;
}

/*!
	\brief Reads a fuzzy weight expression from source and fills the fuzzyseparator structure, returning true on success.

	The function first checks for a "balance" keyword. If found, it parses a parenthesised list of three comma‑separated values representing weight, minweight and maxweight, sets fs->type to
   WT_BALANCE, and stores these values. If "balance" is not present, it reads a single value and assigns it to weight, minweight and maxweight while setting fs->type to 0. After the weight
   information, a semicolon must appear; the function returns qtrue if all parsing steps succeed, otherwise qfalse.

	\param source Pointer to the source tokenizer from which the weight expression is read
	\param fs Pointer to the fuzzyseperator_t structure that will receive the parsed values
	\return qtrue if parsing succeeds, otherwise qfalse
*/
int ReadFuzzyWeight( source_t* source, fuzzyseperator_t* fs )
{
	if( PC_CheckTokenString( source, "balance" ) ) {
		fs->type = WT_BALANCE;

		if( !PC_ExpectTokenString( source, "(" ) ) {
			return qfalse;
		}

		if( !ReadValue( source, &fs->weight ) ) {
			return qfalse;
		}

		if( !PC_ExpectTokenString( source, "," ) ) {
			return qfalse;
		}

		if( !ReadValue( source, &fs->minweight ) ) {
			return qfalse;
		}

		if( !PC_ExpectTokenString( source, "," ) ) {
			return qfalse;
		}

		if( !ReadValue( source, &fs->maxweight ) ) {
			return qfalse;
		}

		if( !PC_ExpectTokenString( source, ")" ) ) {
			return qfalse;
		}

	} else {
		fs->type = 0;

		if( !ReadValue( source, &fs->weight ) ) {
			return qfalse;
		}

		fs->minweight = fs->weight;
		fs->maxweight = fs->weight;
	}

	if( !PC_ExpectTokenString( source, ";" ) ) {
		return qfalse;
	}

	return qtrue;
}

/*!
	\brief Recursively frees a fuzzyseperator_t structure and its hierarchical extensions.

	The function checks for a null pointer and immediately returns if the input is null. It then recursively frees any child nodes followed by any next nodes, ensuring a depth‑first cleanup of the
   linked structure. After all sub‑nodes have been released, the current node is freed using FreeMemory, which should match the allocator used to create the structure.

	\param fs pointer to the fuzzyseperator_t to be freed; its child and next nodes are recursively released before the node itself is freed
*/
void FreeFuzzySeperators_r( fuzzyseperator_t* fs )
{
	if( !fs ) {
		return;
	}

	if( fs->child ) {
		FreeFuzzySeperators_r( fs->child );
	}

	if( fs->next ) {
		FreeFuzzySeperators_r( fs->next );
	}

	FreeMemory( fs );
}

/*!
	\brief Frees a weight configuration and releases all associated resources.

	Iterates over each weight in the configuration, deallocating its fuzzy separators and any allocated name string, then frees the configuration structure itself.

	\param config Pointer to the weight configuration to be freed.
*/
void FreeWeightConfig2( weightconfig_t* config )
{
	int i;

	for( i = 0; i < config->numweights; i++ ) {
		FreeFuzzySeperators_r( config->weights[i].firstseperator );

		if( config->weights[i].name ) {
			FreeMemory( config->weights[i].name );
		}
	}

	FreeMemory( config );
}

void FreeWeightConfig( weightconfig_t* config )
{
	if( !LibVarGetValue( "bot_reloadcharacters" ) ) {
		return;
	}

	FreeWeightConfig2( config );
}

/*!
	\brief Parses fuzzy separator definitions from a source stream and builds a linked list of fuzzy separator nodes.

	Starting with an opening parenthesis, the function reads an integer index, then expects an opening brace to begin a list of fuzzy separator entries. Each entry may be a default case, a case with a
   numerical value, a return entry, a nested switch, or an error condition. For each entry, a fuzzyseperator_t node is allocated, linked into the list, and populated with the appropriate index, value,
   weight (for return), or child pointer (for nested switches). The function ensures that only a single default case is allowed; a duplicate default triggers a source error, frees any partially built
   list, and returns null. If a default case is missing, a synthetic default node is added and a warning is issued. Any syntax error results in freeing the list and returning null.

	The function operates recursively when encountering nested switches, and reports errors via SourceError and warnings via SourceWarning. It never throws exceptions.


	\param source Pointer to the source object from which tokens are read and errors are reported
	\return Pointer to the first node of the constructed fuzzy separator list, or NULL if parsing failed
*/
fuzzyseperator_t* ReadFuzzySeperators_r( source_t* source )
{
	int				  newindent, index, def, founddefault;
	token_t			  token;
	fuzzyseperator_t *fs, *lastfs, *firstfs;

	founddefault = qfalse;
	firstfs		 = NULL;
	lastfs		 = NULL;

	if( !PC_ExpectTokenString( source, "(" ) ) {
		return NULL;
	}

	if( !PC_ExpectTokenType( source, TT_NUMBER, TT_INTEGER, &token ) ) {
		return NULL;
	}

	index = token.intvalue;

	if( !PC_ExpectTokenString( source, ")" ) ) {
		return NULL;
	}

	if( !PC_ExpectTokenString( source, "{" ) ) {
		return NULL;
	}

	if( !PC_ExpectAnyToken( source, &token ) ) {
		return NULL;
	}

	do {
		def = !strcmp( token.string, "default" );

		if( def || !strcmp( token.string, "case" ) ) {
			fs		  = ( fuzzyseperator_t* )GetClearedMemory( sizeof( fuzzyseperator_t ) );
			fs->index = index;

			if( lastfs ) {
				lastfs->next = fs;

			} else {
				firstfs = fs;
			}

			lastfs = fs;

			if( def ) {
				if( founddefault ) {
					SourceError( source, "switch already has a default\n" );
					FreeFuzzySeperators_r( firstfs );
					return NULL;
				}

				fs->value	 = MAX_INVENTORYVALUE;
				founddefault = qtrue;

			} else {
				if( !PC_ExpectTokenType( source, TT_NUMBER, TT_INTEGER, &token ) ) {
					FreeFuzzySeperators_r( firstfs );
					return NULL;
				}

				fs->value = token.intvalue;
			}

			if( !PC_ExpectTokenString( source, ":" ) || !PC_ExpectAnyToken( source, &token ) ) {
				FreeFuzzySeperators_r( firstfs );
				return NULL;
			}

			newindent = qfalse;

			if( !strcmp( token.string, "{" ) ) {
				newindent = qtrue;

				if( !PC_ExpectAnyToken( source, &token ) ) {
					FreeFuzzySeperators_r( firstfs );
					return NULL;
				}
			}

			if( !strcmp( token.string, "return" ) ) {
				if( !ReadFuzzyWeight( source, fs ) ) {
					FreeFuzzySeperators_r( firstfs );
					return NULL;
				}

			} else if( !strcmp( token.string, "switch" ) ) {
				fs->child = ReadFuzzySeperators_r( source );

				if( !fs->child ) {
					FreeFuzzySeperators_r( firstfs );
					return NULL;
				}

			} else {
				SourceError( source, "invalid name %s\n", token.string );
				return NULL;
			}

			if( newindent ) {
				if( !PC_ExpectTokenString( source, "}" ) ) {
					FreeFuzzySeperators_r( firstfs );
					return NULL;
				}
			}

		} else {
			FreeFuzzySeperators_r( firstfs );
			SourceError( source, "invalid name %s\n", token.string );
			return NULL;
		}

		if( !PC_ExpectAnyToken( source, &token ) ) {
			FreeFuzzySeperators_r( firstfs );
			return NULL;
		}
	} while( strcmp( token.string, "}" ) );

	//
	if( !founddefault ) {
		SourceWarning( source, "switch without default\n" );
		fs		   = ( fuzzyseperator_t* )GetClearedMemory( sizeof( fuzzyseperator_t ) );
		fs->index  = index;
		fs->value  = MAX_INVENTORYVALUE;
		fs->weight = 0;
		fs->next   = NULL;
		fs->child  = NULL;

		if( lastfs ) {
			lastfs->next = fs;

		} else {
			firstfs = fs;
		}

		lastfs = fs;
	}

	//
	return firstfs;
}

weightconfig_t* ReadWeightConfig( char* filename )
{
	int				  newindent, avail = 0, n;
	token_t			  token;
	source_t*		  source;
	fuzzyseperator_t* fs;
	weightconfig_t*	  config = NULL;
#ifdef DEBUG
	int starttime;

	starttime = Sys_MilliSeconds();
#endif // DEBUG

	if( !LibVarGetValue( "bot_reloadcharacters" ) ) {
		avail = -1;

		for( n = 0; n < MAX_WEIGHT_FILES; n++ ) {
			config = weightFileList[n];

			if( !config ) {
				if( avail == -1 ) {
					avail = n;
				}

				continue;
			}

			if( strcmp( filename, config->filename ) == 0 ) {
				// botimport.Print( PRT_MESSAGE, "retained %s\n", filename );
				return config;
			}
		}

		if( avail == -1 ) {
			botimport.Print( PRT_ERROR, "weightFileList was full trying to load %s\n", filename );
			return NULL;
		}
	}

	source = LoadSourceFile( filename );

	if( !source ) {
		botimport.Print( PRT_ERROR, "counldn't load %s\n", filename );
		return NULL;
	}

	//
	config			   = ( weightconfig_t* )GetClearedMemory( sizeof( weightconfig_t ) );
	config->numweights = 0;
	Q_strncpyz( config->filename, filename, sizeof( config->filename ) );

	// parse the item config file
	while( PC_ReadToken( source, &token ) ) {
		if( !strcmp( token.string, "weight" ) ) {
			if( config->numweights >= MAX_WEIGHTS ) {
				SourceWarning( source, "too many fuzzy weights\n" );
				break;
			}

			if( !PC_ExpectTokenType( source, TT_STRING, 0, &token ) ) {
				FreeWeightConfig( config );
				FreeSource( source );
				return NULL;
			}

			StripDoubleQuotes( token.string );
			config->weights[config->numweights].name = ( char* )GetClearedMemory( strlen( token.string ) + 1 );
			strcpy( config->weights[config->numweights].name, token.string );

			if( !PC_ExpectAnyToken( source, &token ) ) {
				FreeWeightConfig( config );
				FreeSource( source );
				return NULL;
			}

			newindent = qfalse;

			if( !strcmp( token.string, "{" ) ) {
				newindent = qtrue;

				if( !PC_ExpectAnyToken( source, &token ) ) {
					FreeWeightConfig( config );
					FreeSource( source );
					return NULL;
				}
			}

			if( !strcmp( token.string, "switch" ) ) {
				fs = ReadFuzzySeperators_r( source );

				if( !fs ) {
					FreeWeightConfig( config );
					FreeSource( source );
					return NULL;
				}

				config->weights[config->numweights].firstseperator = fs;

			} else if( !strcmp( token.string, "return" ) ) {
				fs		  = ( fuzzyseperator_t* )GetClearedMemory( sizeof( fuzzyseperator_t ) );
				fs->index = 0;
				fs->value = MAX_INVENTORYVALUE;
				fs->next  = NULL;
				fs->child = NULL;

				if( !ReadFuzzyWeight( source, fs ) ) {
					FreeMemory( fs );
					FreeWeightConfig( config );
					FreeSource( source );
					return NULL;
				}

				config->weights[config->numweights].firstseperator = fs;

			} else {
				SourceError( source, "invalid name %s\n", token.string );
				FreeWeightConfig( config );
				FreeSource( source );
				return NULL;
			}

			if( newindent ) {
				if( !PC_ExpectTokenString( source, "}" ) ) {
					FreeWeightConfig( config );
					FreeSource( source );
					return NULL;
				}
			}

			config->numweights++;

		} else {
			SourceError( source, "invalid name %s\n", token.string );
			FreeWeightConfig( config );
			FreeSource( source );
			return NULL;
		}
	}

	// free the source at the end of a pass
	FreeSource( source );
	// if the file was located in a pak file
	botimport.Print( PRT_MESSAGE, "loaded %s\n", filename );
#ifdef DEBUG

	if( bot_developer ) {
		botimport.Print( PRT_MESSAGE, "weights loaded in %d msec\n", Sys_MilliSeconds() - starttime );
	}

#endif // DEBUG

	//
	if( !LibVarGetValue( "bot_reloadcharacters" ) ) {
		weightFileList[avail] = config;
	}

	//
	return config;
}

#if 0
qboolean WriteFuzzyWeight( FILE *fp, fuzzyseperator_t* fs )
{
	if( fs->type == WT_BALANCE ) {
		if( fprintf( fp, " return balance(" ) < 0 ) {
			return qfalse;
		}

		if( !WriteFloat( fp, fs->weight ) ) {
			return qfalse;
		}

		if( fprintf( fp, "," ) < 0 ) {
			return qfalse;
		}

		if( !WriteFloat( fp, fs->minweight ) ) {
			return qfalse;
		}

		if( fprintf( fp, "," ) < 0 ) {
			return qfalse;
		}

		if( !WriteFloat( fp, fs->maxweight ) ) {
			return qfalse;
		}

		if( fprintf( fp, ");\n" ) < 0 ) {
			return qfalse;
		}

	} else {
		if( fprintf( fp, " return " ) < 0 ) {
			return qfalse;
		}

		if( !WriteFloat( fp, fs->weight ) ) {
			return qfalse;
		}

		if( fprintf( fp, ";\n" ) < 0 ) {
			return qfalse;
		}
	}

	return qtrue;
}

qboolean WriteFuzzySeperators_r( FILE* fp, fuzzyseperator_t* fs, int indent )
{
	if( !WriteIndent( fp, indent ) ) {
		return qfalse;
	}

	if( fprintf( fp, "switch(%d)\n", fs->index ) < 0 ) {
		return qfalse;
	}

	if( !WriteIndent( fp, indent ) ) {
		return qfalse;
	}

	if( fprintf( fp, "{\n" ) < 0 ) {
		return qfalse;
	}

	indent++;

	do {
		if( !WriteIndent( fp, indent ) ) {
			return qfalse;
		}

		if( fs->next ) {
			if( fprintf( fp, "case %d:", fs->value ) < 0 ) {
				return qfalse;
			}

		} else {
			if( fprintf( fp, "default:" ) < 0 ) {
				return qfalse;
			}
		}

		if( fs->child ) {
			if( fprintf( fp, "\n" ) < 0 ) {
				return qfalse;
			}

			if( !WriteIndent( fp, indent ) ) {
				return qfalse;
			}

			if( fprintf( fp, "{\n" ) < 0 ) {
				return qfalse;
			}

			if( !WriteFuzzySeperators_r( fp, fs->child, indent + 1 ) ) {
				return qfalse;
			}

			if( !WriteIndent( fp, indent ) ) {
				return qfalse;
			}

			if( fs->next ) {
				if( fprintf( fp, "} //end case\n" ) < 0 ) {
					return qfalse;
				}

			} else {
				if( fprintf( fp, "} //end default\n" ) < 0 ) {
					return qfalse;
				}
			}

		} else {
			if( !WriteFuzzyWeight( fp, fs ) ) {
				return qfalse;
			}
		}

		fs = fs->next;
	} while( fs );

	indent--;

	if( !WriteIndent( fp, indent ) ) {
		return qfalse;
	}

	if( fprintf( fp, "} //end switch\n" ) < 0 ) {
		return qfalse;
	}

	return qtrue;
}

qboolean WriteWeightConfig( char* filename, weightconfig_t* config )
{
	int		  i;
	FILE*	  fp;
	weight_t* ifw;

	fp = fopen( filename, "wb" );

	if( !fp ) {
		return qfalse;
	}

	for( i = 0; i < config->numweights; i++ ) {
		ifw = &config->weights[i];

		if( fprintf( fp, "\nweight \"%s\"\n", ifw->name ) < 0 ) {
			return qfalse;
		}

		if( fprintf( fp, "{\n" ) < 0 ) {
			return qfalse;
		}

		if( ifw->firstseperator->index > 0 ) {
			if( !WriteFuzzySeperators_r( fp, ifw->firstseperator, 1 ) ) {
				return qfalse;
			}

		} else {
			if( !WriteIndent( fp, 1 ) ) {
				return qfalse;
			}

			if( !WriteFuzzyWeight( fp, ifw->firstseperator ) ) {
				return qfalse;
			}
		}

		if( fprintf( fp, "} //end weight\n" ) < 0 ) {
			return qfalse;
		}
	}

	fclose( fp );
	return qtrue;
}

#endif

int FindFuzzyWeight( weightconfig_t* wc, char* name )
{
	int i;

	for( i = 0; i < wc->numweights; i++ ) {
		if( !strcmp( wc->weights[i].name, name ) ) {
			return i;
		}
	}

	return -1;
}

/*!
	\brief Recursively calculates a fuzzy weight from inventory counts using a separator tree, blending weights between thresholds.

	Starting at the supplied separator node, the function compares the inventory value at the node’s index to the node’s threshold. If the value is below the threshold, it descends into the child node
   if one exists; otherwise it returns the node’s weight. When the value meets or exceeds the threshold and a subsequent node exists, the function checks whether the value lies between the current and
   the next node’s thresholds. If so, it interpolates a weight by linearly blending the weight (or child weight) of the current node with that of the next node, using the relative position of the
   value within the interval as the scaling factor. If the value exceeds the next node’s threshold, the recursion proceeds to that next node. The process continues until a terminal node is
   encountered, whose weight is returned.

	\param inventory Array of integer inventory quantities, indexed by the separator node’s index field
	\param fs Pointer to the current node in the fuzzy separator tree being evaluated
	\return A floating point weight computed for the given inventory entry, representing the fuzzy interpolation between separator nodes.
*/
float FuzzyWeight_r( int* inventory, fuzzyseperator_t* fs )
{
	float scale, w1, w2;

	if( inventory[fs->index] < fs->value ) {
		if( fs->child ) {
			return FuzzyWeight_r( inventory, fs->child );

		} else {
			return fs->weight;
		}

	} else if( fs->next ) {
		if( inventory[fs->index] < fs->next->value ) {
			// first weight
			if( fs->child ) {
				w1 = FuzzyWeight_r( inventory, fs->child );

			} else {
				w1 = fs->weight;
			}

			// second weight
			if( fs->next->child ) {
				w2 = FuzzyWeight_r( inventory, fs->next->child );

			} else {
				w2 = fs->next->weight;
			}

			// the scale factor
			scale = ( inventory[fs->index] - fs->value ) / ( fs->next->value - fs->value );
			// scale between the two weights
			return scale * w1 + ( 1 - scale ) * w2;
		}

		return FuzzyWeight_r( inventory, fs->next );
	}

	return fs->weight;
}

/*!
	\brief Computes a fuzzy weight for an inventory value by recursively traversing separators and blending weights between nodes.

	The function walks a linked list of fuzzy separator nodes. For each node it compares the inventory count at the node’s index to its threshold value. If the count is below the threshold, it
   recurses into the child node if present; otherwise it returns a random weight between the node’s min and max weight fields. If the count meets or exceeds the threshold and a next node exists, the
   function checks whether the count is still below the next node’s threshold. In that case it obtains a weight from the current child (or random if none) and a weight from the next child (or random
   if none), blends them in proportion to how far the count lies between the two thresholds, and returns the blended result. If the count exceeds the next node’s threshold, execution recurses to the
   next node. When no next node exists the stored weight of the current node is returned. Random values are generated by calling random(), yielding a number between 0 and 1.

	\param inventory Array of inventory counts used to evaluate thresholds for each separator node.
	\param fs Pointer to a fuzzy separator node that contains threshold information, weight ranges, and links to child and next nodes.
	\return The floating‑point weight computed for the given inventory state.
*/
float FuzzyWeightUndecided_r( int* inventory, fuzzyseperator_t* fs )
{
	float scale, w1, w2;

	if( inventory[fs->index] < fs->value ) {
		if( fs->child ) {
			return FuzzyWeightUndecided_r( inventory, fs->child );

		} else {
			return fs->minweight + random() * ( fs->maxweight - fs->minweight );
		}

	} else if( fs->next ) {
		if( inventory[fs->index] < fs->next->value ) {
			// first weight
			if( fs->child ) {
				w1 = FuzzyWeightUndecided_r( inventory, fs->child );

			} else {
				w1 = fs->minweight + random() * ( fs->maxweight - fs->minweight );
			}

			// second weight
			if( fs->next->child ) {
				w2 = FuzzyWeight_r( inventory, fs->next->child );

			} else {
				w2 = fs->next->minweight + random() * ( fs->next->maxweight - fs->next->minweight );
			}

			// the scale factor
			scale = ( inventory[fs->index] - fs->value ) / ( fs->next->value - fs->value );
			// scale between the two weights
			return scale * w1 + ( 1 - scale ) * w2;
		}

		return FuzzyWeightUndecided_r( inventory, fs->next );
	}

	return fs->weight;
}

float FuzzyWeight( int* inventory, weightconfig_t* wc, int weightnum )
{
#ifdef EVALUATERECURSIVELY
	return FuzzyWeight_r( inventory, wc->weights[weightnum].firstseperator );
#else
	fuzzyseperator_t* s;

	s = wc->weights[weightnum].firstseperator;

	if( !s ) {
		return 0;
	}

	while( 1 ) {
		if( inventory[s->index] < s->value ) {
			if( s->child ) {
				s = s->child;

			} else {
				return s->weight;
			}

		} else {
			if( s->next ) {
				s = s->next;

			} else {
				return s->weight;
			}
		}
	}

	return 0;
#endif
}

float FuzzyWeightUndecided( int* inventory, weightconfig_t* wc, int weightnum )
{
#ifdef EVALUATERECURSIVELY
	return FuzzyWeightUndecided_r( inventory, wc->weights[weightnum].firstseperator );
#else
	fuzzyseperator_t* s;

	s = wc->weights[weightnum].firstseperator;

	if( !s ) {
		return 0;
	}

	while( 1 ) {
		if( inventory[s->index] < s->value ) {
			if( s->child ) {
				s = s->child;

			} else {
				return s->minweight + random() * ( s->maxweight - s->minweight );
			}

		} else {
			if( s->next ) {
				s = s->next;

			} else {
				return s->minweight + random() * ( s->maxweight - s->minweight );
			}
		}
	}

	return 0;
#endif
}

/*!
	\brief Recursively adjusts the weights of a fuzzy separator tree node.

	This function descends through the linked list of child and next separator nodes, applying evolutionary changes to nodes of type WT_BALANCE. For each such node, it randomly chooses between a rare
   full mutation and a less frequent normal mutation, modifying the weight by a random fraction of the weight range. If the weight falls outside its defined bounds after the change, the bounds are
   tightened to match the new weight values.

	\param fs pointer to the fuzzy separator node being evolved
*/
void EvolveFuzzySeperator_r( fuzzyseperator_t* fs )
{
	if( fs->child ) {
		EvolveFuzzySeperator_r( fs->child );

	} else if( fs->type == WT_BALANCE ) {
		// every once in a while an evolution leap occurs, mutation
		if( random() < 0.01 ) {
			fs->weight += crandom() * ( fs->maxweight - fs->minweight );

		} else {
			fs->weight += crandom() * ( fs->maxweight - fs->minweight ) * 0.5;
		}

		// modify bounds if necesary because of mutation
		if( fs->weight < fs->minweight ) {
			fs->minweight = fs->weight;

		} else if( fs->weight > fs->maxweight ) {
			fs->maxweight = fs->weight;
		}
	}

	if( fs->next ) {
		EvolveFuzzySeperator_r( fs->next );
	}
}

void EvolveWeightConfig( weightconfig_t* config )
{
	int i;

	for( i = 0; i < config->numweights; i++ ) {
		EvolveFuzzySeperator_r( config->weights[i].firstseperator );
	}
}

/*!
	\brief Recursively scales the weight of fuzzy separator nodes, clamping the result to the specified bounds.

	The function traverses a linked list of fuzzy separator nodes via child and next pointers. For nodes of type WT_BALANCE, it first applies the scaling factor to the sum of maximum and minimum
   weights, then restricts the updated weight to lie within the original min and max limits. Child and sibling nodes are processed in depth‑first order.

	\param fs Pointer to the root fuzzy separator node to scale
	\param scale Scalar multiplier applied to the node weight
*/
void ScaleFuzzySeperator_r( fuzzyseperator_t* fs, float scale )
{
	if( fs->child ) {
		ScaleFuzzySeperator_r( fs->child, scale );

	} else if( fs->type == WT_BALANCE ) {
		//
		fs->weight = ( fs->maxweight + fs->minweight ) * scale;

		// get the weight between bounds
		if( fs->weight < fs->minweight ) {
			fs->weight = fs->minweight;

		} else if( fs->weight > fs->maxweight ) {
			fs->weight = fs->maxweight;
		}
	}

	if( fs->next ) {
		ScaleFuzzySeperator_r( fs->next, scale );
	}
}

void ScaleWeight( weightconfig_t* config, char* name, float scale )
{
	int i;

	if( scale < 0 ) {
		scale = 0;

	} else if( scale > 1 ) {
		scale = 1;
	}

	for( i = 0; i < config->numweights; i++ ) {
		if( !strcmp( name, config->weights[i].name ) ) {
			ScaleFuzzySeperator_r( config->weights[i].firstseperator, scale );
			break;
		}
	}
}

/*!
	\brief Recursively scales the weight ranges of balance-type fuzzy separators within a separator tree around their midpoints.

	This function walks the fuzzy separator tree, visiting each node in depth‑first order. When a node is of type WT_BALANCE, its minweight and maxweight are adjusted by moving them towards or away
   from their midpoint according to the supplied scale factor. A scale < 1 contracts the range, whereas a scale > 1 expands it. If the new maximum becomes smaller than the new minimum, the maximum is
   clamped to match the minimum. The function then recurses to the node's sibling via the next pointer. The operation mutates the minweight and maxweight values of the tree.

	It assumes that the supplied pointer is not null and that the type WT_BALANCE is defined in the surrounding code.

	TODO: clarify whether NULL pointers are allowed and if any synchronization is required.

	\param fs Pointer to the fuzzy separator node (or subtree) to be processed. The function recurses through child and next nodes.
	\param scale Factor determining how far the weight range moves from its midpoint. Values less than one shrink the range, values greater than one enlarge it.
*/
void ScaleFuzzySeperatorBalanceRange_r( fuzzyseperator_t* fs, float scale )
{
	if( fs->child ) {
		ScaleFuzzySeperatorBalanceRange_r( fs->child, scale );

	} else if( fs->type == WT_BALANCE ) {
		float mid = ( fs->minweight + fs->maxweight ) * 0.5;
		// get the weight between bounds
		fs->maxweight = mid + ( fs->maxweight - mid ) * scale;
		fs->minweight = mid + ( fs->minweight - mid ) * scale;

		if( fs->maxweight < fs->minweight ) {
			fs->maxweight = fs->minweight;
		}
	}

	if( fs->next ) {
		ScaleFuzzySeperatorBalanceRange_r( fs->next, scale );
	}
}

/*!
	\brief Clamps a scaling factor to the 0–100 range and applies it to the fuzzy balance range of every separator in a weight configuration.

	The function first restricts the supplied scale to the valid interval [0, 100]. It then iterates over all weights stored in the configuration, calling ScaleFuzzySeperatorBalanceRange_r on each
   weight's first separator. This call adjusts that separator's fuzzy balance range according to the clamped scale factor.

	\param config Pointer to a weightconfig_t structure containing the weights whose separators will be scaled
	\param scale The scaling factor used when adjusting each separator; values below 0 are treated as 0, values above 100 are treated as 100
	\return None
*/
void ScaleFuzzyBalanceRange( weightconfig_t* config, float scale )
{
	int i;

	if( scale < 0 ) {
		scale = 0;

	} else if( scale > 100 ) {
		scale = 100;
	}

	for( i = 0; i < config->numweights; i++ ) {
		ScaleFuzzySeperatorBalanceRange_r( config->weights[i].firstseperator, scale );
	}
}

/*!
	\brief Combines two fuzzy separator configurations into a third one, returning a success status.

	The function walks the child and next links of the three separators in parallel.  When a node is of type WT_BALANCE, the weight in fsout is set to the average of the weights from fs1 and fs2, and
   its min and max bounds are adjusted if necessary.  For all other node types the function simply propagates through the structure, recursing on children and on next nodes.  If any mismatches in
   structure—such as a missing child or next link, or differing balance types—are detected, the function prints an error message and returns qfalse.  Success occurs only when all corresponding
   sub‑branches match and all values are merged.

	\param fs1 First fuzzy separator to be merged
	\param fs2 Second fuzzy separator to be merged
	\param fsout Destination separator that receives the combined configuration
	\return qtrue when the two input separators match in structure and the merge succeeds; qfalse otherwise
*/
int InterbreedFuzzySeperator_r( fuzzyseperator_t* fs1, fuzzyseperator_t* fs2, fuzzyseperator_t* fsout )
{
	if( fs1->child ) {
		if( !fs2->child || !fsout->child ) {
			botimport.Print( PRT_ERROR, "cannot interbreed weight configs, unequal child\n" );
			return qfalse;
		}

		if( !InterbreedFuzzySeperator_r( fs2->child, fs2->child, fsout->child ) ) {
			return qfalse;
		}

	} else if( fs1->type == WT_BALANCE ) {
		if( fs2->type != WT_BALANCE || fsout->type != WT_BALANCE ) {
			botimport.Print( PRT_ERROR, "cannot interbreed weight configs, unequal balance\n" );
			return qfalse;
		}

		fsout->weight = ( fs1->weight + fs2->weight ) / 2;

		if( fsout->weight > fsout->maxweight ) {
			fsout->maxweight = fsout->weight;
		}

		if( fsout->weight > fsout->minweight ) {
			fsout->minweight = fsout->weight;
		}
	}

	if( fs1->next ) {
		if( !fs2->next || !fsout->next ) {
			botimport.Print( PRT_ERROR, "cannot interbreed weight configs, unequal next\n" );
			return qfalse;
		}

		if( !InterbreedFuzzySeperator_r( fs1->next, fs2->next, fsout->next ) ) {
			return qfalse;
		}
	}

	return qtrue;
}

void InterbreedWeightConfigs( weightconfig_t* config1, weightconfig_t* config2, weightconfig_t* configout )
{
	int i;

	if( config1->numweights != config2->numweights || config1->numweights != configout->numweights ) {
		botimport.Print( PRT_ERROR, "cannot interbreed weight configs, unequal numweights\n" );
		return;
	}

	for( i = 0; i < config1->numweights; i++ ) {
		InterbreedFuzzySeperator_r( config1->weights[i].firstseperator, config2->weights[i].firstseperator, configout->weights[i].firstseperator );
	}
}

void BotShutdownWeights()
{
	int i;

	for( i = 0; i < MAX_WEIGHT_FILES; i++ ) {
		if( weightFileList[i] ) {
			FreeWeightConfig2( weightFileList[i] );
			weightFileList[i] = NULL;
		}
	}
}
