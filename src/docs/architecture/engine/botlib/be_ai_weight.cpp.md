<!-- doxygenix: sha256=14a28fe7dfb49408beec5a5e7050880c4c2f7f3f8d82f97614417e65432fbd14 -->

# engine/botlib/be_ai_weight.cpp

## File Purpose
- Defines the fuzzy‑logic infrastructure for the bot AI: it loads weight configuration scripts, builds separator trees, evaluates bot decision weights at runtime, and supports mutation, scaling, and merging of configurations.

## Core Responsibilities
- Parse and validate weight configuration files written in a custom script format.
- Construct and cache `weightconfig_t` instances that contain named fuzzy weight trees.
- Provide deterministic and stochastic fuzzy weight evaluation (`FuzzyWeight`, `FuzzyWeightUndecided`).
- Offer runtime mutation (`EvolveWeightConfig`), scaling (`ScaleWeight`, `ScaleFuzzyBalanceRange`), and cross‑configuration blending (`InterbreedWeightConfigs`).
- Manage lifetime of cached configurations, ensuring proper deallocation (`FreeWeightConfig`, `FreeWeightConfig2`, `BotShutdownWeights`).

## Key Types and Functions
- `weightconfig_t` — holds an array of named weights, each with a `fuzzyseperator_t* firstseperator`.
- `fuzzyseperator_t` — node in a tree: `int index; int value; float weight, minweight, maxweight; int type; fuzzyseperator_t *child, *next`.
- `ReadWeightConfig(char* filename)` — parses a weight file into a cached `weightconfig_t`.
- `FuzzyWeight(int* inventory, weightconfig_t* wc, int weightnum)` — recursive or iterative evaluation of a weight based on inventory.
- `FuzzyWeightUndecided(int* inventory, weightconfig_t* wc, int weightnum)` — same as above but returns a random weight within `minweight`–`maxweight`.
- `EvolveWeightConfig(weightconfig_t* config)` — applies random mutations to all balance‑type separators.
- `ScaleWeight(weightconfig_t* config, char* name, float scale)` — multiplies a named weight’s separator by a clamped factor.
- `ScaleFuzzyBalanceRange(weightconfig_t* config, float scale)` — contracts/expands the min‑max range around the midpoint of all balance nodes.
- `InterbreedWeightConfigs(weightconfig_t* a, weightconfig_t* b, weightconfig_t* out)` — blends two configurations into a third by averaging corresponding separators.
- `FreeWeightConfig(weightconfig_t* config)` / `FreeWeightConfig2(weightconfig_t* config)` — deallocates all memory used by a configuration.
- `BotShutdownWeights()` — clears the global cache `weightFileList`.

## Important Control Flow
- When a weight file is requested, `ReadWeightConfig` first checks `weightFileList` for a cached instance; if found, it is returned, otherwise the file is parsed and the resulting `weightconfig_t` is stored for future use.
- File parsing proceeds token‑by‑token: a "weight" token starts a new weight, followed by a name and either "switch" (recursive `ReadFuzzySeperators_r`) or "return" (simple `ReadFuzzyWeight`).
- Each fuzzy separator or switch block is constructed recursively, linking `next` and `child` pointers to form a tree that encodes fuzzy decision logic.
- At runtime the bot calls `FuzzyWeight` or `FuzzyWeightUndecided` to evaluate a weight by walking this tree: comparing the relevant inventory slot to the node’s `value`, descending to `child` if lower, or proceeding to `next` otherwise, until a leaf weight is reached.
- Evolution, scaling, or interpolation of configurations is performed by recursive helper functions (`EvolveFuzzySeperator_r`, `ScaleFuzzySeperator_r`, `InterbreedFuzzySeperator_r`, etc.) that walk the same separator tree in depth-first order, modifying weights in place.
- `BotShutdownWeights` iterates over `weightFileList`, freeing every cached configuration via `FreeWeightConfig2`.

## External Dependencies
- "q_shared.h" – core engine types (source, token, etc.).
- "l_memory.h" – memory allocation helpers (`GetClearedMemory`, `FreeMemory`).
- "l_log.h" – logging (`SourceError`, `SourceWarning`).
- "l_utils.h" – utility routines (`StripDoubleQuotes`).
- "l_script.h" – script parsing (`PC_ExpectTokenString`, `PC_ReadToken`, etc.).
- "l_precomp.h" – precompiled headers.
- "l_struct.h" – shared bot data structures.
- "l_libvar.h" – console variable access (`LibVarGetValue`).
- "aasfile.h"
- "botshared/botlib.h"
- "botshared/be_aas.h"
- "be_aas_funcs.h"
- "be_interface.h"
- "be_ai_weight.h" – bot‑specific headers, constants, and type definitions.

## How It Fits
- Central to the bot AI module (`botlib`) as it supplies weight values that influence item pickup, path choice, and combat decisions.
- Interacts with the inventory system (`int* inventory`) to map game state to fuzzy logic decisions.
- Provides persistent configuration caching (`weightFileList`), enabling quick reloads on character change and integration with developer tooling (`bot_developer`).
- Exposes mutation and scaling functions for evolutionary AI experiments or dynamic difficulty adjustments.
