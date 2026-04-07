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
 * name:		be_aas_bsp.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#ifdef AASINTERN
// loads the given BSP file
int			AAS_LoadBSPFile();
// dump the loaded BSP data
void		AAS_DumpBSPData();
// unlink the given entity from the bsp tree leaves
void		AAS_UnlinkFromBSPLeaves( bsp_link_t* leaves );
// link the given entity to the bsp tree leaves of the given model
bsp_link_t* AAS_BSPLinkEntity( vec3_t absmins, vec3_t absmaxs, int entnum, int modelnum );

// calculates collision with given entity
qboolean	AAS_EntityCollision( int entnum, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int contentmask, bsp_trace_t* trace );
// for debugging
void		AAS_PrintFreeBSPLinks( char* str );
//

#endif // AASINTERN

#define MAX_EPAIRKEY 128

/*!
	\brief Executes a trace across the world, returning collision information.

	The function forwards the provided parameters to the AAS tracing subsystem via botimport.Trace. It accepts a start point, a bounding box (min and max extents), an end point, an entity to ignore
   during the trace, and a content mask specifying which environmental or entity contents should be considered. The resulting bsp_trace_t contains data about the collision such as whether the trace
   started inside a solid volume, the fraction of the distance travelled before impact, the impact position, and surface flags if hit.

	\param start The origin point from which the trace originates.
	\param mins Lower bounds of the trace bounding box relative to start; may be NULL if not used.
	\param maxs Upper bounds of the trace bounding box; may be NULL if not used.
	\param end The destination point of the trace.
	\param passent Entity number to ignore during the trace. Passing 0 means no exception.
	\param contentmask Bitmask of content types to include in the collision test.
	\return A bsp_trace_t structure containing the trace result data, including hit status, intersection point, and related flags.
*/
bsp_trace_t AAS_Trace( vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask );

/*!
	\brief Returns the contents at the given point.

	Calls the bot import function to retrieve the content flags for the provided world coordinate. The result is a bitmask representing the environment type (e.g., solid, water, lava, or slime).

	\param point World coordinates to query.
	\return Bitmask of content flags for the point.
*/
int			AAS_PointContents( vec3_t point );

/*!
	\brief Determines whether the second point lies inside the potential visibility set of the first point.

	Delegates to the bot import interface’s inPVS routine, which performs the visibility test.

	\param p1 The first point to test.
	\param p2 The second point to test for visibility relative to the first.
	\return A qboolean indicating true if p2 is within the potential visibility set of p1, otherwise false.
*/
qboolean	AAS_inPVS( vec3_t p1, vec3_t p2 );

/*!
	\brief Always reports that point p2 is in the Potentially Hidden Set of point p1

	This stub function returns true for any input vectors. In a full implementation it would determine if p2 lies within the potential hidden set relative to p1, taking into account visibility and
   obstruction data.

	\param p1 The point from which visibility is considered
	\param p2 The point whose presence in the Potentially Hidden Set relative to p1 is tested
	\return qtrue if p2 is considered to be in the Potentially Hidden Set of p1
*/
qboolean	AAS_inPHS( vec3_t p1, vec3_t p2 );
// returns true if the given areas are connected
qboolean	AAS_AreasConnected( int area1, int area2 );

/*!
	\brief Populates a list with entity identifiers that overlap the specified bounding box and returns the number of entities found.

	The function examines all entities in the world to determine which ones intersect the axis-aligned box defined by the minimum and maximum coordinates absmins and absmaxs. For each entity that lies
   entirely or partially within that box, its identifier is stored in the array pointed to by list. The process continues until either all relevant entities have been added or the list has reached the
   capacity specified by maxcount. The function then returns the total count of entities placed into the list, which may be less than or equal to maxcount.

	\param absmins The minimal corner of the axis-aligned bounding box (world coordinates).
	\param absmaxs The maximal corner of the bounding box (world coordinates).
	\param list Pointer to an integer array that receives the identifiers of qualifying entities. The array must be large enough to hold at least maxcount entries.
	\param maxcount The maximum number of entity identifiers to store in list. The function will not write more than this number of entries.
	\return The count of entities whose bounding volumes intersect the box, i.e., the number of identifiers written into list. The returned value is less than or equal to maxcount.
*/
int			AAS_BoxEntities( vec3_t absmins, vec3_t absmaxs, int* list, int maxcount );

/*!
	\brief retrieves the bounding box and origin of a BSP model given its number and orientation

	This function obtains the local-space extents (minimum and maximum coordinates) and the origin point of the specified BSP model. The output vectors are filled by calculating the transformed model
   bounds based on the supplied Euler angles.

	\param modelnum identifier of the BSP model to query
	\param angles orientation of the model in Euler angles
	\param mins output vector receiving the minimum 3‑D coordinate of the model’s bounding box
	\param maxs output vector receiving the maximum 3‑D coordinate of the model’s bounding box
	\param origin output vector receiving the model’s origin point
*/
void		AAS_BSPModelMinsMaxsOrigin( int modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin );

/*!
	\brief Returns the next valid entity index after the given entity, or zero if none remain.

	The function increments its argument and checks if the resulting index lies within the range of valid entity identifiers stored in the current BSP world (between 1 and bspworld.numentities-1). If
   it is valid, that index is returned; otherwise, 0 is returned to signal that iteration is complete. It is used for looping over all entities in the area awareness system.

	\param ent Current entity index from which to find the next; 0 starts the iteration at the first entity.
	\return Integer index of the next entity, or 0 if there is no subsequent entity.
*/
int			AAS_NextBSPEntity( int ent );

/*!
	\brief Returns a given key’s value string for a BSP entity.

	The function first clears the output buffer by setting value[0] to a null char. It checks that the entity index is within the valid range using AAS_BSPEntityInRange; if not, it returns false. It
   then iterates over the linked list of epairs attached to the entity. When it finds an epair whose key matches the supplied key, it copies the epair’s value string into the provided buffer, making
   sure to copy at most size-1 characters and null-terminating the string. Finally it returns true. If no matching key is found, the function returns false and the buffer remains empty.

	The function treats its integer return value as a boolean, returning qtrue on success and qfalse on failure. The caller can use the returned value to determine whether the output buffer contains
   meaningful data.

	\param ent Index of the BSP world entity to query
	\param key Null-terminated string containing the key name to search for
	\param value Character array that will receive the matched value string
	\param size Size of the value buffer, in characters, including space for the terminating null
	\return qtrue if key was found and value copied; qfalse otherwise
*/
int			AAS_ValueForBSPEpairKey( int ent, char* key, char* value, int size );

/*!
	\brief Retrieves a 3‑component vector from an entity's BSP epair key and stores it in v.

	The function first clears the vector, then calls AAS_ValueForBSPEpairKey to obtain the key's string value. If the key is not present it returns false. Otherwise it parses up to three
   floating‑point numbers from the string using sscanf and assigns them to the vector components. Any missing components remain zero. It returns a non‑zero value indicating success.

	\param ent identifier of the entity to query
	\param key name of the key whose value contains the vector data
	\param v output 3‑component vector that receives the parsed coordinates; it is cleared before parsing
	\return an integer that evaluates to true when the key was found and parsed successfully, and false otherwise
*/
int			AAS_VectorForBSPEpairKey( int ent, char* key, vec3_t v );

/*!
	\brief Retrieves a floating‑point value associated with a BSP entity pair key.

	The function queries the value of the specified key from the BSP entity list via AAS_ValueForBSPEpairKey. If the key is present, its string representation is converted to float using atof and
   stored in the supplied output pointer. When the key cannot be found, the output is left at 0 and the function indicates failure by returning qfalse. The return value is an integer flag: non‑zero on
   success, zero on failure.

	\param ent Index of the entity to be queried
	\param key Name of the key whose value is requested
	\param value Pointer to a float that receives the parsed value
	\return Non‑zero if the key existed and was successfully converted; zero otherwise.
*/
int			AAS_FloatForBSPEpairKey( int ent, char* key, float* value );

/*!
	\brief Retrieves an integer value associated with a specified BSP entity pair key.

	The function calls AAS_ValueForBSPEpairKey to obtain the string value of the requested key for the given entity. The resulting string is converted to an integer using atoi and written to the
   supplied output pointer. It returns a non‑zero indicator if the key was successfully retrieved; otherwise it returns zero and leaves the output value set to zero.

	\param ent The entity number within the BSP world.
	\param key The name of the key to look up.
	\param value A pointer to an integer where the converted value will be stored.
	\return Non‑zero if the key was found and the value written; zero if the key was not present.
*/
int			AAS_IntForBSPEpairKey( int ent, char* key, int* value );
