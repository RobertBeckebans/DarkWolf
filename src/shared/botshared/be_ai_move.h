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
 * name:		be_ai_move.h
 *
 * desc:		movement AI
 *
 *
 *****************************************************************************/

// movement types
#define MOVE_WALK					1
#define MOVE_CROUCH					2
#define MOVE_JUMP					4
#define MOVE_GRAPPLE				8
#define MOVE_ROCKETJUMP				16
#define MOVE_BFGJUMP				32
// move flags
#define MFL_BARRIERJUMP				1	// bot is performing a barrier jump
#define MFL_ONGROUND				2	// bot is in the ground
#define MFL_SWIMMING				4	// bot is swimming
#define MFL_AGAINSTLADDER			8	// bot is against a ladder
#define MFL_WATERJUMP				16	// bot is waterjumping
#define MFL_TELEPORTED				32	// bot is being teleported
#define MFL_ACTIVEGRAPPLE			64	// bot is using the grapple hook
#define MFL_GRAPPLERESET			128 // bot has reset the grapple
#define MFL_WALK					256 // bot should walk slowly
// move result flags
#define MOVERESULT_MOVEMENTVIEW		1	// bot uses view for movement
#define MOVERESULT_SWIMVIEW			2	// bot uses view for swimming
#define MOVERESULT_WAITING			4	// bot is waiting for something
#define MOVERESULT_MOVEMENTVIEWSET	8	// bot has set the view in movement code
#define MOVERESULT_MOVEMENTWEAPON	16	// bot uses weapon for movement
#define MOVERESULT_ONTOPOFOBSTACLE	32	// bot is ontop of obstacle
#define MOVERESULT_ONTOPOF_FUNCBOB	64	// bot is ontop of a func_bobbing
#define MOVERESULT_ONTOPOF_ELEVATOR 128 // bot is ontop of an elevator (func_plat)
#define MOVERESULT_FUTUREVIEW		256 // RF, if we want to look ahead of time, this is a good direction

//
#define MAX_AVOIDREACH				1
//
#define RESULTTYPE_ELEVATORUP		1
#define RESULTTYPE_INVISIBLEGRAPPLE 2

// structure used to initialize the movement state
// the or_moveflags MFL_ONGROUND, MFL_TELEPORTED and MFL_WATERJUMP come from the playerstate
typedef struct bot_initmove_s {
	vec3_t origin;		 // origin of the bot
	vec3_t velocity;	 // velocity of the bot
	vec3_t viewoffset;	 // view offset
	int	   entitynum;	 // entity number of the bot
	int	   client;		 // client number of the bot
	float  thinktime;	 // time the bot thinks
	int	   presencetype; // presencetype of the bot
	vec3_t viewangles;	 // view angles of the bot
	int	   or_moveflags; // values ored to the movement flags
} bot_initmove_t;

// NOTE: the ideal_viewangles are only valid if MFL_MOVEMENTVIEW is set
typedef struct bot_moveresult_s {
	int	   failure;			 // true if movement failed all together
	int	   type;			 // failure or blocked type
	int	   blocked;			 // true if blocked by an entity
	int	   blockentity;		 // entity blocking the bot
	int	   traveltype;		 // last executed travel type
	int	   flags;			 // result flags
	int	   weapon;			 // weapon used for movement
	vec3_t movedir;			 // movement direction
	vec3_t ideal_viewangles; // ideal viewangles for the movement
} bot_moveresult_t;

/*!
	\brief Resets the specified bot movement state by clearing all its data.

	This function resets the bot movement state by clearing all data in the associated bot_movestate_t structure. It takes a handle to the movement state and ensures that the memory is zeroed out. If
   the handle is invalid or the movement state is not found, the function returns without performing any action.

	\param movestate Handle to the bot movement state to be reset
*/
void BotResetMoveState( int movestate );

/*!
	\brief Moves a bot toward a specified goal using navigation data.

	This function handles the movement logic for a bot to reach a given goal. It initializes the movement result, retrieves the current move state, and resets the grapple state. The function checks if
   a valid goal exists and updates the bot's movement flags based on its current physical state, such as being on the ground, swimming, or against a ladder. It evaluates reachability from the bot's
   last position to determine the appropriate movement path. If the bot is in the goal area, it calls a specific function to handle movement within that area. Otherwise, it considers the current
   reachability and updates the bot's state to continue navigation towards the goal. Special handling is included for entities such as elevators and bobbing platforms to ensure correct movement
   behavior.

	\param result Pointer to the structure that will hold the movement result including flags and blocking information
	\param movestate Handle to the bot's current movement state
	\param goal Pointer to the goal structure that defines the destination
	\param travelflags Flags that specify the types of travel the bot is allowed to use
*/
void BotMoveToGoal( bot_moveresult_t* result, int movestate, bot_goal_t* goal, int travelflags );

/*!
	\brief Moves the bot in the specified direction using either swimming or walking movement based on the bot's current state.

	This function handles bot movement in a specified direction by determining whether the bot is swimming or walking. It uses the provided movement state, direction vector, speed, and type to perform
   the appropriate movement action. The function delegates to either BotSwimInDirection or BotWalkInDirection depending on the bot's swimming state.

	\param movestate Handle to the bot's movement state
	\param dir Direction vector for movement
	\param speed Speed value for the movement
	\param type Type of movement action to perform
	\return Returns qfalse if the movement state is invalid, otherwise returns the result of either BotSwimInDirection or BotWalkInDirection based on the bot's swimming state.
*/
int	 BotMoveInDirection( int movestate, vec3_t dir, float speed, int type );

/*!
	\brief Resets the avoid reachability data for a specified bot movement state

	This function clears the avoid reachability arrays and related tracking data for a given bot movement state. It resets the internal state that tracks areas and times to avoid when navigating. The
   function retrieves the movement state structure using the provided handle, and if valid, clears the avoid reachability information and resets key movement state variables such as the last area
   number, last goal area number, and last reach number. This is typically called when a bot needs to reset its navigation state, such as when starting a new AI function or when repositioning.

	\param movestate Handle to the bot movement state to reset
*/
void BotResetAvoidReach( int movestate );

/*!
	\brief Resets the last avoid reachability for the specified movement state

	This function resets the most recently recorded avoid reachability for a given movement state. It identifies the latest avoid reachability entry based on timestamp and clears it. If the entry has
   associated retry count, it decrements that count as well.

	\param movestate Handle for the movement state to reset
*/
void BotResetLastAvoidReach( int movestate );

/*!
	\brief Determines the reachability area for a bot at the specified origin, considering various environmental factors.

	This function evaluates the bot's position within the game environment to identify which reachability area it belongs to. It takes into account whether the bot is standing on a solid object,
   swimming, or on a func_plat or func_bobbing entity. If the bot is on a solid surface, it checks for valid reachability links. If no direct reachability is found, it traces down to find a valid
   area. The function returns the area number if a valid reachability area is found, otherwise it returns zero.

	\param origin The 3D coordinates of the bot's position
	\param client The client index of the bot
	\return The area number representing the reachability area the bot is in, or zero if no valid area is found
*/
int	 BotReachabilityArea( vec3_t origin, int client );

/*!
	\brief Computes a target point for bot movement view based on the current movement state and goal.

	This function calculates a target point for the bot's view direction by traversing reachabilities from the bot's current position towards the goal. It considers various travel types and avoids
   certain paths like teleporters. The function uses a lookahead distance to determine how far to consider when computing the target. It returns true if a valid target is found, false otherwise.

	\param movestate Handle to the bot's movement state
	\param goal Pointer to the bot's goal structure
	\param travelflags Flags specifying travel constraints
	\param lookahead Maximum distance to look ahead when computing the target
	\param target Output vector to store the computed target point
	\return 1 if a valid target point is computed, 0 otherwise
*/
int	 BotMovementViewTarget( int movestate, bot_goal_t* goal, int travelflags, float lookahead, vec3_t target );

/*!
	\brief Predicts a visible position for a bot to target based on the origin and goal area.

	This function attempts to predict a position that is visible to the bot by tracing a path through navigation areas. It starts from the given origin and navigates through reachable areas until it
   either finds a visible point towards the goal or reaches the goal area itself. The function uses a maximum of 20 hops to avoid infinite loops. It considers travel flags to determine valid movement
   paths and returns the first visible point it finds. If no visible point is found within the limit, it returns false.

	\param origin The starting position from which to predict visibility
	\param areanum The current area number of the bot
	\param goal A pointer to the goal structure containing target information
	\param travelflags Flags that define valid travel methods for the bot
	\param target Output parameter that will contain the predicted visible position
	\return True if a visible position was predicted, false otherwise
*/
int	 BotPredictVisiblePosition( vec3_t origin, int areanum, bot_goal_t* goal, int travelflags, vec3_t target );

/*!
	\brief Allocates and returns a new movement state handle for bot AI.

	This function searches for the first available slot in the botmovestates array to allocate a new movement state. It iterates through indices from 1 to MAX_CLIENTS and returns the first index where
   botmovestates[i] is NULL. If no slot is available, it returns 0. The allocated movement state is initialized with cleared memory.

	\return A handle value representing the allocated movement state, or 0 if no movement state could be allocated.
*/
int	 BotAllocMoveState();

/*!
	\brief Frees the move state associated with the given handle.

	This function releases the memory allocated for a move state identified by the provided handle. It performs bounds checking to ensure the handle is valid and that the move state exists before
   attempting to free it. If the handle is out of range or the move state is invalid, appropriate error messages are printed and the function returns without taking action.

	\param handle The identifier for the move state to be freed
*/
void BotFreeMoveState( int handle );

/*!
	\brief Initializes the movement state for a bot entity using the provided initial movement parameters.

	This function sets up the initial state of a bot's movement by copying data from the provided initmove structure to the bot's movement state structure. It handles copying of origin, velocity, view
   offset, entity number, client index, think time, presence type, and view angles. Additionally, it processes movement flags, ensuring that specific flags such as MFL_ONGROUND, MFL_TELEPORTED,
   MFL_WATERJUMP, and MFL_WALK are properly set or cleared based on the input values.

	\param handle Identifier for the bot movement state to initialize
	\param initmove Structure containing the initial movement parameters for the bot
*/
void BotInitMoveState( int handle, bot_initmove_t* initmove );

/*!
	\brief Initializes brush model types for bot entities based on their BSP entity definitions.

	This function iterates through all BSP entities in the current map and processes those with model and classname attributes. It maps the model numbers from the BSP entities to specific model type
   constants, which are used by the bot AI system to determine how to interact with different brush models. The function must be called after loading a new map to ensure the bot library has accurate
   information about the brush model types present in the level.

*/
void BotSetBrushModelTypes();

/*!
	\brief Initializes the movement AI subsystem for bot clients.

	This function sets up the movement AI by configuring various game parameters related to bot movement. It calls BotSetBrushModelTypes to initialize brush model types, and then reads configuration
   values for maximum step height, maximum barrier height, and gravity from the library variables. These parameters define how bots navigate the game environment. The function returns an error code
   indicating the success or failure of the setup operation.

	\return Error code indicating the result of the initialization process, where BLERR_NOERROR indicates success
*/
int	 BotSetupMoveAI();

/*!
	\brief Shuts down the movement AI for all clients by freeing their allocated memory.

	This function iterates through all possible client indices from 1 to MAX_CLIENTS and checks if a movement state exists for each client. If a movement state is found, it frees the memory associated
   with that state and sets the pointer to NULL. This ensures that all resources allocated for movement AI are properly released when shutting down the AI subsystem.

*/
void BotShutdownMoveAI();

/*!
	\brief Initializes the avoid reachability data for a bot movement state.

	This function initializes the avoid reachability arrays for a bot movement state identified by the given handle. It clears the avoid reachability data structures, which are used to track areas
   that a bot should avoid reaching during navigation. The function first retrieves the movement state from the handle and verifies its validity before clearing the relevant memory areas.

	\param handle The handle identifying the bot movement state to initialize
*/
void BotInitAvoidReach( int handle );
// done.
