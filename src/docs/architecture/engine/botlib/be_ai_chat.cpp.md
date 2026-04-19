<!-- doxygenix: sha256=219761b719d23c8a3c5a5903bdfb3ae51e67893ed4de6df4d1a785b1bd61c3b7 -->

# engine/botlib/be_ai_chat.cpp

## File Purpose
- Implements the chat AI for bots in Return to Castle Wolfenstein (RTCW).
- Handles loading of chat messages, synonyms, random strings, and reply rules from configuration files.
- Provides mechanisms for chat message expansion, random selection, and reply matching based on context and variables.

## Core Responsibilities
- Manage a pool of bot chat states indexed by client handles.
- Persist and reuse chat data for all bots; support hot‑reloading of character files.
- Store and dispatch console‑message queues with proper lifetime management.
- Parse, cache, and evaluate text templates, synonym groups, random string lists, and reply‑chat rules.
- Resolve message placeholders (\v variables, \r randoms, synonym replacements) and produce final spoken text.
- Enforce chat cooldowns and random selection of initial chat lines.
- Provide debugging utilities for dumping internal structures.

## Key Types and Functions
- bot_chatstate_t – chat context (gender, name, buffer, console messages, loaded chat).
- bot_chat_t – holds chat types and their message lists for a character.
- bot_matchtemplate_t – a pattern used to recognize user messages; contains linked bot_matchpiece_t.
- bot_replychat_t – a reply rule with keys, priority, and output messages.
- bot_chatstate_t* BotChatStateFromHandle( int handle ) – validate and retrieve chat state.
- void InitConsoleMessageHeap() – pre‑allocate and initialise a heap of console messages.
- bot_consolemessage_t* AllocConsoleMessage() – pop free message from heap.
- void FreeConsoleMessage( bot_consolemessage_t* message ) – return message to heap.
- void BotQueueConsoleMessage( int chatstate, int type, char* message ) – enqueue console message.
- int BotNextConsoleMessage( int chatstate, bot_consolemessage_t* cm ) – pop next queued message.
- int BotNumConsoleMessages( int chatstate ) – count queued messages.
- int IsWhiteSpace( char c ) – helper for whitespace identification during normalization.
- void UnifyWhiteSpaces( char* string ) – collapse runs of whitespace.
- int StringContains( char* str1, char* str2, int casesensitive ) – substring test.
- char* StringContainsWord( char* str1, char* str2, int casesensitive ) – word‑boundary test.
- void StringReplaceWords( char* string, char* synonym, char* replacement ) – in‑place synonym replacement.
- bot_synonymlist_t* BotLoadSynonyms( char* filename ) – parse synonym file.
- void BotReplaceSynonyms( char* string, unsigned long context ) – replace synonyms for given context.
- void BotReplaceWeightedSynonyms( char* string, unsigned long context ) – weighted synonym replacement.
- void BotReplaceReplySynonyms( char* string, unsigned long context ) – reply‑chat synonym replacement.
- int BotLoadChatMessage( source_t* source, char* message ) – parse a single chat template.
- bot_randomlist_t* BotLoadRandomStrings( char* filename ) – load random string buckets.
- char* RandomString( char* name ) – retrieve a random string from a bucket.
- bot_matchtemplate_t* BotLoadMatchTemplates( char* filename ) – load match templates.
- int StringsMatch( bot_matchpiece_t* pieces, bot_match_t* match ) – evaluate a match pattern.
- int BotFindMatch( char* str, bot_match_t* match, unsigned long context ) – search global match templates.
- void BotMatchVariable( bot_match_t* match, int variable, char* buf, int size ) – copy matched variable.
- void BotCheckChatMessageIntegrety( bot_stringlist_t* list ) – log missing random keywords.
- void BotCheckReplyChatIntegrety( bot_replychat_t* replychat ) – validate reply chat message list.
- void BotCheckInitialChatIntegrety( bot_chat_t* chat ) – validate initial chat messages.
- bot_chat_t* BotLoadInitialChat( char* filename, char* chatname ) – load a character chat file.
- void BotFreeChatFile( int chatstate ) – free chat file for a state.
- int BotExpandChatMessage( char* out, char* src, unsigned long mcontext, bot_matchvariable_t* vars, unsigned long vcontext, int reply ) – expand placeholders once.
- void BotConstructChatMessage( bot_chatstate_t* cs, char* src, unsigned long mcontext, bot_matchvariable_t* vars, unsigned long vcontext, int reply ) – repeat expansion until stable.
- char* BotChooseInitialChatMessage( bot_chatstate_t* cs, char* type ) – pick unused or oldest initial message.
- int BotNumInitialChats( int state, char* type ) – count initial messages of a type.
- void BotInitialChat( int state, char* type, int mcontext, char* var0..var7 ) – select and construct initial chat.
- int BotReplyChat( int state, char* msg, int mcontext, int vcontext, char* var0..var7 ) – find matching reply chat and construct reply.
- void BotEnterChat( int state, int client, int sendto ) – send final chat via EA_Say/EA_SayTeam.
- void BotGetChatMessage( int state, char* buf, int size ) – retrieve and clear chat message.
- void BotSetChatGender( int state, int gender ) – set gender flag.
- void BotSetChatName( int state, char* name ) – set bot name.
- int BotAllocChatState() – allocate new chat state handle.
- void BotFreeChatState( int handle ) – free chat state and its messages.
- int BotSetupChatAI() – initialize synonym, random, match, and reply modules; create console heap.
- void BotShutdownChatAI() – free all global tables and memory.

## Important Control Flow
- BotSetupChatAI loads synonym, random‑string, match‑template, and reply‑chat files, builds global lists, and initializes a heap of console message objects.
- BotAllocChatState allocates a slot, creates a bot_chatstate_t, and returns a handle.
- For each bot, BotLoadChatFile is called to load or reuse a character chat file; the resulting bot_chat_t is stored in the chat state.
- BotEnterChat and BotGetChatMessage use the chat state handle to read or write the current chat message, optionally invoking EA_Say/EA_SayTeam.
- Incoming console messages are queued with BotQueueConsoleMessage, processed with BotNextConsoleMessage, and discarded via BotRemoveConsoleMessage.
- Random, synonym, and reply‑chat processing occurs in BotChooseInitialChatMessage, BotReplyChat, and BotExpandChatMessage where placeholders in templates are replaced iteratively until no more expansions are possible.
- On shutdown, BotShutdownChatAI frees every global list and all chat states, ensuring no leftover memory.

## External Dependencies
- "q_shared.h"
- "l_memory.h", "l_libvar.h", "l_script.h", "l_precomp.h", "l_struct.h", "l_utils.h", "l_log.h"
- "aasfile.h"
- "botshared/botlib.h", "botshared/be_aas.h", "be_aas_funcs.h", "be_interface.h", "botshared/be_ea.h", "botshared/be_ai_chat.h"

## How It Fits
- Part of the botlib subsystem responsible for high‑level bot behaviour.
- Intermediary between AAS movement logic and EA (Event Assistant) messaging to convert parsed chat assets into in‑game speech.
- Uses shared global structures (chatstates, matchtemplates, synonyms, etc.) for efficient reuse and quick lookup during gameplay.
