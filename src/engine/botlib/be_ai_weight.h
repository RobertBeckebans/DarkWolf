/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU
General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*!
	\file engine/botlib/be_ai_weight.h
	\brief This header declares the fuzzy weight subsystem for Return to Castle Wolfenstein bot AI.
	\note doxygenix: sha256=49c90d157107351dadbdda7cd39ce2c8047f100a13c3d8de350fcde6a8a03149

	\par File Purpose
	- This header declares the fuzzy weight subsystem for Return to Castle Wolfenstein bot AI.
	- It defines the structures and interfaces that allow bots to evaluate item desirability based on inventory state using fuzzy logic.
	- The file also provides caching and lifecycle management for weight configurations.

	\par Core Responsibilities
	- Define the data structures for fuzzy weight representation (`fuzzyseperator_t`, `weight_t`, `weightconfig_t`).
	- Provide API to load, cache, query, modify, evolve, and delete weight configurations.
	- Implement fuzzy decision algorithms that map inventory counts to weighted scores.
	- Expose scaling, evolution, and interbreeding utilities to evolve bot behavior over time.

	\par Key Types and Functions
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

	\par Control Flow
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

	\par Dependencies
	- Requires definition of `MAX_QPATH` (likely from `q_shared.h`).
	- Relies on `qboolean` type (also from `q_shared.h`).
	- Uses global console variable `bot_reloadcharacters`.
	- Calls auxiliary functions (`ScaleFuzzySeperator_r`, `EvolveFuzzySeperator_r`, `InterbreedFuzzySeperator_r`) defined elsewhere in the botlib.
	- Depends on standard string and memory utilities (`strcmp`, `malloc`, `free`).

	\par How It Fits
	- Serves as the core weight evaluation engine used by bot decision modules.
	- Weights are loaded from text files in the game content and reused across multiple bot instances to reduce memory usage.
	- The fuzzy logic framework enables bots to perform probabilistic item selection, supporting dynamic difficulty and behavior evolution.
	- The subsystem integrates with the broader botlib by providing shared global caches and configuration interfaces used during gameplay.
*/

/*****************************************************************************
 * name:		be_ai_weight.h
 *
 * desc:		fuzzy weights
 *
 *
 *****************************************************************************/

#define WT_BALANCE	1
#define MAX_WEIGHTS 128

// fuzzy seperator
typedef struct fuzzyseperator_s {
	int						 index;
	int						 value;
	int						 type;
	float					 weight;
	float					 minweight;
	float					 maxweight;
	struct fuzzyseperator_s* child;
	struct fuzzyseperator_s* next;
} fuzzyseperator_t;

// fuzzy weight
typedef struct weight_s {
	char*					 name;
	struct fuzzyseperator_s* firstseperator;
} weight_t;

// weight configuration
typedef struct weightconfig_s {
	int		 numweights;
	weight_t weights[MAX_WEIGHTS];
	char	 filename[MAX_QPATH];
} weightconfig_t;

/*!
	\brief Parses a weight configuration file and returns a weightconfig_t structure, caching it to avoid duplicate loads.

	If bot_reloadcharacters is false and the file has already been loaded, the function returns the existing configuration. Otherwise it loads the file from disk (or a pak file) with LoadSourceFile,
   allocates a weightconfig_t, parses "weight" entries, and builds fuzzy separators as needed. On any failure (file missing, memory error, syntax error) all allocated memory is freed and the function
   returns NULL. The function also logs informational or error messages and, in debug builds, reports loading time. The parsed configuration is stored in the global weightFileList array at the next
   free slot and later reused until reloading is forced.

	\param filename Path to the weight configuration file to load
	\return A pointer to the created weightconfig_t or NULL if an error occurs.
*/
weightconfig_t* ReadWeightConfig( char* filename );

/*!
	\brief Releases a weight configuration when character reloads are enabled.

	The function first checks the 'bot_reloadcharacters' console variable. If the variable is false or unset, the function returns immediately and the configuration is left untouched. When the
   variable indicates that bots can reload characters, the cleanup work is delegated to FreeWeightConfig2, which actually frees the resources associated with the provided configuration.

	\param config Pointer to the weightconfig_t instance to free.
*/
void			FreeWeightConfig( weightconfig_t* config );
// writes a weight configuration, returns true if successfull
qboolean		WriteWeightConfig( char* filename, weightconfig_t* config );

/*!
	\brief Searches a weight configuration for a weight name and returns its index or -1 if not found

	The function iterates over the first wc->numweights entries of the weight array within the supplied weightconfig_t. For each entry it compares the stored name string with the supplied name using a
   case‑sensitive strcmp. If a match is found the loop index is returned immediately. When all entries have been examined without a match, the function returns -1 to signal that the weight was not
   found.

	\param wc pointer to a weight configuration structure containing an array of weight entries and a count of valid entries
	\param name pointer to a null‑terminated string representing the desired weight name
	\return an integer representing the position of the matching weight entry within the weight array, or -1 if no match is found
*/
int				FindFuzzyWeight( weightconfig_t* wc, char* name );

/*!
	\brief Retrieves the fuzzy weight for a given inventory and weight configuration.

	If the requested weight configuration has no separator chain, the function returns 0. Otherwise it walks a linked list of separators: each node describes a threshold on a specific inventory slot.
   When the inventory value at that slot is less than the node’s threshold the traversal continues through the child pointer; otherwise it proceeds to the next sibling. The function returns the weight
   stored in the node that has no appropriate child or next link. The traversal continues until a terminal node is reached, guaranteeing a weight value is returned when a valid chain exists. The
   algorithm is deterministic and has no side effects. The #ifdef EVALUATERECURSIVELY path invokes a recursive helper; otherwise the loop-based implementation is used.

	\param inventory array of item counts indexed by inventory slot
	\param wc pointer to the weight configuration structure containing the separator chains
	\param weightnum index selecting the specific weight entry in the configuration
	\return The computed fuzzy weight as a float
*/
float			FuzzyWeight( int* inventory, weightconfig_t* wc, int weightnum );

/*!
	\brief Compute a fuzzy weight for an item using a separator tree based on the current inventory

	The function walks a tree of fuzzy separators stored in the weight config. For each node it compares the inventory amount at the specified index to a threshold value. If the inventory is less than
   the threshold, it follows the child link; otherwise it follows the next link. When a leaf node is reached (no child or next pointer) a random weight is chosen uniformly between the leaf’s minweight
   and maxweight. If the weight configuration has no separator for the given weight number, the function returns zero. The weight may be used to influence bot item selection when the bot is uncertain
   about the item’s value.

	\param inventory pointer to an array of item counts used as the decision basis
	\param wc pointer to the weight configuration containing separator definitions
	\param weightnum index of the weight entry to evaluate in the config
	\return a floating point weight value, zero if no configuration or the leaf is unreachable, otherwise a random value within the leaf’s specified range
*/
float			FuzzyWeightUndecided( int* inventory, weightconfig_t* wc, int weightnum );

/*!
	\brief Scales a named weight by applying a clamped scale factor to its separator.

	The function first clamps the supplied scale value to the range [0,1]. It then iterates through the weights in the provided configuration until it finds a weight whose name matches the supplied
   name. When a match is found, it calls ScaleFuzzySeperator_r on the weight's first separator with the clamped scale value and exits the loop. If no matching weight is found, the function does
   nothing further.

	\param config Pointer to the weight configuration that holds the array of weights and the count of weights.
	\param name C string containing the name of the weight to be scaled.
	\param scale Floating‑point scale factor to apply; value is clamped between 0 and 1 before use.
*/
void			ScaleWeight( weightconfig_t* config, char* name, float scale );
// scale the balance range
void			ScaleBalanceRange( weightconfig_t* config, float scale );

/*!
	\brief Evolves each weight's fuzzy separator in a weight configuration

	Iterates over the number of weights stored in the configuration and calls EvolveFuzzySeperator_r on the first separator of each weight, updating them in place.

	\param config Pointer to the weight configuration to evolve
*/
void			EvolveWeightConfig( weightconfig_t* config );

/*!
	\brief Combine two weight configurations by interpolating their fuzzy separators into a new configuration

	The function first checks that all three configurations have the same number of weight entries. If the number of weights differs, it prints an error message and returns without modifying the
   output configuration. When the size matches, it iterates through each weight and calls InterbreedFuzzySeperator_r to merge the corresponding fuzzy separators from the two input configurations into
   the output configuration.

	\param config1 The first source configuration whose weights are being combined
	\param config2 The second source configuration whose weights are being combined
	\param configout The configuration that will receive the merged weights; must already have a matching numweights value
*/
void			InterbreedWeightConfigs( weightconfig_t* config1, weightconfig_t* config2, weightconfig_t* configout );

/*!
	\brief Frees and clears all cached weight configurations.

	The function iterates over the global weightFileList array up to the maximum defined by MAX_WEIGHT_FILES. For each non‑null entry it calls FreeWeightConfig2 to release the associated memory and
   then sets the array slot to null, ensuring no dangling references remain.

*/
void			BotShutdownWeights();
