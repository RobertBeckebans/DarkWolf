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
 * name:		be_ai_chat.h
 *
 * desc:		char AI
 *
 *
 *****************************************************************************/

#define MAX_MESSAGE_SIZE   150 // limit in game dll
#define MAX_CHATTYPE_NAME  32
#define MAX_MATCHVARIABLES 8

#define CHAT_GENDERLESS	   0
#define CHAT_GENDERFEMALE  1
#define CHAT_GENDERMALE	   2

#define CHAT_ALL		   0
#define CHAT_TEAM		   1

// a console message
typedef struct bot_consolemessage_s {
	int							 handle;
	float						 time;						// message time
	int							 type;						// message type
	char						 message[MAX_MESSAGE_SIZE]; // message
	struct bot_consolemessage_s *prev, *next;				// prev and next in list
} bot_consolemessage_t;

// match variable
typedef struct bot_matchvariable_s {
	char* ptr;
	int	  length;
} bot_matchvariable_t;
// returned to AI when a match is found
typedef struct bot_match_s {
	char				string[MAX_MESSAGE_SIZE];
	int					type;
	int					subtype;
	bot_matchvariable_t variables[MAX_MATCHVARIABLES];
} bot_match_t;

/*!
	\brief Initializes the chat AI system for bot entities

	Sets up the chat AI by loading synonym, random string, and match template files. Optionally loads reply chat files based on a configuration variable. The function also initializes the console
   message heap. This function is typically called during the bot library initialization process before other AI systems are set up

	\return Error code indicating success or failure of the chat AI setup process
*/
int	 BotSetupChatAI();

/*!
	\brief Shuts down the chat AI system by freeing all allocated chat states and data structures.

	This function cleans up all resources associated with the bot chat AI. It iterates through all possible clients to free any existing chat states, clears cached chat data, and releases memory for
   various chat-related structures including console message heap, match templates, random strings, synonyms, and reply chats. This function is typically called during bot library shutdown to ensure
   all chat AI resources are properly deallocated.

*/
void BotShutdownChatAI();

/*!
	\brief Allocates and returns a new chat state handle for bot communication

	This function searches for the first available slot in the bot chat states array to allocate a new chat state. It iterates through the array starting from index 1 up to MAX_CLIENTS to find an
   unused slot. When an unused slot is found, it allocates memory for a new bot_chatstate_t structure and returns the index of that slot. If no slots are available, it returns 0 to indicate failure.

	\return The handle to the newly allocated chat state, or 0 if no chat state could be allocated
*/
int	 BotAllocChatState();

/*!
	\brief Frees the memory associated with a bot chat state identified by the given handle

	This function releases all resources allocated for a bot chat state. It first validates the handle to ensure it is within the valid range and that the chat state exists. If the bot character
   reloading feature is enabled, it also frees the associated chat file. The function then iterates through all console messages in the chat state and removes them before freeing the main chat state
   memory and setting the handle to NULL

	\param handle The identifier for the bot chat state to be freed
*/
void BotFreeChatState( int handle );

/*!
	\brief Adds a console message to the specified chat state for later processing.

	This function inserts a console message into the chat state queue associated with the given chat state handle. The message is stored with a timestamp and type, and is linked into a doubly-linked
   list within the chat state structure. The function manages the message handle generation and ensures the queue maintains proper ordering. It handles memory allocation for the message and returns
   early if allocation fails or if the chat state is invalid.

	\param chatstate Handle to the chat state where the message will be queued
	\param type Type identifier for the console message
	\param message Text content of the console message to queue
*/
void BotQueueConsoleMessage( int chatstate, int type, char* message );

/*!
	\brief Removes a specific console message from the chat state based on the provided handle

	This function removes a console message from the chat state by iterating through the linked list of messages and finding the one that matches the given handle. It properly updates the pointers of
   adjacent messages to maintain the integrity of the linked list structure. The function performs a linear search through the messages in the chat state and removes the first message that matches the
   provided handle.

	\param chatstate Identifier for the chat state containing the message
	\param handle Unique identifier for the console message to be removed
*/
void BotRemoveConsoleMessage( int chatstate, int handle );

/*!
	\brief Returns the next console message from the specified chat state

	This function retrieves the next available console message from a chat state. It first validates the chat state handle and returns 0 if the state is invalid. If the chat state contains a first
   message, it copies the message data into the provided structure and returns the message handle. Otherwise, it returns 0 to indicate no messages are available. The function is typically used in a
   loop to process all messages in a chat state.

	\param chatstate Handle to the chat state to retrieve the message from
	\param cm Pointer to the console message structure to store the retrieved message
	\return The handle of the retrieved console message if a message is available, or 0 if no message is available or the chat state is invalid
*/
int	 BotNextConsoleMessage( int chatstate, bot_consolemessage_t* cm );

/*!
	\brief Returns the number of console messages currently stored in the specified chat state

	This function retrieves the count of console messages that have been queued in a bot's chat state. It first validates the chat state handle by converting it to a chat state structure, and returns
   zero if the handle is invalid. The function is typically used to determine how many messages are available for processing or to manage message queue sizes.

	\param chatstate Handle to the bot chat state
	\return Number of console messages stored in the chat state, or zero if the chat state is invalid
*/
int	 BotNumConsoleMessages( int chatstate );

/*!
	\brief Enters a chat message of the specified type into the bot's chat state.

	This function initializes a chat message of the given type for a bot. It selects a random message from the chat file associated with the chat state, processes any provided variables, and
   constructs the final chat message. The message is then prepared for display or processing in the game.

	\param chatstate Handle to the bot's chat state
	\param type Type of chat message to select
	\param mcontext Message context used in chat processing
	\param var0 First variable string to substitute in the message
	\param var1 Second variable string to substitute in the message
	\param var2 Third variable string to substitute in the message
	\param var3 Fourth variable string to substitute in the message
	\param var4 Fifth variable string to substitute in the message
	\param var5 Sixth variable string to substitute in the message
	\param var6 Seventh variable string to substitute in the message
	\param var7 Eighth variable string to substitute in the message
*/
void BotInitialChat( int chatstate, char* type, int mcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7 );

/*!
	\brief Returns the number of initial chat messages for a specified chat type in a given chat state

	This function retrieves the count of initial chat messages associated with a specific chat type within a chat state. It first validates the chat state handle, then searches through the chat types
   in the state to find a match with the provided type string. If a match is found, it returns the number of chat messages for that type. If the bot_testichat library variable is set, it also prints
   debug information about the chat type and message count. The function returns zero if the chat state is invalid or if no matching chat type is found.

	\param chatstate Handle to the bot chat state
	\param type String identifier for the chat type to query
	\return The number of initial chat messages for the specified chat type, or zero if the chat state is invalid or no matching type is found
*/
int	 BotNumInitialChats( int chatstate, char* type );

/*!
	\brief Finds and returns an appropriate chat reply for a given message based on chat state and context.

	This function processes a message and evaluates it against a set of predefined reply chat rules. It checks for matching keys in the reply chat system, considering various flags such as name
   presence, gender, and string matching. When a match is found, it selects a random chat message from the matching rules and applies variables to construct the final response. The function returns
   true if a suitable reply is found, otherwise false.

	\param chatstate Handle to the bot's chat state
	\param message The message to process for finding a reply
	\param mcontext Message context identifier
	\param vcontext Variable context identifier
	\param var0 Variable string 0 for replacement in reply
	\param var1 Variable string 1 for replacement in reply
	\param var2 Variable string 2 for replacement in reply
	\param var3 Variable string 3 for replacement in reply
	\param var4 Variable string 4 for replacement in reply
	\param var5 Variable string 5 for replacement in reply
	\param var6 Variable string 6 for replacement in reply
	\param var7 Variable string 7 for replacement in reply
	\return Returns 1 if a suitable chat reply is found and processed, 0 otherwise.
*/
int	 BotReplyChat( int chatstate, char* message, int mcontext, int vcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7 );

/*!
	\brief Returns the length of the chat message associated with the specified chat state.

	This function retrieves the chat state using the provided handle and returns the length of the chat message stored in that state. If the chat state is invalid or does not contain a message, it
   returns zero.

	\param chatstate Handle to the chat state containing the message whose length is to be determined
	\return The length of the chat message in the specified chat state, or zero if the chat state is invalid or empty
*/
int	 BotChatLength( int chatstate );

/*!
	\brief Processes and sends a chat message based on the provided chat state, client, and target

	This function retrieves the chat state using the provided handle, checks if there is a valid chat message, removes tildes from the message, and then sends the message either as a team chat or a
   global chat depending on the sendto parameter. The chat message is cleared from the state after being processed. If the bot_testichat libvar is set, the message is printed to the console instead of
   being sent in-game.

	\param chatstate Handle to the chat state containing the message to be sent
	\param client Client index that will send the chat message
	\param sendto Target type for the chat message, either CHAT_TEAM or global chat
*/
void BotEnterChat( int chatstate, int client, int sendto );

/*!
	\brief Retrieves the chat message from the specified chat state and copies it to the provided buffer.

	This function extracts the chat message stored in the bot chat state and copies it into the provided buffer. It ensures that the buffer is properly null-terminated and clears the chat message from
   the state after retrieval. The function first validates the chat state handle, and if invalid, it returns immediately without performing any operations. The chat message is copied using strncpy to
   prevent buffer overflows, and the buffer is explicitly null-terminated.

	\param chatstate Handle to the bot chat state from which to retrieve the message
	\param buf Buffer to store the retrieved chat message
	\param size Size of the buffer to prevent overflow during copy
*/
void BotGetChatMessage( int chatstate, char* buf, int size );

/*!
	\brief Checks if the first string contains the second string and returns the index of the match or -1 if not found

	The function performs a substring search within the first string to find the second string. It supports case-sensitive and case-insensitive matching based on the casesensitive parameter. The
   function returns the zero-based index where the second string is found within the first string, or -1 if the second string is not found. If either input string is NULL, the function immediately
   returns -1. The search is performed character-by-character with proper handling of the NULL terminator for both strings.

	\param str1 The string to search within
	\param str2 The substring to search for
	\param casesensitive Non-zero value for case-sensitive comparison, zero for case-insensitive
	\return The zero-based index where the second string is found within the first string, or -1 if the second string is not found
*/
int	 StringContains( char* str1, char* str2, int casesensitive );

/*!
	\brief Finds a matching template for the given string based on context and populates the match structure.

	This function searches through a list of predefined match templates to find one that matches the input string. It takes into account the provided context to filter out irrelevant templates. The
   function removes trailing newlines from the input string before matching and resets the match variables before performing the comparison. If a match is found, the match structure is populated with
   the template's type and subtype, and the function returns true. Otherwise, it returns false.

	\param str Input string to be matched against the templates
	\param match Pointer to the match structure that will be populated with the result of the match
	\param context Context flags that determine which match templates are considered for matching
	\return Non-zero value if a match is found, zero otherwise
*/
int	 BotFindMatch( char* str, bot_match_t* match, unsigned long int context );

/*!
	\brief Retrieves a variable from a bot match and copies it to a buffer.

	This function retrieves the value of a specified variable from a bot match structure and copies it to the provided buffer. It performs range checking on the variable index and ensures that the
   buffer size is respected. If the variable index is out of range, a fatal error is printed and an empty string is copied to the buffer. If the variable has no value, an empty string is copied to the
   buffer instead.

	\param match Pointer to the bot match structure containing the variables.
	\param variable Index of the variable to retrieve, must be between 0 and MAX_MATCHVARIABLES-1.
	\param buf Buffer to store the retrieved variable value.
	\param size Size of the buffer in bytes.
*/
void BotMatchVariable( bot_match_t* match, int variable, char* buf, int size );

/*!
	\brief Replaces multiple consecutive whitespace characters in a string with a single space.

	This function processes a null-terminated character string and normalizes whitespace by replacing sequences of whitespace characters with a single space. It modifies the string in-place, removing
   extra spaces while preserving the first space in each sequence. The function handles all standard whitespace characters and ensures that no leading or trailing whitespace is introduced.

	\param string Pointer to the null-terminated character string to normalize
*/
void UnifyWhiteSpaces( char* string );

/*!
	\brief Replaces context-related synonyms in the provided string with their canonical forms.

	This function iterates through a list of synonym groups and checks if each group's context matches the provided context mask. For matching groups, it replaces occurrences of each synonym with the
   first synonym in the group. The replacement is performed using the StringReplaceWords function.

	\param string The input string in which synonyms will be replaced.
	\param context A bitmask indicating the contexts for which synonyms should be replaced.
*/
void BotReplaceSynonyms( char* string, unsigned long int context );

/*!
	\brief Loads a chat file for the specified chat state, initializing chat data and caching it for future use.

	This function loads a chat file associated with a specific chat state, parsing the chat data and storing it for use in bot communication. It first checks if the chat data is already loaded and
   cached, and if so, reuses it to avoid unnecessary processing. The function also manages internal caching of chat data to prevent duplication and to handle memory efficiently. If the chat file
   cannot be loaded or the cache is full, appropriate error codes are returned. The function interacts with the bot library interface to manage chat state and data.

	\param chatstate Handle to the bot chat state to load the file into
	\param chatfile Path to the chat file to be loaded
	\param chatname Name of the chat to load from the file
	\return Error code indicating success or failure of the operation, where BLERR_NOERROR indicates success and BLERR_CANNOTLOADICHAT indicates an error during loading.
*/
int	 BotLoadChatFile( int chatstate, char* chatfile, char* chatname );

/*!
	\brief Sets the gender of a bot in the chat state

	This function configures the gender associated with a bot's chat state. It takes a chat state handle and a gender value, and updates the corresponding chat state structure. The gender can be set
   to female, male, or genderless based on the input value.

	\param chatstate Handle to the bot's chat state
	\param gender Gender value to set (CHAT_GENDERFEMALE, CHAT_GENDERMALE, or CHAT_GENDERLESS)
*/
void BotSetChatGender( int chatstate, int gender );

/*!
	\brief Sets the chat name for a specified bot chat state

	This function stores a bot name in the chat state associated with the given chat state handle. It first retrieves the chat state structure using the provided handle, and if the handle is valid, it
   copies the provided name into the chat state's name buffer. The function ensures that the name is null-terminated and does not exceed the buffer size.

	\param chatstate Handle to the bot chat state
	\param name The name to set for the bot in the chat state
*/
void BotSetChatName( int chatstate, char* name );
