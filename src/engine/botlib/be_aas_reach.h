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
 * name:		be_aas_reach.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#ifdef AASINTERN
// initialize calculating the reachabilities
void AAS_InitReachability();
// continue calculating the reachabilities
int	 AAS_ContinueInitReachability( float time );
//
int	 AAS_BestReachableLinkArea( aas_link_t* areas );
#endif // AASINTERN

/*!
	\brief Returns the number of reachable areas for the specified area, or zero if the area is out of range or disabled.

	If the area number is outside the valid range or the area is flagged as disabled, the function logs an error and returns zero. Otherwise, it returns the stored count of reachable areas for that
   area.

	\param areanum Index of the area whose reachability count is requested.
	\return Number of reachable areas for the specified area; zero indicates no reachabilities or an invalid/disabled area.
*/
int	  AAS_AreaReachability( int areanum );

/*!
	\brief Finds a reachable AAS area for a bounding box placed at a given origin and sets a goal origin within that area.

	The function attempts to locate an AAS area that can be reached by a client bounding box positioned at the supplied origin. It first tries to find a valid point directly in an area; if none is
   found it offsets the origin slightly in a neighbourhood and retries. Once an area is found, it traces a crouch bounding box downwards to locate the ground area and uses the trace end position as
   the goal origin. If the initial search fails, it links an invisible entity representing the bounding box and obtains the best reachable link area; this area number is returned. The provided
   goalorigin is updated to a position inside the chosen area. If the AAS world is not loaded or no area can be found, 0 is returned and an error message is printed.

	\param origin the base position for the bounding box; the origin point of the entity to be placed
	\param mins the minimum extents of the bounding box relative to the origin
	\param maxs the maximum extents of the bounding box relative to the origin
	\param goalorigin output variable that will receive a point inside the reachable area
	\return the area number of a reachable AAS area; 0 if no such area can be found
*/
int	  AAS_BestReachableArea( vec3_t origin, vec3_t mins, vec3_t maxs, vec3_t goalorigin );

/*!
	\brief Finds the next reachability entry associated with a given model index.

	The function iterates over the global reachability array of the current AAS world, starting from the supplied index (or the next one if the supplied index is in bounds). It searches for a
   reachability whose travel type matches either an elevator or a bobbing platform and whose face number refers to the supplied model index: for elevators the face number must equal the model index;
   for bobbing platforms the lower 16 bits of the face number must equal the model index. If a matching entry is found, its array index is returned; otherwise the function returns zero. An index of
   zero is also returned if the supplied index lies beyond the end of the reachability list.

	If the supplied index is less than or equal to zero, the search begins at index one. If the supplied index is greater than or equal to the reachability size, the function immediately returns zero,
   indicating that no further entries exist.

	The function has no side effects other than reading the global `aasworld` state.

	\param num The current reachability index; zero indicates a start search, values larger than zero specify the last found index to continue from; values equal to or greater than the reachability
   size terminate the search immediately. \param modelnum The model index to match against reachability entries. \return An integer index of the next matching reachability entry; zero if none is
   found.
*/
int	  AAS_NextModelReachability( int num, int modelnum );

/*!
	\brief Computes the total area of all ground faces within the specified area

	The function accesses the global AAS world data to locate the area with the supplied index. It iterates over each face referenced by that area, retrieves the face structure, and checks the
   FACE_GROUND flag. Only faces marked as ground contribute to the sum. The area for each qualifying face is obtained via the AAS_FaceArea routine and accumulated. The result is returned as a float.
   Negative face indices are converted to positive with abs – the meaning of a negative index is not specified in the source. No other state is modified.

	\param areanum Index of the area whose ground face area is being calculated
	\return Total area of ground faces, or zero if the area has no ground faces
*/
float AAS_AreaGroundFaceArea( int areanum );

/*!
	\brief Determines if the supplied area number designates a crouch‑only region.

	The function checks the area's presence type. If the area lacks a normal presence flag, it is considered crouch‑only. This value is used when constructing reachabilities to adjust travel times for
   entering crouch areas.

	\param areanum a numeric identifier of the area to examine
	\return non‑zero if the area is crouch‑only; zero otherwise
*/
int	  AAS_AreaCrouch( int areanum );

/*!
	\brief Returns true if the specified area contains liquid and can be swimmable.

	AAS_AreaSwim queries the area settings of the given area number in the global AAS world data. If the AREA_LIQUID flag is present, the function returns qtrue (non-zero); otherwise it returns qfalse
   (zero). The result is used by other reachability logic to identify swim‑capable areas.

	\param areanum The numeric identifier of the area to test for swim capability.
	\return An integer, non‑zero (qtrue) if the area is suitable for swimming, otherwise zero (qfalse).
*/
int	  AAS_AreaSwim( int areanum );

/*!
	\brief Checks whether the specified AAS area is flooded with liquid

	The function examines the area settings of the given area number in the current AAS world. If the AREA_LIQUID flag is present in the areaflags of that area, it returns the constant qtrue;
   otherwise it returns qfalse. The return type is an int used as a boolean value.

	\param areanum index of the area within the AAS world
	\return an integer that is qtrue if the area contains liquid, otherwise qfalse
*/
int	  AAS_AreaLiquid( int areanum );

/*!
	\brief Returns nonzero if the specified area contains lava.

	Queries the area settings bitmask for the lava flag. If the AREACONTENTS_LAVA bit is set in the area’s contents, a nonzero value is returned.

	\param areanum Index of the area being queried.
	\return Nonzero integer if the area contains lava, zero otherwise.
*/
int	  AAS_AreaLava( int areanum );

/*!
	\brief Checks if the specified navigation area contains slime content.

	The AAS world stores settings for each area, including content flags. This function accesses the AREACONTENTS_SLIME flag in the contents field of the area’s settings and performs a bitwise AND. A
   non‑zero result indicates that the area contains slime content; zero means it does not.

	\param areanum The numeric identifier of the area within the navigation mesh.
	\return Non‑zero if the area’s contents include slime; otherwise zero.
*/
int	  AAS_AreaSlime( int areanum );

/*!
	\brief Checks whether a specified area has at least one ground face.

	The function examines the area flags in the AAS world data structure. It returns a non‑zero value if the AREA_GROUNDED bit is set, indicating that the area contains one or more ground faces, and
   zero otherwise. The return value should be interpreted as a Boolean, with non‑zero meaning true. The function assumes that areanum is a valid area index; passing an out‑of‑range value may result in
   undefined behavior.

	\param areanum the numeric index of the area to examine
	\return An integer that is non‑zero if the area has a ground face, zero otherwise.
*/
int	  AAS_AreaGrounded( int areanum );

/*!
	\brief Checks if a specified area contains ladder geometry

	The function reads the area flags for the given area number from the global world structure and tests against the AREA_LADDER flag. A non‑zero result indicates that the area has one or more faces
   that allow ladder traversal; a zero return means no ladder faces are present.

	\param areanum The numeric identifier of the area to inspect
	\return A non‑zero integer when the AREA_LADDER flag is present; zero otherwise
*/
int	  AAS_AreaLadder( int areanum );

/*!
	\brief Determines whether a specified area contains a jump pad.

	Checks the contents bits of the supplied area for the AREACONTENTS_JUMPPAD flag and returns the flag value. A non‑zero result indicates the area is a jump pad; zero indicates it is not. The area
   number should reference a valid area within the current world settings.

	\param areanum Index of the area to test
	\return Non‑zero if the area contains a jump pad, zero otherwise.
*/
int	  AAS_AreaJumpPad( int areanum );

/*!
	\brief Determines whether a given area is flagged as no-enter.

	Checks the area's contents bitmask for the AREACONTENTS_DONOTENTER flag.

	\param areanum The numeric identifier of the area to query.
	\return Non‑zero if the area is marked as no-enter, otherwise zero.
*/
int	  AAS_AreaDoNotEnter( int areanum );

/*!
	\brief Checks whether a specified area has the donotenter large flag set.

	The function accesses the global AAS world structure, retrieves the area settings for the given area number, and performs a bitwise AND between the area’s contents field and the
   AREACONTENTS_DONOTENTER_LARGE flag mask. It returns the result of this operation, so a non‑zero value indicates that the flag is present. The behavior is undefined if the area number is not within
   the bounds of the area settings array.

	\param areanum Index of the area to test.
	\return An integer where any non‑zero value indicates that the area is marked with the donotenter large flag.
*/
int	  AAS_AreaDoNotEnterLarge( int areanum );
