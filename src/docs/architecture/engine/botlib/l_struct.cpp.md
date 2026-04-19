<!-- doxygenix: sha256=57507d98f29c6d52b046de4680ddd42751e9818aed51860c605066958a0ff491 -->

# engine/botlib/l_struct.cpp

## File Purpose
- Central implementation of field‑aware parsing and printing for bot scripts and BSP data structures.

## Core Responsibilities
- Provides a generic interface to parse script‑defined C structures for the bot library and BSP converter.
- Enforces field definitions (type, bounds, array size) defined in `fielddef_t` and `structdef_t`.
- Offers symmetrical serialization helpers for persisting structures to disk with proper indentation and format.

## Key Types and Functions
- fielddef_t — definition of a single field (name, type, bounds, flags, offset).
- structdef_t — collection of `fielddef_t` forming a complete struct (name, size, field array).
- source_t, token_t — parser state and token representation.
- FindField(fielddef_t* defs, char* name) — linear lookup of a field by name.
- ReadNumber(source_t* source, fielddef_t* fd, void* p) — parse numeric token, perform bounds/bit‑size checks, store value.
- ReadChar(source_t* source, fielddef_t* fd, void* p) — read literal or numeric character.
- ReadString(source_t* source, fielddef_t* fd, void* p) — consume quoted string and copy into buffer.
- ReadStructure(source_t* source, structdef_t* def, char* structure) — recursive deserializer for whole structs.
- WriteIndent(FILE* fp, int indent) — output tab characters for visual formatting.
- WriteFloat(FILE* fp, float value) — format float with trimmed trailing zeros.
- WriteStructWithIndent(FILE* fp, structdef_t* def, char* structure, int indent) — pretty printer for structs.
- WriteStructure(FILE* fp, structdef_t* def, char* structure) — wrapper calling `WriteStructWithIndent` with zero indent.

## Important Control Flow
- Parsing begins with `ReadStructure`, which expects an opening `{`. It loops over tokens, finding the matching `fielddef_t` via `FindField`.
- If the field is an array, `ReadStructure` expects a nested `{`; else a single value is read.
- `ReadStructure` dispatches to helper readers: `ReadChar`, `ReadNumber`, `ReadString`, or a recursive `ReadStructure` for nested structs.
- `ReadNumber` consumes a token, handles optional negative sign, validates type‑specific bounds, then casts and stores the value.
- `ReadChar` accepts literal characters or numeric constants (via `ReadNumber`).
- `ReadString` strips quotes and copies into a fixed‑size buffer.
- Writing occurs in reverse: `WriteStructWithIndent` prints a `{\r\n`, iterates fields, emits values with `WriteFloat` for floats and recursive calls for nested structs, closing with `}\r\n`.
- `WriteIndent` and `WriteFloat` provide indentation and compact numeric output.

## External Dependencies
- "q_shared.h"
- "botshared/botlib.h"
- "l_script.h"
- "l_precomp.h"
- "l_struct.h"
- "l_utils.h"
- "be_interface.h"
- "../bspc/qbsp.h"
- "../bspc/l_log.h"
- "../bspc/l_mem.h"

## How It Fits
- Used by `botlib` to load dynamic data structures used in AI behavior scripts.
- Used by the BSP converter (`bspc`) to generate and validate the same data structures while building level geometry.
- Forms part of the engine’s scripting subsystem, bridging flat text input to typed C structures.
