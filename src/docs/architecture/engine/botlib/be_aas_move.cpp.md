<!-- doxygenix: sha256=dd0e474f7f3b666c0deb6176e4ac70f5b0c3dc9634395acc09c140be2e18da43 -->

# engine/botlib/be_aas_move.cpp

## File Purpose
- This source file implements the core movement and physics logic used by the AAS (Area Awareness System) bot library
- It provides runtime functions for detecting environmental conditions (ladder, ground, water), applying fundamental physics (gravity, friction, acceleration), and predicting client movement over multiple frames
- Specialized calculations for rocket‑ and BFG‑jump mechanics are also included, enabling bot AI to reason about weapon‑powered traversal.

## Core Responsibilities
- Define and expose bot movement physics constants via aassettings.
- Provide utility checks for ladder proximity, ground status, and swimming presence.
- Convert direction angles into unit movement vectors, accounting for special up/down cases.
- Predict future positions of a client entity under given commands, handling collisions, steps, jumps, and environmental events.
- Compute per‑frame acceleration and friction for walking, crouching, air, and swimming.
- Calculate special jump Z‑velocity for rocket‑ and BFG‑powered jumps based on weapon radius and blast physics.
- Provide a test harness (AAS_TestMovementPrediction) for debugging movement prediction.

## Key Types and Functions
- aas_settings_t — global structure holding physics constants for bots
- aassettings — instance of aas_settings_t populated by AAS_InitSettings()
- AAS_InitSettings() — initializes default physics parameters used by all movement routines
- AAS_DropToFloor(vec3_t origin, vec3_t mins, vec3_t maxs) — projects origin to ground for dropped entities
- AAS_AgainstLadder(vec3_t origin, int ms_areanum) — tests if origin lies near a ladder face in the AAS world
- AAS_OnGround(vec3_t origin, int presencetype, int passent) — determines if a point is standing on a sufficiently flat surface
- AAS_Swimming(vec3_t origin) — checks if origin is inside lava, slime, or water contents
- AAS_SetMovedir(vec3_t angles, vec3_t movedir) — converts yaw‑pitch‑roll angles to a forward unit vector, with special handling for UP/DOWN
- AAS_JumpReachRunStart(aas_reachability_t* reach, vec3_t runstart) — predicts where a bot should start running to execute a jump reach
- AAS_WeaponJumpZVelocity(vec3_t origin, float radiusdamage) — calculates vertical jump velocity from weapon blast physics
- AAS_RocketJumpZVelocity(vec3_t origin) — wrapper for AAS_WeaponJumpZVelocity() with standard rocket radius
- AAS_BFGJumpZVelocity(vec3_t origin) — wrapper for AAS_WeaponJumpZVelocity() with standard BFG radius
- AAS_Accelerate(vec3_t velocity, float frametime, vec3_t wishdir, float wishspeed, float accel) — accelerates velocity toward desired direction
- AAS_AirControl(vec3_t start, vec3_t end, vec3_t velocity, vec3_t cmdmove) — currently unused placeholder for air control logic
- AAS_ApplyFriction(vec3_t vel, float friction, float stopspeed, float frametime) — applies horizontal friction to velocity
- struct aas_clientmove_s — prediction result container (contains endpos, velocity, trace, stopevent, etc.)
- AAS_PredictClientMovement(aas_clientmove_s* move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize) — simulates a bot's movement over frames, producing events and stopping conditions
- AAS_TestMovementPrediction(int entnum, vec3_t origin, vec3_t dir) — debug helper that predicts a short movement and prints leave‑ground events
- AAS_HorizontalVelocityForJump(float zvel, vec3_t start, vec3_t end, float* velocity) — computes horizontal speed necessary to reach a target given a vertical jump velocity

## Important Control Flow
- The file implements movement‑related helpers for the AAS bot library, focusing on physics constants, ground checks, ladder detection, movement prediction, and jump calculations.
- During initialization, AAS_InitSettings() populates a global aassettings struct with standard physics parameters used by all movement routine.
- AAS_PredictClientMovement() is the core simulation loop: for up to maxframes it calculates physics per frame, applies gravity, friction, acceleration (based on cmdmove, crouch, or swim state), updates velocity, and performs bounding‑box traces to resolve collisions.
- The simulation adjusts the velocity vector on each hit: it projects onto the plane of impact, handles stepping over 0‑height obstacles, flags ground contact, and triggers stop events such as SE_ENTERAREA, SE_HITGROUNDDAMAGE, SE_LEAVEGROUND, or SE_GAP.
- Helper utilities like AAS_AgainstLadder(), AAS_OnGround(), AAS_Swimming() use AAS_PointAreaNum() and AAS_Trace to determine environmental relationships.
- Functions AAS_WeaponJumpZVelocity(), AAS_RocketJumpZVelocity(), AAS_BFGJumpZVelocity() compute vertical components of rocket/BFG jumps by simulating blast impact and applying a simplified knock‑back model.
- Acceleration, friction, and movement vector calculations are handled by AAS_Accelerate(), AAS_ApplyFriction(), AAS_SetMovedir(), and AAS_JumpReachRunStart() to support run‑start calculations for jump reachabilities.

## External Dependencies
- "q_shared.h"
- "l_memory.h"
- "l_script.h"
- "l_precomp.h"
- "l_struct.h"
- "aasfile.h"
- "botshared/botlib.h"
- "botshared/be_aas.h"
- "be_aas_funcs.h"
- "be_aas_def.h"

## How It Fits
- Located in engine/botlib, the file forms part of the bot AI subsystem that consumes AAS navigation data to produce walkable paths
- Movement prediction influences pathfinding decisions, goal seeking, and reachability generation by determining whether a bot can traverse from one area to another, jump over obstacles, or avoid dangerous content
- The functions herein are called by higher‑level bot decision‑making code, but the primary logic here is independent of specific bot personalities, focusing on physics fidelity.
