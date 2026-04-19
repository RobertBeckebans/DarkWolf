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
	\file engine/botlib/be_aas_sample.cpp
	\brief Implements the sample‑size AAS navigation logic used by the bot library.
	\note doxygenix: sha256=d4e18b19b1233467fac88d0a3d2b4b90a8ac0f5f7cc911259998468c32462667

	\par File Purpose
	- Implements the sample‑size AAS navigation logic used by the bot library.
	- Provides traversal, querying, and collision routines on the AAS spatial partition.
	- Manages dynamic allocation of link structures linking entities to areas.

	\par Core Responsibilities
	- Compute presence‑type bounding boxes for normal and crouch agents.
	- Allocate, link, and free a global heap of aas_link_t used to track entity‑area associations.
	- Provide area queries: area number (AAS_PointAreaNum), cluster (AAS_AreaCluster), presence type (AAS_AreaPresenceType, AAS_PointPresenceType).
	- Trace client‑sized bounding boxes through the AAS, detecting solid geometry, entity collision, and area transitions (AAS_TraceClientBBox).
	- Record the sequence of areas intersected by a segment (AAS_TraceAreas).
	- Test point containment in faces and bounding boxes (AAS_InsideFace, AAS_PointInsideFace).
	- Retrieve face planes and ground faces (AAS_FacePlane, AAS_AreaGroundFace).
	- Manage entity link lists: allocate links to intersecting areas, unlink on removal, and query linked areas (AAS_AASLinkEntity, AAS_LinkEntityClientBBox, AAS_UnlinkFromAreas).

	\par Key Types and Functions
	- aas_node_t – tree node in the AAS spatial partition.
	- aas_link_t – represents a link between an entity and an area.
	- aas_trace_t – result of tracing a bounding box through the AAS.
	- AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs) – fills mins/maxs for a given presence type.
	- AAS_InitAASLinkHeap() – allocates and links the global link heap.
	- AAS_FreeAASLinkHeap() – frees the link heap memory.
	- aas_link_t *AAS_AllocAASLink() – retrieves a link from the free pool.
	- void AAS_DeAllocAASLink(aas_link_t *link) – returns a link to the free pool.
	- void AAS_InitAASLinkedEntities() – allocates per‑area entity link array.
	- void AAS_FreeAASLinkedEntities() – releases that array.
	- int AAS_PointAreaNum(vec3_t point) – returns the area containing a point.
	- int AAS_AreaCluster(int areanum) – cluster ID for an area.
	- int AAS_AreaPresenceType(int areanum) – allowed presence types in an area.
	- int AAS_PointPresenceType(vec3_t point) – presence mask valid at a point.
	- qboolean AAS_AreaEntityCollision(int areanum, vec3_t start, vec3_t end, int presencetype, int passent, aas_trace_t *trace) – tests a segment for entity collisions.
	- aas_trace_t AAS_TraceClientBBox(vec3_t start, vec3_t end, int presencetype, int passent) – full trace routine for a client bbox.
	- int AAS_TraceAreas(vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas) – collects intersected areas from a segment.
	- qboolean AAS_InsideFace(aas_face_t *face, vec3_t pnormal, vec3_t point, float epsilon) – test point inside polygon.
	- qboolean AAS_PointInsideFace(int facenum, vec3_t point, float epsilon) – same test using face index.
	- aas_face_t *AAS_AreaGroundFace(int areanum, vec3_t point) – finds the ground face beneath a point.
	- void AAS_FacePlane(int facenum, vec3_t normal, float *dist) – retrieves face plane.
	- aas_face_t *AAS_TraceEndFace(aas_trace_t *trace) – finds face containing trace end.
	- int AAS_BoxOnPlaneSide2(vec3_t absmins, vec3_t absmaxs, aas_plane_t *p) – tests a bbox against a plane.
	- void AAS_UnlinkFromAreas(aas_link_t *areas) – removes an entity’s area links.
	- aas_link_t *AAS_AASLinkEntity(vec3_t absmins, vec3_t absmaxs, int entnum) – links entity’s bbox to all intersecting areas.
	- aas_link_t *AAS_LinkEntityClientBBox(vec3_t absmins, vec3_t absmaxs, int entnum, int presencetype) – wrapper that expands bbox per presence type.
	- aas_plane_t *AAS_PlaneFromNum(int planenum) – fetch a plane by index.
	- int AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas) – fills array with intersected area numbers.

	\par Control Flow
	- Functions first check if the AAS world is loaded, then use explicit stacks to traverse the AAS node tree.
	- AAS_TraceClientBBox starts with the full segment on the trace stack, popping segments until the stack is empty or a hit occurs.
	- For each node, the trace pushes child nodes depending on the signed distances to the node plane, and splits the segment when the line straddles the plane.
	- On reaching an area node, the trace tests presence type compatibility and optionally checks entity collisions before recording the trace outcome.
	- Leaf nodes that are solid simply set startsolid/fraction and return.

	\par Dependencies
	- "q_shared.h" – core math & vector utilities.
	- "l_memory.h" – engine memory allocation helpers.
	- "l_script.h", "l_precomp.h", "l_struct.h" – scripting and preload subsystem headers.
	- "aasfile.h" – definitions of AAS world data structures.
	- "botshared/botlib.h", "botshared/be_aas.h", "be_aas_funcs.h", "be_aas_def.h" – bot library interfaces and macros.

	\par How It Fits
	- Forms the core of bot navigation calculations performed by higher‑level path‑finding code.
	- Operates on the global AAS world loaded from disk, exposing functions for area lookup, collision detection, and entity‑area bookkeeping.
	- Enables bots to sense terrain, avoid obstacles, and move through the level.
*/

/*****************************************************************************
 * name:		be_aas_sample.c
 *
 * desc:		AAS environment sampling
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "botshared/botlib.h"
#include "botshared/be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

extern botlib_import_t botimport;

// #define AAS_SAMPLE_DEBUG

#define BBOX_NORMAL_EPSILON 0.001

#define ON_EPSILON			0 // 0.0005

#define TRACEPLANE_EPSILON	0.125

typedef struct aas_tracestack_s {
	vec3_t start;	 // start point of the piece of line to trace
	vec3_t end;		 // end point of the piece of line to trace
	int	   planenum; // last plane used as splitter
	int	   nodenum;	 // node found after splitting with planenum
} aas_tracestack_t;

void AAS_PresenceTypeBoundingBox( int presencetype, vec3_t mins, vec3_t maxs )
{
	int	   index;
	// bounding box size for each presence type
	vec3_t boxmins[3] = { { 0, 0, 0 }, { -15, -15, -24 }, { -15, -15, -24 } };
	vec3_t boxmaxs[3] = { { 0, 0, 0 }, { 15, 15, 32 }, { 15, 15, 8 } };

	if( presencetype == PRESENCE_NORMAL ) {
		index = 1;

	} else if( presencetype == PRESENCE_CROUCH ) {
		index = 2;

	} else {
		botimport.Print( PRT_FATAL, "AAS_PresenceTypeBoundingBox: unknown presence type\n" );
		index = 2;
	}

	VectorCopy( boxmins[index], mins );
	VectorCopy( boxmaxs[index], maxs );
}

/*!
	\brief Sets up the link heap used by the AAS world, allocating memory if necessary and linking the heap entries into a free list.

	The function first determines the desired size of the link heap from the current linkheapsize value. If no heap has been allocated yet, it uses a default size of 4096, ensuring non‑negative
   values, updates linkheapsize, and allocates an array of aas_link_t structures in hunk memory. It then initializes a doubly‑linked list that connects each element of the heap array, thereby
   establishing a free list. Finally, it points the freelinks member of the world structure to the first free link, marking the entire heap as available for use.

*/
void AAS_InitAASLinkHeap()
{
	int i, max_aaslinks;

	max_aaslinks = ( *aasworld ).linkheapsize;

	// if there's no link heap present
	if( !( *aasworld ).linkheap ) {
		max_aaslinks = ( int )4096; // LibVarValue("max_aaslinks", "4096");

		if( max_aaslinks < 0 ) {
			max_aaslinks = 0;
		}

		( *aasworld ).linkheapsize = max_aaslinks;
		( *aasworld ).linkheap	   = ( aas_link_t* )GetHunkMemory( max_aaslinks * sizeof( aas_link_t ) );
	}

	// link the links on the heap
	( *aasworld ).linkheap[0].prev_ent = NULL;
	( *aasworld ).linkheap[0].next_ent = &( *aasworld ).linkheap[1];

	for( i = 1; i < max_aaslinks - 1; i++ ) {
		( *aasworld ).linkheap[i].prev_ent = &( *aasworld ).linkheap[i - 1];
		( *aasworld ).linkheap[i].next_ent = &( *aasworld ).linkheap[i + 1];
	}

	( *aasworld ).linkheap[max_aaslinks - 1].prev_ent = &( *aasworld ).linkheap[max_aaslinks - 2];
	( *aasworld ).linkheap[max_aaslinks - 1].next_ent = NULL;
	// pointer to the first free link
	( *aasworld ).freelinks = &( *aasworld ).linkheap[0];
}

/*!
	\brief Frees the memory reserved for the link heap in the AAS world and resets its pointers.

	The function checks whether the AAS world has a link heap allocated. If so, it frees that memory. Afterwards it clears the global link heap pointer and sets the heap size to zero to avoid dangling
   references. The operation ensures no leaked heap memory remains when the link heap is no longer required.

*/
void AAS_FreeAASLinkHeap()
{
	if( ( *aasworld ).linkheap ) {
		FreeMemory( ( *aasworld ).linkheap );
	}

	( *aasworld ).linkheap	   = NULL;
	( *aasworld ).linkheapsize = 0;
}

/*!
	\brief Allocate a link from the free AAS link pool.

	The AAS world keeps a linked list of unused link structures. This function removes the first node from that list, updates the head pointer, and clears the previous pointer of the new head. If the
   list is empty, it signals a fatal error and returns a null pointer. The returned link must be initialized before use.

	\return A pointer to an available aas_link_t from the pool, or a null pointer if the pool has been exhausted.
*/
aas_link_t* AAS_AllocAASLink()
{
	aas_link_t* link;

	link = ( *aasworld ).freelinks;

	if( !link ) {
		botimport.Print( PRT_FATAL, "empty aas link heap\n" );
		return NULL;
	}

	if( ( *aasworld ).freelinks ) {
		( *aasworld ).freelinks = ( *aasworld ).freelinks->next_ent;
	}

	if( ( *aasworld ).freelinks ) {
		( *aasworld ).freelinks->prev_ent = NULL;
	}

	return link;
}

/*!
	\brief Adds the given link to the global free list of AAS links.

	Resets the pointers of the supplied link and inserts it at the front of the global list of free links. After calling this function the link can be reused for new connections between AAS areas. The
   function assumes that the provided link is valid and not currently in use.

	\param link The link to be returned to the free list; must not be NULL.
*/
void AAS_DeAllocAASLink( aas_link_t* link )
{
	if( ( *aasworld ).freelinks ) {
		( *aasworld ).freelinks->prev_ent = link;
	}

	link->prev_ent			= NULL;
	link->next_ent			= ( *aasworld ).freelinks;
	link->prev_area			= NULL;
	link->next_area			= NULL;
	( *aasworld ).freelinks = link;
}

/*!
	\brief Initializes the linked entity array for each area in the AAS world

	If the AAS world is not loaded the function exits immediately. If a previous array of linked entities exists it is freed. A new array of pointers is then allocated from hunk memory, cleared to
   zero, and assigned to the AAS world structure. Each entry in the array will later hold a list of entities linked to its area.

*/
void AAS_InitAASLinkedEntities()
{
	if( !( *aasworld ).loaded ) {
		return;
	}

	if( ( *aasworld ).arealinkedentities ) {
		FreeMemory( ( *aasworld ).arealinkedentities );
	}

	( *aasworld ).arealinkedentities = ( aas_link_t** )GetClearedHunkMemory( ( *aasworld ).numareas * sizeof( aas_link_t* ) );
}

/*!
	\brief Frees the globally stored area linked entity array

	If the global arealinkedentities pointer is valid, the function releases its memory through FreeMemory and then resets the pointer to null to avoid dangling references.

*/
void AAS_FreeAASLinkedEntities()
{
	if( ( *aasworld ).arealinkedentities ) {
		FreeMemory( ( *aasworld ).arealinkedentities );
	}

	( *aasworld ).arealinkedentities = NULL;
}

int AAS_PointAreaNum( vec3_t point )
{
	int			 nodenum;
	vec_t		 dist;
	aas_node_t*	 node;
	aas_plane_t* plane;

	if( !( *aasworld ).loaded ) {
		botimport.Print( PRT_ERROR, "AAS_PointAreaNum: aas not loaded\n" );
		return 0;
	}

	// start with node 1 because node zero is a dummy used for solid leafs
	nodenum = 1;

	while( nodenum > 0 ) {
		//		botimport.Print(PRT_MESSAGE, "[%d]", nodenum);
#ifdef AAS_SAMPLE_DEBUG
		if( nodenum >= ( *aasworld ).numnodes ) {
			botimport.Print( PRT_ERROR, "nodenum = %d >= (*aasworld).numnodes = %d\n", nodenum, ( *aasworld ).numnodes );
			return 0;
		}

#endif // AAS_SAMPLE_DEBUG
		node = &( *aasworld ).nodes[nodenum];
#ifdef AAS_SAMPLE_DEBUG

		if( node->planenum < 0 || node->planenum >= ( *aasworld ).numplanes ) {
			botimport.Print( PRT_ERROR, "node->planenum = %d >= (*aasworld).numplanes = %d\n", node->planenum, ( *aasworld ).numplanes );
			return 0;
		}

#endif // AAS_SAMPLE_DEBUG
		plane = &( *aasworld ).planes[node->planenum];
		dist  = DotProduct( point, plane->normal ) - plane->dist;

		if( dist > 0 ) {
			nodenum = node->children[0];

		} else {
			nodenum = node->children[1];
		}
	}

	if( !nodenum ) {
#ifdef AAS_SAMPLE_DEBUG
		botimport.Print( PRT_MESSAGE, "in solid\n" );
#endif // AAS_SAMPLE_DEBUG
		return 0;
	}

	return -nodenum;
}

int AAS_AreaCluster( int areanum )
{
	if( areanum <= 0 || areanum >= ( *aasworld ).numareas ) {
		botimport.Print( PRT_ERROR, "AAS_AreaCluster: invalid area number\n" );
		return 0;
	}

	return ( *aasworld ).areasettings[areanum].cluster;
}

int AAS_AreaPresenceType( int areanum )
{
	if( !( *aasworld ).loaded ) {
		return 0;
	}

	if( areanum <= 0 || areanum >= ( *aasworld ).numareas ) {
		botimport.Print( PRT_ERROR, "AAS_AreaPresenceType: invalid area number\n" );
		return 0;
	}

	return ( *aasworld ).areasettings[areanum].presencetype;
}

int AAS_PointPresenceType( vec3_t point )
{
	int areanum;

	if( !( *aasworld ).loaded ) {
		return 0;
	}

	areanum = AAS_PointAreaNum( point );

	if( !areanum ) {
		return PRESENCE_NONE;
	}

	return ( *aasworld ).areasettings[areanum].presencetype;
}

/*!
	\brief Tests whether an entity collision occurs along a line segment inside a given AAS area using a specified presence type

	The function retrieves the bounding box for the supplied presence type and then iterates over each entity linked to the specified area.  It skips any entity that matches the passed‑entity number.
   For each remaining entity the helper AAS_EntityCollision is called to check for a collision between the start and end points of the segment, taking the presence type’s bounding box into account. If
   a collision is detected the supplied trace structure is filled with the collision details—startsolid flag, entity number, end position, and placeholder values for area and planenum—and the function
   returns true.  If no collision is found the trace structure is left untouched and the function returns false.

	\param areanum identifier of the AAS area to probe
	\param start starting point of the segment in world coordinates
	\param end ending point of the segment in world coordinates
	\param presencetype type used to compute the bounding box for collision checks
	\param passent entity number to ignore during collision detection
	\param trace pointer to a structure that receives collision details when a collision occurs
	\return true if a collision was found; false otherwise. When true the trace structure contains startsolid, ent, endpos, area=0 and planenum=0
*/
qboolean AAS_AreaEntityCollision( int areanum, vec3_t start, vec3_t end, int presencetype, int passent, aas_trace_t* trace )
{
	int			collision;
	vec3_t		boxmins, boxmaxs;
	aas_link_t* link;
	bsp_trace_t bsptrace;

	AAS_PresenceTypeBoundingBox( presencetype, boxmins, boxmaxs );

	memset( &bsptrace, 0, sizeof( bsp_trace_t ) ); // make compiler happy
	// assume no collision
	bsptrace.fraction = 1;
	collision		  = qfalse;

	for( link = ( *aasworld ).arealinkedentities[areanum]; link; link = link->next_ent ) {
		// ignore the pass entity
		if( link->entnum == passent ) {
			continue;
		}

		//
		if( AAS_EntityCollision( link->entnum, start, boxmins, boxmaxs, end, CONTENTS_SOLID | CONTENTS_PLAYERCLIP, &bsptrace ) ) {
			collision = qtrue;
		}
	}

	if( collision ) {
		trace->startsolid = bsptrace.startsolid;
		trace->ent		  = bsptrace.ent;
		VectorCopy( bsptrace.endpos, trace->endpos );
		trace->area		= 0;
		trace->planenum = 0;
		return qtrue;
	}

	return qfalse;
}

aas_trace_t AAS_TraceClientBBox( vec3_t start, vec3_t end, int presencetype, int passent )
{
	int				  side, nodenum, tmpplanenum;
	float			  front, back, frac;
	vec3_t			  cur_start, cur_end, cur_mid, v1, v2;
	aas_tracestack_t  tracestack[127];
	aas_tracestack_t* tstack_p;
	aas_node_t*		  aasnode;
	aas_plane_t*	  plane;
	aas_trace_t		  trace;

	// clear the trace structure
	memset( &trace, 0, sizeof( aas_trace_t ) );

	if( !( *aasworld ).loaded ) {
		return trace;
	}

	tstack_p = tracestack;
	// we start with the whole line on the stack
	VectorCopy( start, tstack_p->start );
	VectorCopy( end, tstack_p->end );
	tstack_p->planenum = 0;
	// start with node 1 because node zero is a dummy for a solid leaf
	tstack_p->nodenum = 1; // starting at the root of the tree
	tstack_p++;

	while( 1 ) {
		// pop up the stack
		tstack_p--;

		// if the trace stack is empty (ended up with a piece of the
		// line to be traced in an area)
		if( tstack_p < tracestack ) {
			tstack_p++;
			// nothing was hit
			trace.startsolid = qfalse;
			trace.fraction	 = 1.0;
			// endpos is the end of the line
			VectorCopy( end, trace.endpos );
			// nothing hit
			trace.ent	   = 0;
			trace.area	   = 0;
			trace.planenum = 0;
			return trace;
		}

		// number of the current node to test the line against
		nodenum = tstack_p->nodenum;

		// if it is an area
		if( nodenum < 0 ) {
#ifdef AAS_SAMPLE_DEBUG

			if( -nodenum > ( *aasworld ).numareasettings ) {
				botimport.Print( PRT_ERROR, "AAS_TraceBoundingBox: -nodenum out of range\n" );
				return trace;
			}

#endif // AAS_SAMPLE_DEBUG

			// botimport.Print(PRT_MESSAGE, "areanum = %d, must be %d\n", -nodenum, AAS_PointAreaNum(start));
			// if can't enter the area because it hasn't got the right presence type
			if( !( ( *aasworld ).areasettings[-nodenum].presencetype & presencetype ) ) {
				// if the start point is still the initial start point
				// NOTE: no need for epsilons because the points will be
				// exactly the same when they're both the start point
				if( tstack_p->start[0] == start[0] && tstack_p->start[1] == start[1] && tstack_p->start[2] == start[2] ) {
					trace.startsolid = qtrue;
					trace.fraction	 = 0.0;

				} else {
					trace.startsolid = qfalse;
					VectorSubtract( end, start, v1 );
					VectorSubtract( tstack_p->start, start, v2 );
					trace.fraction = VectorLength( v2 ) / VectorNormalize( v1 );
					VectorMA( tstack_p->start, -0.125, v1, tstack_p->start );
				}

				VectorCopy( tstack_p->start, trace.endpos );
				trace.ent  = 0;
				trace.area = -nodenum;
				//				VectorSubtract(end, start, v1);
				trace.planenum = tstack_p->planenum;
				// always take the plane with normal facing towards the trace start
				plane = &( *aasworld ).planes[trace.planenum];

				if( DotProduct( v1, plane->normal ) > 0 ) {
					trace.planenum ^= 1;
				}

				return trace;

			} else {
				if( passent >= 0 ) {
					if( AAS_AreaEntityCollision( -nodenum, tstack_p->start, tstack_p->end, presencetype, passent, &trace ) ) {
						if( !trace.startsolid ) {
							VectorSubtract( end, start, v1 );
							VectorSubtract( trace.endpos, start, v2 );
							trace.fraction = VectorLength( v2 ) / VectorLength( v1 );
						}

						return trace;
					}
				}
			}

			trace.lastarea = -nodenum;
			continue;
		}

		// if it is a solid leaf
		if( !nodenum ) {
			// if the start point is still the initial start point
			// NOTE: no need for epsilons because the points will be
			// exactly the same when they're both the start point
			if( tstack_p->start[0] == start[0] && tstack_p->start[1] == start[1] && tstack_p->start[2] == start[2] ) {
				trace.startsolid = qtrue;
				trace.fraction	 = 0.0;

			} else {
				trace.startsolid = qfalse;
				VectorSubtract( end, start, v1 );
				VectorSubtract( tstack_p->start, start, v2 );
				trace.fraction = VectorLength( v2 ) / VectorNormalize( v1 );
				VectorMA( tstack_p->start, -0.125, v1, tstack_p->start );
			}

			VectorCopy( tstack_p->start, trace.endpos );
			trace.ent  = 0;
			trace.area = 0; // hit solid leaf
			//			VectorSubtract(end, start, v1);
			trace.planenum = tstack_p->planenum;
			// always take the plane with normal facing towards the trace start
			plane = &( *aasworld ).planes[trace.planenum];

			if( DotProduct( v1, plane->normal ) > 0 ) {
				trace.planenum ^= 1;
			}

			return trace;
		}

#ifdef AAS_SAMPLE_DEBUG

		if( nodenum > ( *aasworld ).numnodes ) {
			botimport.Print( PRT_ERROR, "AAS_TraceBoundingBox: nodenum out of range\n" );
			return trace;
		}

#endif // AAS_SAMPLE_DEBUG
	   // the node to test against
		aasnode = &( *aasworld ).nodes[nodenum];
		// start point of current line to test against node
		VectorCopy( tstack_p->start, cur_start );
		// end point of the current line to test against node
		VectorCopy( tstack_p->end, cur_end );
		// the current node plane
		plane = &( *aasworld ).planes[aasnode->planenum];

		switch( plane->type ) {
			/*FIXME: wtf doesn't this work? obviously the axial node planes aren't always facing positive!!!
			//check for axial planes
			case PLANE_X:
			{
				front = cur_start[0] - plane->dist;
				back = cur_end[0] - plane->dist;
				break;
			} //end case
			case PLANE_Y:
			{
				front = cur_start[1] - plane->dist;
				back = cur_end[1] - plane->dist;
				break;
			} //end case
			case PLANE_Z:
			{
				front = cur_start[2] - plane->dist;
				back = cur_end[2] - plane->dist;
				break;
			} //end case*/
			default: { // gee it's not an axial plane
				front = DotProduct( cur_start, plane->normal ) - plane->dist;
				back  = DotProduct( cur_end, plane->normal ) - plane->dist;
				break;
			} // end default
		}

		// calculate the hitpoint with the node (split point of the line)
		// put the crosspoint TRACEPLANE_EPSILON pixels on the near side
		if( front < 0 ) {
			frac = ( front + TRACEPLANE_EPSILON ) / ( front - back );

		} else {
			frac = ( front - TRACEPLANE_EPSILON ) / ( front - back );
		}

		// if the whole to be traced line is totally at the front of this node
		// only go down the tree with the front child
		if( ( front >= -ON_EPSILON && back >= -ON_EPSILON ) ) {
			// keep the current start and end point on the stack
			// and go down the tree with the front child
			tstack_p->nodenum = aasnode->children[0];
			tstack_p++;

			if( tstack_p >= &tracestack[127] ) {
				botimport.Print( PRT_ERROR, "AAS_TraceBoundingBox: stack overflow\n" );
				return trace;
			}
		}

		// if the whole to be traced line is totally at the back of this node
		// only go down the tree with the back child
		else if( ( front < ON_EPSILON && back < ON_EPSILON ) ) {
			// keep the current start and end point on the stack
			// and go down the tree with the back child
			tstack_p->nodenum = aasnode->children[1];
			tstack_p++;

			if( tstack_p >= &tracestack[127] ) {
				botimport.Print( PRT_ERROR, "AAS_TraceBoundingBox: stack overflow\n" );
				return trace;
			}
		}

		// go down the tree both at the front and back of the node
		else {
			tmpplanenum = tstack_p->planenum;

			//
			if( frac < 0 ) {
				frac = 0.001; // 0

			} else if( frac > 1 ) {
				frac = 0.999; // 1
			}

			// frac = front / (front-back);
			//
			cur_mid[0] = cur_start[0] + ( cur_end[0] - cur_start[0] ) * frac;
			cur_mid[1] = cur_start[1] + ( cur_end[1] - cur_start[1] ) * frac;
			cur_mid[2] = cur_start[2] + ( cur_end[2] - cur_start[2] ) * frac;

			//			AAS_DrawPlaneCross(cur_mid, plane->normal, plane->dist, plane->type, LINECOLOR_RED);
			// side the front part of the line is on
			side = front < 0;
			// first put the end part of the line on the stack (back side)
			VectorCopy( cur_mid, tstack_p->start );
			// not necesary to store because still on stack
			// VectorCopy(cur_end, tstack_p->end);
			tstack_p->planenum = aasnode->planenum;
			tstack_p->nodenum  = aasnode->children[!side];
			tstack_p++;

			if( tstack_p >= &tracestack[127] ) {
				botimport.Print( PRT_ERROR, "AAS_TraceBoundingBox: stack overflow\n" );
				return trace;
			}

			// now put the part near the start of the line on the stack so we will
			// continue with thats part first. This way we'll find the first
			// hit of the bbox
			VectorCopy( cur_start, tstack_p->start );
			VectorCopy( cur_mid, tstack_p->end );
			tstack_p->planenum = tmpplanenum;
			tstack_p->nodenum  = aasnode->children[side];
			tstack_p++;

			if( tstack_p >= &tracestack[127] ) {
				botimport.Print( PRT_ERROR, "AAS_TraceBoundingBox: stack overflow\n" );
				return trace;
			}
		}
	}

	//	return trace;
}

int AAS_TraceAreas( vec3_t start, vec3_t end, int* areas, vec3_t* points, int maxareas )
{
	int				  side, nodenum, tmpplanenum;
	int				  numareas;
	float			  front, back, frac;
	vec3_t			  cur_start, cur_end, cur_mid;
	aas_tracestack_t  tracestack[127];
	aas_tracestack_t* tstack_p;
	aas_node_t*		  aasnode;
	aas_plane_t*	  plane;

	numareas = 0;
	areas[0] = 0;

	if( !( *aasworld ).loaded ) {
		return numareas;
	}

	tstack_p = tracestack;
	// we start with the whole line on the stack
	VectorCopy( start, tstack_p->start );
	VectorCopy( end, tstack_p->end );
	tstack_p->planenum = 0;
	// start with node 1 because node zero is a dummy for a solid leaf
	tstack_p->nodenum = 1; // starting at the root of the tree
	tstack_p++;

	while( 1 ) {
		// pop up the stack
		tstack_p--;

		// if the trace stack is empty (ended up with a piece of the
		// line to be traced in an area)
		if( tstack_p < tracestack ) {
			return numareas;
		}

		// number of the current node to test the line against
		nodenum = tstack_p->nodenum;

		// if it is an area
		if( nodenum < 0 ) {
#ifdef AAS_SAMPLE_DEBUG

			if( -nodenum > ( *aasworld ).numareasettings ) {
				botimport.Print( PRT_ERROR, "AAS_TraceAreas: -nodenum = %d out of range\n", -nodenum );
				return numareas;
			}

#endif // AAS_SAMPLE_DEBUG
	   // botimport.Print(PRT_MESSAGE, "areanum = %d, must be %d\n", -nodenum, AAS_PointAreaNum(start));
			areas[numareas] = -nodenum;

			if( points ) {
				VectorCopy( tstack_p->start, points[numareas] );
			}

			numareas++;

			if( numareas >= maxareas ) {
				return numareas;
			}

			continue;
		}

		// if it is a solid leaf
		if( !nodenum ) {
			continue;
		}

#ifdef AAS_SAMPLE_DEBUG

		if( nodenum > ( *aasworld ).numnodes ) {
			botimport.Print( PRT_ERROR, "AAS_TraceAreas: nodenum out of range\n" );
			return numareas;
		}

#endif // AAS_SAMPLE_DEBUG
	   // the node to test against
		aasnode = &( *aasworld ).nodes[nodenum];
		// start point of current line to test against node
		VectorCopy( tstack_p->start, cur_start );
		// end point of the current line to test against node
		VectorCopy( tstack_p->end, cur_end );
		// the current node plane
		plane = &( *aasworld ).planes[aasnode->planenum];

		switch( plane->type ) {
			/*FIXME: wtf doesn't this work? obviously the node planes aren't always facing positive!!!
			//check for axial planes
			case PLANE_X:
			{
				front = cur_start[0] - plane->dist;
				back = cur_end[0] - plane->dist;
				break;
			} //end case
			case PLANE_Y:
			{
				front = cur_start[1] - plane->dist;
				back = cur_end[1] - plane->dist;
				break;
			} //end case
			case PLANE_Z:
			{
				front = cur_start[2] - plane->dist;
				back = cur_end[2] - plane->dist;
				break;
			} //end case*/
			default: { // gee it's not an axial plane
				front = DotProduct( cur_start, plane->normal ) - plane->dist;
				back  = DotProduct( cur_end, plane->normal ) - plane->dist;
				break;
			} // end default
		}

		// if the whole to be traced line is totally at the front of this node
		// only go down the tree with the front child
		if( front > 0 && back > 0 ) {
			// keep the current start and end point on the stack
			// and go down the tree with the front child
			tstack_p->nodenum = aasnode->children[0];
			tstack_p++;

			if( tstack_p >= &tracestack[127] ) {
				botimport.Print( PRT_ERROR, "AAS_TraceAreas: stack overflow\n" );
				return numareas;
			}
		}

		// if the whole to be traced line is totally at the back of this node
		// only go down the tree with the back child
		else if( front <= 0 && back <= 0 ) {
			// keep the current start and end point on the stack
			// and go down the tree with the back child
			tstack_p->nodenum = aasnode->children[1];
			tstack_p++;

			if( tstack_p >= &tracestack[127] ) {
				botimport.Print( PRT_ERROR, "AAS_TraceAreas: stack overflow\n" );
				return numareas;
			}
		}

		// go down the tree both at the front and back of the node
		else {
			tmpplanenum = tstack_p->planenum;

			// calculate the hitpoint with the node (split point of the line)
			// put the crosspoint TRACEPLANE_EPSILON pixels on the near side
			if( front < 0 ) {
				frac = ( front ) / ( front - back );

			} else {
				frac = ( front ) / ( front - back );
			}

			if( frac < 0 ) {
				frac = 0;

			} else if( frac > 1 ) {
				frac = 1;
			}

			// frac = front / (front-back);
			//
			cur_mid[0] = cur_start[0] + ( cur_end[0] - cur_start[0] ) * frac;
			cur_mid[1] = cur_start[1] + ( cur_end[1] - cur_start[1] ) * frac;
			cur_mid[2] = cur_start[2] + ( cur_end[2] - cur_start[2] ) * frac;

			//			AAS_DrawPlaneCross(cur_mid, plane->normal, plane->dist, plane->type, LINECOLOR_RED);
			// side the front part of the line is on
			side = front < 0;
			// first put the end part of the line on the stack (back side)
			VectorCopy( cur_mid, tstack_p->start );
			// not necesary to store because still on stack
			// VectorCopy(cur_end, tstack_p->end);
			tstack_p->planenum = aasnode->planenum;
			tstack_p->nodenum  = aasnode->children[!side];
			tstack_p++;

			if( tstack_p >= &tracestack[127] ) {
				botimport.Print( PRT_ERROR, "AAS_TraceAreas: stack overflow\n" );
				return numareas;
			}

			// now put the part near the start of the line on the stack so we will
			// continue with thats part first. This way we'll find the first
			// hit of the bbox
			VectorCopy( cur_start, tstack_p->start );
			VectorCopy( cur_mid, tstack_p->end );
			tstack_p->planenum = tmpplanenum;
			tstack_p->nodenum  = aasnode->children[side];
			tstack_p++;

			if( tstack_p >= &tracestack[127] ) {
				botimport.Print( PRT_ERROR, "AAS_TraceAreas: stack overflow\n" );
				return numareas;
			}
		}
	}

	//	return numareas;
}

//===========================================================================
// a simple cross product
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
// void AAS_OrthogonalToVectors(vec3_t v1, vec3_t v2, vec3_t res)
#define AAS_OrthogonalToVectors( v1, v2, res )                          \
	( res )[0] = ( ( v1 )[1] * ( v2 )[2] ) - ( ( v1 )[2] * ( v2 )[1] ); \
	( res )[1] = ( ( v1 )[2] * ( v2 )[0] ) - ( ( v1 )[0] * ( v2 )[2] ); \
	( res )[2] = ( ( v1 )[0] * ( v2 )[1] ) - ( ( v1 )[1] * ( v2 )[0] );

/*!
	\brief Determines if a point lies inside the planar boundaries of a specified face.

	The function first verifies that the point‑plane data structure is loaded, returning false if it is not. It then iterates over every edge of the given face, constructing for each edge the vector
   that is orthogonal to both the edge direction and the face’s normal – this vector defines a half‑space that contains the interior of the face. By projecting the vector from the edge’s first vertex
   to the point onto this separator normal via the dot product, the function checks whether the point lies on the outside side of any edge. If the dot product is smaller than –epsilon for any edge,
   the point is considered outside and the function returns false. If all edges are satisfied, the point is inside the face and the function returns true. The epsilon parameter allows for a small
   numerical tolerance in the dot‑product comparison.

	\param face reference to the face to test
	\param pnormal normal vector of the plane containing the face
	\param point point to test for containment
	\param epsilon margin of error for the dot‑product check
	\return qboolean true if the point is within the face boundaries, false otherwise
*/
qboolean AAS_InsideFace( aas_face_t* face, vec3_t pnormal, vec3_t point, float epsilon )
{
	int			i, firstvertex, edgenum;
	vec3_t		v0;
	vec3_t		edgevec, pointvec, sepnormal;
	aas_edge_t* edge;
#ifdef AAS_SAMPLE_DEBUG
	int lastvertex = 0;
#endif // AAS_SAMPLE_DEBUG

	if( !( *aasworld ).loaded ) {
		return qfalse;
	}

	for( i = 0; i < face->numedges; i++ ) {
		edgenum = ( *aasworld ).edgeindex[face->firstedge + i];
		edge	= &( *aasworld ).edges[abs( edgenum )];
		// get the first vertex of the edge
		firstvertex = edgenum < 0;
		VectorCopy( ( *aasworld ).vertexes[edge->v[firstvertex]], v0 );
		// edge vector
		VectorSubtract( ( *aasworld ).vertexes[edge->v[!firstvertex]], v0, edgevec );
		//
#ifdef AAS_SAMPLE_DEBUG

		if( lastvertex && lastvertex != edge->v[firstvertex] ) {
			botimport.Print( PRT_MESSAGE, "winding not counter clockwise\n" );
		}

		lastvertex = edge->v[!firstvertex];
#endif // AAS_SAMPLE_DEBUG
	   // vector from first edge point to point possible in face
		VectorSubtract( point, v0, pointvec );
		// get a vector pointing inside the face orthogonal to both the
		// edge vector and the normal vector of the plane the face is in
		// this vector defines a plane through the origin (first vertex of
		// edge) and through both the edge vector and the normal vector
		// of the plane
		AAS_OrthogonalToVectors( edgevec, pnormal, sepnormal );

		// check on wich side of the above plane the point is
		// this is done by checking the sign of the dot product of the
		// vector orthogonal vector from above and the vector from the
		// origin (first vertex of edge) to the point
		// if the dotproduct is smaller than zero the point is outside the face
		if( DotProduct( pointvec, sepnormal ) < -epsilon ) {
			return qfalse;
		}
	}

	return qtrue;
}

/*!
	\brief Determines whether a point lies inside the polygonal area of a specified face, using an epsilon tolerance.

	The function first checks that the AAS world data is loaded, returning false otherwise. It then retrieves the face and its associated plane. For each edge of the face, it constructs the vector
   along the edge and a vector from the edge's first vertex to the test point. By computing the cross product of the edge vector and the plane normal, a separator normal is produced. The dot product
   of the point vector with this separator normal is compared against -epsilon: if it is less, the point is outside the face by more than the allowed tolerance and the function returns false
   immediately. If all edges pass this test, the point is considered to be inside the face and the function returns true.

	\param facenum Index of the face to test; must be a non‑negative face index, as negative values would refer to mirrored faces and are not handled by the function.
	\param point Coordinates of the point to check against the face's polygon.
	\param epsilon Margin of tolerance; points that lie within epsilon of the face’s edge on the outside are treated as inside.
	\return qtrue if the point is inside the face (or within epsilon of being inside); otherwise qfalse.
*/
qboolean AAS_PointInsideFace( int facenum, vec3_t point, float epsilon )
{
	int			 i, firstvertex, edgenum;
	vec_t *		 v1, *v2;
	vec3_t		 edgevec, pointvec, sepnormal;
	aas_edge_t*	 edge;
	aas_plane_t* plane;
	aas_face_t*	 face;

	if( !( *aasworld ).loaded ) {
		return qfalse;
	}

	face  = &( *aasworld ).faces[facenum];
	plane = &( *aasworld ).planes[face->planenum];

	//
	for( i = 0; i < face->numedges; i++ ) {
		edgenum = ( *aasworld ).edgeindex[face->firstedge + i];
		edge	= &( *aasworld ).edges[abs( edgenum )];
		// get the first vertex of the edge
		firstvertex = edgenum < 0;
		v1			= ( *aasworld ).vertexes[edge->v[firstvertex]];
		v2			= ( *aasworld ).vertexes[edge->v[!firstvertex]];
		// edge vector
		VectorSubtract( v2, v1, edgevec );
		// vector from first edge point to point possible in face
		VectorSubtract( point, v1, pointvec );
		//
		CrossProduct( edgevec, plane->normal, sepnormal );

		//
		if( DotProduct( pointvec, sepnormal ) < -epsilon ) {
			return qfalse;
		}
	}

	return qtrue;
}

/*!
	\brief Locate the ground face that lies beneath a given point within a specific area.

	The function first verifies that the navigation world data has been loaded. It then retrieves the area corresponding to <areanum> and iterates over all its faces. For each face flagged as a ground
   face, it determines the orientation of the face’s up normal based on its plane and tests whether the point falls inside the face polygon within a small tolerance using AAS_InsideFace. If a matching
   face is found, a pointer to that face is returned immediately. If no such face exists or the world is not loaded, the function returns null.

	\param areanum Index of the navigation area to search
	\return Pointer to the ground face containing the point, or null if none is found.
*/
aas_face_t* AAS_AreaGroundFace( int areanum, vec3_t point )
{
	int			i, facenum;
	vec3_t		up = { 0, 0, 1 };
	vec3_t		normal;
	aas_area_t* area;
	aas_face_t* face;

	if( !( *aasworld ).loaded ) {
		return NULL;
	}

	area = &( *aasworld ).areas[areanum];

	for( i = 0; i < area->numfaces; i++ ) {
		facenum = ( *aasworld ).faceindex[area->firstface + i];
		face	= &( *aasworld ).faces[abs( facenum )];

		// if this is a ground face
		if( face->faceflags & FACE_GROUND ) {
			// get the up or down normal
			if( ( *aasworld ).planes[face->planenum].normal[2] < 0 ) {
				VectorNegate( up, normal );

			} else {
				VectorCopy( up, normal );
			}

			// check if the point is in the face
			if( AAS_InsideFace( face, normal, point, 0.01 ) ) {
				return face;
			}
		}
	}

	return NULL;
}

void AAS_FacePlane( int facenum, vec3_t normal, float* dist )
{
	aas_plane_t* plane;

	plane = &( *aasworld ).planes[( *aasworld ).faces[facenum].planenum];
	VectorCopy( plane->normal, normal );
	*dist = plane->dist;
}

/*!
	\brief determines the navigation face that contains a trace’s end position

	The function first checks that a navigation world has been loaded and that the trace did not start inside a solid region; otherwise it returns null. It then uses the trace’s last area to iterate
   over all faces belonging to that area. For each face, it compares the face’s plane number (ignoring the side bit) with the trace’s plane number; if they match it tests whether the trace’s end
   position lies inside the face using a small tolerance of 0.01. The first face that contains the point is returned. The function does not perform the commented out optimization that prefers faces
   with fewer edges, so it simply returns the first matching face or null if none are found.

	\param trace the trace structure containing the end position, the last area visited, the plane number of the last hit plane, and a flag indicating if the trace started inside a solid region.
	\return a pointer to the navigation face that encloses the trace’s end position, or null if no such face exists
*/
aas_face_t* AAS_TraceEndFace( aas_trace_t* trace )
{
	int			i, facenum;
	aas_area_t* area;
	aas_face_t *face, *firstface = NULL;

	if( !( *aasworld ).loaded ) {
		return NULL;
	}

	// if started in solid no face was hit
	if( trace->startsolid ) {
		return NULL;
	}

	// trace->lastarea is the last area the trace was in
	area = &( *aasworld ).areas[trace->lastarea];

	// check which face the trace.endpos was in
	for( i = 0; i < area->numfaces; i++ ) {
		facenum = ( *aasworld ).faceindex[area->firstface + i];
		face	= &( *aasworld ).faces[abs( facenum )];

		// if the face is in the same plane as the trace end point
		if( ( face->planenum & ~1 ) == ( trace->planenum & ~1 ) ) {
			// firstface is used for optimization, if theres only one
			// face in the plane then it has to be the good one
			// if there are more faces in the same plane then always
			// check the one with the fewest edges first
			/*			if (firstface)
						{
							if (firstface->numedges < face->numedges)
							{
								if (AAS_InsideFace(firstface,
									(*aasworld).planes[face->planenum].normal, trace->endpos))
								{
									return firstface;
								}
								firstface = face;
							}
							else
							{
								if (AAS_InsideFace(face,
									(*aasworld).planes[face->planenum].normal, trace->endpos))
								{
									return face;
								}
							}
						}
						else
						{
							firstface = face;
						} //end else*/
			if( AAS_InsideFace( face, ( *aasworld ).planes[face->planenum].normal, trace->endpos, 0.01 ) ) {
				return face;
			}
		}
	}

	return firstface;
}

/*!
	\brief Returns a bit mask indicating on which side(s) a bounding box lies relative to a plane.

	The function first selects two opposite vertices of the box that are most likely to be on opposite sides of the plane by checking the sign of each normal component. It then computes the signed
distance from each vertex to the plane. If the first distance is non‑negative, the box touches or lies on the front side of the plane and the mask includes the front flag (1). If the second distance
is negative, the box also touches or lies on the back side, adding the back flag (2). The resulting mask therefore can be 1 (only front), 2 (only back), or 3 (both sides).

	\param absmins Minimum corner of the axis‑aligned bounding box
(absmins[i] is the smallest extent in dimension i).
	\param absmaxs Maximum corner of the axis‑aligned bounding box
(absmaxs[i] is the largest extent in dimension i).
	\param p Pointer to an AAS plane structure that contains a normal vector and a distance value used to define the plane equation.
	\return An integer bit mask: 1 means the box touches or lies on the plane’s front side, 2 means it touches or lies on the back side, and 3 indicates that the box straddles the plane.
*/
int AAS_BoxOnPlaneSide2( vec3_t absmins, vec3_t absmaxs, aas_plane_t* p )
{
	int	   i, sides;
	float  dist1, dist2;
	vec3_t corners[2];

	for( i = 0; i < 3; i++ ) {
		if( p->normal[i] < 0 ) {
			corners[0][i] = absmins[i];
			corners[1][i] = absmaxs[i];

		} else {
			corners[1][i] = absmins[i];
			corners[0][i] = absmaxs[i];
		}
	}

	dist1 = DotProduct( p->normal, corners[0] ) - p->dist;
	dist2 = DotProduct( p->normal, corners[1] ) - p->dist;
	sides = 0;

	if( dist1 >= 0 ) {
		sides = 1;
	}

	if( dist2 < 0 ) {
		sides |= 2;
	}

	return sides;
}

// int AAS_BoxOnPlaneSide(vec3_t absmins, vec3_t absmaxs, aas_plane_t *p)
#define AAS_BoxOnPlaneSide( absmins, absmaxs, p )                                                                                                       \
	( ( ( p )->type < 3 ) ? ( ( ( p )->dist <= ( absmins )[( p )->type] ) ? ( 1 ) : ( ( ( p )->dist >= ( absmaxs )[( p )->type] ) ? ( 2 ) : ( 3 ) ) ) : \
							( AAS_BoxOnPlaneSide2( ( absmins ), ( absmaxs ), ( p ) ) ) )
/*!
	\brief removes an entity's area links from the global area linked lists

	Iterates through the entity's linked list of area references. For each link, it updates the previous and next pointers of neighboring entities in that area to bypass the current link, and if the
   link was the first in the list it updates the area's linked list head. After clearing the link from the area lists the link structure is freed.

	\param areas pointer to the head of the entity's linked list of area references
*/
void AAS_UnlinkFromAreas( aas_link_t* areas )
{
	aas_link_t *link, *nextlink;

	for( link = areas; link; link = nextlink ) {
		// next area the entity is linked in
		nextlink = link->next_area;

		// remove the entity from the linked list of this area
		if( link->prev_ent ) {
			link->prev_ent->next_ent = link->next_ent;

		} else {
			( *aasworld ).arealinkedentities[link->areanum] = link->next_ent;
		}

		if( link->next_ent ) {
			link->next_ent->prev_ent = link->prev_ent;
		}

		// deallocate the link structure
		AAS_DeAllocAASLink( link );
	}
}

//===========================================================================
// link the entity to the areas the bounding box is totally or partly
// situated in. This is done with recursion down the tree using the
// bounding box to test for plane sides
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================

typedef struct {
	int nodenum; // node found after splitting
} aas_linkstack_t;

/*!
	\brief Links an entity into all AAS areas intersected by its bounding box and returns the head of the resulting list of area links.

	The function traverses the AAS node tree using an explicit stack to locate all leaf nodes whose area intersects the provided bounding box. For each intersecting area, it creates an aas_link_t
   structure and inserts it into the doubly linked entity list of the area and into the singly linked list of area links belonging to the entity. The function ensures that duplicate links for the same
   area are not created. Solid leaf nodes are ignored, and when the world is not loaded an error is printed and a NULL pointer is returned. If memory allocation for a link fails, the already gathered
   links are returned. The returned list is linked via the next_area member and can be iterated by callers. The function modifies global area data structures. Stack usage limits are checked and an
   error is printed if overflow would occur. The entity number may be a valid entity or an invalid value such as -1 used for temporary purposes. This routine is not thread safe and should be called in
   a single‑threaded context.


	\param absmins Box minimum corner in world coordinates
	\param absmaxs Box maximum corner in world coordinates
	\param entnum Entity number to link; may be an invalid value such as -1
	\return Pointer to the first aas_link_t of the linked list for the entity, or NULL if the world is not loaded or an error occurs.
*/
aas_link_t* AAS_AASLinkEntity( vec3_t absmins, vec3_t absmaxs, int entnum )
{
	int				 side, nodenum;
	aas_linkstack_t	 linkstack[128];
	aas_linkstack_t* lstack_p;
	aas_node_t*		 aasnode;
	aas_plane_t*	 plane;
	aas_link_t *	 link, *areas;

	if( !( *aasworld ).loaded ) {
		botimport.Print( PRT_ERROR, "AAS_LinkEntity: aas not loaded\n" );
		return NULL;
	}

	areas = NULL;
	//
	lstack_p = linkstack;
	// we start with the whole line on the stack
	// start with node 1 because node zero is a dummy used for solid leafs
	lstack_p->nodenum = 1; // starting at the root of the tree
	lstack_p++;

	while( 1 ) {
		// pop up the stack
		lstack_p--;

		// if the trace stack is empty (ended up with a piece of the
		// line to be traced in an area)
		if( lstack_p < linkstack ) {
			break;
		}

		// number of the current node to test the line against
		nodenum = lstack_p->nodenum;

		// if it is an area
		if( nodenum < 0 ) {
			// NOTE: the entity might have already been linked into this area
			//  because several node children can point to the same area
			for( link = ( *aasworld ).arealinkedentities[-nodenum]; link; link = link->next_ent ) {
				if( link->entnum == entnum ) {
					break;
				}
			}

			if( link ) {
				continue;
			}

			//
			link = AAS_AllocAASLink();

			if( !link ) {
				return areas;
			}

			link->entnum  = entnum;
			link->areanum = -nodenum;
			// put the link into the double linked area list of the entity
			link->prev_area = NULL;
			link->next_area = areas;

			if( areas ) {
				areas->prev_area = link;
			}

			areas = link;
			// put the link into the double linked entity list of the area
			link->prev_ent = NULL;
			link->next_ent = ( *aasworld ).arealinkedentities[-nodenum];

			if( ( *aasworld ).arealinkedentities[-nodenum] ) {
				( *aasworld ).arealinkedentities[-nodenum]->prev_ent = link;
			}

			( *aasworld ).arealinkedentities[-nodenum] = link;
			//
			continue;
		}

		// if solid leaf
		if( !nodenum ) {
			continue;
		}

		// the node to test against
		aasnode = &( *aasworld ).nodes[nodenum];
		// the current node plane
		plane = &( *aasworld ).planes[aasnode->planenum];
		// get the side(s) the box is situated relative to the plane
		side = AAS_BoxOnPlaneSide2( absmins, absmaxs, plane );

		// if on the front side of the node
		if( side & 1 ) {
			lstack_p->nodenum = aasnode->children[0];
			lstack_p++;
		}

		if( lstack_p >= &linkstack[127] ) {
			botimport.Print( PRT_ERROR, "AAS_LinkEntity: stack overflow\n" );
			break;
		}

		// if on the back side of the node
		if( side & 2 ) {
			lstack_p->nodenum = aasnode->children[1];
			lstack_p++;
		}

		if( lstack_p >= &linkstack[127] ) {
			botimport.Print( PRT_ERROR, "AAS_LinkEntity: stack overflow\n" );
			break;
		}
	}

	return areas;
}

/*!
	\brief Links an entity’s bounding box, adjusted for a specific presence type, to the AAS area graph.

	The function first obtains the bounding offsets for the specified presence type via AAS_PresenceTypeBoundingBox. It then expands the supplied absolute bounding coordinates by these offsets to
   create a new set of bounds that represent the space that the entity will occupy in the AAS system. Finally, it calls AAS_AASLinkEntity with the expanded bounds and the entity number, which performs
   the actual insertion into the area graph and returns a linked list of aas_link_t structures that represent the areas the entity now occupies.

	If the entity is not located within any relevant AAS area, AAS_AASLinkEntity returns NULL and this function propagates that value. The caller may later call AAS_UnlinkFromAreas to remove the
   linking or use the return value to iterate over the affected areas.

	Parameters are expected to be absolute values; the calling code typically passes the entity’s world-space mins/maxs after adding its current origin. The presence type indicates how large the
   agent’s bounding box should be considered (e.g., normal posture, crouch, or other special states).

	\param absmins absolute minimum coordinates of the entity’s bounding box
	\param absmaxs absolute maximum coordinates of the entity’s bounding box
	\param entnum entity number used for linking; typically the entity’s identifier but can be -1 for special triggers
	\param presencetype integer representing the presence type; valid values are those accepted by AAS_PresenceTypeBoundingBox
	\return Pointer to the first aas_link_t of the linked areas or NULL if the entity is not in any area.
*/
aas_link_t* AAS_LinkEntityClientBBox( vec3_t absmins, vec3_t absmaxs, int entnum, int presencetype )
{
	vec3_t mins, maxs;
	vec3_t newabsmins, newabsmaxs;

	AAS_PresenceTypeBoundingBox( presencetype, mins, maxs );
	VectorSubtract( absmins, maxs, newabsmins );
	VectorSubtract( absmaxs, mins, newabsmaxs );
	// relink the entity
	return AAS_AASLinkEntity( newabsmins, newabsmaxs, entnum );
}

/*!
	\brief Return a pointer to the plane indicated by planenum, or NULL if the world is not loaded.

	The function first checks whether the global aasworld structure has been loaded. If it hasn't, NULL is returned. Otherwise it returns a pointer to the aas_plane_t object located at the provided
   planenum index within the aasworld.plan arrays. The caller can then examine the plane's properties such as its normal vector. This function does not perform range checking on planenum, so the
   caller must ensure the index is valid.

	\param planenum The index of the plane within the global planes array.
	\return A pointer to the corresponding aas_plane_t, or NULL if the world has not been loaded.
*/
aas_plane_t* AAS_PlaneFromNum( int planenum )
{
	if( !( *aasworld ).loaded ) {
		return NULL;
	}

	return &( *aasworld ).planes[planenum];
}

int AAS_BBoxAreas( vec3_t absmins, vec3_t absmaxs, int* areas, int maxareas )
{
	aas_link_t *linkedareas, *link;
	int			num;

	linkedareas = AAS_AASLinkEntity( absmins, absmaxs, -1 );
	num			= 0;

	for( link = linkedareas; link; link = link->next_area ) {
		areas[num] = link->areanum;
		num++;

		if( num >= maxareas ) {
			break;
		}
	}

	AAS_UnlinkFromAreas( linkedareas );
	return num;
}
