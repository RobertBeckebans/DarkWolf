<!-- doxygenix: sha256=056d66978a989f0ee0e6185abda3e4f7019977a5b345b3ed67d26af6d9788014 -->

# engine/botlib/l_struct.h

## File Purpose
- Act as the core parser and serializer for bot AI script data structures, exposing a simple API to read/write arbitrary C structures described by a runtime schema.
- Serve as the foundation for configuration and state files that drive AI behavior (e.g., weapon choices, mission objectives, AI personality traits).

## Core Responsibilities
- Define constants and masks for script field types and sub‑types, allowing flexible description of primitive and composite data structures.
- Represent individual struct fields and whole structure layouts with fielddef_s and structdef_s, including offset, type, bounds, and optional sub‑structure linkage.
- Parse script input into a populated C structure according to a supplied definition, handling primitives, arrays, bounded values, strings, and nested structs.
- Serialize a structure definition into a readable file representation, supporting indentation and compact float formatting.

## Key Types and Functions
- fielddef_t — structure describing a single field: name, memory offset, type, array size, bounds, and optional sub‑structure pointer.
- structdef_t — structure describing an entire composite type: total size and pointer to a fielddef_t array.
- ReadStructure(source_t* source, structdef_t* def, char* structure) — parses script source according to def and fills the provided memory buffer.
- WriteStructure(FILE* fp, structdef_t* def, char* structure) — outputs a textual representation of the structure definition to a file.
- WriteIndent(FILE* fp, int indent) — emits a specified number of tab characters to the file stream.
- WriteFloat(FILE* fp, float value) — writes a float to the file without trailing zeros.

## Important Control Flow
- ReadStructure begins by consuming an opening brace, then enters a token‑driven loop where each field name token is matched against the provided struct definition; array fields trigger nested brace parsing and per‑element recursive or helper reads; upon encountering a closing brace the loop exits and the populated memory buffer is returned as a success flag.
- WriteStructure calls WriteStructWithIndent with zero indent to serialise the struct name and fields into a file, recursively handling nested sub‑structures and arrays via WriteStructure calls.
- WriteIndent outputs the requested number of tab characters to the given FILE stream, looping until the indent count reaches zero, aborting on any fprintf failure.
- WriteFloat formats a float to a string, trims trailing zeros and a trailing decimal point, then writes the compact representation to the FILE stream.

## External Dependencies
- stdlib/stdio.h for FILE handling (implicit assumption).
- Definition of source_t and Token handling must be included by the compilation unit that uses these prototypes.
- Requires a parsing subsystem that supplies tokens such as opening/closing braces, field names, values, and error reporting via SourceError.

## How It Fits
- Part of the botlib module, this header collaborates with other scripting utilities to provide a lightweight, flexible data definition system.
- Facilitates decoupling AI behavior parameters from hard‑coded C structs, enabling designers to edit scripts without recompiling.
- Provides recursive support for nested structs, which aligns with the hierarchical design of many AI scripts (e.g., missions containing sub‑missions).
