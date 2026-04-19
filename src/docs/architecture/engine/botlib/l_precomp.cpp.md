<!-- doxygenix: sha256=1bb24f6c737a311821b7858f4ac1fa37b68153779bfa877406640b0b07bea503 -->

# engine/botlib/l_precomp.cpp

## File Purpose
- Implements the pre‑compiler that transforms raw script files into token streams for the DarkWolf engine.
- Handles all standard preprocessor directives: #include, #define, #undef, #ifdef, #ifndef, #else, #elif, #endif, #error, #pragma, $evalint, $evalfloat.
- Manages macro tables, expansion logic, and built‑in macros such as __LINE__, __FILE__, __DATE__, and __TIME__.

## Core Responsibilities
- Parse and preprocess script files, performing tokenization, directive handling, macro definition, expansion, and include processing.
- Maintain a source_t context that tracks script stack, token queue, indent stack, skip count, defines, and include path.
- Provide diagnostic reporting through SourceError and SourceWarning.
- Expose a lightweight handle-based API for loading, reading, and freeing sources.
- Support optional hashing of define tables to improve lookup speed.

## Key Types and Functions
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

## Important Control Flow
- PC_ReadToken implements the main token read loop: it fetches tokens, dispatches # and $ directives, expands macros, concatenates strings, and handles whitespace.
- Directive handlers (PC_ReadDirective, PC_ReadDollarDirective) invoke the appropriate # or $ directive functions such as PC_Directive_if, PC_Directive_define, PC_Directive_include, etc.
- Token stream is constructed from script files via LoadScript*, with #include pushing a new script_t onto the source stack and the reader recursing as needed.
- Conditional compilation uses an indentation stack: PC_PushIndent and PC_PopIndent are called to push or pop INDENT_* records; source->skip counts how many levels are suppressed.
- Macro expansion is performed by PC_ExpandDefine and PC_ExpandDefineIntoSource, which replace a name token with a linked list of expansion tokens.
- Expression evaluation for #if/#elif is carried out by PC_Evaluate, which gathers tokens, calls PC_EvaluateTokens, and writes the result back to the token stream.

## External Dependencies
- "q_shared.h"
- "botshared/botlib.h"
- "be_interface.h"
- "l_memory.h"
- "l_script.h"
- "l_precomp.h"
- "l_log.h"
- "qbsp.h"
- "l_mem.h"

## How It Fits
- l_precomp.cpp sits between the file I/O layer (l_script) and the engine’s higher‑level parse/VM components.
- It supplies token streams that the core script interpreter consumes.
- Global defines allow scripts to query build/runtime information.
- The component is used by both the main game engine and the bot library.
