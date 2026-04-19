<!-- doxygenix: sha256=9d84e86429bb65d2f856a2da135867068587d4b8501942efa7f31bf04c49dfe2 -->

# engine/botlib/l_precomp.h

## File Purpose
- Defines the core pre‑processor and tokenizer used by the Return to Castle Wolfenstein bot compiler, enabling bots to compile simple scripting languages that support macros, conditional compilation, and file inclusion.

## Core Responsibilities
- Provide a tokenizing engine for bot scripts with a source_t abstraction that abstracts file layout and include-path resolution.
- Implement preprocessor directives: #include, #define, #undef, #if, #ifdef, #else, #elif, #endif.
- Expand macros, supporting parameters and adjacent string concatenation.
- Offer non‑consuming token queries (PC_CheckTokenString, PC_CheckTokenType) and typed token reads (PC_ExpectTokenType, PC_ExpectTokenString).
- Handle errors and warnings with file and line context via SourceError and SourceWarning.
- Expose a handle table (PC_LoadSourceHandle, PC_FreeSourceHandle, PC_ReadTokenHandle, PC_SourceFileAndLine) so external tools can refer to scripts by index.

## Key Types and Functions
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

## Important Control Flow
- The pre‑compiler constructs a `source_t` that holds the file name, include path, punctuation table, script stack, token buffer, macro table, indentation stack, and a skip flag.
- PC_ReadToken() loops: it calls PC_ReadSourceToken() to get the next raw token; if the token is a preprocessor directive (# or $) it delegates to the appropriate handler such as PC_AddDefine or PC_SetIncludePath.
- If a NAME token matches an entry in the define table, the macro is expanded by inserting its token list into the current token stream via PC_UnreadToken/PC_ReadLine, allowing nested expansions.
- Conditional directives push an indent_t onto the stack; the skip flag indicates whether a branch should be ignored until the matching #endif.
- PC_ReadLine() consumes backslash line‑continuation tokens and rejects tokens that span a new logical line.
- When a source is closed (FreeSource or PC_FreeSourceHandle) the source’s script, token, define, and indent linked lists are traversed and freed.

## External Dependencies
- relies on token_t, script_t, and punctuation_t types defined elsewhere in botlib;
- uses the token‑heap allocator provided by LoadScriptFile, LoadSourceFile, and LoadSourceMemory;
- depends on build macros such as MAX_QPATH and MAX_TOKENLENGTH, usually defined in q_shared.h;
- provides a stub pc_token_t for BSPC when q_shared.h is not included.

## How It Fits
- engine/botlib/l_precomp.h is the foundation of the bot scripting subsystem; higher‑level parser and compiler modules include this header to obtain token streams and macro facilities.
- Its API is used by the bot compilation pipeline to load scripts, read tokens sequentially, and report errors, while the global source handle table allows multiple scripts to be open concurrently.
- By encapsulating tokenisation, macro expansion, and file inclusion, it keeps the rest of the engine modular and isolates bot script syntax from core game logic.
