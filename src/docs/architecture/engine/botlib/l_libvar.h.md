<!-- doxygenix: sha256=ad857544a2911cc65ec1591aae5e5e2139931baed7a309c2edabc44e6fa84399 -->

# engine/botlib/l_libvar.h

## File Purpose
- Declare types and functions that implement the botlib's configuration variable system.
- Document behavior, inputs, and outputs of each operation on the global variable list.

## Core Responsibilities
- Define the libvar_t data structure that holds a name, string, numeric value, flags, modification state, and a link to the next variable.
- Provide a public API for creating, retrieving, setting, and deleting library variables.
- Track whether a variable has been modified since creation or the last unmodified flag reset.
- Expose utilities that return string representations, numeric values, or default values for missing variables.

## Key Types and Functions
- libvar_t — data structure representing a single library variable.
- void LibVarDeAllocAll() — frees the entire linked list of library variables.
- libvar_t* LibVarGet(char* var_name) — returns the node with the given name or NULL.
- char* LibVarGetString(char* var_name) — returns the string value of the named variable or an empty string.
- float LibVarGetValue(char* var_name) — returns the numeric value of the named variable or 0 if missing.
- libvar_t* LibVar(char* var_name, char* value) — retrieves or creates a variable, initializing it with the supplied string.
- float LibVarValue(char* var_name, char* value) — returns the numeric value of a variable, creating it with a default if absent.
- char* LibVarString(char* var_name, char* value) — returns the string value of a variable, creating it with a default if absent.
- void LibVarSet(char* var_name, char* value) — sets the string and numeric value of a variable, creating it if necessary.
- qboolean LibVarChanged(char* var_name) — indicates whether the variable has been modified since creation.
- void LibVarSetNotModified(char* var_name) — clears the modified flag on the specified variable.

## Important Control Flow
- Library variables are stored in a global singly linked list of libvar_t nodes.
- Lookup operations (LibVarGet, LibVarGetString, LibVarGetValue) perform a linear search over the list using a case‑insensitive string comparison (Q_stricmp).
- Insertion (LibVar) either returns an existing node or allocates a new one via LibVarAlloc, copies the provided string, parses it to a float, sets the modified flag, and links it into the list.
- Modification (LibVarSet) finds the node, frees the old string, allocates a new string copy, updates the numeric value, and marks the node as modified.
- Bulk cleanup (LibVarDeAllocAll) iterates over the list, deallocating each node via LibVarDeAlloc and resets the head to NULL.

## External Dependencies
- "q_shared.h" (for qboolean, Q_stricmp, etc.)
- "stdlib.h" (for memory allocation used by implementations).

## How It Fits
- Part of the Return to Castle Wolfenstein botlib: provides a lightweight, in‑memory key/value store used by other botlib modules to read or write configuration parameters.
- Acts as an intermediary between the engine’s configuration (cvars) and bot logic, enabling bots to query and modify runtime settings.
- The global linked list allows fast insertion and lazy lookup without external data structures, keeping the implementation simple and portable.
