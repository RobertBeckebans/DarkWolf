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
 * name:		be_aas_sample.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#ifdef AASINTERN
void		 AAS_InitAASLinkHeap();
void		 AAS_InitAASLinkedEntities();
void		 AAS_FreeAASLinkHeap();
void		 AAS_FreeAASLinkedEntities();
aas_face_t*	 AAS_AreaGroundFace( int areanum, vec3_t point );
aas_face_t*	 AAS_TraceEndFace( aas_trace_t* trace );
aas_plane_t* AAS_PlaneFromNum( int planenum );
aas_link_t*	 AAS_AASLinkEntity( vec3_t absmins, vec3_t absmaxs, int entnum );
aas_link_t*	 AAS_LinkEntityClientBBox( vec3_t absmins, vec3_t absmaxs, int entnum, int presencetype );
qboolean	 AAS_PointInsideFace( int facenum, vec3_t point, float epsilon );
qboolean	 AAS_InsideFace( aas_face_t* face, vec3_t pnormal, vec3_t point, float epsilon );
void		 AAS_UnlinkFromAreas( aas_link_t* areas );
#endif // AASINTERN

/*!
	\brief Retrieves the minimum and maximum coordinates of the bounding box for a specified presence type.

	The bounding box dimensions are predefined for normal and crouch presence types. For a normal presence, a bounding box of (-15, -15, -24) to (15, 15, 32) is used, while a crouch presence uses
   (-15, -15, -24) to (15, 15, 8). Any other type triggers a fatal error message and defaults to the crouch values. The function copies these preset values into the provided mins and maxs vectors
   using VectorCopy.

	\param presencetype the presence type identifier, typically PRESENCE_NORMAL or PRESENCE_CROUCH
	\param mins output vector receiving the minimum corner of the bounding box
	\param maxs output vector receiving the maximum corner of the bounding box
*/
void		AAS_PresenceTypeBoundingBox( int presencetype, vec3_t mins, vec3_t maxs );

/*!
	\brief Provides the cluster identifier for a specified area number.

	The function verifies the area number falls within the valid range of defined areas. If the number is invalid, an error message is printed to the bot import system and the function returns 0. For
   a valid area, it retrieves and returns the cluster value stored in the area settings. A non‑negative value represents a normal cluster; a negative value indicates the area corresponds to a portal,
   in which case the returned negative number encodes that portal identifier.

	\param areanum The numerical identifier of the area whose cluster is being queried.
	\return The cluster number associated with the area, or 0 if the area number is out of range. Negative values denote portal identifiers.
*/
int			AAS_AreaCluster( int areanum );

/*!
	\brief Retrieves the presence type bitmask for the specified area number.

	If the global AAS world data is not loaded or the supplied area number is out of bounds, the function logs an error and returns 0. For a valid area, it reads the presencetype field from the area
   settings and returns that value. The presencetype is a bitmask where bits such as PRESENCE_NORMAL and PRESENCE_CROUCH can be combined to indicate the allowed presence modes for the area.

	\param areanum The numeric index of the area whose presence type is requested.
	\return An integer bitmask of presence types for the specified area, or zero when the world is unloaded or the area number is invalid.
*/
int			AAS_AreaPresenceType( int areanum );

/*!
	\brief Retrieves the presence type flags for a point in the AI navigation mesh.

	The function first checks whether the global AAS world data is loaded; if it is not, zero is returned.
	It then obtains the area number containing the supplied point by calling AAS_PointAreaNum. If the point does not belong to any area, or the area number is zero, the function returns PRESENCE_NONE.
   Otherwise it looks up the presence type stored in the world's area settings for that area and returns it. The returned integer is a bitmask of presence flags such as PRESENCE_NORMAL and
   PRESENCE_CROUCH, representing characteristics that actors may have at the point.

	\param point A three‑component vector specifying the position to query. The vector is interpreted in world space coordinates.
	\return An integer bitmask indicating the presence types applicable to the point. If the AAS world data is not loaded or the point lies outside of any area, zero (often defined as PRESENCE_NONE)
   is returned.
*/
int			AAS_PointPresenceType( vec3_t point );

/*!
	\brief traces a client bounding box through the AAS world and returns collision information

	The function performs a line trace from a start point to an end point, taking into account an entity bounding box that represents a client.
	It uses a manual stack to walk the AAS node tree, beginning at the root node and evaluating each plane encountered. For area nodes it checks whether the presence type of the area (e.g., normal,
   crouch) matches the requested presence type and either continues the traversal or records a hit. For solid leaf nodes it treats the traversal as a collision. When an entity is supplied in the passe
   argument, the trace checks for a collision with that entity using AAS_AreaEntityCollision. During the walk, the function calculates the fraction along the line where the first impact occurs,
   updates the end position of the trace to that hit point, and fills fields such as startsolid, ent (entity hit), area, and planenum. If no collision is detected, the fraction remains 1.0 and the end
   position equals the original end point. The trace result is stored in an aas_trace_t structure, which is returned to the caller.

	\param start initial point of the trace
	\param end endpoint of the trace
	\param presencetype bitmask of allowed presence types for the trace
	\param passent entity number to ignore during collision checks, or -1 for no entity
	\return an aas_trace_t structure holding the trace outcome, including boolean fields for startsolid and end solid state, the fraction of the line traversed before collision, the end position, the
   entity hit (if any), the area number hit, and the plane number
*/
aas_trace_t AAS_TraceClientBBox( vec3_t start, vec3_t end, int presencetype, int passent );

/*!
	\brief Collects the areas traversed by a line segment and returns how many were found.

	The function walks the AAS spatial partition tree using an explicit stack.  It begins with the full segment from start to end and, at each node, determines whether the segment lies entirely in the
   front, entirely in the back, or straddles the node plane.  For leaves that represent an area the area number is stored; for solid leaves nothing is recorded.  If the points array is supplied, the
   intersection point with the segment is copied for that area.  The traversal stops when the stack is empty, an area count reaches maxareas, or a node number is out of range.  If the AAS world is not
   loaded the function immediately returns zero.

	\param areas array that will receive the area numbers in order
	\param end end position of the trace
	\param maxareas maximum number of entries to store in areas/points
	\param points array that receives the segment entry point for each area; may be NULL
	\param start start position of the trace
	\return Number of areas that the segment entered, bounded by maxareas.
*/
int			AAS_TraceAreas( vec3_t start, vec3_t end, int* areas, vec3_t* points, int maxareas );

/*!
	\brief Returns the area number the point lies within, or zero if the point is in solid or data is unavailable.

	The function walks the binary space partition tree of the AAS world. Starting at node 1, each node’s plane is used to test the point’s signed distance; if the distance is positive the traversal
   takes child 0 (left), otherwise child 1 (right). The walk continues until a node with index 0 is reached, which represents a solid leaf; in that case the function returns 0. If a non‑solid leaf is
   found, the function returns the negative of that leaf’s index, which is the identifier of the area containing the point. If the AAS world has not been loaded, an error is logged and zero is
   returned.

	\param point a 3‑D vector specifying the point to query in world coordinates.
	\return A negative integer representing the area number when the point is inside an area, or 0 if the point is in solid or the AAS data is not loaded.
*/
int			AAS_PointAreaNum( vec3_t point );

/*!
	\brief Retrieves the plane of a specified face.

	The function looks up the plane that defines the given face in the global world data, copies the plane normal into the provided normal array, and stores the plane distance through the dist
   pointer.

	\param facenum index of the face to query
	\param normal array of three floats that will receive the plane normal
	\param dist pointer to a float where the plane distance will be written
*/
void		AAS_FacePlane( int facenum, vec3_t normal, float* dist );

/*!
	\brief Collects area numbers that intersect a bounding box into a supplied array and returns how many were found.

	The function first links all areas that intersect the given bounding‑box using AAS_AASLinkEntity. It then iterates over the resulting linked list, copying each area number into the provided array
   until either all linked areas are processed or the maximum number of entries specified by maxareas is reached. After copying, it unlinks the temporary list and returns the count of area numbers
   written. The array is filled sequentially starting at index 0.

	If the bounding‑box does not intersect any area, the function returns 0 and the areas array remains unchanged. The caller is responsible for ensuring that the areas array has space for at least
   maxareas elements.

	\param absmins The minimum coordinates of the axis-aligned bounding box.
	\param absmaxs The maximum coordinates of the axis-aligned bounding box.
	\param areas Output array where the area numbers will be stored.
	\param maxareas Maximum number of entries the caller can safely receive in areas.
	\return The number of area numbers written into the array, or 0 if none were found.
*/
int			AAS_BBoxAreas( vec3_t absmins, vec3_t absmaxs, int* areas, int maxareas );
