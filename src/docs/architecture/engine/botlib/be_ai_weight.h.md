<!-- doxygenix: sha256=49c90d157107351dadbdda7cd39ce2c8047f100a13c3d8de350fcde6a8a03149 -->

# engine/botlib/be_ai_weight.h

## File Purpose
- This header declares the fuzzy weight subsystem for Return to Castle Wolfenstein bot AI.
- It defines the structures and interfaces that allow bots to evaluate item desirability based on inventory state using fuzzy logic.
- The file also provides caching and lifecycle management for weight configurations.

## Core Responsibilities
- Define the data structures for fuzzy weight representation (`fuzzyseperator_t`, `weight_t`, `weightconfig_t`).
- Provide API to load, cache, query, modify, evolve, and delete weight configurations.
- Implement fuzzy decision algorithms that map inventory counts to weighted scores.
- Expose scaling, evolution, and interbreeding utilities to evolve bot behavior over time.

## Key Types and Functions
- fuzzyseperator_t — node of a fuzzy separator tree storing threshold, weight range, and child/next links.
- fuzzyseperator_t::child — pointer to the next condition to evaluate when threshold is not met.
- fuzzyseperator_t::next — pointer to a sibling condition to evaluate when threshold is met.
- weight_t — represents a named weight and its associated separator chain.
- weightconfig_t — container for a set of weights, the file name, and count, used for caching.
- ReadWeightConfig(char* filename) — loads or retrieves a weight configuration, populating separator trees and caching the result.
- FreeWeightConfig(weightconfig_t* config) — releases resources of a weight configuration when reloading is forced.
- WriteWeightConfig(char* filename, weightconfig_t* config) — writes a configuration back to disk.
- FindFuzzyWeight(weightconfig_t* wc, char* name) — returns the index of a weight with a given name, or -1.
- FuzzyWeight(int* inventory, weightconfig_t* wc, int weightnum) — evaluates and returns a deterministic fuzzy weight based on inventory.
- FuzzyWeightUndecided(int* inventory, weightconfig_t* wc, int weightnum) — returns a random weight within the leaf’s range when the decision is undecided.
- ScaleWeight(weightconfig_t* config, char* name, float scale) — scales a named weight’s separator tree by clamping the factor to [0,1] and applying it recursively.
- ScaleBalanceRange(weightconfig_t* config, float scale) — applies a scale factor to the global balance separator.
- EvolveWeightConfig(weightconfig_t* config) — evolves every weight’s separator tree in place via recursive evolution.
- InterbreedWeightConfigs(weightconfig_t* config1, weightconfig_t* config2, weightconfig_t* configout) — blends two configurations into a third by interpolating their separators.
- BotShutdownWeights() — iterates through the global cache, freeing every weight configuration and clearing references.

## Important Control Flow
- When `ReadWeightConfig` is called, it checks if the requested file has already been loaded and cached; if so and reloading is disabled, it returns the cached configuration.
- Otherwise `ReadWeightConfig` loads the file from disk or a pak archive, allocates a `weightconfig_t`, parses `weight` entries, constructs linked fuzzy separator trees, and then stores the configuration in a global cache.
- Subsequent calls can reuse the cached instance until `bot_reloadcharacters` forces a reload.
- When a configuration is no longer needed, `FreeWeightConfig` delegates to `FreeWeightConfig2`, which releases all memory associated with the separators and removes the entry from the cache.
- `FindFuzzyWeight` scans the weight array sequentially, comparing names to locate the index of a specified weight.
- During runtime decision making, `FuzzyWeight` (or `FuzzyWeightUndecided`) traverses a separator chain for a given weight index, stepping through child or sibling nodes based on inventory thresholds, until reaching a leaf node and retrieving its weight or a random value within its range.
- `ScaleWeight` locates a weight by name and recursively applies a clamped scale factor to its separator tree via `ScaleFuzzySeperator_r`.
- `ScaleBalanceRange` applies a scaling factor to the global balance separator.
- `EvolveWeightConfig` iterates over all weights and calls `EvolveFuzzySeperator_r` to evolve each separator in place.
- `InterbreedWeightConfigs` blends two configurations into a third by calling `InterbreedFuzzySeperator_r` on corresponding separators, provided all have matching counts.
- On engine shutdown or weight system reset, `BotShutdownWeights` traverses the global cache and frees all stored weight configurations.

## External Dependencies
- Requires definition of `MAX_QPATH` (likely from `q_shared.h`).
- Relies on `qboolean` type (also from `q_shared.h`).
- Uses global console variable `bot_reloadcharacters`.
- Calls auxiliary functions (`ScaleFuzzySeperator_r`, `EvolveFuzzySeperator_r`, `InterbreedFuzzySeperator_r`) defined elsewhere in the botlib.
- Depends on standard string and memory utilities (`strcmp`, `malloc`, `free`).

## How It Fits
- Serves as the core weight evaluation engine used by bot decision modules.
- Weights are loaded from text files in the game content and reused across multiple bot instances to reduce memory usage.
- The fuzzy logic framework enables bots to perform probabilistic item selection, supporting dynamic difficulty and behavior evolution.
- The subsystem integrates with the broader botlib by providing shared global caches and configuration interfaces used during gameplay.
