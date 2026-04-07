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
 * name:		be_aas_debug.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

/*!
	\brief Clears all displayed debug lines by deleting them and marking them invisible.

	Iterates over the array of debug lines up to MAX_DEBUGLINES. For each line that is currently active, it calls botimport.DebugLineDelete to remove the visual element, sets the debuglines entry to
   0, and marks the line as not visible in debuglinevisible.

*/
void AAS_ClearShownDebugLines();

/*!
	\brief Clears all currently shown debug polygons by deleting them and resetting the array.

	Iterates over the debug polygon array up to MAX_DEBUGPOLYGONS. For each non‑null entry it calls botimport.DebugPolygonDelete to remove the visual representation. After deletion the entry is set to
   zero. A commented-out alternative implementation that deletes by index is present but inactive.

*/
void AAS_ClearShownPolygons();

/*!
	\brief Displays a debug line between two points in a specified color.

	The function searches for a free debug line slot up to MAX_DEBUGLINES. When a free slot is found, it creates a new line if necessary and makes it visible by calling botimport.DebugLineShow. Once
   the line is shown, the function returns. If no free slots are available the function simply exits without creating or showing a line. Existing visible lines are left unchanged.

	\param start 3‑D vector specifying the line start point
	\param end 3‑D vector specifying the line end point
	\param color integer identifying the visual color
*/
void AAS_DebugLine( vec3_t start, vec3_t end, int color );

/*!
	\brief Creates a permanent debug line between start and end points with the specified color.

	The function obtains a new debug line handle via botimport, then configures that line to draw between the given start and end coordinates using the supplied color. The line remains visible in the
   debugging view until it is explicitly removed.

	\param start The starting coordinate of the line.
	\param end The ending coordinate of the line.
	\param color The color code to use when rendering the line.
*/
void AAS_PermanentLine( vec3_t start, vec3_t end, int color );

/*!
	\brief Renders a permanent cross centered at a given origin with the specified size and color

	For each of the three axes, a line is drawn from the origin offset by the size in the positive direction to the origin offset by the size in the negative direction. Both AAS_DebugLine and the
   botimport debug line interface are used to create persistent debug lines, allowing the cross to remain visible for debugging purposes.

	\param origin the 3‑D point at which the cross is centered
	\param size half the length of each arm of the cross
	\param color the color index used for the debug lines
*/
void AAS_DrawPermanentCross( vec3_t origin, float size, int color );

/*!
	\brief Draws a debugging cross in a plane centered on a specified point

	This routine generates two perpendicular line segments lying in the plane described by a normal vector and distance from the origin. By selecting a component of the normal via the plane type, the
   function computes two orthogonal axes, forms segments that extend 12 units across each axis, and then projects their intersection points onto the plane. Debug lines are created or reused to display
   these segments with the given color.

	\param point the point at which the cross is centered
	\param normal the normal of the plane in which the cross lies
	\param dist distance from the origin to the plane along the normal
	\param type encodes which plane component aligns with the normal to determine orthogonal axes
	\param color color identifier for the line rendering
*/
void AAS_DrawPlaneCross( vec3_t point, vec3_t normal, float dist, int type, int color );

/*!
	\brief Displays a 3D bounding box at the specified origin using the given min and max extents.

	The function computes the eight corners of the bounding box by adding the min and max vectors to the origin. It then copies the upper corner positions to create the lower corners, adjusting the z
   coordinate with the minimum extent. Debug lines are reused or created as needed, and the top, bottom, and vertical edges of the box are drawn with a red color for visual debugging.

	\param maxs Maximum extent from the origin defining the upper corner.
	\param mins Minimum extent from the origin defining the lower corner.
	\param origin Center point of the bounding box in world coordinates.
*/
void AAS_ShowBoundingBox( vec3_t origin, vec3_t mins, vec3_t maxs );

/*!
	\brief Visualises the edges and normal of a specified face in the AAS world

	The function expects a face index in the range of the AAS world faces. It retrieves the face data and iterates through each of its edges, drawing debug lines between the edge vertices. The line
   colors cycle through red, green, blue, and yellow for each edge. After processing the edges, it draws a short line along the face's normal from the first vertex to help determine orientation. If
   the provided face or any edge index is out of bounds, an error message is printed via the bot import system.

	\param facenum the index of the face to display, relative to the AAS world's face array
*/
void AAS_ShowFace( int facenum );

/*!
	\brief Shows a convex navigation area by drawing its edges as debug lines.

	The function first ensures the requested area index is valid. It then iterates over each face that belongs to the area, optionally skipping any face that is not marked as ground or ladder when the
   groundfacesonly flag is set. For every included face it collects all unique edges, avoiding duplicates. After gathering the edges, the function creates or reuses debug line objects up to a maximum
   limit, cycles through a sequence of predefined colors, and displays each edge between its two vertices. If the number of available debug lines is exceeded, the drawing stops. Error messages are
   printed for any out‑of‑range area, face, or edge indices, but no exception is thrown.

	\param areanum index of the area to display
	\param groundfacesonly flag to include only ground or ladder faces (non‑zero)
*/
void AAS_ShowArea( int areanum, int groundfacesonly );

/*!
	\brief Displays the polygons of faces belonging to a specified area, optionally filtering to ground or ladder faces

	Checks the supplied area index against the world data, printing an error if it is out of bounds. Then iterates over each face belonging to that area by using the area's face count and offset. For
   each face it resolves the absolute face index, validates it, and optionally skips non‑ground and non‑ladder faces when the third argument is true. The polygon of each selected face is rendered via
   AAS_ShowFacePolygon, using the supplied color and indicating if the face faces away from the current area.

	\param areanum index of the area whose faces are to be shown
	\param color color identifier used when rendering the polygons
	\param groundfacesonly nonzero to display only faces that are flagged as ground or ladder, zero to display all faces
*/
void AAS_ShowAreaPolygons( int areanum, int color, int groundfacesonly );

/*!
	\brief Draws a cross-shaped marker at the specified origin with the given size and color.

	The function generates three line segments extending the specified size from the origin along each coordinate axis. For each axis, a start point is created by adding the size to the origin
   component, and an end point by subtracting the size. The AAS_DebugLine routine is then called to render each segment in the specified color. This results in a 3‑dimensional cross centered at the
   origin.

	\param origin The center point of the cross as a 3‑element vector.
	\param size Half the length of each arm of the cross.
	\param color Identifier for the color used when drawing the lines.
*/
void AAS_DrawCross( vec3_t origin, float size, int color );

/*!
	\brief Prints a human readable description of a travel type value for debugging purposes.

	The function translates an integer travel type constant into its string name within a DEBUG build. It maps many specific constants such as TRAVEL_WALK, TRAVEL_JUMP, TRAVEL_TELEPORT, etc., to a
   readable string and sends the result to the bot logging system. In a non‑DEBUG build the function contains no executable code, effectively becoming a no‑op.

	\param traveltype an integer representing a travel type constant, e.g. TRAVEL_WALK, TRAVEL_JUMP, or other defined travel types.
*/
void AAS_PrintTravelType( int traveltype );

/*!
	\brief Draws a 3D arrow from a start point to an end point using specified colors.

	The function computes the direction vector from start to end and normalizes it. It then determines a perpendicular vector; if the direction is nearly aligned with the global up axis, a fixed
   perpendicular is used, otherwise the cross product with the up vector provides the perpendicular.

	Two base points for the arrowhead are generated by moving back slightly along the direction and offsetting along the perpendicular. Lines are drawn from the start to end for the shaft, and from
   the two base points to end for the head.

	The linecolor parameter controls the color of the shaft; arrowcolor controls the color of both head lines.

	\param start the starting point of the arrow.
	\param end the tip point of the arrow.
	\param linecolor the color value for the main shaft line.
	\param arrowcolor the color value for the two head lines.
*/
void AAS_DrawArrow( vec3_t start, vec3_t end, int linecolor, int arrowcolor );

/*!
	\brief Visualizes a reachability for debugging purposes by drawing area polygons, arrows, and simulating client movement.

	The function first displays the polygons of the area associated with the reachability using AAS_ShowAreaPolygons. It then draws an arrow from the start to the end point to show the direction of
   travel.

	Based on the travel type of the reachability, the function computes the appropriate horizontal and vertical velocities, sets up a command movement vector, and predicts the client’s movement with
   AAS_PredictClientMovement. For normal jumps and walk-offs, it uses a horizontal velocity calculated from the jump settings. For rocket jumps, a jump velocity calculated from the start point is
   used. For jump pads, the edge and face numbers of the reachability are interpreted as horizontal and vertical velocities.

	When the reachability represents a jump, the function calls AAS_JumpReachRunStart to initialize the jump simulation and draws a cross to indicate the starting direction.

	This visual debug information can be used to verify that pathfinding and movement prediction are functioning correctly. Ensure that the provided reach pointer is valid and that the AAS data
   structures have been properly initialized.

	TODO: clarify if the function performs any error handling for invalid reach data.

	\param reach pointer to the reachability structure to display
*/
void AAS_ShowReachability( struct aas_reachability_s* reach );

/*!
	\brief Displays the areas reachable from the specified area, updating the display and printing travel information incrementally

	The function maintains static state to iterate over the list of areas reachable from the supplied area number. When called with a new area number it resets the index; otherwise it continues from
   where it left off. For each reachable area, after a 1.5‑second interval, the function copies the reachability data into a local structure, prints the travel type and travel time, and invokes a
   helper to show the reachability details. If the area has no reachable areas the function exits immediately.

	\param areanum The numeric identifier of the area from which reachable areas are to be displayed.
*/
void AAS_ShowReachableAreas( int areanum );
