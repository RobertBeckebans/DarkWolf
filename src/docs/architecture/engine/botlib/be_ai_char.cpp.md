<!-- doxygenix: sha256=1909f3f88a0f299d27e79cbed10d8af3b33755001ef662a9d141625f847329fa -->

# engine/botlib/be_ai_char.cpp

## File Purpose
- Defines the data structures and management routines for bot personality characteristics, enabling loading from script files, caching, interpolation across skill levels, and providing typed accessor functions for use by the AI system.

## Core Responsibilities
- Representation of bot personality data through bot_character_t and associated characteristic arrays.
- File‑based loading of character definitions with token‑by‑token parsing.
- Caching and reloading control governed by the bot_reloadcharacters libvar.
- Default characteristic merging to ensure full attribute sets for every skill.
- Linear interpolation between skill levels to generate intermediate attributes.
- Safe memory handling: allocation with GetMemory/GetClearedMemory, freeing with FreeMemory, and releasing embedded strings.
- Validation of handle and index ranges with runtime diagnostics.
- Debug logging of loaded characters and interpolation times when DEBUG or bot_developer flags are set.
- Public API providing typed accessors for float, integer, and string characteristics.

## Key Types and Functions
- bot_characteristic_t — holds a characteristic’s type (CT_INTEGER, CT_FLOAT, CT_STRING) and value in a union.
- bot_character_t — represents a full bot character with filename, skill, and a variably sized array of bot_characteristic_t.
- BotCharacterFromHandle(int handle) — retrieves a bot_character_t* from the global array, validating bounds.
- BotDumpCharacter(bot_character_t* ch) — logs a character’s filename, skill, and all characteristics.
- BotFreeCharacterStrings(bot_character_t* ch) — frees any CT_STRING values in a character.
- BotFreeCharacter2(int handle) — validates handle, frees associated strings, deallocates the character, and clears the table entry.
- BotFreeCharacter(int handle) — conditionally frees a character when bot_reloadcharacters is true.
- BotDefaultCharacteristics(bot_character_t* ch, bot_character_t* defaultch) — copies unset characteristics from a default character, allocating strings via GetMemory.
- BotLoadCharacterFromFile(char* charfile, int skill) — parses a character definition file for a specified skill, creating a new bot_character_t.
- BotFindCachedCharacter(char* charfile, int skill) — searches the cache for a matching filename and skill.
- BotLoadCachedCharacter(char* charfile, int skill, int reload) — loads a character into the cache with fallback to defaults and broadening skill requirements.
- BotLoadCharacterSkill(char* charfile, int skill) — loads both default and requested skill levels, merges defaults into the requested character, and returns its handle.
- BotInterpolateCharacters(int handle1, int handle2, int desiredskill) — creates a new character interpolating float characteristics between two source skills.
- BotLoadCharacter(char* charfile, int skill) — high‑level loader that handles direct load, cache, and interpolation for intermediate skill levels.
- CheckCharacteristicIndex(int character, int index) — validates that a handle and characteristic index are within bounds and initialized.
- Characteristic_Float(int character, int index) — returns a float value, converting from integer if needed.
- Characteristic_BFloat(int character, int index, float min, float max) — returns a float value clamped between min and max.
- Characteristic_Integer(int character, int index) — returns an integer value, casting from float when necessary.
- Characteristic_BInteger(int character, int index, int min, int max) — returns an integer value clamped between min and max.
- Characteristic_String(int character, int index, char* buf, int size) — copies a string characteristic into the provided buffer.
- BotShutdownCharacters() — frees all cached bot characters at shutdown.

## Important Control Flow
- botcharacters is a global array indexed by client handles; functions operate on this array using handle values in the range 1 .. MAX_CLIENTS.
- BotCharacterFromHandle validates the handle and returns the associated bot_character_t*.
- BotLoadCharacterFromFile parses a .c file, creates a new bot_character_t, duplicates string values with GetMemory, and returns the allocated structure.
- Caching logic (BotFindCachedCharacter, BotLoadCachedCharacter) first scans botcharacters for a matching filename and skill, then falls back to the default character file, and finally attempts to create a new instance when no cache hit is found.
- BotLoadCharacterSkill loads both the default and requested characters for a skill, copies missing defaults into the requested character, and returns its handle.
- BotInterpolateCharacters locates two source characters, allocates a new one, interpolates floating characteristics, copies integers and strings, and stores it into the global table.
- BotLoadCharacter determines whether to directly load a character, use a cached skill, or interpolate between skill 1 and 4, then returns the resulting handle.
- CheckCharacteristicIndex bounds‑checks a handle and index before any characteristic retrieval.
- Accessors (Characteristic_Float, Characteristic_Integer, Characteristic_String, Characteristic_BFloat, Characteristic_BInteger) obtain typed values (with conversion where possible) and apply user‑supplied bounds.
- BotShutdownCharacters iterates over botcharacters, freeing any allocated entries via BotFreeCharacter2.

## External Dependencies
- q_shared.h – core Quake‑style shared definitions.
- l_log.h, l_memory.h, l_utils.h, l_script.h, l_precomp.h, l_struct.h, l_libvar.h – utility library for logging, memory, scripting, and configuration.
- aasfile.h, botshared/be_aas.h, be_aas_funcs.h, be_interface.h – navigation and AI subsystem interfaces.
- botshared/botlib.h, botshared/be_ai_char.h – botlib API declarations.

## How It Fits
- Serves as the bridge between raw character script files and the runtime AI subsystem; all bot agents derive their behavior parameters from loaded bot_character_t objects.
- Supports dynamic reloading and caching to balance startup performance and moddability.
- Exposes a small, stable API (characteristics retrieval, validation, and shutdown) that other bot components (movement, decision making, etc.) depend on for configuration values.
