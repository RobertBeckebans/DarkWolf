<!-- doxygenix: sha256=076a2be3826a8f47d48163377c694c8f850669d363f80042674acc02ed4630c8 -->

# engine/botlib/be_aas_move.h

## File Purpose
- Header for the AAS (Area Awareness System) movement prediction and helper utilities.
- Declares movement physics functions used by bots during pathfinding and action planning.
- Exposes global settings struct for configuring bot movement behavior.

## Core Responsibilities
- Establish foundational physics parameters for bot navigation.
- Provide collision-aware movement prediction over multiple simulated frames.
- Determine spatial relationships such as ground contact, ladder proximity, and swimming state.
- Compute weapon-powered jump velocities tailored to specific weapon damage radii.
- Assist in planning safe start points for jump reachabilities.
- Ensure that movement vectors are properly defined for both specialized and general directions.
- Drop an entity location to the nearest floor surface in a collision-respecting manner.

## Key Types and Functions
- aas_clientmove_s — structure capturing per-frame movement data for prediction.
- aas_reachability_s — structure describing a navigation reachability (e.g., jump).
- aas_settings_t — global configuration struct holding physics constants.
- AAS_PredictClientMovement(struct aas_clientmove_s* move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize) — simulates movement over multiple frames, storing result in move and returning 1 if stopped by event.
- AAS_OnGround(vec3_t origin, int presencetype, int passent) — returns non‑zero if origin is considered standing on solid ground based on a downward trace.
- AAS_Swimming(vec3_t origin) — returns non‑zero if the point is within lava, slime, or water volumes.
- AAS_JumpReachRunStart(struct aas_reachability_s* reach, vec3_t runstart) — calculates a safe running start point for a jump given the reachability description.
- AAS_AgainstLadder(vec3_t origin, int ms_areanum) — determines if a point lies on or very near a ladder area.
- AAS_RocketJumpZVelocity(vec3_t origin) — returns the Z velocity needed for a rocket-powered jump.
- AAS_BFGJumpZVelocity(vec3_t origin) — returns the Z velocity for a BFG-powered jump.
- AAS_HorizontalVelocityForJump(float zvel, vec3_t start, vec3_t end, float* velocity) — computes the required horizontal speed to reach from start to end using a given vertical jump velocity, returning success flag and speed via pointer.
- AAS_SetMovedir(vec3_t angles, vec3_t movedir) — converts Euler angles into a movement direction vector, handling special up/down cases.
- AAS_DropToFloor(vec3_t origin, vec3_t mins, vec3_t maxs) — moves an origin downwards to the nearest floor while respecting entity bounding box, returning success flag.
- AAS_InitSettings() — populates the global aassettings structure with default physics constants for AI navigation.

## Important Control Flow
- AAS_PredictClientMovement takes the current entity state and command vector, then loops over each frame, updating velocity, position, and collision state until a stopping criterion is met or the maximum frame count is reached.
- AAS_OnGround performs a downward trace of 10 units from the supplied origin, using the current presencetype mask, and sets the ground flag based on the trace result.
- AAS_Swimming adjusts the origin's Z coordinate by -2, queries the world contents, and returns true for lava, slime, or water brushes.
- AAS_JumpReachRunStart computes a horizontal vector toward the jump destination, offsets the origin upward, and invokes movement prediction to find a safe running start position, falling back to the original if hazardous.
- AAS_AgainstLadder locates the area containing the point (or falls back to ms_areanum), checks ladder flags on area faces, and determines if the point lies within 3 units of a ladder plane.
- AAS_RocketJumpZVelocity calls the internal AAS_WeaponJumpZVelocity helper with a radius of 120 to compute the Z component of the required launch velocity.
- AAS_BFGJumpZVelocity also uses AAS_WeaponJumpZVelocity but with radius 1000 to obtain the BFG jump Z velocity.
- AAS_HorizontalVelocityForJump computes the maximum height from a vertical velocity, checks the height reachability of the target, calculates necessary horizontal speed and caps it by the global max speed, conveying success or failure.
- AAS_SetMovedir interprets special up/down angles into fixed direction vectors or converts other Euler angles into a forward vector via AngleVectors.
- AAS_DropToFloor starts a downward trace from the origin, updating origin to the trace endpoint if clear, and returns a flag indicating success.
- AAS_InitSettings initializes a global aas_settings_t structure with default physics parameters for AI movements such as friction, gravity, and jump characteristics.

## External Dependencies
- be_aas.h
- be_aas_reach.h
- q_shared.h
- vec3_t definition in common types

## How It Fits
- Part of the AAS subsystem which handles navigation mesh traversal.
- Serves as the physics simulation core that bots query to test reachabilities and plan actions.
- Provides both predictive and analytical tools that other AAS modules (reachability, navigation, combat) call during decision making.
