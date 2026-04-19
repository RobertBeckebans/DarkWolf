<!-- doxygenix: sha256=38dffee594cc691f736cb4e0fb1af299e345a5dfe6bf20cf51d829e36c1b2d69 -->

# engine/botlib/l_script.h

## File Purpose
- Contains the implementation of a lexicographical script parser used by DarkWolf’s bot library to read and interpret behavior scripts.
- Holds token definitions, parsing routines, error handling, and script management utilities.

## Core Responsibilities
- Define token and script data structures that hold lexical information, whitespace tracking, and file metadata.
- Provide a comprehensive lexer (PS_ReadToken and related helpers) that tokenizes scripts into strings, numbers, literals, names, and punctuation.
- Implement a look‑ahead/unread system for tokens that supports backtracking during parsing.
- Offer utility functions for manipulating token strings (strip quotes) and converting tokens to primitive values (signed int/float).
- Load scripts from files or memory into a script_t object while initializing buffer pointers and line counters.
- Manage whitespace extraction and punctuation lookup tables for flexible parsing.
- Handle error and warning reporting with script context for diagnostics.

## Key Types and Functions
- token_t — represents a single lexical token, storing its string, type, subtype, numeric values, leading/trailing whitespace, location information, and linked list hook.
- script_t — encapsulates a script file’s filename, buffer, pointers for current position, whitespace tracking, line counters, unread token flag, flags, punctuation tables, an internal token slot and chaining capability.
- int PS_ReadToken(script_t* script, token_t* token) — reads the next token from the script, handling unread tokens, whitespace, token type detection, and filling the token structure.
- int PS_ExpectTokenString(script_t* script, char* string) — consumes the next token and verifies it equals a specified string, reporting an error otherwise.
- int PS_ExpectTokenType(script_t* script, int type, int subtype, token_t* token) — consumes a token and checks both its type and expected subtype mask, with detailed error reporting.
- int PS_ExpectAnyToken(script_t* script, token_t* token) — reads a token, failing with an error if none can be read.
- int PS_CheckTokenString(script_t* script, char* string) — peeks the next token, accepts it if it matches the string, otherwise restores the script position.
- int PS_CheckTokenType(script_t* script, int type, int subtype, token_t* token) — peeks for a token matching given type/subtype; consumes it only if it matches.
- int PS_SkipUntilString(script_t* script, char* string) — scans forward until a token with the specified string is found, consuming tokens along the way.
- void PS_UnreadLastToken(script_t* script) — marks the script’s last fetched token as available for rereading.
- void PS_UnreadToken(script_t* script, token_t* token) — copies a supplied token into the script’s internal buffer and flags it for unread.
- char PS_NextWhiteSpaceChar(script_t* script) — retrieves the next character from a script’s current whitespace buffer.
- void StripDoubleQuotes(char* string) — removes leading/trailing double quotes from an in‑place string.
- void StripSingleQuotes(char* string) — removes leading/trailing single quotes from an in‑place string.
- signed long int ReadSignedInt(script_t* script) — reads the next token as a signed integer, handling a leading minus sign.
- long double ReadSignedFloat(script_t* script) — reads the next token as a signed floating‑point number, handling a leading minus sign.
- void SetScriptPunctuations(script_t* script, punctuation_t* p) — assigns a punctuation lookup table to a script.
- void SetScriptFlags(script_t* script, int flags) — writes flag bits into the script.
- int GetScriptFlags(script_t* script) — retrieves the current flag bitmask of a script.
- void ResetScript(script_t* script) — reinitializes a script to its initial state, resetting pointers, flags, and line counters.
- int EndOfScript(script_t* script) — returns true if the script’s current pointer has reached or passed the buffer’s end.
- char* PunctuationFromNum(script_t* script, int num) — maps a punctuation enumeration value to its string representation.
- script_t* LoadScriptFile(const char* filename) — opens a file, reads it into a buffer, and constructs a script_t instance.
- script_t* LoadScriptMemory(char* ptr, int length, char* name) — creates a script_t from a pre‑existing memory block.
- void FreeScript(script_t* script) — releases a script’s allocated memory and its punctuation table if present.
- void PS_SetBaseFolder(char* path) — records the base directory used for future file loads.
- void QDECL ScriptError(script_t* script, char* str, ...) — logs a fatal script error including file and line information; may terminate parsing.
- void QDECL ScriptWarning(script_t* script, char* str, ...) — logs a non‑fatal warning unless warnings are suppressed.

## Important Control Flow
- When a script is parsed, the parser uses the current pointer in the script buffer to skip leading whitespace, then identifies the next token type using PS_ReadToken, which dispatches to specialized helpers (e.g., PS_ReadString, PS_ReadNumber) and finally stores the parsed token back into the script’s token slot.
- The script structure maintains a back‑reference (lastscript_p) for token look‑ahead and implements an unread mechanism (PS_UnreadToken, PS_UnreadLastToken) that allows tokens to be pushed back so subsequent reads can re‑consume them.
- Error reporting is performed via ScriptError and ScriptWarning, which format messages using the script’s filename and current line numbers and exit the parsing process when critical errors are encountered.

## External Dependencies
- stdlib.h, stdio.h, string.h, stdarg.h for memory, I/O, and formatting operations
- engine/botlib/l_script.h itself; other engine modules may call its API (e.g., botlib parsers, game script loaders)
- Compile‑time symbols BOTLIB, PUNCTABLE, MEQCC, BSPC control optional features
- Platform‑specific filesystem APIs when BOTLIB is defined

## How It Fits
- The lexer is central to the bot script execution engine; higher‑level bot code requests script tokens through this API to implement AI decisions and scripted events.
- It exposes a lightweight interface that can be invoked from both runtime bot logic and editor or command‑line tooling.
- Memory‑and file‑loading functions integrate with the engine’s file system abstractions, enabling bot scripts to be distributed with the game binaries.
