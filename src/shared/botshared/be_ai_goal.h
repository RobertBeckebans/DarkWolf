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

/*****************************************************************************
 * name:		be_ai_goal.h
 *
 * desc:		goal AI
 *
 *
 *****************************************************************************/

#define MAX_AVOIDGOALS	   64
#define MAX_GOALSTACK	   8

#define GFL_NONE		   0
#define GFL_ITEM		   1
#define GFL_ROAM		   2
#define GFL_NOSLOWAPPROACH 4

// a bot goal
typedef struct bot_goal_s {
	vec3_t origin;	   // origin of the goal
	int	   areanum;	   // area number of the goal
	vec3_t mins, maxs; // mins and maxs of the goal
	int	   entitynum;  // number of the goal entity
	int	   number;	   // goal number
	int	   flags;	   // goal flags
	int	   iteminfo;   // item information
} bot_goal_t;

/*!
	\brief Resets the goal state for a bot while preserving item weights.

	This function resets the goal state of a bot by clearing the goal stack and resetting avoid goals. It does not clear item weights, which are maintained across resets. The function operates on a
   goal state identified by a handle and performs no action if the handle is invalid.

	\param goalstate Handle to the bot goal state to reset
*/
void  BotResetGoalState( int goalstate );

/*!
	\brief Resets the avoid goals for a specified bot goal state

	This function clears the avoid goals and their associated timers for a given bot goal state. It retrieves the goal state structure using the provided handle, and if valid, zeros out the memory for
   both the avoid goals array and the avoid goals times array. This allows the bot to reconsider previously avoided goals and potentially re-evaluate them during pathfinding.

	\param goalstate Handle identifying the bot goal state to reset
*/
void  BotResetAvoidGoals( int goalstate );

/*!
	\brief Removes a specified goal number from the avoid goals list for a given bot goal state

	This function removes a goal number from the avoid goals list of a bot goal state. It searches through the avoid goals array and clears the entry if it finds a match with the specified number and
   if the goal time is still valid. The function does not perform any action if the goal state handle is invalid or if the goal number is not found in the avoid goals list

	\param goalstate Handle to the bot goal state
	\param number The goal number to remove from avoid goals
*/
void  BotRemoveFromAvoidGoals( int goalstate, int number );

/*!
	\brief Pushes a goal onto the bot's goal stack for the specified goal state

	This function adds a goal to the bot's goal stack by first validating the goal state handle and checking for stack overflow. If the goal state is invalid or the stack is full, the function returns
   without making changes. Otherwise, it increments the stack pointer and copies the provided goal into the stack

	\param goalstate Identifier for the bot's goal state
	\param goal Pointer to the goal structure to be pushed onto the stack
*/
void  BotPushGoal( int goalstate, bot_goal_t* goal );

/*!
	\brief Removes the topmost goal from the goal stack for the specified goal state.

	This function pops the most recently added goal from the goal stack associated with the given goal state. It decrements the stack pointer only if the stack is not already empty. The function
   operates on a goal state handle and ensures that the goal state exists before attempting to modify its stack.

	\param goalstate The handle to the goal state from which to pop the goal
*/
void  BotPopGoal( int goalstate );

/*!
	\brief Empties the goal stack for the specified bot goal state.

	This function resets the goal stack top index to zero, effectively clearing all goals that have been pushed onto the stack for the given bot goal state. It retrieves the goal state structure using
   the provided handle and checks for validity before clearing the stack.

	\param goalstate Handle to the bot goal state whose goal stack needs to be emptied
*/
void  BotEmptyGoalStack( int goalstate );

/*!
	\brief Dumps the list of avoid goals for a specified bot goal state to the log

	This function retrieves the avoid goals associated with a given bot goal state and logs their names along with the remaining time they should be avoided. The avoid goals are stored in a goal state
   structure and are only logged if their avoidance time has not yet expired. This function is typically used for debugging purposes to understand which goals a bot is currently avoiding.

	\param goalstate The handle to the bot goal state structure containing the avoid goals to dump
*/
void  BotDumpAvoidGoals( int goalstate );

/*!
	\brief Dumps the current goal stack for the specified bot goal state to the log

	This function retrieves the goal state associated with the provided handle and iterates through the goal stack, logging each goal's name along with its position in the stack. The goal stack is
   represented as a collection of goals that the bot is working towards, with the top goal being the most immediate target. The function is primarily used for debugging purposes to understand the
   bot's current goal progression.

	\param goalstate Handle to the bot goal state whose stack should be dumped
*/
void  BotDumpGoalStack( int goalstate );

/*!
	\brief Retrieves the name of a bot goal identified by a number and stores it in the provided buffer.

	The function looks up a bot goal by its number in the level items configuration and copies the corresponding name into the provided buffer. If the goal number is not found, it sets the name buffer
   to an empty string. The function ensures the result is null-terminated and respects the buffer size limit.

	\param number The identifier number of the bot goal to look up
	\param name Buffer to store the name of the goal
	\param size Size of the name buffer in bytes
*/
void  BotGoalName( int number, char* name, int size );

/*!
	\brief Retrieves the top goal from the bot's goal stack for the specified goal state

	This function fetches the most recent goal from the bot's goal stack identified by the given goal state handle. It copies the goal data into the provided goal structure and returns a boolean
   indicating success or failure. The function first validates the goal state handle and checks if there are any goals on the stack before copying the data.

	\param goalstate Handle to the bot goal state
	\param goal Pointer to the structure where the retrieved goal will be stored
	\return Returns qtrue if a goal was successfully retrieved from the stack, qfalse otherwise
*/
int	  BotGetTopGoal( int goalstate, bot_goal_t* goal );

/*!
	\brief Retrieves the second goal from the bot's goal stack, if it exists.

	This function accesses the goal stack associated with the provided goalstate and copies the second-to-top goal into the provided goal structure. It returns true if a second goal exists and was
   successfully copied, otherwise it returns false.

	\param goalstate Handle to the bot goal state
	\param goal Pointer to the goal structure where the second goal will be stored
	\return True if a second goal exists and was copied, false otherwise
*/
int	  BotGetSecondGoal( int goalstate, bot_goal_t* goal );

/*!
	\brief Chooses the best long term goal item for a bot based on inventory and travel flags.

	This function evaluates available items in the game environment to determine the most beneficial item for the bot to pursue as a long-term goal. It considers the bot's current position, inventory
   state, and travel restrictions. The function uses fuzzy weight calculations to prioritize items, taking into account factors such as respawn time, travel time to the item's location, and whether
   the item should be avoided. If a suitable item is found, it is added to the bot's goal stack and the function returns true. The function also handles cases where no suitable items are found by
   potentially setting a roaming goal. The item selection process is influenced by game type specific restrictions and avoids items that the bot recently visited or that are marked for avoidance.

	\param goalstate Handle to the bot's goal state
	\param origin Current 3D position of the bot
	\param inventory Array of integers representing the bot's inventory
	\param travelflags Flags that control what travel types are allowed
	\return Integer value indicating success (true) or failure (false) of item selection
*/
int	  BotChooseLTGItem( int goalstate, vec3_t origin, int* inventory, int travelflags );

/*!
	\brief Selects the best nearby item goal for a bot based on inventory, travel flags, and time constraints.

	This function determines the most favorable item for a bot to target next by evaluating item weights, travel times, and avoidance times. It considers the bot's current position, inventory state,
   and a provided long-term goal to calculate the optimal item. If a suitable item is found, it adds the item goal to the bot's goal stack and updates the avoidance time for that item. The function
   returns true if a valid item is chosen, or false otherwise. The algorithm filters items based on game type restrictions and checks for reachable areas to ensure the bot can travel to the item.
   Travel times are scaled using a configurable factor to balance item favorability with travel cost.

	\param goalstate Handle to the bot's goal state, used to track and manage goals.
	\param origin The 3D coordinates of the bot's current position in the game world.
	\param inventory An array containing the bot's current item inventory.
	\param travelflags Flags that determine allowed travel methods to reach the goal.
	\param ltg Pointer to the long-term goal; used to calculate travel time back to the goal.
	\param maxtime Maximum allowed travel time to consider an item goal viable.
	\return Returns qtrue if a suitable item goal is selected and added to the bot's goal stack, otherwise returns qfalse.
*/
int	  BotChooseNBGItem( int goalstate, vec3_t origin, int* inventory, int travelflags, bot_goal_t* ltg, float maxtime );

/*!
	\brief Checks if a bot origin is touching a specified goal area

	This function determines whether a bot's origin position intersects with the bounding box of a goal area. It calculates the absolute bounds of the goal by adjusting its minimum and maximum
   coordinates based on the normal presence type bounding box, then checks if the given origin point falls within those bounds. The function uses a safety margin to make the check slightly more
   lenient.

	\param origin The 3D coordinates of the bot's origin position
	\param goal Pointer to the goal structure containing the goal's bounds and origin
	\return Non-zero value if the bot origin is inside the goal area, zero otherwise
*/
int	  BotTouchingGoal( vec3_t origin, bot_goal_t* goal );

/*!
	\brief Determines if a bot goal item is visible in the world but not visible to the bot viewer.

	This function checks if a given bot goal item is in a visible location but not currently visible to the specified viewer. It uses the viewer's position and angles to perform a trace to the item's
   middle point. If the trace is unobstructed, it then validates the item's entity information to determine if the item is missing or outdated, indicating it should be visible but currently isn't.
   This can help bots identify items that are obscured or not properly updated in the game world.

	\param viewer The entity number of the bot viewer that is checking visibility
	\param eye The 3D position of the viewer's eye
	\param viewangles The 3D angles of the viewer's view direction
	\param goal Pointer to the bot goal structure specifying the item to check visibility for
	\return Returns true if the goal item should be visible but isn't, false otherwise
*/
int	  BotItemGoalInVisButNotVisible( int viewer, vec3_t eye, vec3_t viewangles, bot_goal_t* goal );

/*!
	\brief Retrieves the goal information for a level item based on its index and class name

	This function searches through the list of level items to find one that matches the specified index and class name. It checks against game type restrictions such as single player, team, or free
   for all modes. When a matching item is found, it populates the provided goal structure with information about the item's location, dimensions, and entity number. The function returns the item
   number if successful, or -1 if no matching item is found.

	\param index The index number of the level item to search for
	\param classname The class name of the item to search for
	\param goal Pointer to the goal structure that will be populated with item information
	\return The item number if a matching level item is found, or -1 if no matching item is found
*/
int	  BotGetLevelItemGoal( int index, char* classname, bot_goal_t* goal );

/*!
	\brief Retrieves the next camp spot goal from a linked list of camp spots and returns its index.

	This function iterates through a linked list of camp spots starting from the first one. It decrements a counter based on the input num parameter and returns the next camp spot when the counter
   reaches zero. The function modifies the provided goal structure with the camp spot's area number, origin, and bounding box coordinates. If the counter is less than zero, it resets it to zero. If no
   camp spot is found, it returns zero.

	\param num The index of the camp spot to retrieve, adjusted to be non-negative if negative
	\param goal Pointer to the bot_goal_t structure where the camp spot data will be stored
	\return The index of the next camp spot in the sequence, or zero if no camp spot is found.
*/
int	  BotGetNextCampSpotGoal( int num, bot_goal_t* goal );

/*!
	\brief Retrieves the map location goal data for a specified location name.

	This function searches through a list of predefined map locations to find a match with the given name. When a match is found, it copies the location data into the provided goal structure,
   including the area number, origin coordinates, and bounding box dimensions. The function returns a boolean value indicating whether the location was found.

	\param name The name of the map location to search for
	\param goal Pointer to the goal structure where the location data will be stored
	\return Returns 1 if the map location is found and the data is successfully copied into the goal structure, otherwise returns 0.
*/
int	  BotGetMapLocationGoal( char* name, bot_goal_t* goal );

/*!
	\brief Returns the remaining time a bot should avoid a specific goal

	This function checks if a bot has a goal in its avoid list and returns the remaining time the bot should avoid that goal. If the goal is not in the avoid list or if the avoid time has expired, it
   returns zero. The function is used to prevent bots from immediately re-targeting goals they recently avoided.

	\param goalstate Handle to the bot's goal state
	\param number Identifier of the goal to check
	\return Remaining time in seconds that the bot should avoid the specified goal, or zero if the goal is not being avoided
*/
float BotAvoidGoalTime( int goalstate, int number );

/*!
	\brief Initializes the items in the level for bot navigation and goal selection.

	This function processes all entities in the current level's BSP file to identify game items that bots can interact with. It initializes the level item heap, scans through each entity to find
   matching item classes, retrieves their spawn flags and origin positions, and sets up the necessary data structures for bot AI to understand item locations. The function also handles special cases
   like stationary items and logs warnings for missing or invalid item configurations.

*/
void  BotInitLevelItems();

/*!
	\brief Updates the list of entity items in the game world, managing dropped weapons, flags, and other collectible items.

	This function maintains a list of items that are currently present in the game world. It periodically checks for entity items that are no longer valid or have timed out, and removes them from the
   list. It also identifies new entity items that should be added to the list, determining their properties, positions, and goal areas for bot navigation. The function handles entity items that are
   not moving, are on the ground, and are not already tracked in the list of level items.

*/
void  BotUpdateEntityItems();

/*!
	\brief Combines the fuzzy logic item weight configurations from two parent goal states to create a child goal state configuration

	This function performs interbreeding of goal fuzzy logic by combining the item weight configurations from two parent bot goal states and applying the result to a child goal state. It retrieves the
   goal state structures from their respective handles and then calls the interbreeding function for the weight configurations. This is used in bot AI development to combine traits from parent bot
   behaviors to create offspring behaviors with mixed characteristics.

	\param parent1 Handle to the first parent bot goal state
	\param parent2 Handle to the second parent bot goal state
	\param child Handle to the child bot goal state that will receive the interbred configuration
*/
void  BotInterbreedGoalFuzzyLogic( int parent1, int parent2, int child );

/*!
	\brief Saves the goal fuzzy logic configuration to a file

	This function saves the fuzzy logic configuration associated with a specified goal state to a disk file. The function retrieves the goal state using a handle and then writes the item weight
   configuration to the provided filename. This allows for persistence of bot behavior configurations between game sessions.

	\param goalstate Handle to the goal state whose fuzzy logic configuration will be saved
	\param filename Path to the file where the fuzzy logic configuration will be written
*/
void  BotSaveGoalFuzzyLogic( int goalstate, char* filename );

/*!
	\brief Mutates the fuzzy logic configuration for a bot goal state within the specified range.

	This function modifies the fuzzy logic weights used by the bot for goal selection. It retrieves the goal state using the provided handle and evolves the item weight configuration associated with
   that goal state. The mutation is performed within the specified range to introduce variation in the bot's decision-making process.

	\param goalstate Handle to the bot goal state to be mutated
	\param range Range within which to mutate the fuzzy logic weights
*/
void  BotMutateGoalFuzzyLogic( int goalstate, float range );

/*!
	\brief Loads item weights for a bot goal state from a specified file.

	This function initializes the item weight configuration for a bot's goal state by reading from a file. It first retrieves the goal state structure using the provided handle, then attempts to load
   the weight configuration from the specified filename. If successful, it creates an item weight index using the loaded configuration and the global item configuration. The function returns an error
   code indicating success or failure of the operation.

	\param goalstate Handle to the bot goal state for which item weights are being loaded
	\param filename Path to the file containing the item weight configuration data
	\return Error code indicating the result of the operation, BLERR_NOERROR on success or BLERR_CANNOTLOADITEMWEIGHTS on failure
*/
int	  BotLoadItemWeights( int goalstate, char* filename );

/*!
	\brief Frees the item weights associated with the specified bot goal state

	This function releases the memory allocated for item weights in the bot goal state. It first retrieves the goal state structure using the provided handle, and then checks if item weight
   configuration and index exist. If they do, it frees the corresponding memory blocks. This function is typically called when cleaning up bot behavior data or when switching between different level
   items.

	\param goalstate Handle to the bot goal state for which item weights should be freed
*/
void  BotFreeItemWeights( int goalstate );

/*!
	\brief Allocates and returns a new goal state handle for the specified client

	This function searches for the first available slot in the botgoalstates array to allocate a new goal state. It initializes the allocated goal state with the provided client number and returns the
   handle to the allocated slot. If no slots are available, it returns zero.

	\param client The client identifier for which to allocate a goal state
	\return The handle of the newly allocated goal state, or zero if no slots are available
*/
int	  BotAllocGoalState( int client );

/*!
	\brief Frees the goal state associated with the given handle.

	This function releases the memory allocated for a bot's goal state. It first validates that the handle is within the valid range and that the goal state exists. If either validation fails, it
   prints a fatal error message and returns without taking further action. Otherwise, it frees the item weights associated with the goal state and then deallocates the memory for the goal state
   itself, setting the corresponding entry in the global array to NULL.

	\param handle The handle identifying the goal state to be freed
*/
void  BotFreeGoalState( int handle );

/*!
	\brief Initializes the goal-based AI system for bots.

	This function sets up the goal AI by checking if teamplay is enabled, loading the item configuration file specified by the 'itemconfig' library variable, and handling errors if the configuration
   fails to load. It ensures that the bot AI has the necessary item configuration data to make decisions about goal selection and item acquisition.

	\return Error code indicating success or failure of the setup process. Returns BLERR_NOERROR on success, or BLERR_CANNOTLOADITEMCONFIG if the item configuration file cannot be loaded.
*/
int	  BotSetupGoalAI();

/*!
	\brief Shuts down the goal AI subsystem by freeing all allocated memory and cleaning up bot goal states.

	This function performs a complete shutdown of the goal AI system. It frees the item configuration memory and resets all related pointers. It also frees the level item heap and resets all related
   level item pointers. Additionally, it frees all bot goal states for clients from 1 to MAX_CLIENTS and calls BotFreeInfoEntities to clean up information entities.

*/
void  BotShutdownGoalAI();
