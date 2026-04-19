<!-- doxygenix: sha256=bd128bf66bbcb738a042a7118097fba2b9053b1a9b71801d3833a916dac89d47 -->

# engine/botlib/be_ai_goal.cpp

## File Purpose
- Implement all data structures and algorithms needed for the goal‑based portion of the bot AI, including goal selection, stacking, avoidance, and fuzzy‑logic weight handling.
- Expose an API used by the bot engine to create, reset, free, and query goal states and to load item configuration and weights.

## Core Responsibilities
- Provide persistent goal state objects that encapsulate a bot’s goal stack, avoid‑goal list, and item weight configurations.
- Load, maintain, and mutate fuzzy‑logic weight tables for item selection.
- Track all in‑world items and map entities (locations, camp spots) used by bots for navigation.
- Implement a stack‑based goal hierarchy and avoid‑goal timing system for dynamic AI behavior.
- Expose utility functions for goal lifecycle management (push, pop, reset, dump) and item weight manipulation (load, free, interbreed, mutate, save).

## Key Types and Functions
- bot_goalstate_t — structure holding a bot’s goal stack, avoid‑goal timers, item weight config, and bookkeeping data.
- maplocation_t — linked‑list node representing a named map location entity.
- campspot_t — linked‑list node representing a camp spot entity used for defensive positioning.
- levelitem_t — node in the doubly‑linked list of dynamic or static items in the level.
- iteminfo_t — descriptor for a type of item, including bounding box, model name, drop time, etc.
- itemconfig_t — container for an array of iteminfo_t read from disk.
- BotGoalStateFromHandle(int handle) — retrieves a bot_goalstate_t pointer corresponding to the given handle, validating bounds and existence.
- BotInterbreedGoalFuzzyLogic(int parent1, int parent2, int child) — merges two goal state weight configs into a child config using InterbreedWeightConfigs.
- BotSaveGoalFuzzyLogic(int goalstate, char* filename) — writes a goal state’s weight config to a file (stubbed).
- BotMutateGoalFuzzyLogic(int goalstate, float range) — applies EvolveWeightConfig to mutate a goal state’s item weights.
- LoadItemConfig(char* filename) — parses an item configuration file, building itemconfig_t and iteminfo_t structures.
- ItemWeightIndex(weightconfig_t* iwc, itemconfig_t* ic) — allocates an int array mapping each iteminfo to the index of its fuzzy weight function.
- InitLevelItemHeap() — pre‑allocates a contiguous block of levelitem_t objects and links them into a free list.
- AllocLevelItem() — pops a levelitem_t from the free list and returns it for use by the engine.
- FreeLevelItem(levelitem_t* li) — returns a levelitem_t to the free list.
- AddLevelItemToList(levelitem_t* li) — inserts a level item node at the head of the global doubly‑linked list.
- RemoveLevelItemFromList(levelitem_t* li) — detaches a level item from the global list.
- BotFreeInfoEntities() — frees all allocations for map locations and camp spots.
- BotInitInfoEntities() — scans the BSP for target_location and info_camp entities, populating maplocation_t and campspot_t lists.
- BotInitLevelItems() — populates the level item heap, scans for item entities, and creates levelitem_t entries with calculated reachability data.
- BotGoalName(int number, char* name, int size) — copies the name of a level item into the supplied buffer.
- BotResetAvoidGoals(int goalstate) — zeros the avoid goal arrays for a given bot.
- BotDumpAvoidGoals(int goalstate) — logs all currently avoided goals and their remaining times.
- BotAddToAvoidGoals(bot_goalstate_t* gs, int number, float avoidtime) — registers a goal number to be avoided until the timeout expires.
- BotRemoveFromAvoidGoals(int goalstate, int number) — removes a specified goal number from a bot’s avoid list.
- BotAvoidGoalTime(int goalstate, int number) — returns the remaining avoidance time for a goal, or 0 if none.
- BotChooseLTGItem(int goalstate, vec3_t origin, int* inventory, int travelflags) — selects the best long‑term goal item based on fuzzy weights, travel time, and avoidance, pushing a GFL_ITEM goal onto the stack.
- BotChooseNBGItem(int goalstate, vec3_t origin, int* inventory, int travelflags, bot_goal_t* ltg, float maxtime) — selects the best nearby item relative to a long‑term goal, similar to BotChooseLTGItem but constrained by maxtime.
- BotTouchingGoal(vec3_t origin, bot_goal_t* goal) — tests whether a bot’s origin lies within the bounding box of a goal volume.
- BotItemGoalInVisButNotVisible(int viewer, vec3_t eye, vec3_t viewangles, bot_goal_t* goal) — determines if an item goal should be visible but isn’t due to trace or entity timing.

## Important Control Flow
- At engine start, BotSetupGoalAI loads global item definitions from a file and initializes the item config structure.
- During gameplay, each bot receives a goal state handle; BotAllocGoalState creates or reuses a goal state structure, storing the client number and allocating item weight configuration space.
- BotLoadItemWeights reads fuzzy weight data for a goal state, builds an index mapping item info classes to weight functions, and updates the bot’s internal weight tables.
- BotInitLevelItems scans the level’s BSP for in‑world items, assigns them to the level item heap, determines their goal area, and adds them to a linked list of active items.
- Per frame, BotUpdateEntityItems cycles through dynamic entity items, updates their positions, creates new level item entries for freshly dropped objects, and removes timed‑out items from the list.
- Goal stack mechanics (Push/Pop/Empty/GetTop/GetSecond) allow each bot to maintain a LIFO list of current objectives.
- Avoidance logic (Add/Remove/Time) records items temporarily undesired by a bot, preventing immediate re‑targeting for a configurable period.
- Item‑selection functions (BotChooseLTGItem, BotChooseNBGItem) compute the best next goal by evaluating fuzzy weights, travel times, and avoidance timers, then push the chosen goal onto the stack.
- Whenever the game state changes (e.g. end of level), BotShutdownGoalAI frees all heap structures, goal states, and configuration data.

## External Dependencies
- "q_shared.h"
- "l_utils.h"
- "l_libvar.h"
- "l_memory.h"
- "l_log.h"
- "l_script.h"
- "l_precomp.h"
- "l_struct.h"
- "aasfile.h"
- "botshared/botlib.h"
- "botshared/be_aas.h"
- "be_aas_funcs.h"
- "be_interface.h"
- "be_ai_weight.h"
- "botshared/be_ai_goal.h"
- "botshared/be_ai_move.h"

## How It Fits
- This file is one of the core modules of the bot subsystem; it sits atop the AAS navigation engine and relies on the weight‑config subsystem to prioritize items.
- Bot AI calls into these routines during the navigation loop to decide what item or area to head toward.
- The goal module interfaces with other bot components (movement, shooting, perception) by supplying goal data and by updating the bot’s internal state through the goal stack and avoid lists.
- The item‑config and weight‑config data are global shared resources that enable consistent item selection across all bots.
