<!-- doxygenix: sha256=aac7fc1072252be5a2b9778b1f727f8607366f373fcaf86918834bf8737928fa -->

# engine/botlib/be_interface.h

## File Purpose
- This header establishes the public interface for the bot subsystem.  It defines the global structure used to pass runtime information, declares external variables for state, import functions, and a developer flag, and declares a utility function to obtain elapsed milliseconds.  It also controls compile‑time features through preprocessor defines.

## Core Responsibilities
- Declare global bot library state and interface variables. Define configuration macros that enable debugging, randomization, ZIP handling, and target game engine compatibility. Expose a platform‑independent millisecond timer function. Hold forward declarations for the bot import type and developer flag.

## Key Types and Functions
- botlib_globals_t — structure holding global bot library state fields (setup flag, entity limits, time, debug settings).
- Sys_MilliSeconds() — returns an int count of milliseconds elapsed since program start or the first call (Windows-specific base), used for timing AI operations.

## Important Control Flow
- botlib_globals_t holds global state for the bot library; its members are set during setup and updated each frame, providing shared data such as time, client counts, debug flags. The extern variables botlibglobals, botimport, and bot_developer are referenced by the bot subsystem to access this shared state and platform imports, while Sys_MilliSeconds() can be called to obtain a high‑resolution time stamp for timing AI logic or profiling.

## External Dependencies
- botlib_import_t is defined elsewhere, typically in a header such as "botlib_import.h".  The header relies on basic types like qboolean, vec3_t, and standard C library types; these are usually provided by engine/common headers.

## How It Fits
- botlib/be_interface.h lies at the boundary between the engine and the bot AI module.  It supplies the bot code with shared state (time, entity counts, debug settings) and the import functions needed to query the engine world.  By centralizing globals and configuration flags this file enables consistent behavior across all bot source files and facilitates platform‑specific extensions such as ZIP loading for AAS files.
