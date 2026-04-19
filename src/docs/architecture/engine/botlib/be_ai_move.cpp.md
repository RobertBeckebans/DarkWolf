<!-- doxygenix: sha256=d369e8294a77b786fb881ecd6a2734f3a42085022432cace2ef9d67566a4b60f -->

# engine/botlib/be_ai_move.cpp

## File Purpose
- Implements the movement AI for Return to Castle Wolfenstein bots, translating AAS navigation data into EA commands.
- Provides per‑bot movement state management and goal‐oriented traversal logic.
- Handles all travel types supported by the engine (walk, crouch, jump, ladder, teleport, elevator, bobbing platforms, grappling, rocket jumps, water jumps).
- Implements obstacle detection, platform handling, and visibility prediction to enable realistic bot navigation.
- Forms the bridge between the bot planner (goal generation) and the engine physics (EA actions).

## Core Responsibilities
- Allocate, free, and validate per‑bot movement state handles via BotAllocMoveState, BotMoveStateFromHandle, and BotFreeMoveState.
- Initialize movement state data with BotInitMoveState and clear avoid lists with BotInitAvoidReach.
- Query AAS navigation to find reachable areas (BotFuzzyPointReachabilityArea, BotReachabilityArea) and next reachability links (BotGetReachabilityToGoal).
- Detect platform and ladder interactions: BotOnMover, MoverDown, BotOnTopOfEntity, AAS_AgainstLadder.
- Generate EA commands for every travel type through BotTravel_* and finish handlers (BotFinishTravel_*).
- Manage grapple hooks: GrappleState, BotResetGrapple, BotTravel_Grapple.
- Support visibility and distance checks: BotVisible, BotPredictVisiblePosition, BotGapDistance.
- Maintain avoidance lists and timing: BotAddToAvoidReach, BotResetAvoidReach, BotResetLastAvoidReach.
- Reset or clear entire movement state: BotResetMoveState.
- Boot and tear down the movement subsystem: BotSetupMoveAI, BotShutdownMoveAI.

## Key Types and Functions
- bot_movestate_t — per‑bot movement state holding position, flags, reachability stacks, and timestamps.
- BotAllocMoveState() — allocates and returns a new movement state handle.
- BotFreeMoveState(int handle) — frees the memory for a movement state.
- BotMoveStateFromHandle(int handle) — validates and returns a bot_movestate_t pointer.
- BotInitMoveState(int handle, bot_initmove_t* init) — copies initial movement data into the state.
- BotFuzzyPointReachabilityArea(vec3_t origin) — heuristic lookup of the closest reachable area for an arbitrary point.
- BotReachabilityArea(vec3_t origin, int client) — obtains the navigation area the bot should occupy at a given position.
- BotOnMover(vec3_t origin, int entnum, aas_reachability_t* reach) — checks if an entity is standing on a moving platform.
- BotSetBrushModelTypes() — parses the current map’s BSP entities and records model types (func_plat, func_bobbing).
- BotCheckBlocked(bot_movestate_t* ms, vec3_t dir, int checkbottom, bot_moveresult_t* result) — traces for solid blockage in the desired direction.
- BotAddToAvoidReach(bot_movestate_t* ms, int number, float avoidtime) — registers a reachability to avoid temporarily.
- BotGetReachabilityToGoal(...) — selects the best next reachability from the current area toward a goal, respecting avoid lists and travel flags.
- BotMovementViewTarget(int movestate, bot_goal_t* goal, int travelflags, float lookahead, vec3_t target) — computes a view‑target point along the path.
- BotVisible(int ent, vec3_t eye, vec3_t target) — checks line‑of‑sight while ignoring a specific entity.
- BotPredictVisiblePosition(vec3_t origin, int areanum, bot_goal_t* goal, int travelflags, vec3_t target) — traces along a route to find a visible goal point.
- BotMoveInDirection(int movestate, vec3_t dir, float speed, int type) — chooses swim or walk movement based on bot’s state.
- BotMoveInGoalArea(bot_movestate_t* ms, bot_goal_t* goal) — walks or swims directly toward a goal area.
- BotMoveToGoal(bot_moveresult_t* result, int movestate, bot_goal_t* goal, int travelflags) — high‑level routine that invokes travel handlers.
- BotTravel_Walk(...), BotTravel_Crouch(...), BotTravel_Jump(...), BotTravel_Ladder(...), BotTravel_Elevator(...), BotTravel_Grapple(...), BotTravel_RocketJump(...), BotTravel_JumpPad(...), BotTravel_FuncBobbing(...) — travel‑type handlers implementing EA logic for each movement mode.
- BotFinishTravel_Walk(...), BotFinishTravel_Jump(...), BotFinishTravel_Elevator(...), BotFinishTravel_Grapple(...), BotFinishTravel_WeaponJump(...), BotFinishTravel_JumpPad(...), BotFinishTravel_FuncBobbing(...) — final steps for multi‑frame travel types.
- BotReachabilityTime(aas_reachability_t* reach) — returns the timeout (in ticks) for a specific travel type.
- BotSetupMoveAI() — reads world limits (sv_step, sv_maxbarrier, sv_gravity) and calls BotSetBrushModelTypes.
- BotShutdownMoveAI() — frees all allocated movement states.
- GrappleState(bot_movestate_t* ms, aas_reachability_t* reach) — inspects the world to determine if a grapple hook is still flying or has hooked a surface.
- BotResetGrapple(bot_movestate_t* ms) — turns off an active grapple when no longer needed.
- AngleDiff(float ang1, float ang2) — returns the signed difference between two angles in degrees.
- BotClearMoveResult(bot_moveresult_t* result) — clears all flags and fields of a moveresult.
- BotCheckBlocked(...), BotClearMoveResult(...), BotTravel_* and BotFinishTravel_* — together generate EA commands, handle blocking, and maintain movement state.

## Important Control Flow
- BotMoveToGoal is triggered each think cycle and drives a bot from its current state toward a goal.
- Inside BotMoveToGoal the bot’s state is loaded, grapple flags reset, and movement flags updated (ground, swimming, ladder).
- The current AAS area is determined; if on an elevator or bobbing platform it is handled specially.
- If the bot has a cached reachability that is still viable it is reused; otherwise BotGetReachabilityToGoal finds a new link toward the goal.
- The chosen travel type (walk, jump, ladder, etc.) selects a BotTravel_* routine.
- Each BotTravel_* routine clears a result, calculates a direction, checks for blockages, issues EA primitive commands, and sets view angles or weapons as needed.
- The resulting bot_moveresult_t is returned to update the entity’s actions for the next simulation step.
- On initialization BotSetupMoveAI loads world limits and populates brush model types; on shutdown BotShutdownMoveAI frees all movement states.

## External Dependencies
- "q_shared.h"
- "l_memory.h"
- "l_libvar.h"
- "l_utils.h"
- "l_script.h"
- "l_precomp.h"
- "l_struct.h"
- "aasfile.h"
- "botshared/botlib.h"
- "botshared/be_aas.h"
- "be_aas_funcs.h"
- "be_interface.h"
- "botshared/be_ea.h"
- "botshared/be_ai_goal.h"
- "botshared/be_ai_move.h"

## How It Fits
- Core module of the bot library’s movement stack, invoked each AI think frame.
- Takes the planner’s goal (bot_goal_t) and uses AAS to compute reachability links.
- Issues EA primitives to influence the bot client’s physical simulator.
- Interacts with other bot subsystems (goal, think, sight) and the engine’s AAS system.
- Provides a public API (BotMoveToGoal, BotSetupMoveAI, BotShutdownMoveAI) used by higher‑level AI code.
