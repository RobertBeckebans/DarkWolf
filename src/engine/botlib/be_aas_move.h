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
	\file engine/botlib/be_aas_move.h
	\brief Header for the AAS (Area Awareness System) movement prediction and helper utilities.
	\note doxygenix: sha256=076a2be3826a8f47d48163377c694c8f850669d363f80042674acc02ed4630c8

	\par File Purpose
	- Header for the AAS (Area Awareness System) movement prediction and helper utilities.
	- Declares movement physics functions used by bots during pathfinding and action planning.
	- Exposes global settings struct for configuring bot movement behavior.

	\par Core Responsibilities
	- Establish foundational physics parameters for bot navigation.
	- Provide collision-aware movement prediction over multiple simulated frames.
	- Determine spatial relationships such as ground contact, ladder proximity, and swimming state.
	- Compute weapon-powered jump velocities tailored to specific weapon damage radii.
	- Assist in planning safe start points for jump reachabilities.
	- Ensure that movement vectors are properly defined for both specialized and general directions.
	- Drop an entity location to the nearest floor surface in a collision-respecting manner.

	\par Key Types and Functions
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

	\par Control Flow
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

	\par Dependencies
	- be_aas.h
	- be_aas_reach.h
	- q_shared.h
	- vec3_t definition in common types

	\par How It Fits
	- Part of the AAS subsystem which handles navigation mesh traversal.
	- Serves as the physics simulation core that bots query to test reachabilities and plan actions.
	- Provides both predictive and analytical tools that other AAS modules (reachability, navigation, combat) call during decision making.
*/

/*****************************************************************************
 * name:		be_aas_move.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#ifdef AASINTERN
extern aas_settings_t aassettings;
#endif // AASINTERN

/*!
	\brief Predicts a client's movement over a series of frames based on physics and input parameters.

	This function simulates the movement of a client entity in the game world over a specified number of frames. It takes into account various physics parameters such as gravity, friction,
   acceleration, and velocity. The simulation considers factors like whether the entity is on the ground, swimming, or crouching, and applies appropriate physics modifications. The function can also
   predict movement until certain events occur, such as entering a specific area, hitting the ground, or entering water. The result is stored in the provided move structure, which includes the final
   position, velocity, and other relevant state information.

	\param cmdframes Number of frames to apply command movement
	\param cmdmove Command movement vector
	\param entnum Entity number of the client
	\param frametime Time increment for each frame
	\param maxframes Maximum number of frames to predict ahead
	\param move Structure to store the movement prediction result
	\param onground Whether the client is on the ground
	\param origin Starting position of the client
	\param presencetype Type of presence (normal, crouch, etc.)
	\param stopareanum Area number to stop prediction when entered
	\param stopevent Event that stops the prediction
	\param velocity Current velocity of the client
	\param visualize Whether to visualize the prediction process
	\return Returns 1 if the prediction stops due to a specified event, 0 otherwise
*/
int	  AAS_PredictClientMovement( struct aas_clientmove_s* move,
	  int												  entnum,
	  vec3_t											  origin,
	  int												  presencetype,
	  int												  onground,
	  vec3_t											  velocity,
	  vec3_t											  cmdmove,
	  int												  cmdframes,
	  int												  maxframes,
	  float												  frametime,
	  int												  stopevent,
	  int												  stopareanum,
	  int												  visualize );

/*!
	\brief Checks whether a point in space is on a solid surface that would support a bot.

	A downward trace is performed from the supplied origin ten units below to determine if there is a collision plane that is sufficiently steep to act as ground. If the trace starts inside a solid
   volume, or the trace ends within ten units of the origin on a plane whose normal is steep enough (dot product with the unit up vector is at least the maximum steepness setting), the function
   reports that the point is on the ground. Otherwise it reports otherwise. The function does not consider empty space or overly steep surfaces as ground.

	\param origin the XYZ coordinates at which to test for ground contact
	\param presencetype collision mask indicating which types of surfaces to trace against
	\param passent entity number to ignore during the trace
	\return qtrue (non‑zero) if the origin is determined to be on solid ground, qfalse (zero) otherwise
*/
int	  AAS_OnGround( vec3_t origin, int presencetype, int passent );

/*!
	\brief Determines whether the given origin is within a swimming volume.

	The function copies the supplied origin into a temporary vector, subtracts 2 from the z coordinate, and then checks the contents at that point. If the contents indicate lava, slime, or water, the
   function returns qtrue indicating that the entity would be swimming; otherwise it returns qfalse.

	\param origin The point to test for swimming eligibility.
	\return qtrue (a non‑zero integer) if the point is within swimming contents; qfalse (zero) otherwise.
*/
int	  AAS_Swimming( vec3_t origin );

/*!
	\brief Determines the position from which a client should start running to carry out a jump reachability.

	The function computes a horizontal direction to the jump destination, offsets the start point upward, and then predicts client movement over a short command to locate a safe running start
   position. It copies the predicted end position back into the runstart output. If the simulation indicates that executing the move would cause the client to enter lava or take ground damage, the
   original start point is restored as the safe run start. The calculation uses a fixed horizontal speed and several environmental stopping event flags to ensure viable and safe movement.

	\param reach pointer to an aas_reachability_s structure that contains the jump's start and end positions.
	\param runstart output array that receives the calculated run start point; it is modified by the function.
*/
void  AAS_JumpReachRunStart( struct aas_reachability_s* reach, vec3_t runstart );

/*!
	\brief Determines whether a point lies on or very close to a ladder in the navigation mesh.

	The function first locates the area containing the given point. If the location is not inside any area, the supplied ms_areanum is used as a fallback. It then checks that the area is marked as a
   ladder area and that normal presence (normal entities can enter) is allowed. For each face of the area that is flagged as a ladder, the function calculates the distance from the point to the face's
   plane; if that distance is less than 3 units and the point projects inside the face polygon, the point is considered to be against a ladder. If any ladder face satisfies these conditions, the
   function returns true. Otherwise, false is returned.

	\param origin 3D world coordinate to test against ladder proximity
	\param ms_areanum Area number to use if the origin is not inside any recognised area
	\return 1 if the origin is against a ladder, 0 otherwise
*/
int	  AAS_AgainstLadder( vec3_t origin, int ms_areanum );

/*!
	\brief Returns the vertical velocity required to perform a rocket jump from the given origin point.

	The function determines how high to launch a client when using a rocket for a jump. It calls the helper AAS_WeaponJumpZVelocity with a damage radius of 120, which is the radius for the standard
   rocket launcher. The returned float represents the Z component of the velocity vector that should be applied to the movement command.

	The origin parameter is the world space position at which the rocket is fired, and the function produces the velocity independent of horizontal direction, which is handled elsewhere.

	No exceptions are thrown by this function.

	\param origin the world space position at which the rocket jump is performed
	\return the Z velocity to use for the rocket jump
*/
float AAS_RocketJumpZVelocity( vec3_t origin );

/*!
	\brief Computes the vertical velocity required for a BFG jump starting from the given origin.

	The BFG jump uses a damage radius of 1000. This function delegates the calculation to AAS_WeaponJumpZVelocity with a radius of 120, returning the Z component of the resulting jump velocity. It is
   used in reachability calculations for weapon-powered jumps.


	\param origin The world-space coordinates of the jump start location.
	\return A floating‑point value representing the Z‑axis velocity needed for the jump.
*/
float AAS_BFGJumpZVelocity( vec3_t origin );

/*!
	\brief Computes the horizontal speed required to reach from a start position to an end position using a given vertical velocity and indicates whether the result is achievable within the movement
   constraints.

	The function first calculates the maximum height attainable from the provided vertical velocity using the current gravity setting. It then determines if the target end height is reachable; if not,
   it assigns the maximum allowed horizontal velocity and signals failure. Otherwise, it calculates the time needed to fall the vertical difference and the horizontal displacement vector. The required
   horizontal speed is derived from the horizontal distance and total falling time, adjusted by the initial vertical velocity. If this speed exceeds the global maximum speed setting, the function caps
   it to that maximum and reports failure. A success result is indicated by returning 1, and the computed speed is stored through the supplied pointer.

	\param zvel initial vertical velocity used for the jump
	\param start starting position vector
	\param end target position vector
	\param velocity pointer to store the computed horizontal speed
	\return 1 if the computed horizontal speed is less than or equal to the maximum allowed speed, otherwise 0.
*/
int	  AAS_HorizontalVelocityForJump( float zvel, vec3_t start, vec3_t end, float* velocity );

/*!
	\brief Calculates and assigns a movement direction vector from a given angle vector, handling upward and downward special cases.

	VectorCompare checks if the angle vector is identical to the predefined VEC_UP or VEC_DOWN. When the angle is VEC_UP, the output vector is set to the constant MOVEDIR_UP. If the angle is VEC_DOWN,
   the output vector is set to MOVEDIR_DOWN. For any other direction, AngleVectors converts the Euler angles into a forward direction vector, which is then copied into the movedir parameter. The
   resulting vector is normalized as part of AngleVectors, ensuring a unit length.

	\param angles Euler angles describing the desired movement direction
	\param movedir Output vector that will receive the computed direction; must be allocated by the caller
*/
void  AAS_SetMovedir( vec3_t angles, vec3_t movedir );

/*!
	\brief Drops a point downwards until it contacts the floor while respecting the entity's bounding box and updates the origin.

	The function performs a downward trace starting from the given origin and 100 units below. If the trace starts inside solid geometry, the function returns false. Otherwise, it copies the trace's
   ending position to the origin parameter and returns true, indicating the point was successfully dropped to the floor.

	\param origin The entity's position; this parameter is updated to the lowest floor point reached by the trace.
	\param mins The minimum extents of the entity's bounding box used for collision tracing.
	\param maxs The maximum extents of the entity's bounding box used for collision tracing.
	\return Integer value: 1 if the point was dropped to the floor, 0 if the original position was inside solid geometry.
*/
int	  AAS_DropToFloor( vec3_t origin, vec3_t mins, vec3_t maxs );

/*!
	\brief Initializes default physics settings for AI movement.

	Sets a series of physical environment parameters that influence movement, such as friction, gravity, maximum velocities, accelerations, step height, steepness tolerance, and jump characteristics.
   These values are stored in the global aassettings structure. The maximum barrier height is set to a fixed value of 49, even though there is a commented formula that would compute it from gravity
   and jump velocity.

*/
void  AAS_InitSettings();
