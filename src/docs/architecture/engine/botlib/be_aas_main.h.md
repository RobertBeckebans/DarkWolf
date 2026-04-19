<!-- doxygenix: sha256=7a99a8343b7a4da8910c5e480eac47f0b1429352ee4ce7a7374cde41aad3036f -->

# engine/botlib/be_aas_main.h

## File Purpose
- Declaration of the AAS (Area Awareness System) interface used by the bot library.
- Definition of internal linkage functions guarded by AASINTERN for the engine’s AAS core implementation.
- Forward declaration of the global aasworld pointer and helper query functions.

## Core Responsibilities
- Provide a public API for determining AAS initialization and load status.
- Expose runtime time to bot logic.
- Map model indices to names (placeholder) and vice versa.
- Allow switching between multiple pre‑loaded AAS worlds.
- Perform setup, frame start, and shutdown sequences for the AI navigation subsystem.

## Key Types and Functions
- AAS_Error(char* fmt, ...) — emits a formatted error message and aborts AAS operations.
- AAS_SetInitialized() — sets the aasworld->initialized flag to indicate readiness.
- AAS_Setup() — allocates and configures the global AAS world, returning a status code.
- AAS_Shutdown() — frees resources and resets the AAS world to an uninitialized state.
- AAS_LoadMap(const char* mapname) — loads navigation data for the specified map into aasworld.
- AAS_StartFrame(float time) — records a new simulation timestamp and performs per‑frame housekeeping.
- AAS_Initialized() — returns the initialization flag of the global AAS world.
- AAS_Loaded() — indicates whether the navigation file has been successfully loaded.
- AAS_ModelFromIndex(int index) — placeholder; returns model name string for a given configuration index.
- AAS_IndexFromModel(char* modelname) — placeholder; maps a model file name to its configuration index.
- AAS_Time() — returns the current simulation time stored in aasworld.
- AAS_SetCurrentWorld(int index) — selects the active AAS world by index after bounds checking.

## Important Control Flow
- AAS_Initialized() reads the global aasworld->initialized flag and returns it, used by bots to guard queries.
- AAS_Loaded() accesses aasworld->loaded and reports whether the navigation file has been successfully parsed.
- AAS_Time() retrieves the current simulation time from aasworld->time, enabling time‑synchronised behaviours.
- AAS_SetCurrentWorld(index) checks index bounds against MAX_AAS_WORLDS; on success it sets the global pointer to the chosen world entry, otherwise emits an AAS_Error.
- AAS_Setup() prepares the AAS system by allocating global state and marking it as initialized via AAS_SetInitialized().
- AAS_LoadMap(mapname) loads map‑specific navigation data into aasworld, sets loaded flag, and returns a status code.
- AAS_StartFrame(time) records the new frame timestamp, updates aasworld->time, and performs per‑frame housekeeping.
- Internal functions AAS_Error, AAS_Shutdown, and AAS_SetInitialized drive error reporting and cleanup of the AAS world.

## External Dependencies
- engine/aas.h (defines aas_t and constants like MAX_AAS_WORLDS)
- common/q_shared.h for QDECL macro and printf‑style debugging
- C standard headers for variadic functions (e.g., stdarg.h)
- Potentially engine/aas_shared.h for configuration string access (for model lookup placeholders)

## How It Fits
- Serves as the contract between the bot code and the engine's path‑finding/navigation data.
- Encapsulates all operations that load, query or modify the navigation graph.
- Acts as a thin virtual layer allowing future extensions (e.g., multiple world support) without affecting bot logic.
- Integrates with the engine’s global initialization cycle; bots call AAS_Initialized() before performing navigation tasks.
