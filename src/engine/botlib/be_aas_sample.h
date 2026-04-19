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
	\file engine/botlib/be_aas_sample.h
	\brief Define the public interface for bot navigation within the Return to Castle Wolfenstein engine; this header exposes functions that return positioning and collision information to enable AI pathfinding and perception.
	\note doxygenix: sha256=a48c38eaa0700923df1c655c09c01afeeb2c9e2d686f517b09aa219d5380512b

	\par File Purpose
	- Define the public interface for bot navigation within the Return to Castle Wolfenstein engine; this header exposes functions that return positioning and collision information to enable AI pathfinding and perception.

	\par Core Responsibilities
	- Declare runtime API for bot navigation and AAS queries; expose functions for area cluster/ presence type lookup, bbox area collection, face plane retrieval, bounding box area enumeration, entity linking, and collision tracing of client bounding boxes.
	- Provide internal helper prototypes guarded by AASINTERN for memory heap/ linkage initialization and cleanup.

	\par Key Types and Functions
	- AAS_InitAASLinkHeap() — initializes the heap used for linking AAS areas to entities.
	- AAS_InitAASLinkedEntities() — prepares linked lists for entities within areas.
	- AAS_FreeAASLinkHeap() — frees memory allocated for the link heap.
	- AAS_FreeAASLinkedEntities() — clears the entity link data structures.
	- AAS_AreaGroundFace(int areanum, vec3_t point) — returns the lowest ground face in a specified area containing a point.
	- AAS_TraceEndFace(aas_trace_t* trace) — gives the face a trace ends on after traversal.
	- AAS_PlaneFromNum(int planenum) — retrieves a plane structure by its index.
	- AAS_AASLinkEntity(vec3_t absmins, vec3_t absmaxs, int entnum) — links an entity’s bounding box to intersecting AAS areas.
	- AAS_LinkEntityClientBBox(vec3_t absmins, vec3_t absmaxs, int entnum, int presencetype) — generates links for a client‑style bounding box within the AAS.
	- AAS_PointInsideFace(int facenum, vec3_t point, float epsilon) — tests point inclusion within a face with tolerance.
	- AAS_InsideFace(aas_face_t* face, vec3_t pnormal, vec3_t point, float epsilon) — checks if a point lies inside a face given its normal.
	- AAS_UnlinkFromAreas(aas_link_t* areas) — removes links from all areas to a given linked list.
	- AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs) — copies preset mins/maxs for NORMAL or CROUCH presence types.
	- AAS_AreaCluster(int areanum) — returns the cluster number or portal identifier for a given area.
	- AAS_AreaPresenceType(int areanum) — obtains the presence‑type bitmask allowed in an area.
	- AAS_PointPresenceType(vec3_t point) — determines which presence types apply to a world point.
	- AAS_TraceClientBBox(vec3_t start, vec3_t end, int presencetype, int passent) — traces a client‑style bounding box through the AAS and reports collision data.
	- AAS_TraceAreas(vec3_t start, vec3_t end, int* areas, vec3_t* points, int maxareas) — collects all areas intersected by a line segment and optional entry points.
	- AAS_PointAreaNum(vec3_t point) — returns the area number containing a point or zero if solid/unloaded.
	- AAS_FacePlane(int facenum, vec3_t normal, float* dist) — retrieves the plane normal and distance for a face.
	- AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int* areas, int maxareas) — enumerates all areas overlapping a bounding box and outputs their numbers.

	\par Control Flow
	- AAS world data is queried through a binary space partition tree or linked lists; stack traversal or pointers walk nodes to determine areas, faces, and planes; bounding box linking occurs via AAS_AASLinkEntity, after which results are collected into caller provided arrays; trace functions use iterative stacks to walk the spatial hierarchy and accumulate collision information at runtime.

	\par Dependencies
	- None explicitly listed – the header assumes inclusion of core AAS types (aas_face_t, aas_link_t, aas_trace_t, vec3_t) and global world data structures defined elsewhere in the botlib.

	\par How It Fits
	- Forms the contract between the bot runtime and the AAS subsystem; other bot modules call these functions to obtain area identifiers, presence data, and perform spatial queries; it is central to navigation, collision avoidance, and area-based state management.
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
	\brief Performs a trace of a client bounding box through the AAS navigation mesh from start to end positions, considering specified presence type and pass entity.

	This function traces a client bounding box through the AAS (Area Awareness System) navigation mesh from a starting point to an ending point. It uses a stack-based approach to traverse the AAS
   spatial partitioning tree, checking for collisions with nodes and areas. The trace considers the presence type to determine which areas can be entered and checks for entity collisions, ignoring a
   specified entity if provided. The function handles various scenarios including solid leaf encounters, area traversals, and collision detection with planes. It returns detailed trace information
   including whether the start point was solid, the fraction of the line traversed, the end position, the area number hit, the entity hit, and the plane number.

	\param end endpoint of the trace
	\param passent entity number to ignore during collision checks, or -1 for no entity
	\param presencetype bitmask of allowed presence types for the trace
	\param start initial point of the trace
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
