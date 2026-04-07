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
 * name:		be_ai_chat.c
 *
 * desc:		bot chat AI
 *
 *
 *****************************************************************************/

#include "q_shared.h"
// #include "../server/server.h"
#include "l_memory.h"
#include "l_libvar.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_utils.h"
#include "l_log.h"
#include "aasfile.h"
#include "botshared/botlib.h"
#include "botshared/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "botshared/be_ea.h"
#include "botshared/be_ai_chat.h"

// escape character
#define ESCAPE_CHAR			   0x01 //'_'
//
// "hi ", people, " ", 0, " entered the game"
// becomes:
// "hi _rpeople_ _v0_ entered the game"
//

// match piece types
#define MT_VARIABLE			   1 // variable match piece
#define MT_STRING			   2 // string match piece
// reply chat key flags
#define RCKFL_AND			   1   // key must be present
#define RCKFL_NOT			   2   // key must be absent
#define RCKFL_NAME			   4   // name of bot must be present
#define RCKFL_STRING		   8   // key is a string
#define RCKFL_VARIABLES		   16  // key is a match template
#define RCKFL_BOTNAMES		   32  // key is a series of botnames
#define RCKFL_GENDERFEMALE	   64  // bot must be female
#define RCKFL_GENDERMALE	   128 // bot must be male
#define RCKFL_GENDERLESS	   256 // bot must be genderless
// time to ignore a chat message after using it
#define CHATMESSAGE_RECENTTIME 20

// the actuall chat messages
typedef struct bot_chatmessage_s {
	char*					  chatmessage; // chat message string
	float					  time;		   // last time used
	struct bot_chatmessage_s* next;		   // next chat message in a list
} bot_chatmessage_t;

// bot chat type with chat lines
typedef struct bot_chattype_s {
	char				   name[MAX_CHATTYPE_NAME];
	int					   numchatmessages;
	bot_chatmessage_t*	   firstchatmessage;
	struct bot_chattype_s* next;
} bot_chattype_t;

// bot chat lines
typedef struct bot_chat_s {
	bot_chattype_t* types;
} bot_chat_t;

// random string
typedef struct bot_randomstring_s {
	char*					   string;
	struct bot_randomstring_s* next;
} bot_randomstring_t;

// list with random strings
typedef struct bot_randomlist_s {
	char*					 string;
	int						 numstrings;
	bot_randomstring_t*		 firstrandomstring;
	struct bot_randomlist_s* next;
} bot_randomlist_t;

// synonym
typedef struct bot_synonym_s {
	char*				  string;
	float				  weight;
	struct bot_synonym_s* next;
} bot_synonym_t;

// list with synonyms
typedef struct bot_synonymlist_s {
	unsigned long int		  context;
	float					  totalweight;
	bot_synonym_t*			  firstsynonym;
	struct bot_synonymlist_s* next;
} bot_synonymlist_t;

// fixed match string
typedef struct bot_matchstring_s {
	char*					  string;
	struct bot_matchstring_s* next;
} bot_matchstring_t;

// piece of a match template
typedef struct bot_matchpiece_s {
	int						 type;
	bot_matchstring_t*		 firststring;
	int						 variable;
	struct bot_matchpiece_s* next;
} bot_matchpiece_t;

// match template
typedef struct bot_matchtemplate_s {
	unsigned long int			context;
	int							type;
	int							subtype;
	bot_matchpiece_t*			first;
	struct bot_matchtemplate_s* next;
} bot_matchtemplate_t;

// reply chat key
typedef struct bot_replychatkey_s {
	int						   flags;
	char*					   string;
	bot_matchpiece_t*		   match;
	struct bot_replychatkey_s* next;
} bot_replychatkey_t;

// reply chat
typedef struct bot_replychat_s {
	bot_replychatkey_t*		keys;
	float					priority;
	int						numchatmessages;
	bot_chatmessage_t*		firstchatmessage;
	struct bot_replychat_s* next;
} bot_replychat_t;

// string list
typedef struct bot_stringlist_s {
	char*					 string;
	struct bot_stringlist_s* next;
} bot_stringlist_t;

// chat state of a bot
typedef struct bot_chatstate_s {
	int					  gender;	// 0=it, 1=female, 2=male
	char				  name[32]; // name of the bot
	char				  chatmessage[MAX_MESSAGE_SIZE];
	int					  handle;
	// the console messages visible to the bot
	bot_consolemessage_t* firstmessage; // first message is the first typed message
	bot_consolemessage_t* lastmessage;	// last message is the last typed message, bottom of console
	// number of console messages stored in the state
	int					  numconsolemessages;
	// the bot chat lines
	bot_chat_t*			  chat;
} bot_chatstate_t;

typedef struct {
	bot_chat_t* chat;
	int			inuse;
	char		filename[MAX_QPATH];
	char		chatname[MAX_QPATH];
} bot_ichatdata_t;

bot_ichatdata_t		  ichatdata[MAX_CLIENTS];

bot_chatstate_t*	  botchatstates[MAX_CLIENTS + 1];
// console message heap
bot_consolemessage_t* consolemessageheap  = NULL;
bot_consolemessage_t* freeconsolemessages = NULL;
// list with match strings
bot_matchtemplate_t*  matchtemplates = NULL;
// list with synonyms
bot_synonymlist_t*	  synonyms = NULL;
// list with random strings
bot_randomlist_t*	  randomstrings = NULL;
// reply chats
bot_replychat_t*	  replychats = NULL;

/*!
	\brief Retrieves the chat state structure for a given handle after verifying its validity.

	The handle must be within the range 1 to MAX_CLIENTS inclusive. If it is out of range or if no chat state exists for that index, a fatal error is logged and NULL is returned. Otherwise the
   botchatstates entry for that handle is returned.

	\param handle Index identifying the chat state to retrieve.
	\return Pointer to the bot_chatstate_t structure corresponding to the handle, or NULL if the handle is invalid or unassigned.
*/
bot_chatstate_t*	  BotChatStateFromHandle( int handle )
{
	if( handle <= 0 || handle > MAX_CLIENTS ) {
		botimport.Print( PRT_FATAL, "chat state handle %d out of range\n", handle );
		return NULL;
	}

	if( !botchatstates[handle] ) {
		botimport.Print( PRT_FATAL, "invalid chat state %d\n", handle );
		return NULL;
	}

	return botchatstates[handle];
}

/*!
	\brief Initializes the console message heap by allocating storage for a configurable number of message objects and setting up a linked list of free messages.

	If an existing heap exists it is freed first. The function determines the desired number of messages from the console variable "max_messages", defaulting to 1024, and allocates that many clean
   bot_consolemessage_t structures. Each structure’s prev and next pointers are wired into a doubly‑linked list, with the first element's prev set to null and the last element's next set to null. The
   global pointer freeconsolemessages is then updated to point at the start of this list, marking all messages as free for future use.

*/
void InitConsoleMessageHeap()
{
	int i, max_messages;

	if( consolemessageheap ) {
		FreeMemory( consolemessageheap );
	}

	//
	max_messages			   = ( int )LibVarValue( "max_messages", "1024" );
	consolemessageheap		   = ( bot_consolemessage_t* )GetClearedHunkMemory( max_messages * sizeof( bot_consolemessage_t ) );
	consolemessageheap[0].prev = NULL;
	consolemessageheap[0].next = &consolemessageheap[1];

	for( i = 1; i < max_messages - 1; i++ ) {
		consolemessageheap[i].prev = &consolemessageheap[i - 1];
		consolemessageheap[i].next = &consolemessageheap[i + 1];
	}

	consolemessageheap[max_messages - 1].prev = &consolemessageheap[max_messages - 2];
	consolemessageheap[max_messages - 1].next = NULL;
	// pointer to the free console messages
	freeconsolemessages = consolemessageheap;
}

/*!
	\brief Retrieves a console message from the global free list

	AllocConsoleMessage removes the head node of the global freeconsolemessages linked list and returns it for use. If more nodes remain, the next node's prev pointer is set to NULL. The function
   returns a pointer to a bot_consolemessage_t instance, or null when the free list is empty.

	\return A pointer to a bot_consolemessage_t or null if the free list is empty
*/
bot_consolemessage_t* AllocConsoleMessage()
{
	bot_consolemessage_t* message;
	message = freeconsolemessages;

	if( freeconsolemessages ) {
		freeconsolemessages = freeconsolemessages->next;
	}

	if( freeconsolemessages ) {
		freeconsolemessages->prev = NULL;
	}

	return message;
}

/*!
	\brief Adds a console message to the reusable list of free messages

	The function inserts the supplied message at the front of the global free list of console messages. If a free list already exists it links the new message via its prev pointer; otherwise the list
   is empty. The message’s prev field is cleared and its next pointer is set to the old head. Finally the global pointer is updated to point to this message, making it ready for future reuse.

	\param message pointer to the console message to be added to the free list
*/
void FreeConsoleMessage( bot_consolemessage_t* message )
{
	if( freeconsolemessages ) {
		freeconsolemessages->prev = message;
	}

	message->prev		= NULL;
	message->next		= freeconsolemessages;
	freeconsolemessages = message;
}

void BotRemoveConsoleMessage( int chatstate, int handle )
{
	bot_consolemessage_t *m, *nextm;
	bot_chatstate_t*	  cs;

	cs = BotChatStateFromHandle( chatstate );

	if( !cs ) {
		return;
	}

	for( m = cs->firstmessage; m; m = nextm ) {
		nextm = m->next;

		if( m->handle == handle ) {
			if( m->next ) {
				m->next->prev = m->prev;

			} else {
				cs->lastmessage = m->prev;
			}

			if( m->prev ) {
				m->prev->next = m->next;

			} else {
				cs->firstmessage = m->next;
			}

			FreeConsoleMessage( m );
			cs->numconsolemessages--;
			break;
		}
	}
}

void BotQueueConsoleMessage( int chatstate, int type, char* message )
{
	bot_consolemessage_t* m;
	bot_chatstate_t*	  cs;

	cs = BotChatStateFromHandle( chatstate );

	if( !cs ) {
		return;
	}

	m = AllocConsoleMessage();

	if( !m ) {
		botimport.Print( PRT_ERROR, "empty console message heap\n" );
		return;
	}

	cs->handle++;

	if( cs->handle <= 0 || cs->handle > 8192 ) {
		cs->handle = 1;
	}

	m->handle = cs->handle;
	m->time	  = AAS_Time();
	m->type	  = type;
	strncpy( m->message, message, MAX_MESSAGE_SIZE );
	m->next = NULL;

	if( cs->lastmessage ) {
		cs->lastmessage->next = m;
		m->prev				  = cs->lastmessage;
		cs->lastmessage		  = m;

	} else {
		cs->lastmessage	 = m;
		cs->firstmessage = m;
		m->prev			 = NULL;
	}

	cs->numconsolemessages++;
}

int BotNextConsoleMessage( int chatstate, bot_consolemessage_t* cm )
{
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle( chatstate );

	if( !cs ) {
		return 0;
	}

	if( cs->firstmessage ) {
		memcpy( cm, cs->firstmessage, sizeof( bot_consolemessage_t ) );
		cm->next = cm->prev = NULL;
		return cm->handle;
	}

	return 0;
}

int BotNumConsoleMessages( int chatstate )
{
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle( chatstate );

	if( !cs ) {
		return 0;
	}

	return cs->numconsolemessages;
}

/*!
	\brief Determines if a character should be treated as whitespace during string normalization.

	This function returns a non‑zero value if the supplied character is not an alphanumeric character or one of a small set of punctuation symbols used to delimit words. Characters considered
   alphanumeric (a–z, A–Z, 0–9) and the punctuation characters '(', ')', '?', ''', ':', ',', '[', ']', '-', '_', '+', and '=' are treated as non‑whitespace and cause the function to return 0. All
   other characters—including space, tab, newline, and other symbols—are considered whitespace and cause a non‑zero return value. The function is used by UnifyWhiteSpaces to collapse repeated
   whitespace and trim leading or trailing spaces.

	\param c Character to test.
	\return Non‑zero if the character is to be treated as whitespace; zero otherwise.
*/
int IsWhiteSpace( char c )
{
	if( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) || c == '(' || c == ')' || c == '?' || c == '\'' || c == ':' || c == ',' || c == '[' || c == ']' || c == '-' ||
		c == '_' || c == '+' || c == '=' ) {
		return qfalse;
	}

	return qtrue;
}

/*!
	\brief Removes all tilde characters from a null-terminated string in place.

	The function iterates over the input string. Whenever a '~' is encountered, the remaining portion of the string is moved one character to the left using memmove, effectively deleting that
   character. The string remains null-terminated.

	\param message Pointer to the string to be processed; the string is modified directly.
*/
void BotRemoveTildes( char* message )
{
	int i;

	// remove all tildes from the chat message
	for( i = 0; message[i]; i++ ) {
		if( message[i] == '~' ) {
			memmove( &message[i], &message[i + 1], strlen( &message[i + 1] ) + 1 );
		}
	}
}

void UnifyWhiteSpaces( char* string )
{
	char *ptr, *oldptr;

	for( ptr = oldptr = string; *ptr; oldptr = ptr ) {
		while( *ptr && IsWhiteSpace( *ptr ) ) {
			ptr++;
		}

		if( ptr > oldptr ) {
			// if not at the start and not at the end of the string
			// write only one space
			if( oldptr > string && *ptr ) {
				*oldptr++ = ' ';
			}

			// remove all other white spaces
			if( ptr > oldptr ) {
				memmove( oldptr, ptr, strlen( ptr ) + 1 );
			}
		}

		while( *ptr && !IsWhiteSpace( *ptr ) ) {
			ptr++;
		}
	}
}

int StringContains( char* str1, char* str2, int casesensitive )
{
	int len, i, j, index;

	if( str1 == NULL || str2 == NULL ) {
		return -1;
	}

	len	  = strlen( str1 ) - strlen( str2 );
	index = 0;

	for( i = 0; i <= len; i++, str1++, index++ ) {
		for( j = 0; str2[j]; j++ ) {
			if( casesensitive ) {
				if( str1[j] != str2[j] ) {
					break;
				}

			} else {
				if( toupper( str1[j] ) != toupper( str2[j] ) ) {
					break;
				}
			}
		}

		if( !str2[j] ) {
			return index;
		}
	}

	return -1;
}

/*!
	\brief Searches a source string for an entire word match and returns a pointer to its start.

	The function iterates over each word in the source string and compares it with the target word.
	It supports optional case sensitivity and ensures that only complete words are matched by checking that the match ends at a word boundary or the end of the string.
	If a match is found, a pointer to the beginning of the matched word inside the source string is returned; otherwise the function returns null.

	\param str1 The source string in which to search for the word.
	\param str2 The word to search for.
	\param casesensitive Non‑zero to require exact case; zero to ignore case.
	\return A pointer to the start of the matched word within the source string, or null if the word does not occur.
*/
char* StringContainsWord( char* str1, char* str2, int casesensitive )
{
	int len, i, j;

	len = strlen( str1 ) - strlen( str2 );

	for( i = 0; i <= len; i++, str1++ ) {
		// if not at the start of the string
		if( i ) {
			// skip to the start of the next word
			while( *str1 && *str1 != ' ' ) {
				str1++;
			}

			if( !*str1 ) {
				break;
			}

			str1++;
		}

		// compare the word
		for( j = 0; str2[j]; j++ ) {
			if( casesensitive ) {
				if( str1[j] != str2[j] ) {
					break;
				}

			} else {
				if( toupper( str1[j] ) != toupper( str2[j] ) ) {
					break;
				}
			}
		}

		// if there was a word match
		if( !str2[j] ) {
			// if the first string has an end of word
			if( !str1[j] || str1[j] == ' ' ) {
				return str1;
			}
		}
	}

	return NULL;
}

/*!
	\brief Replaces every full‑word occurrence of a given synonym with a replacement string inside a mutable C string, while avoiding overlapping replacements during the process.

	The function scans the input string for occurrences of the specified synonym word using StringContainsWord. For each match, it verifies that the replacement string does not already overlap that
   position—avoiding accidental double replacements that could arise if the replacement text contains the synonym. If the position is clear, the characters following the synonym are moved to make room
   for or to remove the extra characters introduced by a longer or shorter replacement. The replacement word is then copied over the synonym. This operation continues until no more occurrences of the
   synonym remain. The caller must supply a writable, null‑terminated buffer whose size is sufficient to hold the potentially longer result; otherwise a buffer overflow may occur. All modifications
   happen in place, so the original string is overwritten.

	\param string Pointer to the input string that will be modified in place; must be writable and null‑terminated
	\param synonym Word to search for within the string; replacement occurs only when matched as a whole word
	\param replacement Word that will replace each matched synonym; the buffer is adjusted to accommodate the new length
	\throws None
*/
void StringReplaceWords( char* string, char* synonym, char* replacement )
{
	char *str, *str2;

	// find the synonym in the string
	str = StringContainsWord( string, synonym, qfalse );

	// if the synonym occured in the string
	while( str ) {
		// if the synonym isn't part of the replacement which is already in the string
		// usefull for abreviations
		str2 = StringContainsWord( string, replacement, qfalse );

		while( str2 ) {
			if( str2 <= str && str < str2 + strlen( replacement ) ) {
				break;
			}

			str2 = StringContainsWord( str2 + 1, replacement, qfalse );
		}

		if( !str2 ) {
			memmove( str + strlen( replacement ), str + strlen( synonym ), strlen( str + strlen( synonym ) ) + 1 );
			// append the synonum replacement
			memcpy( str, replacement, strlen( replacement ) );
		}

		// find the next synonym in the string
		str = StringContainsWord( str + strlen( replacement ), synonym, qfalse );
	}
}

/*!
	\brief Outputs the contents of a synonym list to the log file

	Iterates through a linked list of synonym contexts. For each context it writes the context ID followed by a bracketed list of synonym strings and their associated weights. Each synonym is printed
   as "\"string\", weight" and separated by commas. The output is written to the file obtained from Log_FilePointer(). If the file pointer is null, the function does nothing.

	\param synlist Pointer to the head of a bot_synonymlist_t linked list containing synonym contexts.
*/
void BotDumpSynonymList( bot_synonymlist_t* synlist )
{
	FILE*			   fp;
	bot_synonymlist_t* syn;
	bot_synonym_t*	   synonym;

	fp = Log_FilePointer();

	if( !fp ) {
		return;
	}

	for( syn = synlist; syn; syn = syn->next ) {
		fprintf( fp, "%d : [", ( int )syn->context );

		for( synonym = syn->firstsynonym; synonym; synonym = synonym->next ) {
			fprintf( fp, "(\"%s\", %1.2f)", synonym->string, synonym->weight );

			if( synonym->next ) {
				fprintf( fp, ", " );
			}
		}

		fprintf( fp, "]\n" );
	}
}

/*!
	\brief Loads a file of bot synonym definitions and constructs a linked list of synonym groups.

	The function performs a two‑pass parse of the supplied file. In the first pass it scans the contents to compute the total amount of memory required for the bot_synonymlist_t and bot_synonym_t
   structures. In the second pass it allocates one contiguous block of memory with GetClearedHunkMemory and fills it with the data, preserving context information encoded as numeric tokens and
   tracking the total weight for each synonym group. Each group must contain at least two synonym entries; otherwise a syntax error is reported. Errors such as failure to open the file, malformed
   syntax, or exceeding maximum context depth result in a diagnostic message via botimport.Print, cleanup of any allocated resources, and a return value of null. On success the function logs that the
   file was loaded and returns a pointer to the head of the constructed list.

	\param filename Path to the file containing the synonym definitions; the function expects the file to exist and be readable.
	\return A pointer to the first element of the linked list of synonym groups, or null if loading fails.
*/
bot_synonymlist_t* BotLoadSynonyms( char* filename )
{
	int				   pass, size, contextlevel, numsynonyms;
	unsigned long int  context, contextstack[32];
	char*			   ptr = NULL;
	source_t*		   source;
	token_t			   token;
	bot_synonymlist_t *synlist, *lastsyn, *syn;
	bot_synonym_t *	   synonym, *lastsynonym;

	size	= 0;
	synlist = NULL; // make compiler happy
	syn		= NULL; // make compiler happy
	synonym = NULL; // make compiler happy

	// the synonyms are parsed in two phases
	for( pass = 0; pass < 2; pass++ ) {
		//
		if( pass && size ) {
			ptr = ( char* )GetClearedHunkMemory( size );
		}

		//
		source = LoadSourceFile( filename );

		if( !source ) {
			botimport.Print( PRT_ERROR, "counldn't load %s\n", filename );
			return NULL;
		}

		//
		context		 = 0;
		contextlevel = 0;
		synlist		 = NULL; // list synonyms
		lastsyn		 = NULL; // last synonym in the list

		//
		while( PC_ReadToken( source, &token ) ) {
			if( token.type == TT_NUMBER ) {
				context |= token.intvalue;
				contextstack[contextlevel] = token.intvalue;
				contextlevel++;

				if( contextlevel >= 32 ) {
					SourceError( source, "more than 32 context levels" );
					FreeSource( source );
					return NULL;
				}

				if( !PC_ExpectTokenString( source, "{" ) ) {
					FreeSource( source );
					return NULL;
				}

			} else if( token.type == TT_PUNCTUATION ) {
				if( !strcmp( token.string, "}" ) ) {
					contextlevel--;

					if( contextlevel < 0 ) {
						SourceError( source, "too many }" );
						FreeSource( source );
						return NULL;
					}

					context &= ~contextstack[contextlevel];

				} else if( !strcmp( token.string, "[" ) ) {
					size += sizeof( bot_synonymlist_t );

					if( pass ) {
						syn = ( bot_synonymlist_t* )ptr;
						ptr += sizeof( bot_synonymlist_t );
						syn->context	  = context;
						syn->firstsynonym = NULL;
						syn->next		  = NULL;

						if( lastsyn ) {
							lastsyn->next = syn;

						} else {
							synlist = syn;
						}

						lastsyn = syn;
					}

					numsynonyms = 0;
					lastsynonym = NULL;

					while( 1 ) {
						if( !PC_ExpectTokenString( source, "(" ) || !PC_ExpectTokenType( source, TT_STRING, 0, &token ) ) {
							FreeSource( source );
							return NULL;
						}

						StripDoubleQuotes( token.string );

						if( strlen( token.string ) <= 0 ) {
							SourceError( source, "empty string", token.string );
							FreeSource( source );
							return NULL;
						}

						size += sizeof( bot_synonym_t ) + strlen( token.string ) + 1;

						if( pass ) {
							synonym = ( bot_synonym_t* )ptr;
							ptr += sizeof( bot_synonym_t );
							synonym->string = ptr;
							ptr += strlen( token.string ) + 1;
							strcpy( synonym->string, token.string );

							//
							if( lastsynonym ) {
								lastsynonym->next = synonym;

							} else {
								syn->firstsynonym = synonym;
							}

							lastsynonym = synonym;
						}

						numsynonyms++;

						if( !PC_ExpectTokenString( source, "," ) || !PC_ExpectTokenType( source, TT_NUMBER, 0, &token ) || !PC_ExpectTokenString( source, ")" ) ) {
							FreeSource( source );
							return NULL;
						}

						if( pass ) {
							synonym->weight = token.floatvalue;
							syn->totalweight += synonym->weight;
						}

						if( PC_CheckTokenString( source, "]" ) ) {
							break;
						}

						if( !PC_ExpectTokenString( source, "," ) ) {
							FreeSource( source );
							return NULL;
						}
					}

					if( numsynonyms < 2 ) {
						SourceError( source, "synonym must have at least two entries\n" );
						FreeSource( source );
						return NULL;
					}

				} else {
					SourceError( source, "unexpected %s", token.string );
					FreeSource( source );
					return NULL;
				}
			}
		}

		//
		FreeSource( source );

		//
		if( contextlevel > 0 ) {
			SourceError( source, "missing }" );
			return NULL;
		}
	}

	botimport.Print( PRT_MESSAGE, "loaded %s\n", filename );
	//
	// BotDumpSynonymList(synlist);
	//
	return synlist;
}

//===========================================================================
// replace all the synonyms in the string
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotReplaceSynonyms( char* string, unsigned long int context )
{
	bot_synonymlist_t* syn;
	bot_synonym_t*	   synonym;

	for( syn = synonyms; syn; syn = syn->next ) {
		if( !( syn->context & context ) ) {
			continue;
		}

		for( synonym = syn->firstsynonym->next; synonym; synonym = synonym->next ) {
			StringReplaceWords( string, synonym->string, syn->firstsynonym->string );
		}
	}
}

/*!
	\brief Replaces all synonyms in a string with a randomly selected replacement from each applicable synonym group, weighted by synonym weights and constrained by a context mask.

	The function iterates over a global list of synonym groups, each having an associated context bitmask. For each group whose context matches the supplied mask, it selects a replacement synonym at
   random weighted by the individual synonym weights. All occurrences of the original synonyms in the input string are then replaced with the chosen synonym using the StringReplaceWords routine. The
   string is modified in place.

	\param string The string to modify in place.
	\param context Bitmask selecting which synonym groups are active during replacement.
*/
void BotReplaceWeightedSynonyms( char* string, unsigned long int context )
{
	bot_synonymlist_t* syn;
	bot_synonym_t *	   synonym, *replacement;
	float			   weight, curweight;

	for( syn = synonyms; syn; syn = syn->next ) {
		if( !( syn->context & context ) ) {
			continue;
		}

		// choose a weighted random replacement synonym
		weight = random() * syn->totalweight;

		if( !weight ) {
			continue;
		}

		curweight = 0;

		for( replacement = syn->firstsynonym; replacement; replacement = replacement->next ) {
			curweight += replacement->weight;

			if( weight < curweight ) {
				break;
			}
		}

		if( !replacement ) {
			continue;
		}

		// replace all synonyms with the replacement
		for( synonym = syn->firstsynonym; synonym; synonym = synonym->next ) {
			if( synonym == replacement ) {
				continue;
			}

			StringReplaceWords( string, synonym->string, replacement->string );
		}
	}
}

/*!
	\brief Replaces words in a string with their synonyms based on a context mask.

	The function scans the provided string word by word. For each word it iterates over a global list of synonym groups and checks whether the word matches any synonym in a group whose context bits
   intersect with the supplied context. When a match is found, the word is replaced in place by the first synonym of that group. The replacement uses memmove and memcpy so the buffer must have
   sufficient capacity; the function then continues scanning from the end of the replaced word. Only the first applicable synonym is replaced per word. The string is modified in place and no value is
   returned.

	\param context a bitmask that selects which synonym groups are considered
	\param string the character array to be modified in place
*/
void BotReplaceReplySynonyms( char* string, unsigned long int context )
{
	char *			   str1, *str2, *replacement;
	bot_synonymlist_t* syn;
	bot_synonym_t*	   synonym;

	for( str1 = string; *str1; ) {
		// go to the start of the next word
		while( *str1 && *str1 <= ' ' ) {
			str1++;
		}

		if( !*str1 ) {
			break;
		}

		//
		for( syn = synonyms; syn; syn = syn->next ) {
			if( !( syn->context & context ) ) {
				continue;
			}

			for( synonym = syn->firstsynonym->next; synonym; synonym = synonym->next ) {
				str2 = synonym->string;
				// if the synonym is not at the front of the string continue
				str2 = StringContainsWord( str1, synonym->string, qfalse );

				if( !str2 || str2 != str1 ) {
					continue;
				}

				//
				replacement = syn->firstsynonym->string;
				// if the replacement IS in front of the string continue
				str2 = StringContainsWord( str1, replacement, qfalse );

				if( str2 && str2 == str1 ) {
					continue;
				}

				//
				memmove( str1 + strlen( replacement ), str1 + strlen( synonym->string ), strlen( str1 + strlen( synonym->string ) ) + 1 );
				// append the synonum replacement
				memcpy( str1, replacement, strlen( replacement ) );
				//
				break;
			}

			// if a synonym has been replaced
			if( synonym ) {
				break;
			}
		}

		// skip over this word
		while( *str1 && *str1 > ' ' ) {
			str1++;
		}

		if( !*str1 ) {
			break;
		}
	}
}

/*!
	\brief Parses a chat message template from the source and writes the assembled string into a supplied buffer.

	The function iterates over tokens from the source until a terminating ';' is found, building a chat message in the provided buffer. It supports three token types:
	- TT_STRING: a literal string, stripped of quotation marks and appended.
	- TT_NUMBER (integer): converted into a placeholder using ESCAPE_CHAR, 'v', the number, and ESCAPE_CHAR.
	- TT_NAME: converted into a placeholder using ESCAPE_CHAR, 'r', the name, and ESCAPE_CHAR.

	Each addition checks against MAX_MESSAGE_SIZE; if exceeded, an error is reported via SourceError. The message is terminated with a single null character. The function returns a non‑zero value on
   success and zero on failure (parsing error, unknown token, or length overflow).

	\param source Pointer to the source token stream used for parsing the message
	\return Non‑zero on success, zero on failure
*/
int BotLoadChatMessage( source_t* source, char* chatmessagestring )
{
	char*	ptr;
	token_t token;

	ptr	 = chatmessagestring;
	*ptr = 0;

	//
	while( 1 ) {
		if( !PC_ExpectAnyToken( source, &token ) ) {
			return qfalse;
		}

		// fixed string
		if( token.type == TT_STRING ) {
			StripDoubleQuotes( token.string );

			if( strlen( ptr ) + strlen( token.string ) + 1 > MAX_MESSAGE_SIZE ) {
				SourceError( source, "chat message too long\n" );
				return qfalse;
			}

			strcat( ptr, token.string );
		}

		// variable string
		else if( token.type == TT_NUMBER && ( token.subtype & TT_INTEGER ) ) {
			if( strlen( ptr ) + 7 > MAX_MESSAGE_SIZE ) {
				SourceError( source, "chat message too long\n" );
				return qfalse;
			}

			sprintf( &ptr[strlen( ptr )], "%cv%d%c", ESCAPE_CHAR, ( int )token.intvalue, ESCAPE_CHAR );
		}

		// random string
		else if( token.type == TT_NAME ) {
			if( strlen( ptr ) + 7 > MAX_MESSAGE_SIZE ) {
				SourceError( source, "chat message too long\n" );
				return qfalse;
			}

			sprintf( &ptr[strlen( ptr )], "%cr%s%c", ESCAPE_CHAR, token.string, ESCAPE_CHAR );

		} else {
			SourceError( source, "unknown message component %s\n", token.string );
			return qfalse;
		}

		if( PC_CheckTokenString( source, ";" ) ) {
			break;
		}

		if( !PC_ExpectTokenString( source, "," ) ) {
			return qfalse;
		}
	}

	//
	return qtrue;
}

/*!
	\brief Logs the contents of a random string list to a log file.

	The function obtains a file pointer via Log_FilePointer. If successful, it iterates over the linked list of random string groups. For each group it prints the group name followed by a
   brace-separated list of its associated random strings, each quoted, ending with a closing brace and a newline. If Log_FilePointer fails, the function exits immediately without output.

	\param randomlist pointer to the head of a singly linked list of random string groups
*/
void BotDumpRandomStringList( bot_randomlist_t* randomlist )
{
	FILE*				fp;
	bot_randomlist_t*	random;
	bot_randomstring_t* rs;

	fp = Log_FilePointer();

	if( !fp ) {
		return;
	}

	for( random = randomlist; random; random = random->next ) {
		fprintf( fp, "%s = {", random->string );

		for( rs = random->firstrandomstring; rs; rs = rs->next ) {
			fprintf( fp, "\"%s\"", rs->string );

			if( rs->next ) {
				fprintf( fp, ", " );

			} else {
				fprintf( fp, "}\n" );
			}
		}
	}
}

/*!
	\brief Loads random string lists from a file and returns a linked list of bot_randomlist_t structures.

	This routine executes a two‑pass scan of the specified file. The first pass calculates the memory needed for all random lists and their strings. The second pass allocates contiguous storage with
   GetClearedHunkMemory and fills it, creating bot_randomlist_t nodes for each list name and bot_randomstring_t nodes for each string. The syntax expected is name = { string1 string2 … }. Strings are
   read via BotLoadChatMessage. On any syntax or file error the function frees the source object and returns NULL. When loading succeeds it prints a completion message, logs elapsed time in debug
   builds, and returns the head of the constructed linked list.

	\param filename the file path containing random string definitions
	\return Pointer to the first bot_randomlist_t node of the parsed list; NULL if a failure occurs
*/
bot_randomlist_t* BotLoadRandomStrings( char* filename )
{
	int					pass, size;
	char *				ptr = NULL, chatmessagestring[MAX_MESSAGE_SIZE];
	source_t*			source;
	token_t				token;
	bot_randomlist_t *	randomlist, *lastrandom, *random;
	bot_randomstring_t* randomstring;

#ifdef DEBUG
	int starttime = Sys_MilliSeconds();
#endif // DEBUG

	size	   = 0;
	randomlist = NULL;
	random	   = NULL;

	// the synonyms are parsed in two phases
	for( pass = 0; pass < 2; pass++ ) {
		//
		if( pass && size ) {
			ptr = ( char* )GetClearedHunkMemory( size );
		}

		//
		source = LoadSourceFile( filename );

		if( !source ) {
			botimport.Print( PRT_ERROR, "counldn't load %s\n", filename );
			return NULL;
		}

		//
		randomlist = NULL; // list
		lastrandom = NULL; // last

		//
		while( PC_ReadToken( source, &token ) ) {
			if( token.type != TT_NAME ) {
				SourceError( source, "unknown random %s", token.string );
				FreeSource( source );
				return NULL;
			}

			size += sizeof( bot_randomlist_t ) + strlen( token.string ) + 1;

			if( pass ) {
				random = ( bot_randomlist_t* )ptr;
				ptr += sizeof( bot_randomlist_t );
				random->string = ptr;
				ptr += strlen( token.string ) + 1;
				strcpy( random->string, token.string );
				random->firstrandomstring = NULL;
				random->numstrings		  = 0;

				//
				if( lastrandom ) {
					lastrandom->next = random;

				} else {
					randomlist = random;
				}

				lastrandom = random;
			}

			if( !PC_ExpectTokenString( source, "=" ) || !PC_ExpectTokenString( source, "{" ) ) {
				FreeSource( source );
				return NULL;
			}

			while( !PC_CheckTokenString( source, "}" ) ) {
				if( !BotLoadChatMessage( source, chatmessagestring ) ) {
					FreeSource( source );
					return NULL;
				}

				size += sizeof( bot_randomstring_t ) + strlen( chatmessagestring ) + 1;

				if( pass ) {
					randomstring = ( bot_randomstring_t* )ptr;
					ptr += sizeof( bot_randomstring_t );
					randomstring->string = ptr;
					ptr += strlen( chatmessagestring ) + 1;
					strcpy( randomstring->string, chatmessagestring );
					//
					random->numstrings++;
					randomstring->next		  = random->firstrandomstring;
					random->firstrandomstring = randomstring;
				}
			}
		}

		// free the source after one pass
		FreeSource( source );
	}

	botimport.Print( PRT_MESSAGE, "loaded %s\n", filename );
	//
#ifdef DEBUG
	botimport.Print( PRT_MESSAGE, "random strings %d msec\n", Sys_MilliSeconds() - starttime );
	// BotDumpRandomStringList(randomlist);
#endif // DEBUG
	//
	return randomlist;
}

/*!
	\brief Selects and returns a random string associated with the supplied key, or NULL if the key is not found.

	The function walks through a global list of random string groups, searching for a group whose name matches the supplied parameter. If it finds a matching group, it uses the built‑in random()
   function to pick an index into that group's string list. It then iterates through the strings until that index and returns the string at that position. If no matching group exists or the list is
   empty, the function returns NULL.

	The returned string is stored inside the application's data structures and should not be freed or modified by the caller.

	\param name The identifier of the random string group to search for.
	\return A pointer to the selected string, or NULL if the key does not exist or the list is empty.
*/
char* RandomString( char* name )
{
	bot_randomlist_t*	random;
	bot_randomstring_t* rs;
	int					i;

	for( random = randomstrings; random; random = random->next ) {
		if( !strcmp( random->string, name ) ) {
			i = random() * random->numstrings;

			for( rs = random->firstrandomstring; rs; rs = rs->next ) {
				if( --i < 0 ) {
					break;
				}
			}

			if( rs ) {
				return rs->string;
			}
		}
	}

	return NULL;
}

/*!
	\brief Dumps a linked list of match templates to the debug log.

	Iterates over each matchtemplate, printing each matchpiece. String type pieces are printed as quoted strings; if multiple strings are part of one piece the strings are joined with '|'. Variable
   type pieces are printed as integer indices. After all pieces of a template are printed, the template's type and subtype are printed and a closing brace is added. If the shared log file pointer is
   null the function simply exits.

	\param matches pointer to the first element of a singly linked list of match templates.
*/
void BotDumpMatchTemplates( bot_matchtemplate_t* matches )
{
	FILE*				 fp;
	bot_matchtemplate_t* mt;
	bot_matchpiece_t*	 mp;
	bot_matchstring_t*	 ms;

	fp = Log_FilePointer();

	if( !fp ) {
		return;
	}

	for( mt = matches; mt; mt = mt->next ) {
		// fprintf(fp, "%8d { ");
		for( mp = mt->first; mp; mp = mp->next ) {
			if( mp->type == MT_STRING ) {
				for( ms = mp->firststring; ms; ms = ms->next ) {
					fprintf( fp, "\"%s\"", ms->string );

					if( ms->next ) {
						fprintf( fp, "|" );
					}
				}

			} else if( mp->type == MT_VARIABLE ) {
				fprintf( fp, "%d", mp->variable );
			}

			if( mp->next ) {
				fprintf( fp, ", " );
			}
		}

		fprintf( fp, " = (%d, %d);}\n", mt->type, mt->subtype );
	}
}

/*!
	\brief Frees a linked list of matchpieces and any nested strings.

	The function walks through each matchpiece in the list and releases the memory for the node. If a matchpiece contains a chain of string nodes (indicated by the type MT_STRING), each string node is
   freed first. After processing a matchpiece, the node itself is freed. This routine does not modify any global state apart from the referenced pointers. It may be called with a null pointer, which
   results in no action.

	\param matchpieces pointer to the first matchpiece in the list to be freed
*/
void BotFreeMatchPieces( bot_matchpiece_t* matchpieces )
{
	bot_matchpiece_t * mp, *nextmp;
	bot_matchstring_t *ms, *nextms;

	for( mp = matchpieces; mp; mp = nextmp ) {
		nextmp = mp->next;

		if( mp->type == MT_STRING ) {
			for( ms = mp->firststring; ms; ms = nextms ) {
				nextms = ms->next;
				FreeMemory( ms );
			}
		}

		FreeMemory( mp );
	}
}

/*!
	\brief Loads a linked list of match pieces from a source until an end token is reached.

	The function reads tokens sequentially from the provided source. Numeric tokens are interpreted as variable references; the value is validated to be within the allowed range and two consecutive
   variable tokens are not permitted. String tokens form a match piece that can contain one or more string alternatives separated by the '|' character; each alternative is stored in a sub‑list. After
   each match piece the end token is checked – if it is not found a comma must follow. If parsing fails the source is freed, any allocated match pieces are released, and the function returns NULL. On
   success it returns a pointer to the first element of the constructed singly linked list.

	\param source a pointer to the source from which tokens are read
	\param endtoken the string that signals the termination of the match piece list
	\return pointer to the first bot_matchpiece_t in the list, or NULL if an error occurs
*/
bot_matchpiece_t* BotLoadMatchPieces( source_t* source, char* endtoken )
{
	int				   lastwasvariable, emptystring;
	token_t			   token;
	bot_matchpiece_t * matchpiece, *firstpiece, *lastpiece;
	bot_matchstring_t *matchstring, *lastmatchstring;

	firstpiece = NULL;
	lastpiece  = NULL;
	//
	lastwasvariable = qfalse;

	//
	while( PC_ReadToken( source, &token ) ) {
		if( token.type == TT_NUMBER && ( token.subtype & TT_INTEGER ) ) {
			if( token.intvalue < 0 || token.intvalue >= MAX_MATCHVARIABLES ) {
				SourceError( source, "can't have more than %d match variables\n", MAX_MATCHVARIABLES );
				FreeSource( source );
				BotFreeMatchPieces( firstpiece );
				return NULL;
			}

			if( lastwasvariable ) {
				SourceError( source, "not allowed to have adjacent variables\n" );
				FreeSource( source );
				BotFreeMatchPieces( firstpiece );
				return NULL;
			}

			lastwasvariable = qtrue;
			//
			matchpiece			 = ( bot_matchpiece_t* )GetClearedHunkMemory( sizeof( bot_matchpiece_t ) );
			matchpiece->type	 = MT_VARIABLE;
			matchpiece->variable = token.intvalue;
			matchpiece->next	 = NULL;

			if( lastpiece ) {
				lastpiece->next = matchpiece;

			} else {
				firstpiece = matchpiece;
			}

			lastpiece = matchpiece;

		} else if( token.type == TT_STRING ) {
			//
			matchpiece				= ( bot_matchpiece_t* )GetClearedHunkMemory( sizeof( bot_matchpiece_t ) );
			matchpiece->firststring = NULL;
			matchpiece->type		= MT_STRING;
			matchpiece->variable	= 0;
			matchpiece->next		= NULL;

			if( lastpiece ) {
				lastpiece->next = matchpiece;

			} else {
				firstpiece = matchpiece;
			}

			lastpiece = matchpiece;
			//
			lastmatchstring = NULL;
			emptystring		= qfalse;

			//
			do {
				if( matchpiece->firststring ) {
					if( !PC_ExpectTokenType( source, TT_STRING, 0, &token ) ) {
						FreeSource( source );
						BotFreeMatchPieces( firstpiece );
						return NULL;
					}
				}

				StripDoubleQuotes( token.string );
				matchstring			= ( bot_matchstring_t* )GetClearedHunkMemory( sizeof( bot_matchstring_t ) + strlen( token.string ) + 1 );
				matchstring->string = ( char* )matchstring + sizeof( bot_matchstring_t );
				strcpy( matchstring->string, token.string );

				if( !strlen( token.string ) ) {
					emptystring = qtrue;
				}

				matchstring->next = NULL;

				if( lastmatchstring ) {
					lastmatchstring->next = matchstring;

				} else {
					matchpiece->firststring = matchstring;
				}

				lastmatchstring = matchstring;
			} while( PC_CheckTokenString( source, "|" ) );

			// if there was no empty string found
			if( !emptystring ) {
				lastwasvariable = qfalse;
			}

		} else {
			SourceError( source, "invalid token %s\n", token.string );
			FreeSource( source );
			BotFreeMatchPieces( firstpiece );
			return NULL;
		}

		if( PC_CheckTokenString( source, endtoken ) ) {
			break;
		}

		if( !PC_ExpectTokenString( source, "," ) ) {
			FreeSource( source );
			BotFreeMatchPieces( firstpiece );
			return NULL;
		}
	}

	return firstpiece;
}

/*!
	\brief Frees a linked list of match templates and their associated pieces.

	The function walks through each node of the supplied match template list, freeing the internal match pieces using BotFreeMatchPieces on the first member before releasing the node's memory. This
   ensures all allocated resources for the templates are properly deallocated with no remaining leaks.

	\param mt Pointer to the head of a singly-linked list of bot_matchtemplate_t structures to be freed.
*/
void BotFreeMatchTemplates( bot_matchtemplate_t* mt )
{
	bot_matchtemplate_t* nextmt;

	for( ; mt; mt = nextmt ) {
		nextmt = mt->next;
		BotFreeMatchPieces( mt->first );
		FreeMemory( mt );
	}
}

/*!
	\brief Loads match templates from a configuration file and produces a linked list of bot_matchtemplate_t structures.

	The function attempts to open the file specified by matchfile. If the file cannot be opened, it logs an error and returns NULL. When the file is available, the parser iterates over the file token
   by token. Each template begins with an integer token representing the context, followed by an opening brace. Inside the block the function allocates a new bot_matchtemplate_t, sets its context, and
   loads its pieces using BotLoadMatchPieces. It then expects a numerical type and subtype inside parentheses, concluding the definition with a closing parenthesis and a semicolon. Each successfully
   parsed template is appended to a singly linked list: if this is the first template it becomes the head; otherwise, it is chained after the previously parsed template. On any parsing error the
   function frees any templates that have already been created, closes the source file, and returns NULL. If all tokens are parsed successfully, the source file is closed, a success message is
   printed, and the head of the list is returned. The returned list may be traversed to access individual templates.


	\param matchfile Path to the file containing the match template definitions.
	\return Pointer to the head of a singly linked list of bot_matchtemplate_t structures, or NULL if loading fails.
*/
bot_matchtemplate_t* BotLoadMatchTemplates( char* matchfile )
{
	source_t*			 source;
	token_t				 token;
	bot_matchtemplate_t *matchtemplate, *matches, *lastmatch;
	unsigned long int	 context;

	source = LoadSourceFile( matchfile );

	if( !source ) {
		botimport.Print( PRT_ERROR, "counldn't load %s\n", matchfile );
		return NULL;
	}

	//
	matches	  = NULL; // list with matches
	lastmatch = NULL; // last match in the list

	while( PC_ReadToken( source, &token ) ) {
		if( token.type != TT_NUMBER || !( token.subtype & TT_INTEGER ) ) {
			SourceError( source, "expected integer, found %s\n", token.string );
			BotFreeMatchTemplates( matches );
			FreeSource( source );
			return NULL;
		}

		// the context
		context = token.intvalue;

		//
		if( !PC_ExpectTokenString( source, "{" ) ) {
			BotFreeMatchTemplates( matches );
			FreeSource( source );
			return NULL;
		}

		//
		while( PC_ReadToken( source, &token ) ) {
			if( !strcmp( token.string, "}" ) ) {
				break;
			}

			//
			PC_UnreadLastToken( source );
			//
			matchtemplate		   = ( bot_matchtemplate_t* )GetClearedHunkMemory( sizeof( bot_matchtemplate_t ) );
			matchtemplate->context = context;
			matchtemplate->next	   = NULL;

			// add the match template to the list
			if( lastmatch ) {
				lastmatch->next = matchtemplate;

			} else {
				matches = matchtemplate;
			}

			lastmatch = matchtemplate;
			// load the match template
			matchtemplate->first = BotLoadMatchPieces( source, "=" );

			if( !matchtemplate->first ) {
				BotFreeMatchTemplates( matches );
				return NULL;
			}

			// read the match type
			if( !PC_ExpectTokenString( source, "(" ) || !PC_ExpectTokenType( source, TT_NUMBER, TT_INTEGER, &token ) ) {
				BotFreeMatchTemplates( matches );
				FreeSource( source );
				return NULL;
			}

			matchtemplate->type = token.intvalue;

			// read the match subtype
			if( !PC_ExpectTokenString( source, "," ) || !PC_ExpectTokenType( source, TT_NUMBER, TT_INTEGER, &token ) ) {
				BotFreeMatchTemplates( matches );
				FreeSource( source );
				return NULL;
			}

			matchtemplate->subtype = token.intvalue;

			// read trailing punctuations
			if( !PC_ExpectTokenString( source, ")" ) || !PC_ExpectTokenString( source, ";" ) ) {
				BotFreeMatchTemplates( matches );
				FreeSource( source );
				return NULL;
			}
		}
	}

	// free the source
	FreeSource( source );
	botimport.Print( PRT_MESSAGE, "loaded %s\n", matchfile );
	//
	// BotDumpMatchTemplates(matches);
	//
	return matches;
}

/*!
	\brief Tests if a string satisfies a sequence of string and variable match pieces, recording the variable substrings.

	The function iterates over the linked list of match pieces. For MT_STRING pieces it looks for the piece’s substring inside the current portion of the match string; if found, it updates the pointer
   for that variable part and continues. For MT_VARIABLE pieces it records the start location of that variable in the match string. If all pieces are matched and the entire string is consumed (or the
   last piece was a variable), the function finalizes the lengths of any variable substrings and returns true. Otherwise it aborts and returns false.

	\param pieces Pointer to the first match piece in a linked list that defines the expected pattern of substrings and variables.
	\param match Pointer to a match structure that holds the string to be matched and an array used to store the start pointers and lengths of variable substrings.
	\return Non‑zero value (qtrue) if the string matches the sequence and all variables were captured; zero (qfalse) otherwise.
*/
int StringsMatch( bot_matchpiece_t* pieces, bot_match_t* match )
{
	int				   lastvariable, index;
	char *			   strptr, *newstrptr;
	bot_matchpiece_t*  mp;
	bot_matchstring_t* ms;

	// no last variable
	lastvariable = -1;
	// pointer to the string to compare the match string with
	strptr = match->string;

	// Log_Write("match: %s", strptr);
	// compare the string with the current match string
	for( mp = pieces; mp; mp = mp->next ) {
		// if it is a piece of string
		if( mp->type == MT_STRING ) {
			newstrptr = NULL;

			for( ms = mp->firststring; ms; ms = ms->next ) {
				if( !strlen( ms->string ) ) {
					newstrptr = strptr;
					break;
				}

				// Log_Write("MT_STRING: %s", mp->string);
				index = StringContains( strptr, ms->string, qfalse );

				if( index >= 0 ) {
					newstrptr = strptr + index;

					if( lastvariable >= 0 ) {
						match->variables[lastvariable].length = newstrptr - match->variables[lastvariable].ptr;
						lastvariable						  = -1;
						break;

					} else if( index == 0 ) {
						break;
					}

					newstrptr = NULL;
				}
			}

			if( !newstrptr ) {
				return qfalse;
			}

			strptr = newstrptr + strlen( ms->string );
		}

		// if it is a variable piece of string
		else if( mp->type == MT_VARIABLE ) {
			// Log_Write("MT_VARIABLE");
			match->variables[mp->variable].ptr = strptr;
			lastvariable					   = mp->variable;
		}
	}

	// if a match was found
	if( !mp && ( lastvariable >= 0 || !strlen( strptr ) ) ) {
		// if the last piece was a variable string
		if( lastvariable >= 0 ) {
			match->variables[lastvariable].length = strlen( match->variables[lastvariable].ptr );
		}

		return qtrue;
	}

	return qfalse;
}

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotFindMatch( char* str, bot_match_t* match, unsigned long int context )
{
	int					 i;
	bot_matchtemplate_t* ms;

	strncpy( match->string, str, MAX_MESSAGE_SIZE );

	// remove any trailing enters
	while( strlen( match->string ) && match->string[strlen( match->string ) - 1] == '\n' ) {
		match->string[strlen( match->string ) - 1] = '\0';
	}

	// compare the string with all the match strings
	for( ms = matchtemplates; ms; ms = ms->next ) {
		if( !( ms->context & context ) ) {
			continue;
		}

		// reset the match variable pointers
		for( i = 0; i < MAX_MATCHVARIABLES; i++ ) {
			match->variables[i].ptr = NULL;
		}

		//
		if( StringsMatch( ms->first, match ) ) {
			match->type	   = ms->type;
			match->subtype = ms->subtype;
			return qtrue;
		}
	}

	return qfalse;
}

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotMatchVariable( bot_match_t* match, int variable, char* buf, int size )
{
	if( variable < 0 || variable >= MAX_MATCHVARIABLES ) {
		botimport.Print( PRT_FATAL, "BotMatchVariable: variable out of range\n" );
		strcpy( buf, "" );
		return;
	}

	if( match->variables[variable].ptr ) {
		if( match->variables[variable].length < size ) {
			size = match->variables[variable].length + 1;
		}

		strncpy( buf, match->variables[variable].ptr, size - 1 );
		buf[size - 1] = '\0';

	} else {
		strcpy( buf, "" );
	}

	return;
}

/*!
	\brief Returns the node in a linked string list that matches a given string or null if no match.

	The function iterates over a singly linked list of string nodes. For each node it compares the stored string with the target string using strcmp. If an exact match is found, the matching node is
   returned immediately. If the end of the list is reached without finding a match, the function returns null. The list is not modified.

	\param list The head of the linked list to search
	\param string The null‑terminated string to find
	\return A pointer to the list node containing the matching string, or null if the string is not found in the list.
*/
bot_stringlist_t* BotFindStringInList( bot_stringlist_t* list, char* string )
{
	bot_stringlist_t* s;

	for( s = list; s; s = s->next ) {
		if( !strcmp( s->string, string ) ) {
			return s;
		}
	}

	return NULL;
}

/*!
	\brief Scans a chat message for escape sequences, validates random keywords, and appends any missing ones to a linked list.

	The function walks through each character of the supplied message, looking for escape sequences introduced by ESCAPE_CHAR. If a variable escape sequence ('\v') is encountered, the entire sequence
   is simply skipped. For a random keyword escape ('\r'), the keyword is extracted into a temporary buffer up to the next escape character. It is then checked: if the keyword is not recognized by
   RandomString and does not already exist in the provided stringlist, it is logged as missing (using Log_Write) and added to the front of the list. Encountering an unrecognized escape character
   results in a fatal error print. Once the entire message has been processed, the (potentially extended) stringlist is returned. The function does not throw exceptions; invalid escape characters
   cause a fatal error output instead.

	\param message Text of the chat message to be examined; may contain escape sequences.
	\param stringlist Linked list of already-known random keywords; may be appended to with newly found ones.
	\return Pointer to the (possibly updated) stringlist of random keywords.
*/
bot_stringlist_t* BotCheckChatMessageIntegrety( char* message, bot_stringlist_t* stringlist )
{
	int				  i;
	char*			  msgptr;
	char			  temp[MAX_MESSAGE_SIZE];
	bot_stringlist_t* s;

	msgptr = message;

	//
	while( *msgptr ) {
		if( *msgptr == ESCAPE_CHAR ) {
			msgptr++;

			switch( *msgptr ) {
				case 'v': { // variable
					// step over the 'v'
					msgptr++;

					while( *msgptr && *msgptr != ESCAPE_CHAR ) {
						msgptr++;
					}

					// step over the trailing escape char
					if( *msgptr ) {
						msgptr++;
					}

					break;
				} // end case

				case 'r': { // random
					// step over the 'r'
					msgptr++;

					for( i = 0; ( *msgptr && *msgptr != ESCAPE_CHAR ); i++ ) {
						temp[i] = *msgptr++;
					}

					temp[i] = '\0';

					// step over the trailing escape char
					if( *msgptr ) {
						msgptr++;
					}

					// find the random keyword
					if( !RandomString( temp ) ) {
						if( !BotFindStringInList( stringlist, temp ) ) {
							Log_Write( "%s = {\"%s\"} //MISSING RANDOM\r\n", temp, temp );
							s		  = ( bot_stringlist_t* )GetClearedMemory( sizeof( bot_stringlist_t ) + strlen( temp ) + 1 );
							s->string = ( char* )s + sizeof( bot_stringlist_t );
							strcpy( s->string, temp );
							s->next	   = stringlist;
							stringlist = s;
						}
					}

					break;
				} // end case

				default: {
					botimport.Print( PRT_FATAL, "BotCheckChatMessageIntegrety: message \"%s\" invalid escape char\n", message );
					break;
				} // end default
			}

		} else {
			msgptr++;
		}
	}

	return stringlist;
}

/*!
	\brief Traverses a linked list of reply chats, validates each contained chat message, and cleans up any temporary string lists.

	The function iterates over every reply chat element in the linked list provided by the replychat argument. For each chat message inside an element, it calls BotCheckChatMessageIntegrety to verify
   the message and collect any related strings in a temporary list. After processing all reply chats, the function frees the memory allocated for this temporary string list. The specific nature of the
   consistency checks performed by BotCheckChatMessageIntegrety is not documented here; TODO: clarify what integrity checks are executed.

	\param replychat Pointer to the head of a linked list of reply chat structures. Each element contains a list of chat messages to be validated.
*/
void BotCheckReplyChatIntegrety( bot_replychat_t* replychat )
{
	bot_replychat_t*   rp;
	bot_chatmessage_t* cm;
	bot_stringlist_t * stringlist, *s, *nexts;

	stringlist = NULL;

	for( rp = replychat; rp; rp = rp->next ) {
		for( cm = rp->firstchatmessage; cm; cm = cm->next ) {
			stringlist = BotCheckChatMessageIntegrety( cm->chatmessage, stringlist );
		}
	}

	for( s = stringlist; s; s = nexts ) {
		nexts = s->next;
		FreeMemory( s );
	}
}

/*!
	\brief Ensures the integrity of a bot's chat messages and frees temporary string resources.

	Iterates through every chat type and message in the supplied bot chat. For each message it calls BotCheckChatMessageIntegrety, which validates the message’s string references and returns a list of
   temporary string structures that must be released. After all messages have been processed the function loops over this list and frees each allocated block, cleaning up any temporary data.

	\param chat pointer to the bot_chat_t structure whose messages should be inspected
	\throws TODO: clarify if any assertions or errors are thrown during integrity checks
*/
void BotCheckInitialChatIntegrety( bot_chat_t* chat )
{
	bot_chattype_t*	   t;
	bot_chatmessage_t* cm;
	bot_stringlist_t * stringlist, *s, *nexts;

	stringlist = NULL;

	for( t = chat->types; t; t = t->next ) {
		for( cm = t->firstchatmessage; cm; cm = cm->next ) {
			stringlist = BotCheckChatMessageIntegrety( cm->chatmessage, stringlist );
		}
	}

	for( s = stringlist; s; s = nexts ) {
		nexts = s->next;
		FreeMemory( s );
	}
}

/*!
	\brief Prints the contents of a bot reply chat list to the log file.

	Uses Log_FilePointer to obtain the log file. It iterates over each reply chat element printing its key list, where '&' indicates an AND flag, '!' a NOT flag, and the key type (name, gender,
   string, or variable pattern) is displayed. Variable patterns are shown as comma‑separated string or integer values. After the keys the priority is written. The associated chat messages are then
   output one per line inside braces. The function returns immediately if the log file cannot be obtained.

	\param replychat pointer to the head of a linked list of bot reply chat structures.
*/
void BotDumpReplyChat( bot_replychat_t* replychat )
{
	FILE*				fp;
	bot_replychat_t*	rp;
	bot_replychatkey_t* key;
	bot_chatmessage_t*	cm;
	bot_matchpiece_t*	mp;

	fp = Log_FilePointer();

	if( !fp ) {
		return;
	}

	fprintf( fp, "BotDumpReplyChat:\n" );

	for( rp = replychat; rp; rp = rp->next ) {
		fprintf( fp, "[" );

		for( key = rp->keys; key; key = key->next ) {
			if( key->flags & RCKFL_AND ) {
				fprintf( fp, "&" );

			} else if( key->flags & RCKFL_NOT ) {
				fprintf( fp, "!" );
			}

			//
			if( key->flags & RCKFL_NAME ) {
				fprintf( fp, "name" );

			} else if( key->flags & RCKFL_GENDERFEMALE ) {
				fprintf( fp, "female" );

			} else if( key->flags & RCKFL_GENDERMALE ) {
				fprintf( fp, "male" );

			} else if( key->flags & RCKFL_GENDERLESS ) {
				fprintf( fp, "it" );

			} else if( key->flags & RCKFL_VARIABLES ) {
				fprintf( fp, "(" );

				for( mp = key->match; mp; mp = mp->next ) {
					if( mp->type == MT_STRING ) {
						fprintf( fp, "\"%s\"", mp->firststring->string );

					} else {
						fprintf( fp, "%d", mp->variable );
					}

					if( mp->next ) {
						fprintf( fp, ", " );
					}
				}

				fprintf( fp, ")" );

			} else if( key->flags & RCKFL_STRING ) {
				fprintf( fp, "\"%s\"", key->string );
			}

			if( key->next ) {
				fprintf( fp, ", " );

			} else {
				fprintf( fp, "] = %1.0f\n", rp->priority );
			}
		}

		fprintf( fp, "{\n" );

		for( cm = rp->firstchatmessage; cm; cm = cm->next ) {
			fprintf( fp, "\t\"%s\";\n", cm->chatmessage );
		}

		fprintf( fp, "}\n" );
	}
}

/*!
	\brief Recursively releases all memory associated with a replychat list, including keys, match pieces, strings, and chat messages.

	The function walks through each replychat node, then traverses each nested key and chat message linked list, freeing allocated strings, match piece data, and the structures themselves before
   moving to the next replychat node. It ensures no memory leaks remain for any component of the replychat structure hierarchy.

	\param replychat Pointer to the head of a linked list of replychat structures that should be freed.
*/
void BotFreeReplyChat( bot_replychat_t* replychat )
{
	bot_replychat_t *	rp, *nextrp;
	bot_replychatkey_t *key, *nextkey;
	bot_chatmessage_t * cm, *nextcm;

	for( rp = replychat; rp; rp = nextrp ) {
		nextrp = rp->next;

		for( key = rp->keys; key; key = nextkey ) {
			nextkey = key->next;

			if( key->match ) {
				BotFreeMatchPieces( key->match );
			}

			if( key->string ) {
				FreeMemory( key->string );
			}

			FreeMemory( key );
		}

		for( cm = rp->firstchatmessage; cm; cm = nextcm ) {
			nextcm = cm->next;
			FreeMemory( cm );
		}

		FreeMemory( rp );
	}
}

/*!
	\brief Loads bot reply chat definitions from a file and builds a linked list of reply chat objects.

	The function opens the specified file and tokenizes its contents using the custom source lexer. Each reply‑chat entry is expected to start with a '[' token, followed by one or more key
   specifications, an '=' sign, a numeric priority, and a block of chat messages enclosed in '{' and '}'. Keys may be simple strings, special flag identifiers, or complex structures such as variable
   match lists or bot name lists. For each parsed entry the function allocates memory for a bot_replychat_t structure, constructs its key list, records the priority, and then reads individual messages
   into a nested linked list. If any syntactic error, missing token, or allocation failure occurs the function frees all previously allocated data and returns NULL while printing an error message. On
   success it prints a confirmation and, if the developer flag is enabled, performs integrity checks. The returned pointer is the head of the linked list, with newer entries linked at the front so the
   final list reflects the reverse of the file order.

	\param filename Path to the file containing the reply chat definitions.
	\return A pointer to the first bot_replychat_t in the constructed linked list, or NULL if parsing failed or no entries were found.
*/
bot_replychat_t* BotLoadReplyChat( char* filename )
{
	char				chatmessagestring[MAX_MESSAGE_SIZE];
	char				namebuffer[MAX_MESSAGE_SIZE];
	source_t*			source;
	token_t				token;
	bot_chatmessage_t*	chatmessage = NULL;
	bot_replychat_t *	replychat, *replychatlist;
	bot_replychatkey_t* key;

	source = LoadSourceFile( filename );

	if( !source ) {
		botimport.Print( PRT_ERROR, "counldn't load %s\n", filename );
		return NULL;
	}

	//
	replychatlist = NULL;

	//
	while( PC_ReadToken( source, &token ) ) {
		if( strcmp( token.string, "[" ) ) {
			SourceError( source, "expected [, found %s", token.string );
			BotFreeReplyChat( replychatlist );
			FreeSource( source );
			return NULL;
		}

		//
		replychat		= ( bot_replychat_t* )GetClearedHunkMemory( sizeof( bot_replychat_t ) );
		replychat->keys = NULL;
		replychat->next = replychatlist;
		replychatlist	= replychat;

		// read the keys, there must be at least one key
		do {
			// allocate a key
			key				= ( bot_replychatkey_t* )GetClearedHunkMemory( sizeof( bot_replychatkey_t ) );
			key->flags		= 0;
			key->string		= NULL;
			key->match		= NULL;
			key->next		= replychat->keys;
			replychat->keys = key;

			// check for MUST BE PRESENT and MUST BE ABSENT keys
			if( PC_CheckTokenString( source, "&" ) ) {
				key->flags |= RCKFL_AND;

			} else if( PC_CheckTokenString( source, "!" ) ) {
				key->flags |= RCKFL_NOT;
			}

			// special keys
			if( PC_CheckTokenString( source, "name" ) ) {
				key->flags |= RCKFL_NAME;

			} else if( PC_CheckTokenString( source, "female" ) ) {
				key->flags |= RCKFL_GENDERFEMALE;

			} else if( PC_CheckTokenString( source, "male" ) ) {
				key->flags |= RCKFL_GENDERMALE;

			} else if( PC_CheckTokenString( source, "it" ) ) {
				key->flags |= RCKFL_GENDERLESS;

			} else if( PC_CheckTokenString( source,
						   "(" ) ) { // match key
				key->flags |= RCKFL_VARIABLES;
				key->match = BotLoadMatchPieces( source, ")" );

				if( !key->match ) {
					BotFreeReplyChat( replychatlist );
					return NULL;
				}

			} else if( PC_CheckTokenString( source, "<" ) ) { // bot names
				key->flags |= RCKFL_BOTNAMES;
				strcpy( namebuffer, "" );

				do {
					if( !PC_ExpectTokenType( source, TT_STRING, 0, &token ) ) {
						BotFreeReplyChat( replychatlist );
						FreeSource( source );
						return NULL;
					}

					StripDoubleQuotes( token.string );

					if( strlen( namebuffer ) ) {
						strcat( namebuffer, "\\" );
					}

					strcat( namebuffer, token.string );
				} while( PC_CheckTokenString( source, "," ) );

				if( !PC_ExpectTokenString( source, ">" ) ) {
					BotFreeReplyChat( replychatlist );
					FreeSource( source );
					return NULL;
				}

				key->string = ( char* )GetClearedHunkMemory( strlen( namebuffer ) + 1 );
				strcpy( key->string, namebuffer );

			} else { // normal string key
				key->flags |= RCKFL_STRING;

				if( !PC_ExpectTokenType( source, TT_STRING, 0, &token ) ) {
					BotFreeReplyChat( replychatlist );
					FreeSource( source );
					return NULL;
				}

				StripDoubleQuotes( token.string );
				key->string = ( char* )GetClearedHunkMemory( strlen( token.string ) + 1 );
				strcpy( key->string, token.string );
			}

			//
			PC_CheckTokenString( source, "," );
		} while( !PC_CheckTokenString( source, "]" ) );

		// read the = sign and the priority
		if( !PC_ExpectTokenString( source, "=" ) || !PC_ExpectTokenType( source, TT_NUMBER, 0, &token ) ) {
			BotFreeReplyChat( replychatlist );
			FreeSource( source );
			return NULL;
		}

		replychat->priority = token.floatvalue;

		// read the leading {
		if( !PC_ExpectTokenString( source, "{" ) ) {
			BotFreeReplyChat( replychatlist );
			FreeSource( source );
			return NULL;
		}

		replychat->numchatmessages = 0;

		// while the trailing } is not found
		while( !PC_CheckTokenString( source, "}" ) ) {
			if( !BotLoadChatMessage( source, chatmessagestring ) ) {
				BotFreeReplyChat( replychatlist );
				FreeSource( source );
				return NULL;
			}

			chatmessage				 = ( bot_chatmessage_t* )GetClearedHunkMemory( sizeof( bot_chatmessage_t ) + strlen( chatmessagestring ) + 1 );
			chatmessage->chatmessage = ( char* )chatmessage + sizeof( bot_chatmessage_t );
			strcpy( chatmessage->chatmessage, chatmessagestring );
			chatmessage->time = -2 * CHATMESSAGE_RECENTTIME;
			chatmessage->next = replychat->firstchatmessage;
			// add the chat message to the reply chat
			replychat->firstchatmessage = chatmessage;
			replychat->numchatmessages++;
		}
	}

	FreeSource( source );
	botimport.Print( PRT_MESSAGE, "loaded %s\n", filename );

	//
	// BotDumpReplyChat(replychatlist);
	if( bot_developer ) {
		BotCheckReplyChatIntegrety( replychatlist );
	}

	//
	if( !replychatlist ) {
		botimport.Print( PRT_MESSAGE, "no rchats\n" );
	}

	//
	return replychatlist;
}

/*!
	\brief Outputs the initial chat types and messages for a bot into the log.

	Iterates over the provided bot_chat_t structure, logging each chat type name and its number of messages. For each type it writes an opening brace, then logs the number of chat messages, each
   message string, and finally a closing brace, enclosing the overall structure with a closing brace.

	\param chat Pointer to a bot_chat_t structure containing the initial chat types and messages.
*/
void BotDumpInitialChat( bot_chat_t* chat )
{
	bot_chattype_t*	   t;
	bot_chatmessage_t* m;

	Log_Write( "{" );

	for( t = chat->types; t; t = t->next ) {
		Log_Write( " type \"%s\"", t->name );
		Log_Write( " {" );
		Log_Write( "  numchatmessages = %d", t->numchatmessages );

		for( m = t->firstchatmessage; m; m = m->next ) {
			Log_Write( "  \"%s\"", m->chatmessage );
		}

		Log_Write( " }" );
	}

	Log_Write( "}" );
}

/*!
	\brief Loads a bot chat definition from a file and returns a bot_chat_t structure for the specified chat name, or NULL on failure.

	The function parses a custom chat definition file in two passes. In the first pass it calculates the total size needed for the data structures by reading the file and counting the chat types and
   messages. In the second pass it allocates the required memory and populates a bot_chat_t structure, including linked lists of bot_chattype_t and bot_chatmessage_t nodes. Each chat type contains its
   name, a list of messages, and a count of those messages. The function handles missing files, syntax errors, and reports failures through botimport.Print. On success it optionally validates the
   loaded chat with BotCheckInitialChatIntegrety if the developer flag is set and returns the constructed chat structure; if the chat name is not found or an error occurs, NULL is returned.

	\param chatfile path to the chat definition file
	\param chatname name of the chat to load from that file
	\return a pointer to the newly created bot_chat_t structure, or NULL if the chat could not be loaded
*/
bot_chat_t* BotLoadInitialChat( char* chatfile, char* chatname )
{
	int				   pass, foundchat, indent, size;
	char*			   ptr = NULL;
	char			   chatmessagestring[MAX_MESSAGE_SIZE];
	source_t*		   source;
	token_t			   token;
	bot_chat_t*		   chat		   = NULL;
	bot_chattype_t*	   chattype	   = NULL;
	bot_chatmessage_t* chatmessage = NULL;
#ifdef DEBUG
	int starttime;

	starttime = Sys_MilliSeconds();
#endif // DEBUG
	//
	size	  = 0;
	foundchat = qfalse;

	// a bot chat is parsed in two phases
	for( pass = 0; pass < 2; pass++ ) {
		// allocate memory
		if( pass && size ) {
			ptr = ( char* )GetClearedMemory( size );
		}

		// load the source file
		source = LoadSourceFile( chatfile );

		if( !source ) {
			botimport.Print( PRT_ERROR, "counldn't load %s\n", chatfile );
			return NULL;
		}

		// chat structure
		if( pass ) {
			chat = ( bot_chat_t* )ptr;
			ptr += sizeof( bot_chat_t );
		}

		size = sizeof( bot_chat_t );

		//
		while( PC_ReadToken( source, &token ) ) {
			if( !strcmp( token.string, "chat" ) ) {
				if( !PC_ExpectTokenType( source, TT_STRING, 0, &token ) ) {
					FreeSource( source );
					return NULL;
				}

				StripDoubleQuotes( token.string );

				// after the chat name we expect a opening brace
				if( !PC_ExpectTokenString( source, "{" ) ) {
					FreeSource( source );
					return NULL;
				}

				// if the chat name is found
				if( !Q_stricmp( token.string, chatname ) ) {
					foundchat = qtrue;

					// read the chat types
					while( 1 ) {
						if( !PC_ExpectAnyToken( source, &token ) ) {
							FreeSource( source );
							return NULL;
						}

						if( !strcmp( token.string, "}" ) ) {
							break;
						}

						if( strcmp( token.string, "type" ) ) {
							SourceError( source, "expected type found %s\n", token.string );
							FreeSource( source );
							return NULL;
						}

						// expect the chat type name
						if( !PC_ExpectTokenType( source, TT_STRING, 0, &token ) || !PC_ExpectTokenString( source, "{" ) ) {
							FreeSource( source );
							return NULL;
						}

						StripDoubleQuotes( token.string );

						if( pass ) {
							chattype = ( bot_chattype_t* )ptr;
							strncpy( chattype->name, token.string, MAX_CHATTYPE_NAME );
							chattype->firstchatmessage = NULL;
							// add the chat type to the chat
							chattype->next = chat->types;
							chat->types	   = chattype;
							//
							ptr += sizeof( bot_chattype_t );
						}

						size += sizeof( bot_chattype_t );

						// read the chat messages
						while( !PC_CheckTokenString( source, "}" ) ) {
							if( !BotLoadChatMessage( source, chatmessagestring ) ) {
								FreeSource( source );
								return NULL;
							}

							if( pass ) {
								chatmessage		  = ( bot_chatmessage_t* )ptr;
								chatmessage->time = -2 * CHATMESSAGE_RECENTTIME;
								// put the chat message in the list
								chatmessage->next		   = chattype->firstchatmessage;
								chattype->firstchatmessage = chatmessage;
								// store the chat message
								ptr += sizeof( bot_chatmessage_t );
								chatmessage->chatmessage = ptr;
								strcpy( chatmessage->chatmessage, chatmessagestring );
								ptr += strlen( chatmessagestring ) + 1;
								// the number of chat messages increased
								chattype->numchatmessages++;
							}

							size += sizeof( bot_chatmessage_t ) + strlen( chatmessagestring ) + 1;
						}
					}

				} else { // skip the bot chat
					indent = 1;

					while( indent ) {
						if( !PC_ExpectAnyToken( source, &token ) ) {
							FreeSource( source );
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
				return NULL;
			}
		}

		// free the source
		FreeSource( source );

		// if the requested character is not found
		if( !foundchat ) {
			botimport.Print( PRT_ERROR, "couldn't find chat %s in %s\n", chatname, chatfile );
			return NULL;
		}
	}

	//
	botimport.Print( PRT_MESSAGE, "loaded %s from %s\n", chatname, chatfile );

	//
	// BotDumpInitialChat(chat);
	if( bot_developer ) {
		BotCheckInitialChatIntegrety( chat );
	}

#ifdef DEBUG
	botimport.Print( PRT_MESSAGE, "initial chats loaded in %d msec\n", Sys_MilliSeconds() - starttime );
#endif // DEBUG
	// character was read succesfully
	return chat;
}

/*!
	\brief Frees the chat buffer associated with a chat state and clears its pointer.

	The function retrieves the internal chat state structure using the provided handle. If the state is valid and contains a non‑null chat buffer, the buffer is deallocated with FreeMemory. After
   freeing, the chat pointer in the state is set to NULL to prevent dangling references.

	\param chatstate Handle identifying the chat state whose chat buffer should be freed.
*/
void BotFreeChatFile( int chatstate )
{
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle( chatstate );

	if( !cs ) {
		return;
	}

	if( cs->chat ) {
		FreeMemory( cs->chat );
	}

	cs->chat = NULL;
}

int BotLoadChatFile( int chatstate, char* chatfile, char* chatname )
{
	bot_chatstate_t* cs;
	int				 n, avail = 0;

	cs = BotChatStateFromHandle( chatstate );

	if( !cs ) {
		return BLERR_CANNOTLOADICHAT;
	}

	BotFreeChatFile( chatstate );

	if( !LibVarGetValue( "bot_reloadcharacters" ) ) {
		avail = -1;

		for( n = 0; n < MAX_CLIENTS; n++ ) {
			if( !ichatdata[n].inuse ) {
				if( avail == -1 ) {
					avail = n;
				}

				continue;
			}

			if( strcmp( chatfile, ichatdata[n].filename ) != 0 ) {
				continue;
			}

			if( strcmp( chatname, ichatdata[n].chatname ) != 0 ) {
				continue;
			}

			cs->chat = ichatdata[n].chat;
			//		botimport.Print( PRT_MESSAGE, "retained %s from %s\n", chatname, chatfile );
			return BLERR_NOERROR;
		}

		if( avail == -1 ) {
			botimport.Print( PRT_FATAL, "ichatdata table full; couldn't load chat %s from %s\n", chatname, chatfile );
			return BLERR_CANNOTLOADICHAT;
		}
	}

	cs->chat = BotLoadInitialChat( chatfile, chatname );

	if( !cs->chat ) {
		botimport.Print( PRT_FATAL, "couldn't load chat %s from %s\n", chatname, chatfile );
		return BLERR_CANNOTLOADICHAT;
	}

	if( !LibVarGetValue( "bot_reloadcharacters" ) ) {
		ichatdata[avail].chat = cs->chat;
		Q_strncpyz( ichatdata[avail].chatname, chatname, sizeof( ichatdata[avail].chatname ) );
		Q_strncpyz( ichatdata[avail].filename, chatfile, sizeof( ichatdata[avail].filename ) );
		ichatdata[avail].inuse = qtrue;
	}

	return BLERR_NOERROR;
}

/*!
	\brief Expands placeholders in a chat message template, handling variables, random elements, and synonym replacement

	The function scans the supplied message for escaped placeholders. It substitutes numbered variables using the provided variables array, optionally replacing synonyms depending on the reply flag.
   It also supports random placeholders, selecting a random string and marking that a random expansion occurred. After all replacements, weighted synonym replacement is applied. The resulting text is
   written to the output buffer, with error checks for buffer overrun and unknown variables; on failure it returns 0, otherwise it returns 1 if any random placeholder was expanded during that call.

	\param outmessage destination buffer for the fully expanded message
	\param message source chat template containing escape sequences
	\param mcontext context information used for weighted synonym replacement
	\param variables array of bot_matchvariable_t structures holding variable values
	\param vcontext context information used when replacing synonyms within variables
	\param reply flag indicating whether the message is a reply, which changes synonym handling
	\return 1 if a random placeholder was expanded, 0 if no random placeholder was expanded or if an error occurred such as an unknown variable or output overflow
*/
int BotExpandChatMessage( char* outmessage, char* message, unsigned long mcontext, bot_matchvariable_t* variables, unsigned long vcontext, int reply )
{
	int	  num, len, i, expansion;
	char *outputbuf, *ptr, *msgptr;
	char  temp[MAX_MESSAGE_SIZE];

	expansion = qfalse;
	msgptr	  = message;
	outputbuf = outmessage;
	len		  = 0;

	//
	while( *msgptr ) {
		if( *msgptr == ESCAPE_CHAR ) {
			msgptr++;

			switch( *msgptr ) {
				case 'v': { // variable
					msgptr++;
					num = 0;

					while( *msgptr && *msgptr != ESCAPE_CHAR ) {
						num = num * 10 + ( *msgptr++ ) - '0';
					}

					// step over the trailing escape char
					if( *msgptr ) {
						msgptr++;
					}

					if( num > MAX_MATCHVARIABLES ) {
						botimport.Print( PRT_ERROR, "BotConstructChat: message %s variable %d out of range\n", message, num );
						return qfalse;
					}

					ptr = variables[num].ptr;

					if( ptr ) {
						for( i = 0; i < variables[num].length; i++ ) {
							temp[i] = ptr[i];
						}

						temp[i] = 0;

						// if it's a reply message
						if( reply ) {
							// replace the reply synonyms in the variables
							BotReplaceReplySynonyms( temp, vcontext );

						} else {
							// replace synonyms in the variable context
							BotReplaceSynonyms( temp, vcontext );
						}

						//
						if( len + strlen( temp ) >= MAX_MESSAGE_SIZE ) {
							botimport.Print( PRT_ERROR, "BotConstructChat: message %s too long\n", message );
							return qfalse;
						}

						strcpy( &outputbuf[len], temp );
						len += strlen( temp );
					}

					break;
				} // end case

				case 'r': { // random
					msgptr++;

					for( i = 0; ( *msgptr && *msgptr != ESCAPE_CHAR ); i++ ) {
						temp[i] = *msgptr++;
					}

					temp[i] = '\0';

					// step over the trailing escape char
					if( *msgptr ) {
						msgptr++;
					}

					// find the random keyword
					ptr = RandomString( temp );

					if( !ptr ) {
						botimport.Print( PRT_ERROR, "BotConstructChat: unknown random string %s\n", temp );
						return qfalse;
					}

					if( len + strlen( ptr ) >= MAX_MESSAGE_SIZE ) {
						botimport.Print( PRT_ERROR, "BotConstructChat: message \"%s\" too long\n", message );
						return qfalse;
					}

					strcpy( &outputbuf[len], ptr );
					len += strlen( ptr );
					expansion = qtrue;
					break;
				} // end case

				default: {
					botimport.Print( PRT_FATAL, "BotConstructChat: message \"%s\" invalid escape char\n", message );
					break;
				} // end default
			}

		} else {
			outputbuf[len++] = *msgptr++;

			if( len >= MAX_MESSAGE_SIZE ) {
				botimport.Print( PRT_ERROR, "BotConstructChat: message \"%s\" too long\n", message );
				break;
			}
		}
	}

	outputbuf[len] = '\0';
	// replace synonyms weighted in the message context
	BotReplaceWeightedSynonyms( outputbuf, mcontext );
	// return true if a random was expanded
	return expansion;
}

/*!
	\brief Expands a chat message template into a final message stored in the provided chat state.

	The function repeatedly attempts to expand placeholders in the given message using the bot’s match variables and context, copying the result into the chat state’s message buffer. Expansion is
   attempted up to ten times; if more attempts are needed, a warning is printed. The source message is updated after each successful expansion so that nested expansions can be resolved.

	\param chatstate pointer to the bot’s chat state where the resulting message will be stored
	\param message input template containing placeholders to expand
	\param mcontext numeric context hint used during expansion
	\param variables array of match variables that may be referenced in the message
	\param vcontext numeric context for the variables
	\param reply flag indicating whether the message is a reply, influencing expansion logic
*/
void BotConstructChatMessage( bot_chatstate_t* chatstate, char* message, unsigned long mcontext, bot_matchvariable_t* variables, unsigned long vcontext, int reply )
{
	int	 i;
	char srcmessage[MAX_MESSAGE_SIZE];

	strcpy( srcmessage, message );

	for( i = 0; i < 10; i++ ) {
		if( !BotExpandChatMessage( chatstate->chatmessage, srcmessage, mcontext, variables, vcontext, reply ) ) {
			break;
		}

		strcpy( srcmessage, chatstate->chatmessage );
	}

	if( i >= 10 ) {
		botimport.Print( PRT_WARNING, "too many expansions in chat message\n" );
		botimport.Print( PRT_WARNING, "%s\n", chatstate->chatmessage );
	}
}

/*!
	\brief Selects an initial chat message of a specified type, avoiding recently used messages

	The function searches the chat state for the requested type and counts messages whose scheduled time is not in the future. If all messages have been used recently, it selects the message with the
   oldest scheduled time. Otherwise, it randomly picks from the unused messages, marks the chosen message as recently used by updating its time, and returns its text. If the type does not exist or no
   message can be selected, it returns null.

	\param cs Chat state containing loaded chat data
	\param type Name of the chat message type to choose from
	\return Pointer to the selected chat message string or null when no message of the requested type is available
*/
char* BotChooseInitialChatMessage( bot_chatstate_t* cs, char* type )
{
	int				   n, numchatmessages;
	float			   besttime;
	bot_chattype_t*	   t;
	bot_chatmessage_t *m, *bestchatmessage;
	bot_chat_t*		   chat;

	chat = cs->chat;

	for( t = chat->types; t; t = t->next ) {
		if( !Q_stricmp( t->name, type ) ) {
			numchatmessages = 0;

			for( m = t->firstchatmessage; m; m = m->next ) {
				if( m->time > AAS_Time() ) {
					continue;
				}

				numchatmessages++;
			}

			// if all chat messages have been used recently
			if( numchatmessages <= 0 ) {
				besttime		= 0;
				bestchatmessage = NULL;

				for( m = t->firstchatmessage; m; m = m->next ) {
					if( !besttime || m->time < besttime ) {
						bestchatmessage = m;
						besttime		= m->time;
					}
				}

				if( bestchatmessage ) {
					return bestchatmessage->chatmessage;
				}

			} else { // choose a chat message randomly
				n = random() * numchatmessages;

				for( m = t->firstchatmessage; m; m = m->next ) {
					if( m->time > AAS_Time() ) {
						continue;
					}

					if( --n < 0 ) {
						m->time = AAS_Time() + CHATMESSAGE_RECENTTIME;
						return m->chatmessage;
					}
				}
			}

			return NULL;
		}
	}

	return NULL;
}

int BotNumInitialChats( int chatstate, char* type )
{
	bot_chatstate_t* cs;
	bot_chattype_t*	 t;

	cs = BotChatStateFromHandle( chatstate );

	if( !cs ) {
		return 0;
	}

	for( t = cs->chat->types; t; t = t->next ) {
		if( !Q_stricmp( t->name, type ) ) {
			if( LibVarGetValue( "bot_testichat" ) ) {
				botimport.Print( PRT_MESSAGE, "%s has %d chat lines\n", type, t->numchatmessages );
				botimport.Print( PRT_MESSAGE, "-------------------\n" );
			}

			return t->numchatmessages;
		}
	}

	return 0;
}

void BotInitialChat( int chatstate, char* type, int mcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7 )
{
	char*				message;
	bot_matchvariable_t variables[MAX_MATCHVARIABLES];
	bot_chatstate_t*	cs;

	cs = BotChatStateFromHandle( chatstate );

	if( !cs ) {
		return;
	}

	// if no chat file is loaded
	if( !cs->chat ) {
		return;
	}

	// choose a chat message randomly of the given type
	message = BotChooseInitialChatMessage( cs, type );

	// if there's no message of the given type
	if( !message ) {
#ifdef DEBUG
		botimport.Print( PRT_MESSAGE, "no chat messages of type %s\n", type );
#endif // DEBUG
		return;
	}

	//
	memset( variables, 0, sizeof( variables ) );

	if( var0 ) {
		variables[0].ptr	= var0;
		variables[0].length = strlen( var0 );
	}

	if( var1 ) {
		variables[1].ptr	= var1;
		variables[1].length = strlen( var1 );
	}

	if( var2 ) {
		variables[2].ptr	= var2;
		variables[2].length = strlen( var2 );
	}

	if( var3 ) {
		variables[3].ptr	= var3;
		variables[3].length = strlen( var3 );
	}

	if( var4 ) {
		variables[4].ptr	= var4;
		variables[4].length = strlen( var4 );
	}

	if( var5 ) {
		variables[5].ptr	= var5;
		variables[5].length = strlen( var5 );
	}

	if( var6 ) {
		variables[6].ptr	= var6;
		variables[6].length = strlen( var6 );
	}

	if( var7 ) {
		variables[7].ptr	= var7;
		variables[7].length = strlen( var7 );
	}

	//
	BotConstructChatMessage( cs, message, mcontext, variables, 0, qfalse );
}

/*!
	\brief Displays the replychat's key list and priority in a readable format.

	The function iterates over the linked list of keys within a replychat and outputs each key's flags and content using the botimport.Print system. Flags such as AND (&) and NOT (!) are printed
   first, followed by the key's type descriptor: name, gender indicators, a quoted string value, or a parenthesized list of variable match pieces. After all keys are processed, the replychat's
   priority is displayed, and a '{\n' is printed to signify the start of additional output. This routine is intended for debugging or logging purposes.

	\param replychat a pointer to the bot_replychat_t structure whose keys are to be printed.
*/
void BotPrintReplyChatKeys( bot_replychat_t* replychat )
{
	bot_replychatkey_t* key;
	bot_matchpiece_t*	mp;

	botimport.Print( PRT_MESSAGE, "[" );

	for( key = replychat->keys; key; key = key->next ) {
		if( key->flags & RCKFL_AND ) {
			botimport.Print( PRT_MESSAGE, "&" );

		} else if( key->flags & RCKFL_NOT ) {
			botimport.Print( PRT_MESSAGE, "!" );
		}

		//
		if( key->flags & RCKFL_NAME ) {
			botimport.Print( PRT_MESSAGE, "name" );

		} else if( key->flags & RCKFL_GENDERFEMALE ) {
			botimport.Print( PRT_MESSAGE, "female" );

		} else if( key->flags & RCKFL_GENDERMALE ) {
			botimport.Print( PRT_MESSAGE, "male" );

		} else if( key->flags & RCKFL_GENDERLESS ) {
			botimport.Print( PRT_MESSAGE, "it" );

		} else if( key->flags & RCKFL_VARIABLES ) {
			botimport.Print( PRT_MESSAGE, "(" );

			for( mp = key->match; mp; mp = mp->next ) {
				if( mp->type == MT_STRING ) {
					botimport.Print( PRT_MESSAGE, "\"%s\"", mp->firststring->string );

				} else {
					botimport.Print( PRT_MESSAGE, "%d", mp->variable );
				}

				if( mp->next ) {
					botimport.Print( PRT_MESSAGE, ", " );
				}
			}

			botimport.Print( PRT_MESSAGE, ")" );

		} else if( key->flags & RCKFL_STRING ) {
			botimport.Print( PRT_MESSAGE, "\"%s\"", key->string );
		}

		if( key->next ) {
			botimport.Print( PRT_MESSAGE, ", " );

		} else {
			botimport.Print( PRT_MESSAGE, "] = %1.0f\n", replychat->priority );
		}
	}

	botimport.Print( PRT_MESSAGE, "{\n" );
}

int BotReplyChat( int chatstate, char* message, int mcontext, int vcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7 )
{
	bot_replychat_t *	rchat, *bestrchat;
	bot_replychatkey_t* key;
	bot_chatmessage_t * m, *bestchatmessage;
	bot_match_t			match, bestmatch;
	int					bestpriority, num, found, res, numchatmessages;
	bot_chatstate_t*	cs;

	cs = BotChatStateFromHandle( chatstate );

	if( !cs ) {
		return qfalse;
	}

	memset( &match, 0, sizeof( bot_match_t ) );
	strcpy( match.string, message );
	bestpriority	= -1;
	bestchatmessage = NULL;
	bestrchat		= NULL;

	// go through all the reply chats
	for( rchat = replychats; rchat; rchat = rchat->next ) {
		found = qfalse;

		for( key = rchat->keys; key; key = key->next ) {
			res = qfalse;

			// get the match result
			if( key->flags & RCKFL_NAME ) {
				res = ( StringContains( message, cs->name, qfalse ) != -1 );

			} else if( key->flags & RCKFL_BOTNAMES ) {
				res = ( StringContains( key->string, cs->name, qfalse ) != -1 );

			} else if( key->flags & RCKFL_GENDERFEMALE ) {
				res = ( cs->gender == CHAT_GENDERFEMALE );

			} else if( key->flags & RCKFL_GENDERMALE ) {
				res = ( cs->gender == CHAT_GENDERMALE );

			} else if( key->flags & RCKFL_GENDERLESS ) {
				res = ( cs->gender == CHAT_GENDERLESS );

			} else if( key->flags & RCKFL_VARIABLES ) {
				res = StringsMatch( key->match, &match );

			} else if( key->flags & RCKFL_STRING ) {
				res = ( StringContainsWord( message, key->string, qfalse ) != NULL );
			}

			// if the key must be present
			if( key->flags & RCKFL_AND ) {
				if( !res ) {
					found = qfalse;
					break;
				}

				// botnames is an exception
				// if (!(key->flags & RCKFL_BOTNAMES)) found = qtrue;
			}

			// if the key must be absent
			else if( key->flags & RCKFL_NOT ) {
				if( res ) {
					found = qfalse;
					break;
				}

			} else if( res ) {
				found = qtrue;
			}
		}

		//
		if( found ) {
			if( rchat->priority > bestpriority ) {
				numchatmessages = 0;

				for( m = rchat->firstchatmessage; m; m = m->next ) {
					if( m->time > AAS_Time() ) {
						continue;
					}

					numchatmessages++;
				}

				num = random() * numchatmessages;

				for( m = rchat->firstchatmessage; m; m = m->next ) {
					if( --num < 0 ) {
						break;
					}

					if( m->time > AAS_Time() ) {
						continue;
					}
				}

				// if the reply chat has a message
				if( m ) {
					memcpy( &bestmatch, &match, sizeof( bot_match_t ) );
					bestchatmessage = m;
					bestrchat		= rchat;
					bestpriority	= rchat->priority;
				}
			}
		}
	}

	if( bestchatmessage ) {
		if( var0 ) {
			bestmatch.variables[0].ptr	  = var0;
			bestmatch.variables[0].length = strlen( var0 );
		}

		if( var1 ) {
			bestmatch.variables[1].ptr	  = var1;
			bestmatch.variables[1].length = strlen( var1 );
		}

		if( var2 ) {
			bestmatch.variables[2].ptr	  = var2;
			bestmatch.variables[2].length = strlen( var2 );
		}

		if( var3 ) {
			bestmatch.variables[3].ptr	  = var3;
			bestmatch.variables[3].length = strlen( var3 );
		}

		if( var4 ) {
			bestmatch.variables[4].ptr	  = var4;
			bestmatch.variables[4].length = strlen( var4 );
		}

		if( var5 ) {
			bestmatch.variables[5].ptr	  = var5;
			bestmatch.variables[5].length = strlen( var5 );
		}

		if( var6 ) {
			bestmatch.variables[6].ptr	  = var6;
			bestmatch.variables[6].length = strlen( var6 );
		}

		if( var7 ) {
			bestmatch.variables[7].ptr	  = var7;
			bestmatch.variables[7].length = strlen( var7 );
		}

		if( LibVarGetValue( "bot_testrchat" ) ) {
			for( m = bestrchat->firstchatmessage; m; m = m->next ) {
				BotConstructChatMessage( cs, m->chatmessage, mcontext, bestmatch.variables, vcontext, qtrue );
				BotRemoveTildes( cs->chatmessage );
				botimport.Print( PRT_MESSAGE, "%s\n", cs->chatmessage );
			}

		} else {
			bestchatmessage->time = AAS_Time() + CHATMESSAGE_RECENTTIME;
			BotConstructChatMessage( cs, bestchatmessage->chatmessage, mcontext, bestmatch.variables, vcontext, qtrue );
		}

		return qtrue;
	}

	return qfalse;
}

int BotChatLength( int chatstate )
{
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle( chatstate );

	if( !cs ) {
		return 0;
	}

	return strlen( cs->chatmessage );
}

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotEnterChat( int chatstate, int client, int sendto )
{
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle( chatstate );

	if( !cs ) {
		return;
	}

	if( strlen( cs->chatmessage ) ) {
		BotRemoveTildes( cs->chatmessage );

		if( LibVarGetValue( "bot_testichat" ) ) {
			botimport.Print( PRT_MESSAGE, "%s\n", cs->chatmessage );

		} else {
			if( sendto == CHAT_TEAM ) {
				EA_SayTeam( client, cs->chatmessage );

			} else {
				EA_Say( client, cs->chatmessage );
			}
		}

		// clear the chat message from the state
		strcpy( cs->chatmessage, "" );
	}
}

void BotGetChatMessage( int chatstate, char* buf, int size )
{
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle( chatstate );

	if( !cs ) {
		return;
	}

	BotRemoveTildes( cs->chatmessage );
	strncpy( buf, cs->chatmessage, size - 1 );
	buf[size - 1] = '\0';
	// clear the chat message from the state
	strcpy( cs->chatmessage, "" );
}

void BotSetChatGender( int chatstate, int gender )
{
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle( chatstate );

	if( !cs ) {
		return;
	}

	switch( gender ) {
		case CHAT_GENDERFEMALE:
			cs->gender = CHAT_GENDERFEMALE;
			break;

		case CHAT_GENDERMALE:
			cs->gender = CHAT_GENDERMALE;
			break;

		default:
			cs->gender = CHAT_GENDERLESS;
			break;
	}
}

void BotSetChatName( int chatstate, char* name )
{
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle( chatstate );

	if( !cs ) {
		return;
	}

	memset( cs->name, 0, sizeof( cs->name ) );
	strncpy( cs->name, name, sizeof( cs->name ) );
	cs->name[sizeof( cs->name ) - 1] = '\0';
}

/*!
	\brief Reset all chat message timers to zero

	Iterates over the global replychat linked list and, for each chat, iterates over its linked chat messages, setting each message's time field to 0. This effectively clears any timeout or cooldown
   state for chat messages so they can be reused immediately.

*/
void BotResetChatAI()
{
	bot_replychat_t*   rchat;
	bot_chatmessage_t* m;

	for( rchat = replychats; rchat; rchat = rchat->next ) {
		for( m = rchat->firstchatmessage; m; m = m->next ) {
			m->time = 0;
		}
	}
}

int BotAllocChatState()
{
	int i;

	for( i = 1; i <= MAX_CLIENTS; i++ ) {
		if( !botchatstates[i] ) {
			botchatstates[i] = ( bot_chatstate_t* )GetClearedMemory( sizeof( bot_chatstate_t ) );
			return i;
		}
	}

	return 0;
}

void BotFreeChatState( int handle )
{
	bot_chatstate_t*	 cs;
	bot_consolemessage_t m;
	int					 h;

	if( handle <= 0 || handle > MAX_CLIENTS ) {
		botimport.Print( PRT_FATAL, "chat state handle %d out of range\n", handle );
		return;
	}

	if( !botchatstates[handle] ) {
		botimport.Print( PRT_FATAL, "invalid chat state %d\n", handle );
		return;
	}

	cs = botchatstates[handle];

	if( LibVarGetValue( "bot_reloadcharacters" ) ) {
		BotFreeChatFile( handle );
	}

	// free all the console messages left in the chat state
	for( h = BotNextConsoleMessage( handle, &m ); h; h = BotNextConsoleMessage( handle, &m ) ) {
		// remove the console message
		BotRemoveConsoleMessage( handle, h );
	}

	FreeMemory( botchatstates[handle] );
	botchatstates[handle] = NULL;
}

int BotSetupChatAI()
{
	char* file;

#ifdef DEBUG
	int starttime = Sys_MilliSeconds();
#endif // DEBUG

	file		   = LibVarString( "synfile", "syn.c" );
	synonyms	   = BotLoadSynonyms( file );
	file		   = LibVarString( "rndfile", "rnd.c" );
	randomstrings  = BotLoadRandomStrings( file );
	file		   = LibVarString( "matchfile", "match.c" );
	matchtemplates = BotLoadMatchTemplates( file );

	//
	if( !LibVarValue( "nochat", "0" ) ) {
		file	   = LibVarString( "rchatfile", "rchat.c" );
		replychats = BotLoadReplyChat( file );
	}

	InitConsoleMessageHeap();

#ifdef DEBUG
	botimport.Print( PRT_MESSAGE, "setup chat AI %d msec\n", Sys_MilliSeconds() - starttime );
#endif // DEBUG
	return BLERR_NOERROR;
}

void BotShutdownChatAI()
{
	int i;

	// free all remaining chat states
	for( i = 0; i < MAX_CLIENTS; i++ ) {
		if( botchatstates[i] ) {
			BotFreeChatState( i );
		}
	}

	// free all cached chats
	for( i = 0; i < MAX_CLIENTS; i++ ) {
		if( ichatdata[i].inuse ) {
			FreeMemory( ichatdata[i].chat );
			ichatdata[i].inuse = qfalse;
		}
	}

	if( consolemessageheap ) {
		FreeMemory( consolemessageheap );
	}

	consolemessageheap = NULL;

	if( matchtemplates ) {
		BotFreeMatchTemplates( matchtemplates );
	}

	matchtemplates = NULL;

	if( randomstrings ) {
		FreeMemory( randomstrings );
	}

	randomstrings = NULL;

	if( synonyms ) {
		FreeMemory( synonyms );
	}

	synonyms = NULL;

	if( replychats ) {
		BotFreeReplyChat( replychats );
	}

	replychats = NULL;
}
