<!-- doxygenix: sha256=27498bec7106e1c38b6289e8e61dfc8cf2a5dbdf621badba9158c77a390eadf6 -->

# engine/botlib/be_ai_weap.cpp

## File Purpose
- Implement the weapon‑related AI logic in the RTSW bot library.
- Handle loading of weapon and projectile definitions, validation, and linkage.
- Provide a framework for per‑bot weapon state management and optimal weapon selection based on fuzzy weights.
- Integrate with the global bot system by offering setup, shutdown, and state‑manipulation functions.

## Core Responsibilities
- Define and manage weapon and projectile data structures used by bots.
- Load, validate, and link weapon and projectile definitions from a script file.
- Maintain a per-bot weapon state containing weight configuration and index arrays.
- Provide utilities for checking weapon validity and retrieving weapon information.
- Compute the optimal fighting weapon for a bot based on fuzzy weight analysis of inventory and weights.
- Manage allocation, reset, and deallocation of bot weapon states.
- Expose initialization and shutdown routines for the weapon AI subsystem.

## Key Types and Functions
- weaponconfig_t — container holding the number of weapons, projectiles, and pointers to their arrays.
- bot_weaponstate_t — per‑bot structure storing a weight configuration and weight index array.
- weaponinfo_t — definition of a weapon (number, name, model, projectile, flags, etc.).
- projectileinfo_t — definition of a projectile (name, model, gravity, damage, etc.).
- weaponinfo_fields[] — field description table enabling script-to-structure mapping.
- projectileinfo_fields[] — field description table for projectile script parsing.
- BotValidWeaponNumber(int weaponnum) — verifies that a weapon number lies within the loaded weapon configuration.
- BotWeaponStateFromHandle(int handle) — retrieves a bot_weaponstate_t* for a given handle, performing bounds checking.
- LoadWeaponConfig(char* filename) — parses a weapon script file into weaponconfig_t, linking weapons to projectiles.
- WeaponWeightIndex(weightconfig_t* wwc, weaponconfig_t* wc) — builds an array mapping each weapon to its fuzzy weight index.
- BotFreeWeaponWeights(int weaponstate) — frees a bot’s weight configuration and index array.
- BotLoadWeaponWeights(int weaponstate, char* filename) — loads fuzzy weight data for a bot and creates weight indices.
- BotGetWeaponInfo(int weaponstate, int weapon, weaponinfo_t* weaponinfo) — copies weapon data from the global config into caller’s buffer.
- BotChooseBestFightWeapon(int weaponstate, int* inventory) — selects the highest‑weight weapon for the given bot’s inventory.
- BotResetWeaponState(int weaponstate) — restores the weight configuration in a clean state structure.
- BotAllocWeaponState() — allocates an unused weapon state slot for a new bot, returning the handle.
- BotFreeWeaponState(int handle) — releases all memory associated with a weapon state handle, including weights.
- BotSetupWeaponAI() — global initialization: loads the weapon config file specified by “weaponconfig” libvar.
- BotShutdownWeaponAI() — global cleanup: frees the weapon config and all bot weapon states.

## Important Control Flow
- During initialization, BotSetupWeaponAI loads a weapon configuration file into a global weaponconfig_t via LoadWeaponConfig.
- Weapon configuration is parsed into contiguous arrays of weaponinfo_t and projectileinfo_t, then weapon instances are linked to their projectile data.
- For each bot, BotAllocWeaponState allocates a bot_weaponstate_t in the global botweaponstates array.
- When weapon weights are required, BotLoadWeaponWeights loads a fuzzy weight configuration file, calls WeaponWeightIndex to create an array mapping each weapon to its weight index, and stores both structures in the bot's weapon state.
- During combat decisions, BotChooseBestFightWeapon iterates over all valid weapons, applies FuzzyWeight to the bot's inventory using the stored weight configuration, and returns the weapon with the highest weight.
- BotGetWeaponInfo copies the full weaponinfo_t for a given weapon number into a caller-provided buffer for use by other AI modules.
- BotResetWeaponState preserves a bot's weight configuration structures while resetting the state structure to a clean state.
- On shutdown, BotShutdownWeaponAI frees the global weaponconfig_t and all per-bot weapon states via BotFreeWeaponState.

## External Dependencies
- q_shared.h
- l_libvar.h
- l_log.h
- l_memory.h
- l_utils.h
- l_script.h
- l_precomp.h
- l_struct.h
- aasfile.h
- botshared/botlib.h
- botshared/be_aas.h
- be_aas_funcs.h
- be_interface.h
- be_ai_weight.h
- botshared/be_ai_weap.h

## How It Fits
- Acts as a core subsystem within botlib, enabling higher‑level AI to request weapon info and best choice.
- Relies on weight configuration (fuzzy logic) supplied by be_ai_weight.h to rank weapons.
- Interacts with AAS and navigation modules indirectly via the bot weapon state, but provides only weapon utilities.
- Exposes a clean API (BotAllocWeaponState, BotLoadWeaponWeights, BotChooseBestFightWeapon, etc.) used by mission and combat planners.
- Encapsulates weapon data so that changes to weapon scripts propagate automatically without needing to recompile bot logic.
