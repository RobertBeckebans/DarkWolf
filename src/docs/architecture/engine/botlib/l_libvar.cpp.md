<!-- doxygenix: sha256=917fe3d76a49afac73b5229b4ffc9d68bddf89324c4eebe76dfb8658005b457b -->

# engine/botlib/l_libvar.cpp

## File Purpose
- Implementation of the libvar subsystem for the bot library, handling string and numeric configuration variables.

## Core Responsibilities
- Provide a lightweight key‑value store for bot library variables, each holding a name, string value, numeric value, and a modification flag.
- Offer basic CRUD operations (create, read, update, delete) with automatic string‑to‑float conversion.
- Manage the memory of all variable nodes via the engine's custom allocator (`GetMemory`, `FreeMemory`).

## Key Types and Functions
- libvar_t — structure representing a single library variable (defined in l_libvar.h).
- LibVarStringValue(char*) — converts a decimal string to float, returning 0 on parse failure.
- LibVarAlloc(char*) — allocates and inserts a new libvar_t node with the given name.
- LibVarDeAlloc(libvar_t*) — frees a single libvar_t node and its string buffer.
- LibVarDeAllocAll() — frees the entire libvar list.
- LibVarGet(char*) — searches and returns a libvar_t by name (case‑insensitive).
- LibVarGetString(char*) — returns the stored string or an empty literal if not found.
- LibVarGetValue(char*) — returns the numeric value or 0 if absent.
- LibVar(char*,char*) — retrieves or creates a variable, initializing string and numeric value, and marks it modified.
- LibVarString(char*,char*) — returns the string value, creating the variable if necessary.
- LibVarValue(char*,char*) — returns the numeric value, creating the variable if necessary.
- LibVarSet(char*,char*) — assigns a new string to an existing or new variable and updates its numeric value and modified flag.
- LibVarChanged(char*) — returns true if the variable has been modified since creation.
- LibVarSetNotModified(char*) — clears the modified flag for a given variable.

## Important Control Flow
- LibVar functions form a global linked‑list of libvar_t entries maintained via the head pointer `libvarlist`.
- `LibVarAlloc` allocates a new node, copies the name, inserts it at the list head.
- Lookup functions (`LibVarGet`, `LibVarGetString`, `LibVarGetValue`) iterate through the list until a name match is found or end is reached.
- Modification functions (`LibVarSet`, `LibVarSetNotModified`) find or create a node, replace its string buffer, recalculate its numeric value, and set the `modified` flag.
- Utility `LibVarStringValue` parses an ASCII decimal string into a float, used when initializing or updating a variable.
- Cleanup (`LibVarDeAlloc`, `LibVarDeAllocAll`) frees string buffers and the node itself.

## External Dependencies
- "q_shared.h"
- "l_memory.h"
- "l_libvar.h"

## How It Fits
- This file supplies persistent, globally‑accessible variables for bot scripts and other subsystems.
- It sits atop the engine memory utilities and the core `q_shared` definitions, providing a simple API for variable manipulation.
- Other bot components query or modify variables via the public functions in this file.
