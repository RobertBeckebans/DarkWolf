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
	\file engine/botlib/be_aas_reach.cpp
	\brief Implement all logic for calculating reachability links used by bots to plan paths across an AAS area network.
	\note doxygenix: sha256=a581e1a508fafd993ac052a8346dc6bd575af4355a59659d01e3d437114b9f7a

	\par File Purpose
	- Implement all logic for calculating reachability links used by bots to plan paths across an AAS area network.
	- Provide geometry helpers that determine whether a bot can move from one area to another using various movement primitives.
	- Expose API functions such as AAS_BestReachableArea, AAS_ReachabilityExists and the incremental init routine AAS_ContinueInitReachability.

	\par Core Responsibilities
	- Build the reach‑ability graph of the AAS navigation mesh, representing all viable one‑off movements between navigation areas.
	- Compute per‑area travel links for various traversal types (walk, swim, jump, ladder, gravity‑affected moves, teleport, elevator, function‑bobbing, etc.).
	- Allocate, reuse and free temporary reachability objects from a fixed heap for memory efficiency.
	- Provide utility queries for area properties (grounded, crouch, liquid, ladder, jump‑pad, etc.) used during link construction.
	- Store the finalized reachabilities into the AAS world structure for fast access by the bot navigation code.

	\par Key Types and Functions
	- aas_lreachability_t ‑ temporary reach‑link structure used during graph construction.
	- AAS_BestReachableLinkArea(aas_link_t* areas) ‑ selects the best candidate area from a linked list.
	- AAS_BestReachableArea(vec3_t origin, vec3_t mins, vec3_t maxs, vec3_t goalorigin) ‑ finds a reachable area for a bounding box.
	- AAS_SetupReachabilityHeap() ‑ pre‑allocates the memory pool for aas_lreachability_t.
	- AAS_ShutDownReachabilityHeap() ‑ frees the heap.
	- AAS_AllocReachability() ‑ pulls a link from the free pool.
	- AAS_FreeReachability(aas_lreachability_t* lreach) ‑ returns a link to the pool.
	- AAS_AreaReachability(int areanum) ‑ returns how many links an area currently has.
	- AAS_FaceArea(aas_face_t* face) ‑ computes the area of a face.
	- AAS_AreaVolume(int areanum) ‑ calculates the convex-hull volume of an area.
	- AAS_AreaGroundFaceArea(int areanum) ‑ sums the areas of ground faces of an area.
	- AAS_FaceCenter(int facenum, vec3_t center) ‑ arithmetic mean of a face’s vertices.
	- AAS_FallDamageDistance() ‑ maximum height before fall damage.
	- AAS_FallDelta(float distance) ‑ damage factor from falling a given distance.
	- AAS_MaxJumpHeight(float sv_jumpvel) ‑ theoretical peak height for a given jump velocity.
	- AAS_MaxJumpDistance(float sv_jumpvel) ‑ maximum horizontal distance attainable in a jump.
	- AAS_AreaCrouch(int areanum) ‑ whether the area contains only crouch presence.
	- AAS_AreaSwim(int areanum), AAS_AreaLiquid(aasworld, areanum), AAS_AreaLava(aasworld, areanum), AAS_AreaSlime(aasworld, areanum), AAS_AreaGrounded(aasworld, areanum), AAS_AreaLadder(aasworld, areanum), AAS_AreaJumpPad(aasworld, areanum), AAS_AreaTeleporter(aasworld, areanum), AAS_AreaDoNotEnter(aasworld, areanum), AAS_AreaDoNotEnterLarge(aasworld, areanum) ‑ various presence & content checks.
	- AAS_BarrierJumpTravelTime() ‑ computes time for a barrier jump.
	- AAS_ReachabilityExists(int area1num, int area2num) ‑ whether a reach link already exists.
	- AAS_NearbySolidOrGap(vec3_t start, vec3_t end) ‑ tests for obstacles or gaps ahead.
	- AAS_Reachability_Swim(int area1num, int area2num) ‑ creates swim links between adjacent swim areas.
	- AAS_Reachability_EqualFloorHeight(int area1num, int area2num) ‑ links ground areas with equal floor height.
	- AAS_Reachability_Step_Barrier_WaterJump_WalkOffLedge(int area1num, int area2num) ‑ links for step, barrier, water‑jump, and walk‑off‑ledge moves.
	- AAS_Reachability_Ladder(int area1num, int area2num) ‑ links where ladder faces share an edge.
	- AAS_Reachability_Teleport() ‑ links between trigger_teleport and target_teleporter destinations.
	- AAS_Reachability_Elevator() ‑ links for func_plat elevator movement.
	- AAS_FindFaceReachabilities(vec3_t* facepoints, int numpoints, aas_plane_t* plane, int towardsface) ‑ helper for func_bobbing.
	- AAS_Reachability_FuncBobbing() ‑ processes func_bobbing entities to create links.
	- AAS_Reachability_JumpPad() ‑ handles trigger_push / jumppad reachabilities.
	- AAS_Reachability_Grapple(int area1num, int area2num) ‑ creates grapple‑hook links (currently disabled).
	- AAS_SetWeaponJumpAreaFlags() ‑ marks areas that can be weapon‑jumped to.
	- AAS_Reachability_WeaponJump(int area1num, int area2num) ‑ creates rocket/bfg jump links to weapon‑jump areas.
	- AAS_Reachability_WalkOffLedge(int areanum) ‑ adds walk‑off‑ledge links for grounded areas.
	- AAS_StoreReachability() ‑ copies the temporary linked list into the final linear reachability array in aasworld.
	- AAS_ContinueInitReachability(float time) ‑ incremental construction loop; returns qtrue until all reachabilities are built.
	- AAS_InitReachability() ‑ entry point that sets everything up and triggers the incremental loop.

	\par Control Flow
	- When an AAS world is loaded, AAS_InitReachability initializes structures, the heap, per‑area reachability pointers, and may set a savefile flag for incremental building.
	- AAS_ContinueInitReachability is repeatedly called with a time budget; it iterates over unprocessed areas and for each distinct pair (i,j) attempts to create reachability links for movement types such as swim, equal‑floor walk, step/​barrier/​water‑jump/​walk‑off‑ledge, ladder, jump, and subsequently grapple/​weapon‑jump (currently disabled).
	- If the time slice expires or the progress percentage changes, the function returns qtrue to request another iteration; once all links are built it stores them with AAS_StoreReachability, frees the heap, and initiates cluster calculation.
	- The construction uses AAS_AllocReachability / AAS_FreeReachability to manage a pre‑allocated pool, and geometry checks like AAS_PointAreaNum, AAS_TraceClientBBox, and face‑area calculations to validate feasibility.

	\par Dependencies
	- "q_shared.h", "l_log.h", "l_memory.h", "l_script.h", "l_libvar.h", "l_precomp.h", "l_struct.h"
	- "aasfile.h", "botshared/botlib.h", "botshared/be_aas.h", "be_aas_funcs.h", "be_aas_def.h"
	- "botimport" global table for printing, logging and Sys_MilliSeconds()
	- "aassettings" for gravity, jump velocity, step height and other engine constants

	\par How It Fits
	- This file is part of the bot library subsystem; it transforms static level geometry into a graph of actionable moves for the AI path‑finder.
	- It interfaces with the AAS world data generated by the level loader and with the main bot code through exported functions.
	- The reach‑ability graph is the foundation for all higher‑level navigation: after building the links, the AI can perform a graph search (e.g., Dijkstra/A*).
*/

/*****************************************************************************
 * name:		be_aas_reach.c
 *
 * desc:		reachability calculations
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_log.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_libvar.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "botshared/botlib.h"
#include "botshared/be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

extern int			   Sys_MilliSeconds();

// #include "../../../gladiator/bspc/aas_store.h"

extern botlib_import_t botimport;

// #define REACHDEBUG

// NOTE: all travel times are in hundreth of a second
// maximum fall delta before getting damaged

// Ridah, tweaked for Wolf AI
#define FALLDELTA_5DAMAGE			25 // 40
#define FALLDELTA_10DAMAGE			40 // 60
// done.

// maximum number of reachability links
#define AAS_MAX_REACHABILITYSIZE	65536
// number of areas reachability is calculated for each frame
#define REACHABILITYAREASPERCYCLE	15
// number of units reachability points are placed inside the areas
#define INSIDEUNITS					2

// Ridah, tweaked this, routing issues around small areas
#define INSIDEUNITS_WALKEND			5 // original
// #define INSIDEUNITS_WALKEND					0.2		// new

// Ridah, added this for better walking off ledges
#define INSIDEUNITS_WALKOFFLEDGEEND 15

#define INSIDEUNITS_WALKSTART		0.1
#define INSIDEUNITS_WATERJUMP		15
// travel times in hundreth of a second

// Ridah, tweaked these for Wolf AI
#define REACH_MIN_TIME				4	// always at least this much time for a reachability
#define WATERJUMP_TIME				700 // 7 seconds
#define TELEPORT_TIME				50	// 0.5 seconds
#define BARRIERJUMP_TIME			900 // fixed value?
#define STARTCROUCH_TIME			300 // 3 sec to start crouching
#define STARTGRAPPLE_TIME			500 // using the grapple costs a lot of time
#define STARTWALKOFFLEDGE_TIME		300 // 3 seconds
#define STARTJUMP_TIME				500 // 3 seconds for jumping

#define FALLDAMAGE_5_TIME			400 // extra travel time when falling hurts
#define FALLDAMAGE_10_TIME			900 // extra travel time when falling hurts
// done.

// maximum height the bot may fall down when jumping
#define MAX_JUMPFALLHEIGHT			450
// area flag used for weapon jumping
#define AREA_WEAPONJUMP				8192 // valid area to weapon jump to
// number of reachabilities of each type
int reach_swim;			// swim
int reach_equalfloor;	// walk on floors with equal height
int reach_step;			// step up
int reach_walk;			// walk of step
int reach_barrier;		// jump up to a barrier
int reach_waterjump;	// jump out of water
int reach_walkoffledge; // walk of a ledge
int reach_jump;			// jump
int reach_ladder;		// climb or descent a ladder
int reach_teleport;		// teleport
int reach_elevator;		// use an elevator
int reach_funcbob;		// use a func bob
int reach_grapple;		// grapple hook
int reach_doublejump;	// double jump
int reach_rampjump;		// ramp jump
int reach_strafejump;	// strafe jump (just normal jump but further)
int reach_rocketjump;	// rocket jump
int reach_bfgjump;		// bfg jump
int reach_jumppad;		// jump pads
// if true grapple reachabilities are skipped
int calcgrapplereach;
// linked reachability
typedef struct aas_lreachability_s {
	int							areanum;	// number of the reachable area
	int							facenum;	// number of the face towards the other area
	int							edgenum;	// number of the edge towards the other area
	vec3_t						start;		// start point of inter area movement
	vec3_t						end;		// end point of inter area movement
	int							traveltype; // type of travel required to get to the area
	unsigned short int			traveltime; // travel time of the inter area movement
	//
	struct aas_lreachability_s* next;
} aas_lreachability_t;

// temporary reachabilities
aas_lreachability_t*  reachabilityheap; // heap with reachabilities
aas_lreachability_t*  nextreachability; // next free reachability from the heap
aas_lreachability_t** areareachability; // reachability links for every area
int					  numlreachabilities;

/*!
	\brief Returns the number of the most suitable area from a list of candidate link areas.

	The function iterates over a singly linked list of aas_link_t structures. It first looks for an area that is either grounded or a swim area and returns that area's number when found. If no such
   area exists, it then searches for any non‑zero area number in the list and returns it. If the list contains no valid area numbers, the function returns 0. The area number 0 is used to indicate that
   no reachable area was found.

	\param areas pointer to a linked list of aas_link_t structures representing candidate areas for a reachability link.
	\return the area number, or 0 if no suitable area is found.
*/
int					  AAS_BestReachableLinkArea( aas_link_t* areas )
{
	aas_link_t* link;

	for( link = areas; link; link = link->next_area ) {
		if( AAS_AreaGrounded( link->areanum ) || AAS_AreaSwim( link->areanum ) ) {
			return link->areanum;
		}
	}

	//
	for( link = areas; link; link = link->next_area ) {
		if( link->areanum ) {
			return link->areanum;
		}

		// FIXME: cannot enable next line right now because the reachability
		//  does not have to be calculated when the level items are loaded
		// if (AAS_AreaReachability(link->areanum)) return link->areanum;
	}

	return 0;
}

int AAS_BestReachableArea( vec3_t origin, vec3_t mins, vec3_t maxs, vec3_t goalorigin )
{
	int			areanum, i, j, k, l;
	aas_link_t* areas;
	vec3_t		absmins, absmaxs;
	// vec3_t bbmins, bbmaxs;
	vec3_t		start, end;
	aas_trace_t trace;

	if( !( *aasworld ).loaded ) {
		botimport.Print( PRT_ERROR, "AAS_BestReachableArea: aas not loaded\n" );
		return 0;
	}

	// find a point in an area
	VectorCopy( origin, start );
	areanum = AAS_PointAreaNum( start );

	// while no area found fudge around a little
	for( i = 0; i < 5 && !areanum; i++ ) {
		for( j = 0; j < 5 && !areanum; j++ ) {
			for( k = -1; k <= 1 && !areanum; k++ ) {
				for( l = -1; l <= 1 && !areanum; l++ ) {
					VectorCopy( origin, start );
					start[0] += ( float )j * 4 * k;
					start[1] += ( float )j * 4 * l;
					start[2] += ( float )i * 4;
					areanum = AAS_PointAreaNum( start );
				}
			}
		}
	}

	// if an area was found
	if( areanum ) {
		// drop client bbox down and try again
		VectorCopy( start, end );
		start[2] += 0.25;
		end[2] -= 50;
		trace = AAS_TraceClientBBox( start, end, PRESENCE_CROUCH, -1 );

		if( !trace.startsolid ) {
			areanum = AAS_PointAreaNum( trace.endpos );
			VectorCopy( trace.endpos, goalorigin );

			// FIXME: cannot enable next line right now because the reachability
			//  does not have to be calculated when the level items are loaded
			// if the origin is in an area with reachability
			// if (AAS_AreaReachability(areanum)) return areanum;
			if( areanum ) {
				return areanum;
			}

		} else {
			// it can very well happen that the AAS_PointAreaNum function tells that
			// a point is in an area and that starting a AAS_TraceClientBBox from that
			// point will return trace.startsolid qtrue
			/*
			if (AAS_PointAreaNum(start))
			{
				Log_Write("point %f %f %f in area %d but trace startsolid", start[0], start[1], start[2], areanum);
				AAS_DrawPermanentCross(start, 4, LINECOLOR_RED);
			}
			botimport.Print(PRT_MESSAGE, "AAS_BestReachableArea: start solid\n");
			*/
			VectorCopy( start, goalorigin );
			return areanum;
		}
	}

	//
	// AAS_PresenceTypeBoundingBox(PRESENCE_CROUCH, bbmins, bbmaxs);
	// NOTE: the goal origin does not have to be in the goal area
	// because the bot will have to move towards the item origin anyway
	VectorCopy( origin, goalorigin );
	//
	VectorAdd( origin, mins, absmins );
	VectorAdd( origin, maxs, absmaxs );
	// add bounding box size
	// VectorSubtract(absmins, bbmaxs, absmins);
	// VectorSubtract(absmaxs, bbmins, absmaxs);
	// link an invalid (-1) entity
	areas = AAS_AASLinkEntity( absmins, absmaxs, -1 );
	// get the reachable link arae
	areanum = AAS_BestReachableLinkArea( areas );
	// unlink the invalid entity
	AAS_UnlinkFromAreas( areas );
	//
	return areanum;
}

/*!
	\brief Initializes the global reachability heap by allocating and linking a pool of reachability structures.

	The function allocates an array of aas_lreachability_t entries of size AAS_MAX_REACHABILITYSIZE, clears them, and chains them into a singly linked list through next pointers, with the last entry
   pointing to null. It then sets the global nextreachability pointer to the start of the list and resets the count of used reachabilities to zero. This establishes a pool from which reachability
   records can be allocated.

*/
void AAS_SetupReachabilityHeap()
{
	int i;

	reachabilityheap = ( aas_lreachability_t* )GetClearedMemory( AAS_MAX_REACHABILITYSIZE * sizeof( aas_lreachability_t ) );

	for( i = 0; i < AAS_MAX_REACHABILITYSIZE - 1; i++ ) {
		reachabilityheap[i].next = &reachabilityheap[i + 1];
	}

	reachabilityheap[AAS_MAX_REACHABILITYSIZE - 1].next = NULL;
	nextreachability									= reachabilityheap;
	numlreachabilities									= 0;
}

/*!
	\brief Frees the memory used for the reachability heap and resets the number of reachabilities to zero.

	The function calls FreeMemory on the global reachabilityheap pointer and sets numlreachabilities to zero, releasing all resources associated with the reachability heap.

*/
void AAS_ShutDownReachabilityHeap()
{
	FreeMemory( reachabilityheap );
	numlreachabilities = 0;
}

/*!
	\brief Allocates a reachability link from the internal pool and returns it.

	If the global pool variable nextreachability points to a valid element, the function removes that element from the pool and returns it. The pool pointer is advanced once the element is taken, and
   the global counter numlreachabilities is incremented. If no elements remain, the function returns NULL. The function also emits an allocation error message only once when the pool will become empty
   after the current allocation.

	\return a pointer to a aas_lreachability_t object if available, otherwise NULL
*/
aas_lreachability_t* AAS_AllocReachability()
{
	aas_lreachability_t* r;

	if( !nextreachability ) {
		return NULL;
	}

	// make sure the error message only shows up once
	if( !nextreachability->next ) {
		AAS_Error( "AAS_MAX_REACHABILITYSIZE" );
	}

	//
	r				 = nextreachability;
	nextreachability = nextreachability->next;
	numlreachabilities++;
	return r;
}

/*!
	\brief Recycles a reachability structure back into the global free list.

	The function clears the memory of the provided reachability structure, adds it to the global free list, and decrements the count of live reachabilities. This allows the structure to be reused
   later while keeping the global state consistent.

	\param lreach pointer to the reachability entry to be returned to the free list
*/
void AAS_FreeReachability( aas_lreachability_t* lreach )
{
	memset( lreach, 0, sizeof( aas_lreachability_t ) );

	lreach->next	 = nextreachability;
	nextreachability = lreach;
	numlreachabilities--;
}

int AAS_AreaReachability( int areanum )
{
	if( areanum < 0 || areanum >= ( *aasworld ).numareas ) {
		AAS_Error( "AAS_AreaReachability: areanum %d out of range", areanum );
		return 0;
	}

	// RF, if this area is disabled, then fail
	if( ( *aasworld ).areasettings[areanum].areaflags & AREA_DISABLED ) {
		return 0;
	}

	return ( *aasworld ).areasettings[areanum].numreachableareas;
}

/*!
	\brief Returns the surface area of the specified face.

	The function triangulates a face by taking the first vertex as a common point and creating triangles with each pair of successive vertices in the face. For every triangle it computes two edge
   vectors, takes their cross product, and adds half of the resulting vector’s length to a running total. This sum gives the total surface area of the face. The function expects a valid face pointer
   and relies on the global geometry stored in the aasworld object. It works for convex or concave polygons as long as the vertices are listed around the perimeter.

	\param face pointer to the face whose area is desired
	\return the computed area as a float
*/
float AAS_FaceArea( aas_face_t* face )
{
	int			i, edgenum, side;
	float		total;
	vec_t*		v;
	vec3_t		d1, d2, cross;
	aas_edge_t* edge;

	edgenum = ( *aasworld ).edgeindex[face->firstedge];
	side	= edgenum < 0;
	edge	= &( *aasworld ).edges[abs( edgenum )];
	v		= ( *aasworld ).vertexes[edge->v[side]];

	total = 0;

	for( i = 1; i < face->numedges - 1; i++ ) {
		edgenum = ( *aasworld ).edgeindex[face->firstedge + i];
		side	= edgenum < 0;
		edge	= &( *aasworld ).edges[abs( edgenum )];
		VectorSubtract( ( *aasworld ).vertexes[edge->v[side]], v, d1 );
		VectorSubtract( ( *aasworld ).vertexes[edge->v[!side]], v, d2 );
		CrossProduct( d1, d2, cross );
		total += 0.5 * VectorLength( cross );
	}

	return total;
}

/*!
	\brief Computes the volume of a 3D area by summing the volumes of tetrahedrons formed with a corner point and each face.

	The function selects a corner vertex from the first edge of the area, then iterates over all faces of the area. For each face, it calculates the distance from the corner to the face plane along
   the plane normal, multiplies that distance by the face's area, and adds the result to a running total. After all faces are processed, the sum is divided by three to obtain the volume of the convex
   hull defined by the faces and the selected corner.

	The calculation assumes that the area is defined by planar faces and that the area’s faces are oriented correctly so that the normal points outward. The result is the signed volume; if the faces
   are oriented consistently, the volume will be positive.

	\param areanum Index of the area whose volume is to be computed; must be valid within the global AAS world structure.
	\return The computed volume as a float.
*/
float AAS_AreaVolume( int areanum )
{
	int			 i, edgenum, facenum;
	vec_t		 d, a, volume;
	vec3_t		 corner;
	aas_plane_t* plane;
	aas_edge_t*	 edge;
	aas_face_t*	 face;
	aas_area_t*	 area;

	area	= &( *aasworld ).areas[areanum];
	facenum = ( *aasworld ).faceindex[area->firstface];
	face	= &( *aasworld ).faces[abs( facenum )];
	edgenum = ( *aasworld ).edgeindex[face->firstedge];
	edge	= &( *aasworld ).edges[abs( edgenum )];
	//
	VectorCopy( ( *aasworld ).vertexes[edge->v[0]], corner );

	// make tetrahedrons to all other faces
	volume = 0;

	for( i = 0; i < area->numfaces; i++ ) {
		facenum = abs( ( *aasworld ).faceindex[area->firstface + i] );
		face	= &( *aasworld ).faces[facenum];
		plane	= &( *aasworld ).planes[face->planenum];
		d		= -( DotProduct( corner, plane->normal ) - plane->dist );
		a		= AAS_FaceArea( face );
		volume += d * a;
	}

	volume /= 3;
	return volume;
}

float AAS_AreaGroundFaceArea( int areanum )
{
	int			i;
	float		total;
	aas_area_t* area;
	aas_face_t* face;

	total = 0;
	area  = &( *aasworld ).areas[areanum];

	for( i = 0; i < area->numfaces; i++ ) {
		face = &( *aasworld ).faces[abs( ( *aasworld ).faceindex[area->firstface + i] )];

		if( !( face->faceflags & FACE_GROUND ) ) {
			continue;
		}

		//
		total += AAS_FaceArea( face );
	}

	return total;
}

/*!
	\brief Calculates the geometric center of a face and stores it in the supplied vector.

	The function iterates over each edge of the specified face, accumulating the positions of the associated vertices. After summing all vertex positions, it scales the resulting vector by half the
   reciprocal of the number of edges, effectively computing the arithmetic mean of the vertices.

	It accesses the global AAS world data structure to locate the face, its edges, and the vertex positions. The result is written into the center parameter; the caller should provide a vector large
   enough to hold the result.

	Valid facenum values must correspond to an existing face index; passing an out-of-range index results in undefined behavior.

	\param facenum Index of the face whose center is to be computed.
	\param center Output vector where the computed center will be stored.
*/
void AAS_FaceCenter( int facenum, vec3_t center )
{
	int			i;
	float		scale;
	aas_face_t* face;
	aas_edge_t* edge;

	face = &( *aasworld ).faces[facenum];

	VectorClear( center );

	for( i = 0; i < face->numedges; i++ ) {
		edge = &( *aasworld ).edges[abs( ( *aasworld ).edgeindex[face->firstedge + i] )];
		VectorAdd( center, ( *aasworld ).vertexes[edge->v[0]], center );
		VectorAdd( center, ( *aasworld ).vertexes[edge->v[1]], center );
	}

	scale = 0.5 / face->numedges;
	VectorScale( center, scale, center );
}

/*!
	\brief Computes the maximum fall distance a player can travel before falling damage is applied in the AAS system.

	The function determines the distance required for a free‑fall to reach a velocity of sqrt(30 × 10 000), which represents the velocity threshold at which fall damage would be incurred according to
   the damage formula damage = deltaV² × 0.0001. It uses the current gravity value from aassettings.sv_gravity, calculates the falling time to reach that velocity, and then applies the kinematic
   equation d = ½ g t². The resulting floating‑point distance is truncated to an integer for the return value.

	\return The truncated maximum drop distance, in game units, before fall damage is triggered.
*/
int AAS_FallDamageDistance()
{
	float maxzvelocity, gravity, t;

	maxzvelocity = sqrt( 30 * 10000 );
	gravity		 = aassettings.sv_gravity;
	t			 = maxzvelocity / gravity;
	return 0.5 * gravity * t * t;
}

/*!
	\brief Calculates a damage factor based on a fall distance using the current gravity setting.

	The fall distance argument is interpreted as the vertical distance a character falls.  The function first computes the time it would take to fall that distance under constant gravity (distance =
   0.5 * g * t^2).  It then derives the impact velocity as g * t and returns the damage delta which is velocity squared multiplied by 0.0001, a scaling used by the game engine.  The gravity value used
   is taken from the global settings aassettings.sv_gravity, ensuring consistency with other physics calculations.

	\param distance vertical distance fallen (positive or negative), in game units.
	\return damage delta proportional to the square of the impact velocity; typically a small float used to augment travel time or damage calculations.
*/
float AAS_FallDelta( float distance )
{
	float t, delta, gravity;

	gravity = aassettings.sv_gravity;
	t		= sqrt( fabs( distance ) * 2 / gravity );
	delta	= t * gravity;
	return delta * delta * 0.0001;
}

/*!
	\brief Computes the maximum vertical height achievable from a given initial jump velocity using the current engine gravity.

	The function retrieves the global gravity value from the AAS settings structure. It then applies basic projectile physics to calculate the peak height reached by a player starting with the
   supplied upward velocity: height = velocity^2 / (2 * gravity). This result is used by various reachability routines to test whether a target area can be reached by a jump.

	\param sv_jumpvel Initial upward velocity applied at the start of the jump.
	\return The theoretical maximum height (in world units) that can be reached with the specified initial velocity.
*/
float AAS_MaxJumpHeight( float sv_jumpvel )
{
	float sv_gravity;

	sv_gravity = aassettings.sv_gravity;
	// maximum height a player can jump with the given initial z velocity
	return 0.5 * sv_gravity * ( sv_jumpvel / sv_gravity ) * ( sv_jumpvel / sv_gravity );
}

/*!
	\brief Computes the horizontal distance a player can clear when jumping.

	The distance depends on the configured gravity, maximum horizontal speed, and the initial upward velocity supplied to the function. It first estimates the time it takes to fall down a predefined
   maximum jump fall height, then adds the time contributed by the vertical jump velocity. The product of this total time with the maximum horizontal speed yields the result, expressed in world units.

	\param sv_jumpvel initial upward velocity of the jump, as defined in the server settings.
	\return maximum horizontal distance, in game units, a player can cover in a single jump.
*/
float AAS_MaxJumpDistance( float sv_jumpvel )
{
	float sv_gravity, sv_maxvelocity, t;

	sv_gravity	   = aassettings.sv_gravity;
	sv_maxvelocity = aassettings.sv_maxvelocity;
	// time a player takes to fall the height
	t = sqrt( MAX_JUMPFALLHEIGHT / ( 0.5 * sv_gravity ) );
	// maximum distance
	return sv_maxvelocity * ( t + sv_jumpvel / sv_gravity );
}

int AAS_AreaCrouch( int areanum )
{
	if( !( ( *aasworld ).areasettings[areanum].presencetype & PRESENCE_NORMAL ) ) {
		return qtrue;

	} else {
		return qfalse;
	}
}

int AAS_AreaSwim( int areanum )
{
	if( ( *aasworld ).areasettings[areanum].areaflags & AREA_LIQUID ) {
		return qtrue;

	} else {
		return qfalse;
	}
}

int AAS_AreaLiquid( int areanum )
{
	if( ( *aasworld ).areasettings[areanum].areaflags & AREA_LIQUID ) {
		return qtrue;

	} else {
		return qfalse;
	}
}

int AAS_AreaLava( int areanum )
{
	return ( ( *aasworld ).areasettings[areanum].contents & AREACONTENTS_LAVA );
}

int AAS_AreaSlime( int areanum )
{
	return ( ( *aasworld ).areasettings[areanum].contents & AREACONTENTS_SLIME );
}

int AAS_AreaGrounded( int areanum )
{
	return ( ( *aasworld ).areasettings[areanum].areaflags & AREA_GROUNDED );
}

int AAS_AreaLadder( int areanum )
{
	return ( ( *aasworld ).areasettings[areanum].areaflags & AREA_LADDER );
}

int AAS_AreaJumpPad( int areanum )
{
	return ( ( *aasworld ).areasettings[areanum].contents & AREACONTENTS_JUMPPAD );
}

/*!
	\brief Determines whether a given area is marked as a teleporter.

	Accesses the global world data to inspect the contents flag of the specified area. The content flag AREACONTENTS_TELEPORTER is checked via a bitwise AND operation; a non‑zero result indicates the
   area is a teleporter, whereas zero means it is not. This check is used in reachability calculations to identify teleporter or jump locations.

	\param areanum Index of the area to test
	\return Non‑zero when the area contains a teleporter flag, zero otherwise
*/
int AAS_AreaTeleporter( int areanum )
{
	return ( ( *aasworld ).areasettings[areanum].contents & AREACONTENTS_TELEPORTER );
}

int AAS_AreaDoNotEnter( int areanum )
{
	return ( ( *aasworld ).areasettings[areanum].contents & AREACONTENTS_DONOTENTER );
}

int AAS_AreaDoNotEnterLarge( int areanum )
{
	return ( ( *aasworld ).areasettings[areanum].contents & AREACONTENTS_DONOTENTER_LARGE );
}

/*!
	\brief Returns the time required for a barrier jump.

	The travel time is calculated by dividing the jump velocity by ten times the gravity constant, producing a value that represents the duration of a barrier jump in game time units. It is used as
   the traversal time for barrier‑jump reachabilities.

	\return duration in game units the barrier jump takes.
*/
unsigned short int AAS_BarrierJumpTravelTime()
{
	return aassettings.sv_jumpvel / ( aassettings.sv_gravity * 0.1 );
}

/*!
	\brief Checks whether a reachability link already exists from one area to another in the navigation graph.

	This function iterates through the linked list of reachabilities that originate from the source area. If any of those reachabilities point to the target area, the function returns qtrue.
   Otherwise, it returns qfalse. It is commonly used by path‑finding and area traversal logic to prevent duplicate links being added.

	\param area1num The numeric identifier of the source area.
	\param area2num The numeric identifier of the target area.
	\return qtrue if a reachability from area1 to area2 exists, otherwise qfalse.
*/
qboolean AAS_ReachabilityExists( int area1num, int area2num )
{
	aas_lreachability_t* r;

	for( r = areareachability[area1num]; r; r = r->next ) {
		if( r->areanum == area2num ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*!
	\brief Detects whether a solid obstacle or empty space lies immediately beyond a line from start to end.

	The function projects a horizontal line from the end point toward the start point, zeroing the vertical component of the direction vector and normalizing it. It then evaluates two test points
   ahead of the end point.

	First, it checks a point 48 units along the direction. If that point is not inside any area, the test point is raised 16 units; if it remains outside any area a gap is detected and the function
   returns true.

	Next, it checks a point 64 units along the direction. If an area is found there and that area is neither a swimming nor a grounded area, it is treated as an obstacle, causing the function to
   return true.

	If neither of these conditions is met, the function concludes that there is no nearby solid or gap and returns false.

	\param start The starting point of the line segment being tested.
	\param end The ending point of the line segment being tested.
	\return Non‑zero (qtrue) if a solid or a gap is found immediately beyond the end point; zero (qfalse) otherwise.
*/
int AAS_NearbySolidOrGap( vec3_t start, vec3_t end )
{
	vec3_t dir, testpoint;
	int	   areanum;

	VectorSubtract( end, start, dir );
	dir[2] = 0;
	VectorNormalize( dir );
	VectorMA( end, 48, dir, testpoint );

	areanum = AAS_PointAreaNum( testpoint );

	if( !areanum ) {
		testpoint[2] += 16;
		areanum = AAS_PointAreaNum( testpoint );

		if( !areanum ) {
			return qtrue;
		}
	}

	VectorMA( end, 64, dir, testpoint );
	areanum = AAS_PointAreaNum( testpoint );

	if( areanum ) {
		if( !AAS_AreaSwim( areanum ) && !AAS_AreaGrounded( areanum ) ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*!
	\brief Creates and links a swim reachability between two adjacent swim‑capable areas when they share a face.

	The function first verifies that both areas are swim‑capable and that the destination area allows normal presence. It then ensures the two areas are within a small spatial buffer so they are
   considered adjacent. For each face of the first area, it looks for a matching face on the second area. When a common face is found, the face center’s content is checked to be water, lava or slime;
   if so, a new reachability record is allocated. The record contains the target area number, the shared face, an edge number of 0, the start position at the face center and an end position slightly
   inside the target area. The travel type is marked as swim, with a base travel time of 1 second and an extra penalty for very small volumes. The new reachability is added to the source area’s linked
   list. The function returns a success code if the link was created, otherwise it returns failure.


	\param area1num index of the source area
	\param area2num index of the target area
	\return integer flag indicating success (true) or failure (false)
*/
int AAS_Reachability_Swim( int area1num, int area2num )
{
	int					 i, j, face1num, face2num, side1;
	aas_area_t *		 area1, *area2;
	aas_areasettings_t*	 areasettings;
	aas_lreachability_t* lreach;
	aas_face_t*			 face1;
	aas_plane_t*		 plane;
	vec3_t				 start;

	if( !AAS_AreaSwim( area1num ) || !AAS_AreaSwim( area2num ) ) {
		return qfalse;
	}

	// if the second area is crouch only
	if( !( ( *aasworld ).areasettings[area2num].presencetype & PRESENCE_NORMAL ) ) {
		return qfalse;
	}

	area1 = &( *aasworld ).areas[area1num];
	area2 = &( *aasworld ).areas[area2num];

	// if the areas are not near anough
	for( i = 0; i < 3; i++ ) {
		if( area1->mins[i] > area2->maxs[i] + 10 ) {
			return qfalse;
		}

		if( area1->maxs[i] < area2->mins[i] - 10 ) {
			return qfalse;
		}
	}

	// find a shared face and create a reachability link
	for( i = 0; i < area1->numfaces; i++ ) {
		face1num = ( *aasworld ).faceindex[area1->firstface + i];
		side1	 = face1num < 0;
		face1num = abs( face1num );

		//
		for( j = 0; j < area2->numfaces; j++ ) {
			face2num = abs( ( *aasworld ).faceindex[area2->firstface + j] );

			//
			if( face1num == face2num ) {
				AAS_FaceCenter( face1num, start );

				//
				if( AAS_PointContents( start ) & ( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER ) ) {
					//
					face1		 = &( *aasworld ).faces[face1num];
					areasettings = &( *aasworld ).areasettings[area1num];
					// create a new reachability link
					lreach = AAS_AllocReachability();

					if( !lreach ) {
						return qfalse;
					}

					lreach->areanum = area2num;
					lreach->facenum = face1num;
					lreach->edgenum = 0;
					VectorCopy( start, lreach->start );
					plane = &( *aasworld ).planes[face1->planenum ^ side1];
					VectorMA( lreach->start, INSIDEUNITS, plane->normal, lreach->end );
					lreach->traveltype = TRAVEL_SWIM;
					lreach->traveltime = 1;

					// if the volume of the area is rather small
					if( AAS_AreaVolume( area2num ) < 800 ) {
						lreach->traveltime += 200;
					}

					// if (!(AAS_PointContents(start) & MASK_WATER)) lreach->traveltime += 500;
					// link the reachability
					lreach->next			   = areareachability[area1num];
					areareachability[area1num] = lreach;
					reach_swim++;
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}

/*!
	\brief Determines if two adjacent ground areas share a common floor height and creates a walk reachability between them.

	The routine first checks that both areas are grounded and within a small XY distance, and that the second area is not entirely above the first. It then iterates over ground faces of each area to
   find common edges. For each shared edge it evaluates the lowest point just inside the two faces, measuring the edge length and comparing heights to pick the lowest, longest suitable edge. If an
   eligible edge is found a new reachability is allocated, linked into the destination list and time adjustments are made for crouch transitions. If no suitable edge exists the function returns false.

	The travel time defaults to one unit; when moving into a crouch area an extra crouch startup cost is added.

	\param area1num index of the first area
	\param area2num index of the second area
	\return 1 if a reachability was added, otherwise 0
*/
int AAS_Reachability_EqualFloorHeight( int area1num, int area2num )
{
	int					i, j, edgenum, edgenum1, edgenum2, foundreach, side;
	float				height, bestheight, length, bestlength;
	vec3_t				dir, start, end, normal, invgravity, gravitydirection = { 0, 0, -1 };
	vec3_t				edgevec;
	aas_area_t *		area1, *area2;
	aas_face_t *		face1, *face2;
	aas_edge_t*			edge;
	aas_plane_t*		plane2;
	aas_lreachability_t lr, *lreach;

	if( !AAS_AreaGrounded( area1num ) || !AAS_AreaGrounded( area2num ) ) {
		return qfalse;
	}

	area1 = &( *aasworld ).areas[area1num];
	area2 = &( *aasworld ).areas[area2num];

	// if the areas are not near anough in the x-y direction
	for( i = 0; i < 2; i++ ) {
		if( area1->mins[i] > area2->maxs[i] + 10 ) {
			return qfalse;
		}

		if( area1->maxs[i] < area2->mins[i] - 10 ) {
			return qfalse;
		}
	}

	// if area 2 is too high above area 1
	if( area2->mins[2] > area1->maxs[2] ) {
		return qfalse;
	}

	//
	VectorCopy( gravitydirection, invgravity );
	VectorInverse( invgravity );
	//
	bestheight = 99999;
	bestlength = 0;
	foundreach = qfalse;
	memset( &lr, 0, sizeof( aas_lreachability_t ) ); // make the compiler happy

	//
	// check if the areas have ground faces with a common edge
	// if existing use the lowest common edge for a reachability link
	for( i = 0; i < area1->numfaces; i++ ) {
		face1 = &( *aasworld ).faces[abs( ( *aasworld ).faceindex[area1->firstface + i] )];

		if( !( face1->faceflags & FACE_GROUND ) ) {
			continue;
		}

		//
		for( j = 0; j < area2->numfaces; j++ ) {
			face2 = &( *aasworld ).faces[abs( ( *aasworld ).faceindex[area2->firstface + j] )];

			if( !( face2->faceflags & FACE_GROUND ) ) {
				continue;
			}

			// if there is a common edge
			for( edgenum1 = 0; edgenum1 < face1->numedges; edgenum1++ ) {
				for( edgenum2 = 0; edgenum2 < face2->numedges; edgenum2++ ) {
					if( abs( ( *aasworld ).edgeindex[face1->firstedge + edgenum1] ) != abs( ( *aasworld ).edgeindex[face2->firstedge + edgenum2] ) ) {
						continue;
					}

					edgenum = ( *aasworld ).edgeindex[face1->firstedge + edgenum1];
					side	= edgenum < 0;
					edge	= &( *aasworld ).edges[abs( edgenum )];
					// get the length of the edge
					VectorSubtract( ( *aasworld ).vertexes[edge->v[1]], ( *aasworld ).vertexes[edge->v[0]], dir );
					length = VectorLength( dir );
					// get the start point
					VectorAdd( ( *aasworld ).vertexes[edge->v[0]], ( *aasworld ).vertexes[edge->v[1]], start );
					VectorScale( start, 0.5, start );
					VectorCopy( start, end );
					// get the end point several units inside area2
					// and the start point several units inside area1
					// NOTE: normal is pointing into area2 because the
					// face edges are stored counter clockwise
					VectorSubtract( ( *aasworld ).vertexes[edge->v[side]], ( *aasworld ).vertexes[edge->v[!side]], edgevec );
					plane2 = &( *aasworld ).planes[face2->planenum];
					CrossProduct( edgevec, plane2->normal, normal );
					VectorNormalize( normal );
					//
					// VectorMA(start, -1, normal, start);
					VectorMA( end, INSIDEUNITS_WALKEND, normal, end );
					VectorMA( start, INSIDEUNITS_WALKSTART, normal, start );
					end[2] += 0.125;
					//
					height = DotProduct( invgravity, start );

					// NOTE: if there's nearby solid or a gap area after this area
					// disabled this crap
					// if (AAS_NearbySolidOrGap(start, end)) height += 200;
					// NOTE: disabled because it disables reachabilities to very small areas
					// if (AAS_PointAreaNum(end) != area2num) continue;
					// get the longest lowest edge
					if( height < bestheight || ( height < bestheight + 1 && length > bestlength ) ) {
						bestheight = height;
						bestlength = length;
						// create a new reachability link
						lr.areanum = area2num;
						lr.facenum = 0;
						lr.edgenum = edgenum;
						VectorCopy( start, lr.start );
						VectorCopy( end, lr.end );
						lr.traveltype = TRAVEL_WALK;
						lr.traveltime = 1;
						foundreach	  = qtrue;
					}
				}
			}
		}
	}

	if( foundreach ) {
		// create a new reachability link
		lreach = AAS_AllocReachability();

		if( !lreach ) {
			return qfalse;
		}

		lreach->areanum = lr.areanum;
		lreach->facenum = lr.facenum;
		lreach->edgenum = lr.edgenum;
		VectorCopy( lr.start, lreach->start );
		VectorCopy( lr.end, lreach->end );
		lreach->traveltype		   = lr.traveltype;
		lreach->traveltime		   = lr.traveltime;
		lreach->next			   = areareachability[area1num];
		areareachability[area1num] = lreach;

		// if going into a crouch area
		if( !AAS_AreaCrouch( area1num ) && AAS_AreaCrouch( area2num ) ) {
			lreach->traveltime += STARTCROUCH_TIME;
		}

		/*
		//NOTE: if there's nearby solid or a gap area after this area
		if (!AAS_NearbySolidOrGap(lreach->start, lreach->end))
		{
		  lreach->traveltime += 100;
		}
		*/
		// avoid rather small areas
		// if (AAS_AreaGroundFaceArea(lreach->areanum) < 500) lreach->traveltime += 100;
		//
		reach_equalfloor++;
		return qtrue;
	}

	return qfalse;
}

/*!
	\brief Checks whether two AAS areas can be connected by stepping, blocking, waterjumping or walking off a ledge.

	The function first verifies that both source and destination areas are either grounded or swimable; it then ensures the areas are close in the XY plane within a small margin. It examines all faces
   and edges of the first area, constructing a vertical plane through each qualifying edge, and compares it to the ground faces of the second area to detect potential step or barrier connections.
   Special handling is applied for water faces, computing best distances for walk‑off and water‑jump scenarios. If a suitable connection is found, the function returns true; otherwise it returns
   false.

	\param area1num Index of the first area in the AAS area table
	\param area2num Index of the second area in the AAS area table
	\return An integer indicating success (1) or failure (0).
*/
int AAS_Reachability_Step_Barrier_WaterJump_WalkOffLedge( int area1num, int area2num )
{
	int					 i, j, k, l, edge1num, edge2num;
	int					 ground_bestarea2groundedgenum, ground_foundreach;
	int					 water_bestarea2groundedgenum, water_foundreach;
	int					 side1, area1swim, faceside1, groundface1num;
	float				 dist, dist1, dist2, diff, invgravitydot, ortdot;
	float				 x1, x2, x3, x4, y1, y2, y3, y4, tmp, y;
	float				 length, ground_bestlength, water_bestlength, ground_bestdist, water_bestdist;
	vec3_t				 v1, v2, v3, v4, tmpv, p1area1, p1area2, p2area1, p2area2;
	vec3_t				 normal, ort, edgevec, start, end, dir;
	vec3_t				 ground_beststart, ground_bestend, ground_bestnormal;
	vec3_t				 water_beststart, water_bestend, water_bestnormal;
	vec3_t				 invgravity = { 0, 0, 1 };
	vec3_t				 testpoint;
	aas_plane_t*		 plane;
	aas_area_t *		 area1, *area2;
	aas_face_t *		 groundface1, *groundface2, *ground_bestface1, *water_bestface1;
	aas_edge_t *		 edge1, *edge2;
	aas_lreachability_t* lreach;
	aas_trace_t			 trace;

	// must be able to walk or swim in the first area
	if( !AAS_AreaGrounded( area1num ) && !AAS_AreaSwim( area1num ) ) {
		return qfalse;
	}

	//
	if( !AAS_AreaGrounded( area2num ) && !AAS_AreaSwim( area2num ) ) {
		return qfalse;
	}

	//
	area1 = &( *aasworld ).areas[area1num];
	area2 = &( *aasworld ).areas[area2num];
	// if the first area contains a liquid
	area1swim = AAS_AreaSwim( area1num );

	// if the areas are not near anough in the x-y direction
	for( i = 0; i < 2; i++ ) {
		if( area1->mins[i] > area2->maxs[i] + 10 ) {
			return qfalse;
		}

		if( area1->maxs[i] < area2->mins[i] - 10 ) {
			return qfalse;
		}
	}

	//
	ground_foundreach			  = qfalse;
	ground_bestdist				  = 99999;
	ground_bestlength			  = 0;
	ground_bestarea2groundedgenum = 0;
	//
	water_foundreach			 = qfalse;
	water_bestdist				 = 99999;
	water_bestlength			 = 0;
	water_bestarea2groundedgenum = 0;

	//
	for( i = 0; i < area1->numfaces; i++ ) {
		groundface1num = ( *aasworld ).faceindex[area1->firstface + i];
		faceside1	   = groundface1num < 0;
		groundface1	   = &( *aasworld ).faces[abs( groundface1num )];

		// if this isn't a ground face
		if( !( groundface1->faceflags & FACE_GROUND ) ) {
			// if we can swim in the first area
			if( area1swim ) {
				// face plane must be more or less horizontal
				plane = &( *aasworld ).planes[groundface1->planenum ^ ( !faceside1 )];

				if( DotProduct( plane->normal, invgravity ) < 0.7 ) {
					continue;
				}

			} else {
				// if we can't swim in the area it must be a ground face
				continue;
			}
		}

		//
		for( k = 0; k < groundface1->numedges; k++ ) {
			edge1num = ( *aasworld ).edgeindex[groundface1->firstedge + k];
			side1	 = ( edge1num < 0 );

			// NOTE: for water faces we must take the side area 1 is
			//  on into account because the face is shared and doesn't
			//  have to be oriented correctly
			if( !( groundface1->faceflags & FACE_GROUND ) ) {
				side1 = ( side1 == faceside1 );
			}

			edge1num = abs( edge1num );
			edge1	 = &( *aasworld ).edges[edge1num];
			// vertexes of the edge
			VectorCopy( ( *aasworld ).vertexes[edge1->v[!side1]], v1 );
			VectorCopy( ( *aasworld ).vertexes[edge1->v[side1]], v2 );
			// get a vertical plane through the edge
			// NOTE: normal is pointing into area 2 because the
			// face edges are stored counter clockwise
			VectorSubtract( v2, v1, edgevec );
			CrossProduct( edgevec, invgravity, normal );
			VectorNormalize( normal );
			dist = DotProduct( normal, v1 );

			// check the faces from the second area
			for( j = 0; j < area2->numfaces; j++ ) {
				groundface2 = &( *aasworld ).faces[abs( ( *aasworld ).faceindex[area2->firstface + j] )];

				// must be a ground face
				if( !( groundface2->faceflags & FACE_GROUND ) ) {
					continue;
				}

				// check the edges of this ground face
				for( l = 0; l < groundface2->numedges; l++ ) {
					edge2num = abs( ( *aasworld ).edgeindex[groundface2->firstedge + l] );
					edge2	 = &( *aasworld ).edges[edge2num];
					// vertexes of the edge
					VectorCopy( ( *aasworld ).vertexes[edge2->v[0]], v3 );
					VectorCopy( ( *aasworld ).vertexes[edge2->v[1]], v4 );
					// check the distance between the two points and the vertical plane
					// through the edge of area1
					diff = DotProduct( normal, v3 ) - dist;

					if( diff < -0.1 || diff > 0.1 ) {
						continue;
					}

					diff = DotProduct( normal, v4 ) - dist;

					if( diff < -0.1 || diff > 0.1 ) {
						continue;
					}

					//
					// project the two ground edges into the step side plane
					// and calculate the shortest distance between the two
					// edges if they overlap in the direction orthogonal to
					// the gravity direction
					CrossProduct( invgravity, normal, ort );
					invgravitydot = DotProduct( invgravity, invgravity );
					ortdot		  = DotProduct( ort, ort );
					// projection into the step plane
					// NOTE: since gravity is vertical this is just the z coordinate
					y1 = v1[2]; // DotProduct(v1, invgravity) / invgravitydot;
					y2 = v2[2]; // DotProduct(v2, invgravity) / invgravitydot;
					y3 = v3[2]; // DotProduct(v3, invgravity) / invgravitydot;
					y4 = v4[2]; // DotProduct(v4, invgravity) / invgravitydot;
					//
					x1 = DotProduct( v1, ort ) / ortdot;
					x2 = DotProduct( v2, ort ) / ortdot;
					x3 = DotProduct( v3, ort ) / ortdot;
					x4 = DotProduct( v4, ort ) / ortdot;

					//
					if( x1 > x2 ) {
						tmp = x1;
						x1	= x2;
						x2	= tmp;
						tmp = y1;
						y1	= y2;
						y2	= tmp;
						VectorCopy( v1, tmpv );
						VectorCopy( v2, v1 );
						VectorCopy( tmpv, v2 );
					}

					if( x3 > x4 ) {
						tmp = x3;
						x3	= x4;
						x4	= tmp;
						tmp = y3;
						y3	= y4;
						y4	= tmp;
						VectorCopy( v3, tmpv );
						VectorCopy( v4, v3 );
						VectorCopy( tmpv, v4 );
					}

					// if the two projected edge lines have no overlap
					if( x2 <= x3 || x4 <= x1 ) {
						//						Log_Write("lines no overlap: from area %d to %d\r\n", area1num, area2num);
						continue;
					}

					// if the two lines fully overlap
					if( ( x1 - 0.5 < x3 && x4 < x2 + 0.5 ) && ( x3 - 0.5 < x1 && x2 < x4 + 0.5 ) ) {
						dist1 = y3 - y1;
						dist2 = y4 - y2;
						VectorCopy( v1, p1area1 );
						VectorCopy( v2, p2area1 );
						VectorCopy( v3, p1area2 );
						VectorCopy( v4, p2area2 );

					} else {
						// if the points are equal
						if( x1 > x3 - 0.1 && x1 < x3 + 0.1 ) {
							dist1 = y3 - y1;
							VectorCopy( v1, p1area1 );
							VectorCopy( v3, p1area2 );

						} else if( x1 < x3 ) {
							y	  = y1 + ( x3 - x1 ) * ( y2 - y1 ) / ( x2 - x1 );
							dist1 = y3 - y;
							VectorCopy( v3, p1area1 );
							p1area1[2] = y;
							VectorCopy( v3, p1area2 );

						} else {
							y	  = y3 + ( x1 - x3 ) * ( y4 - y3 ) / ( x4 - x3 );
							dist1 = y - y1;
							VectorCopy( v1, p1area1 );
							VectorCopy( v1, p1area2 );
							p1area2[2] = y;
						}

						// if the points are equal
						if( x2 > x4 - 0.1 && x2 < x4 + 0.1 ) {
							dist2 = y4 - y2;
							VectorCopy( v2, p2area1 );
							VectorCopy( v4, p2area2 );

						} else if( x2 < x4 ) {
							y	  = y3 + ( x2 - x3 ) * ( y4 - y3 ) / ( x4 - x3 );
							dist2 = y - y2;
							VectorCopy( v2, p2area1 );
							VectorCopy( v2, p2area2 );
							p2area2[2] = y;

						} else {
							y	  = y1 + ( x4 - x1 ) * ( y2 - y1 ) / ( x2 - x1 );
							dist2 = y4 - y;
							VectorCopy( v4, p2area1 );
							p2area1[2] = y;
							VectorCopy( v4, p2area2 );
						}
					}

					// if both distances are pretty much equal
					// then we take the middle of the points
					if( dist1 > dist2 - 1 && dist1 < dist2 + 1 ) {
						dist = dist1;
						VectorAdd( p1area1, p2area1, start );
						VectorScale( start, 0.5, start );
						VectorAdd( p1area2, p2area2, end );
						VectorScale( end, 0.5, end );

					} else if( dist1 < dist2 ) {
						dist = dist1;
						VectorCopy( p1area1, start );
						VectorCopy( p1area2, end );

					} else {
						dist = dist2;
						VectorCopy( p2area1, start );
						VectorCopy( p2area2, end );
					}

					// get the length of the overlapping part of the edges of the two areas
					VectorSubtract( p2area2, p1area2, dir );
					length = VectorLength( dir );

					//
					if( groundface1->faceflags & FACE_GROUND ) {
						// if the vertical distance is smaller
						if( dist < ground_bestdist ||
							// or the vertical distance is pretty much the same
							// but the overlapping part of the edges is longer
							( dist < ground_bestdist + 1 && length > ground_bestlength ) ) {
							ground_bestdist				  = dist;
							ground_bestlength			  = length;
							ground_foundreach			  = qtrue;
							ground_bestarea2groundedgenum = edge1num;
							ground_bestface1			  = groundface1;
							// best point towards area1
							VectorCopy( start, ground_beststart );
							// normal is pointing into area2
							VectorCopy( normal, ground_bestnormal );
							// best point towards area2
							VectorCopy( end, ground_bestend );
						}

					} else {
						// if the vertical distance is smaller
						if( dist < water_bestdist ||
							// or the vertical distance is pretty much the same
							// but the overlapping part of the edges is longer
							( dist < water_bestdist + 1 && length > water_bestlength ) ) {
							water_bestdist				 = dist;
							water_bestlength			 = length;
							water_foundreach			 = qtrue;
							water_bestarea2groundedgenum = edge1num;
							water_bestface1				 = groundface1;
							// best point towards area1
							VectorCopy( start, water_beststart );
							// normal is pointing into area2
							VectorCopy( normal, water_bestnormal );
							// best point towards area2
							VectorCopy( end, water_bestend );
						}
					}
				}
			}
		}
	}

	//
	// NOTE: swim reachabilities are already filtered out
	//
	// Steps
	//
	//        ---------
	//        |          step height -> TRAVEL_WALK
	//--------|
	//
	//        ---------
	//~~~~~~~~|          step height and low water -> TRAVEL_WALK
	//--------|
	//
	//~~~~~~~~~~~~~~~~~~
	//        ---------
	//        |          step height and low water up to the step -> TRAVEL_WALK
	//--------|
	//
	// check for a step reachability
	if( ground_foundreach ) {
		// if area2 is higher but lower than the maximum step height
		// NOTE: ground_bestdist >= 0 also catches equal floor reachabilities
		if( ground_bestdist >= 0 && ground_bestdist < aassettings.sv_maxstep ) {
			// create walk reachability from area1 to area2
			lreach = AAS_AllocReachability();

			if( !lreach ) {
				return qfalse;
			}

			lreach->areanum = area2num;
			lreach->facenum = 0;
			lreach->edgenum = ground_bestarea2groundedgenum;
			VectorMA( ground_beststart, INSIDEUNITS_WALKSTART, ground_bestnormal, lreach->start );
			VectorMA( ground_bestend, INSIDEUNITS_WALKEND, ground_bestnormal, lreach->end );
			lreach->traveltype = TRAVEL_WALK;
			lreach->traveltime = 0; // 1;

			// if going into a crouch area
			if( !AAS_AreaCrouch( area1num ) && AAS_AreaCrouch( area2num ) ) {
				lreach->traveltime += STARTCROUCH_TIME;
			}

			lreach->next			   = areareachability[area1num];
			areareachability[area1num] = lreach;
			// NOTE: if there's nearby solid or a gap area after this area
			/*
			if (!AAS_NearbySolidOrGap(lreach->start, lreach->end))
			{
				lreach->traveltime += 100;
			}
			*/
			// avoid rather small areas
			// if (AAS_AreaGroundFaceArea(lreach->areanum) < 500) lreach->traveltime += 100;
			//
			reach_step++;
			return qtrue;
		}
	}

	//
	// Water Jumps
	//
	//        ---------
	//        |
	//~~~~~~~~|
	//        |
	//        |          higher than step height and water up to waterjump height -> TRAVEL_WATERJUMP
	//--------|
	//
	//~~~~~~~~~~~~~~~~~~
	//        ---------
	//        |
	//        |
	//        |
	//        |          higher than step height and low water up to the step -> TRAVEL_WATERJUMP
	//--------|
	//
	// check for a waterjump reachability
	if( water_foundreach ) {
		// get a test point a little bit towards area1
		VectorMA( water_bestend, -INSIDEUNITS, water_bestnormal, testpoint );
		// go down the maximum waterjump height
		testpoint[2] -= aassettings.sv_maxwaterjump;

		// if there IS water the sv_maxwaterjump height below the bestend point
		if( ( *aasworld ).areasettings[AAS_PointAreaNum( testpoint )].areaflags & AREA_LIQUID ) {
			// don't create rediculous water jump reachabilities from areas very far below
			// the water surface
			if( water_bestdist < aassettings.sv_maxwaterjump + 24 ) {
				// waterjumping from or towards a crouch only area is not possible in Quake2
				if( ( ( *aasworld ).areasettings[area1num].presencetype & PRESENCE_NORMAL ) && ( ( *aasworld ).areasettings[area2num].presencetype & PRESENCE_NORMAL ) ) {
					// create water jump reachability from area1 to area2
					lreach = AAS_AllocReachability();

					if( !lreach ) {
						return qfalse;
					}

					lreach->areanum = area2num;
					lreach->facenum = 0;
					lreach->edgenum = water_bestarea2groundedgenum;
					VectorCopy( water_beststart, lreach->start );
					VectorMA( water_bestend, INSIDEUNITS_WATERJUMP, water_bestnormal, lreach->end );
					lreach->traveltype		   = TRAVEL_WATERJUMP;
					lreach->traveltime		   = WATERJUMP_TIME;
					lreach->next			   = areareachability[area1num];
					areareachability[area1num] = lreach;
					// we've got another waterjump reachability
					reach_waterjump++;
					return qtrue;
				}
			}
		}
	}

	//
	// Barrier Jumps
	//
	//        ---------
	//        |
	//        |
	//        |
	//        |         higher than step height lower than barrier height -> TRAVEL_BARRIERJUMP
	//--------|
	//
	//        ---------
	//        |
	//        |
	//        |
	//~~~~~~~~|         higher than step height lower than barrier height
	//--------|         and a thin layer of water in the area to jump from -> TRAVEL_BARRIERJUMP
	//
	// check for a barrier jump reachability
	if( ground_foundreach ) {
		// if area2 is higher but lower than the maximum barrier jump height
		if( ground_bestdist > 0 && ground_bestdist < aassettings.sv_maxbarrier ) {
			// if no water in area1 or a very thin layer of water on the ground
			if( !water_foundreach || ( ground_bestdist - water_bestdist < 16 ) ) {
				// cannot perform a barrier jump towards or from a crouch area in Quake2
				if( !AAS_AreaCrouch( area1num ) && !AAS_AreaCrouch( area2num ) ) {
					// create barrier jump reachability from area1 to area2
					lreach = AAS_AllocReachability();

					if( !lreach ) {
						return qfalse;
					}

					lreach->areanum = area2num;
					lreach->facenum = 0;
					lreach->edgenum = ground_bestarea2groundedgenum;
					VectorMA( ground_beststart, INSIDEUNITS_WALKSTART, ground_bestnormal, lreach->start );
					VectorMA( ground_bestend, INSIDEUNITS_WALKEND, ground_bestnormal, lreach->end );
					lreach->traveltype		   = TRAVEL_BARRIERJUMP;
					lreach->traveltime		   = BARRIERJUMP_TIME; // AAS_BarrierJumpTravelTime();
					lreach->next			   = areareachability[area1num];
					areareachability[area1num] = lreach;
					// we've got another barrierjump reachability
					reach_barrier++;
					return qtrue;
				}
			}
		}
	}

	//
	// Walk and Walk Off Ledge
	//
	//--------|
	//        |          can walk or step back -> TRAVEL_WALK
	//        ---------
	//
	//--------|
	//        |
	//        |
	//        |
	//        |          cannot walk/step back -> TRAVEL_WALKOFFLEDGE
	//        ---------
	//
	//--------|
	//        |
	//        |~~~~~~~~
	//        |
	//        |          cannot step back but can waterjump back -> TRAVEL_WALKOFFLEDGE
	//        ---------  FIXME: create TRAVEL_WALK reach??
	//
	// check for a walk or walk off ledge reachability
	if( ground_foundreach ) {
		if( ground_bestdist < 0 ) {
			if( ground_bestdist > -aassettings.sv_maxstep ) {
				// create walk reachability from area1 to area2
				lreach = AAS_AllocReachability();

				if( !lreach ) {
					return qfalse;
				}

				lreach->areanum = area2num;
				lreach->facenum = 0;
				lreach->edgenum = ground_bestarea2groundedgenum;
				VectorMA( ground_beststart, INSIDEUNITS_WALKSTART, ground_bestnormal, lreach->start );

				// Ridah
				//				VectorMA(ground_bestend, INSIDEUNITS_WALKEND, ground_bestnormal, lreach->end);
				VectorMA( ground_bestend, INSIDEUNITS_WALKOFFLEDGEEND, ground_bestnormal, lreach->end );

				lreach->traveltype		   = TRAVEL_WALK;
				lreach->traveltime		   = 1;
				lreach->next			   = areareachability[area1num];
				areareachability[area1num] = lreach;
				// we've got another walk reachability
				reach_walk++;
				return qtrue;
			}

			// trace a bounding box vertically to check for solids
			VectorMA( ground_bestend, INSIDEUNITS, ground_bestnormal, ground_bestend );
			VectorCopy( ground_bestend, start );
			start[2] = ground_beststart[2];
			VectorCopy( ground_bestend, end );
			end[2] += 4;
			trace = AAS_TraceClientBBox( start, end, PRESENCE_NORMAL, -1 );

			// if no solids were found
			if( !trace.startsolid && trace.fraction >= 1.0 ) {
				// the trace end point must be in the goal area
				trace.endpos[2] += 1;

				if( AAS_PointAreaNum( trace.endpos ) == area2num ) {
					// create a walk off ledge reachability from area1 to area2
					lreach = AAS_AllocReachability();

					if( !lreach ) {
						return qfalse;
					}

					lreach->areanum = area2num;
					lreach->facenum = 0;
					lreach->edgenum = ground_bestarea2groundedgenum;
					VectorCopy( ground_beststart, lreach->start );
					VectorCopy( ground_bestend, lreach->end );
					lreach->traveltype = TRAVEL_WALKOFFLEDGE;
					lreach->traveltime = STARTWALKOFFLEDGE_TIME + fabs( ground_bestdist ) * 50 / aassettings.sv_gravity;

					// if falling from too high and not falling into water
					if( !AAS_AreaSwim( area2num ) && !AAS_AreaJumpPad( area2num ) ) {
						if( AAS_FallDelta( ground_bestdist ) > FALLDELTA_5DAMAGE ) {
							lreach->traveltime += FALLDAMAGE_5_TIME;
						}

						if( AAS_FallDelta( ground_bestdist ) > FALLDELTA_10DAMAGE ) {
							lreach->traveltime += FALLDAMAGE_10_TIME;
						}
					}

					lreach->next			   = areareachability[area1num];
					areareachability[area1num] = lreach;
					//
					reach_walkoffledge++;
					// NOTE: don't create a weapon (rl, bfg) jump reachability here
					// because it interferes with other reachabilities
					// like the ladder reachability
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}

/*!
	\brief Determines whether a point lies between two other points on a line segment.

	The function computes vectors from each of the two reference points (v1 and v2) to the test point v and evaluates the dot product of these two vectors. If the dot product is less than or equal to
   zero, the vectors point in opposite directions, indicating that v is situated on the line segment connecting v1 and v2 (including the endpoints). The result is returned as an integer value of 1 for
   true and 0 for false.

	\param v test point to check if it lies between v1 and v2
	\param v1 first endpoint of the segment
	\param v2 second endpoint of the segment
	\return 1 if v is between v1 and v2, otherwise 0
*/
int VectorBetweenVectors( vec3_t v, vec3_t v1, vec3_t v2 )
{
	vec3_t dir1, dir2;

	VectorSubtract( v, v1, dir1 );
	VectorSubtract( v, v2, dir2 );
	return ( DotProduct( dir1, dir2 ) <= 0 );
}

/*!
	\brief Computes the midpoint between two 3D vectors.

	Adds the two input vectors together and then scales the result by 0.5 to produce the midpoint. The operation is performed in place on the third parameter.

	\param middle output vector that will contain the calculated midpoint
	\param v1 first input vector
	\param v2 second input vector
*/
void VectorMiddle( vec3_t v1, vec3_t v2, vec3_t middle )
{
	VectorAdd( v1, v2, middle );
	VectorScale( middle, 0.5, middle );
}

/*!
	\brief Computes the minimum distance between two edges in 3D space, updating the closest points and distance when a shorter distance is found.

	This function calculates the closest points between two 3D edges defined by their endpoints and associated ground planes. It determines the minimum distance between the edges and updates the
   provided output vectors with the closest points on each edge. The function uses line equations to find the closest points on each edge and checks if these points lie within the bounds of the other
   edge. When a shorter distance is found, the function updates the best distance and records the corresponding closest points on both edges. The function returns the updated minimum distance, which
   may be unchanged if no shorter distance was found.

	\param bestdist current best known distance; updated if a shorter distance is found
	\param bestend1 output vector for the other closest point on the first edge
	\param bestend2 output vector for the other closest point on the second edge
	\param beststart1 output vector for the closest point on the first edge
	\param beststart2 output vector for the closest point on the second edge
	\param plane1 ground plane containing the first edge
	\param plane2 ground plane containing the second edge
	\param v1 first endpoint of the first edge
	\param v2 second endpoint of the first edge
	\param v3 first endpoint of the second edge
	\param v4 second endpoint of the second edge
	\return the updated best distance between the two edges
*/
float AAS_ClosestEdgePoints(
	vec3_t v1, vec3_t v2, vec3_t v3, vec3_t v4, aas_plane_t* plane1, aas_plane_t* plane2, vec3_t beststart1, vec3_t bestend1, vec3_t beststart2, vec3_t bestend2, float bestdist )
{
	vec3_t dir1, dir2, p1, p2, p3, p4;
	float  a1, a2, b1, b2, dist, dist1, dist2;
	int	   founddist;

	// edge vectors
	VectorSubtract( v2, v1, dir1 );
	VectorSubtract( v4, v3, dir2 );
	// get the horizontal directions
	dir1[2] = 0;
	dir2[2] = 0;

	//
	// p1 = point on an edge vector of area2 closest to v1
	// p2 = point on an edge vector of area2 closest to v2
	// p3 = point on an edge vector of area1 closest to v3
	// p4 = point on an edge vector of area1 closest to v4
	//
	if( dir2[0] ) {
		a2 = dir2[1] / dir2[0];
		b2 = v3[1] - a2 * v3[0];
		// point on the edge vector of area2 closest to v1
		p1[0] = ( DotProduct( v1, dir2 ) - ( a2 * dir2[0] + b2 * dir2[1] ) ) / dir2[0];
		p1[1] = a2 * p1[0] + b2;
		// point on the edge vector of area2 closest to v2
		p2[0] = ( DotProduct( v2, dir2 ) - ( a2 * dir2[0] + b2 * dir2[1] ) ) / dir2[0];
		p2[1] = a2 * p2[0] + b2;

	} else {
		// point on the edge vector of area2 closest to v1
		p1[0] = v3[0];
		p1[1] = v1[1];
		// point on the edge vector of area2 closest to v2
		p2[0] = v3[0];
		p2[1] = v2[1];
	}

	//
	if( dir1[0] ) {
		//
		a1 = dir1[1] / dir1[0];
		b1 = v1[1] - a1 * v1[0];
		// point on the edge vector of area1 closest to v3
		p3[0] = ( DotProduct( v3, dir1 ) - ( a1 * dir1[0] + b1 * dir1[1] ) ) / dir1[0];
		p3[1] = a1 * p3[0] + b1;
		// point on the edge vector of area1 closest to v4
		p4[0] = ( DotProduct( v4, dir1 ) - ( a1 * dir1[0] + b1 * dir1[1] ) ) / dir1[0];
		p4[1] = a1 * p4[0] + b1;

	} else {
		// point on the edge vector of area1 closest to v3
		p3[0] = v1[0];
		p3[1] = v3[1];
		// point on the edge vector of area1 closest to v4
		p4[0] = v1[0];
		p4[1] = v4[1];
	}

	// start with zero z-coordinates
	p1[2] = 0;
	p2[2] = 0;
	p3[2] = 0;
	p4[2] = 0;
	// calculate the z-coordinates from the ground planes
	p1[2] = ( plane2->dist - DotProduct( plane2->normal, p1 ) ) / plane2->normal[2];
	p2[2] = ( plane2->dist - DotProduct( plane2->normal, p2 ) ) / plane2->normal[2];
	p3[2] = ( plane1->dist - DotProduct( plane1->normal, p3 ) ) / plane1->normal[2];
	p4[2] = ( plane1->dist - DotProduct( plane1->normal, p4 ) ) / plane1->normal[2];
	//
	founddist = qfalse;

	//
	if( VectorBetweenVectors( p1, v3, v4 ) ) {
		dist = VectorDistance( v1, p1 );

		if( dist > bestdist - 0.5 && dist < bestdist + 0.5 ) {
			dist1 = VectorDistance( beststart1, v1 );
			dist2 = VectorDistance( beststart2, v1 );

			if( dist1 > dist2 ) {
				if( dist1 > VectorDistance( beststart1, beststart2 ) ) {
					VectorCopy( v1, beststart2 );
				}

			} else {
				if( dist2 > VectorDistance( beststart1, beststart2 ) ) {
					VectorCopy( v1, beststart1 );
				}
			}

			dist1 = VectorDistance( bestend1, p1 );
			dist2 = VectorDistance( bestend2, p1 );

			if( dist1 > dist2 ) {
				if( dist1 > VectorDistance( bestend1, bestend2 ) ) {
					VectorCopy( p1, bestend2 );
				}

			} else {
				if( dist2 > VectorDistance( bestend1, bestend2 ) ) {
					VectorCopy( p1, bestend1 );
				}
			}

		} else if( dist < bestdist ) {
			bestdist = dist;
			VectorCopy( v1, beststart1 );
			VectorCopy( v1, beststart2 );
			VectorCopy( p1, bestend1 );
			VectorCopy( p1, bestend2 );
		}

		founddist = qtrue;
	}

	if( VectorBetweenVectors( p2, v3, v4 ) ) {
		dist = VectorDistance( v2, p2 );

		if( dist > bestdist - 0.5 && dist < bestdist + 0.5 ) {
			dist1 = VectorDistance( beststart1, v2 );
			dist2 = VectorDistance( beststart2, v2 );

			if( dist1 > dist2 ) {
				if( dist1 > VectorDistance( beststart1, beststart2 ) ) {
					VectorCopy( v2, beststart2 );
				}

			} else {
				if( dist2 > VectorDistance( beststart1, beststart2 ) ) {
					VectorCopy( v2, beststart1 );
				}
			}

			dist1 = VectorDistance( bestend1, p2 );
			dist2 = VectorDistance( bestend2, p2 );

			if( dist1 > dist2 ) {
				if( dist1 > VectorDistance( bestend1, bestend2 ) ) {
					VectorCopy( p2, bestend2 );
				}

			} else {
				if( dist2 > VectorDistance( bestend1, bestend2 ) ) {
					VectorCopy( p2, bestend1 );
				}
			}

		} else if( dist < bestdist ) {
			bestdist = dist;
			VectorCopy( v2, beststart1 );
			VectorCopy( v2, beststart2 );
			VectorCopy( p2, bestend1 );
			VectorCopy( p2, bestend2 );
		}

		founddist = qtrue;
	}

	if( VectorBetweenVectors( p3, v1, v2 ) ) {
		dist = VectorDistance( v3, p3 );

		if( dist > bestdist - 0.5 && dist < bestdist + 0.5 ) {
			dist1 = VectorDistance( beststart1, p3 );
			dist2 = VectorDistance( beststart2, p3 );

			if( dist1 > dist2 ) {
				if( dist1 > VectorDistance( beststart1, beststart2 ) ) {
					VectorCopy( p3, beststart2 );
				}

			} else {
				if( dist2 > VectorDistance( beststart1, beststart2 ) ) {
					VectorCopy( p3, beststart1 );
				}
			}

			dist1 = VectorDistance( bestend1, v3 );
			dist2 = VectorDistance( bestend2, v3 );

			if( dist1 > dist2 ) {
				if( dist1 > VectorDistance( bestend1, bestend2 ) ) {
					VectorCopy( v3, bestend2 );
				}

			} else {
				if( dist2 > VectorDistance( bestend1, bestend2 ) ) {
					VectorCopy( v3, bestend1 );
				}
			}

		} else if( dist < bestdist ) {
			bestdist = dist;
			VectorCopy( p3, beststart1 );
			VectorCopy( p3, beststart2 );
			VectorCopy( v3, bestend1 );
			VectorCopy( v3, bestend2 );
		}

		founddist = qtrue;
	}

	if( VectorBetweenVectors( p4, v1, v2 ) ) {
		dist = VectorDistance( v4, p4 );

		if( dist > bestdist - 0.5 && dist < bestdist + 0.5 ) {
			dist1 = VectorDistance( beststart1, p4 );
			dist2 = VectorDistance( beststart2, p4 );

			if( dist1 > dist2 ) {
				if( dist1 > VectorDistance( beststart1, beststart2 ) ) {
					VectorCopy( p4, beststart2 );
				}

			} else {
				if( dist2 > VectorDistance( beststart1, beststart2 ) ) {
					VectorCopy( p4, beststart1 );
				}
			}

			dist1 = VectorDistance( bestend1, v4 );
			dist2 = VectorDistance( bestend2, v4 );

			if( dist1 > dist2 ) {
				if( dist1 > VectorDistance( bestend1, bestend2 ) ) {
					VectorCopy( v4, bestend2 );
				}

			} else {
				if( dist2 > VectorDistance( bestend1, bestend2 ) ) {
					VectorCopy( v4, bestend1 );
				}
			}

		} else if( dist < bestdist ) {
			bestdist = dist;
			VectorCopy( p4, beststart1 );
			VectorCopy( p4, beststart2 );
			VectorCopy( v4, bestend1 );
			VectorCopy( v4, bestend2 );
		}

		founddist = qtrue;
	}

	// if no shortest distance was found the shortest distance
	// is between one of the vertexes of edge1 and one of edge2
	if( !founddist ) {
		dist = VectorDistance( v1, v3 );

		if( dist < bestdist ) {
			bestdist = dist;
			VectorCopy( v1, beststart1 );
			VectorCopy( v1, beststart2 );
			VectorCopy( v3, bestend1 );
			VectorCopy( v3, bestend2 );
		}

		dist = VectorDistance( v1, v4 );

		if( dist < bestdist ) {
			bestdist = dist;
			VectorCopy( v1, beststart1 );
			VectorCopy( v1, beststart2 );
			VectorCopy( v4, bestend1 );
			VectorCopy( v4, bestend2 );
		}

		dist = VectorDistance( v2, v3 );

		if( dist < bestdist ) {
			bestdist = dist;
			VectorCopy( v2, beststart1 );
			VectorCopy( v2, beststart2 );
			VectorCopy( v3, bestend1 );
			VectorCopy( v3, bestend2 );
		}

		dist = VectorDistance( v2, v4 );

		if( dist < bestdist ) {
			bestdist = dist;
			VectorCopy( v2, beststart1 );
			VectorCopy( v2, beststart2 );
			VectorCopy( v4, bestend1 );
			VectorCopy( v4, bestend2 );
		}
	}

	return bestdist;
}

/*!
	\brief Determines if a jump reachability can be established between two areas in the navigation graph.

	This function evaluates whether a player can jump from one area to another in the AAS (Area Awareness System). It checks various conditions including if both areas are grounded, if either area is
   crouched, and if the distance and height difference between the areas are within the jump capabilities. It calculates the minimum distance between edges of ground faces of the two areas, and then
   computes if a jump with proper velocity is possible. The function also checks if the landing position is safe and does not cause damage to the player.

	\param area1num the number of the originating area in the navigation graph
	\param area2num the number of the target area in the navigation graph
	\return 1 if a jump reachability can be created between the areas, 0 otherwise
*/
int AAS_Reachability_Jump( int area1num, int area2num )
{
	int					 i, j, k, l, face1num, face2num, edge1num, edge2num, traveltype;
	float				 sv_jumpvel, maxjumpdistance, maxjumpheight, height, bestdist, speed;
	vec_t *				 v1, *v2, *v3, *v4;
	vec3_t				 beststart, beststart2, bestend, bestend2;
	vec3_t				 teststart, testend, dir, velocity, cmdmove, up = { 0, 0, 1 };
	aas_area_t *		 area1, *area2;
	aas_face_t *		 face1, *face2;
	aas_edge_t *		 edge1, *edge2;
	aas_plane_t *		 plane1, *plane2, *plane;
	aas_trace_t			 trace;
	aas_clientmove_t	 move;
	aas_lreachability_t* lreach;

	if( !AAS_AreaGrounded( area1num ) || !AAS_AreaGrounded( area2num ) ) {
		return qfalse;
	}

	// cannot jump from or to a crouch area
	if( AAS_AreaCrouch( area1num ) || AAS_AreaCrouch( area2num ) ) {
		return qfalse;
	}

	//
	area1 = &( *aasworld ).areas[area1num];
	area2 = &( *aasworld ).areas[area2num];
	//
	sv_jumpvel = aassettings.sv_jumpvel;
	// maximum distance a player can jump
	maxjumpdistance = 2 * AAS_MaxJumpDistance( sv_jumpvel );
	// maximum height a player can jump with the given initial z velocity
	maxjumpheight = AAS_MaxJumpHeight( sv_jumpvel );

	// if the areas are not near anough in the x-y direction
	for( i = 0; i < 2; i++ ) {
		if( area1->mins[i] > area2->maxs[i] + maxjumpdistance ) {
			return qfalse;
		}

		if( area1->maxs[i] < area2->mins[i] - maxjumpdistance ) {
			return qfalse;
		}
	}

	// if area2 is way to high to jump up to
	if( area2->mins[2] > area1->maxs[2] + maxjumpheight ) {
		return qfalse;
	}

	//
	bestdist = 999999;

	//
	for( i = 0; i < area1->numfaces; i++ ) {
		face1num = ( *aasworld ).faceindex[area1->firstface + i];
		face1	 = &( *aasworld ).faces[abs( face1num )];

		// if not a ground face
		if( !( face1->faceflags & FACE_GROUND ) ) {
			continue;
		}

		//
		for( j = 0; j < area2->numfaces; j++ ) {
			face2num = ( *aasworld ).faceindex[area2->firstface + j];
			face2	 = &( *aasworld ).faces[abs( face2num )];

			// if not a ground face
			if( !( face2->faceflags & FACE_GROUND ) ) {
				continue;
			}

			//
			for( k = 0; k < face1->numedges; k++ ) {
				edge1num = abs( ( *aasworld ).edgeindex[face1->firstedge + k] );
				edge1	 = &( *aasworld ).edges[edge1num];

				for( l = 0; l < face2->numedges; l++ ) {
					edge2num = abs( ( *aasworld ).edgeindex[face2->firstedge + l] );
					edge2	 = &( *aasworld ).edges[edge2num];
					// calculate the minimum distance between the two edges
					v1 = ( *aasworld ).vertexes[edge1->v[0]];
					v2 = ( *aasworld ).vertexes[edge1->v[1]];
					v3 = ( *aasworld ).vertexes[edge2->v[0]];
					v4 = ( *aasworld ).vertexes[edge2->v[1]];
					// get the ground planes
					plane1 = &( *aasworld ).planes[face1->planenum];
					plane2 = &( *aasworld ).planes[face2->planenum];
					//
					bestdist = AAS_ClosestEdgePoints( v1, v2, v3, v4, plane1, plane2, beststart, bestend, beststart2, bestend2, bestdist );
				}
			}
		}
	}

	VectorMiddle( beststart, beststart2, beststart );
	VectorMiddle( bestend, bestend2, bestend );

	if( bestdist > 4 && bestdist < maxjumpdistance ) {
		//		Log_Write("shortest distance between %d and %d is %f\r\n", area1num, area2num, bestdist);
		// if the fall would damage the bot
		//
		if( AAS_HorizontalVelocityForJump( 0, beststart, bestend, &speed ) ) {
			// FIXME: why multiply with 1.2???
			speed *= 1.2;
			traveltype = TRAVEL_WALKOFFLEDGE;

		} else if( bestdist <= 48 && fabs( beststart[2] - bestend[2] ) < 8 ) {
			speed	   = 400;
			traveltype = TRAVEL_WALKOFFLEDGE;

		} else {
			// get the horizontal speed for the jump, if it isn't possible to calculate this
			// speed (the jump is not possible) then there's no jump reachability created
			if( !AAS_HorizontalVelocityForJump( sv_jumpvel, beststart, bestend, &speed ) ) {
				return qfalse;
			}

			traveltype = TRAVEL_JUMP;
			//
			// NOTE: test if the horizontal distance isn't too small
			VectorSubtract( bestend, beststart, dir );
			dir[2] = 0;

			if( VectorLength( dir ) < 10 ) {
				return qfalse;
			}
		}

		//
		VectorSubtract( bestend, beststart, dir );
		VectorNormalize( dir );
		VectorMA( beststart, 1, dir, teststart );
		//
		VectorCopy( teststart, testend );
		testend[2] -= 100;
		trace = AAS_TraceClientBBox( teststart, testend, PRESENCE_NORMAL, -1 );

		//
		if( trace.startsolid ) {
			return qfalse;
		}

		if( trace.fraction < 1 ) {
			plane = &( *aasworld ).planes[trace.planenum];

			if( DotProduct( plane->normal, up ) >= 0.7 ) {
				if( !( AAS_PointContents( trace.endpos ) & CONTENTS_LAVA ) ) { //----(SA)	modified since slime is no longer deadly
					//				if (!(AAS_PointContents(trace.endpos) & (CONTENTS_LAVA|CONTENTS_SLIME)))
					if( teststart[2] - trace.endpos[2] <= aassettings.sv_maxbarrier ) {
						return qfalse;
					}
				}
			}
		}

		//
		VectorMA( bestend, -1, dir, teststart );
		//
		VectorCopy( teststart, testend );
		testend[2] -= 100;
		trace = AAS_TraceClientBBox( teststart, testend, PRESENCE_NORMAL, -1 );

		//
		if( trace.startsolid ) {
			return qfalse;
		}

		if( trace.fraction < 1 ) {
			plane = &( *aasworld ).planes[trace.planenum];

			if( DotProduct( plane->normal, up ) >= 0.7 ) {
				if( !( AAS_PointContents( trace.endpos ) & ( CONTENTS_LAVA | CONTENTS_SLIME ) ) ) {
					if( teststart[2] - trace.endpos[2] <= aassettings.sv_maxbarrier ) {
						return qfalse;
					}
				}
			}
		}

		//
		VectorSubtract( bestend, beststart, dir );
		dir[2] = 0;
		VectorNormalize( dir );
		//
		VectorScale( dir, speed, velocity );
		// get command movement
		VectorClear( cmdmove );

		if( traveltype == TRAVEL_JUMP ) {
			cmdmove[2] = aassettings.sv_jumpvel;

		} else {
			cmdmove[2] = 0;
		}

		//
		AAS_PredictClientMovement(
			&move, -1, beststart, PRESENCE_NORMAL, qtrue, velocity, cmdmove, 3, 30, 0.1, SE_HITGROUND | SE_ENTERWATER | SE_ENTERSLIME | SE_ENTERLAVA | SE_HITGROUNDDAMAGE, 0, qfalse );

		// if prediction time wasn't enough to fully predict the movement
		if( move.frames >= 30 ) {
			return qfalse;
		}

		// don't enter slime or lava and don't fall from too high
		if( move.stopevent & SE_ENTERLAVA ) {
			return qfalse; //----(SA)	modified since slime is no longer deadly
		}

		//		if (move.stopevent & (SE_ENTERSLIME|SE_ENTERLAVA)) return qfalse;
		// the end position should be in area2, also test a little bit back
		// because the predicted jump could have rushed through the area
		for( i = 0; i <= 32; i += 8 ) {
			VectorMA( move.endpos, -i, dir, teststart );
			teststart[2] += 0.125;

			if( AAS_PointAreaNum( teststart ) == area2num ) {
				break;
			}
		}

		if( i > 32 ) {
			return qfalse;
		}

		//
#ifdef REACHDEBUG
		// create the reachability
		Log_Write( "jump reachability between %d and %d\r\n", area1num, area2num );
#endif // REACHDEBUG
	   // create a new reachability link
		lreach = AAS_AllocReachability();

		if( !lreach ) {
			return qfalse;
		}

		lreach->areanum = area2num;
		lreach->facenum = 0;
		lreach->edgenum = 0;
		VectorCopy( beststart, lreach->start );
		VectorCopy( bestend, lreach->end );
		lreach->traveltype = traveltype;

		VectorSubtract( bestend, beststart, dir );
		height = dir[2];
		dir[2] = 0;

		if( traveltype == TRAVEL_WALKOFFLEDGE && height > VectorLength( dir ) ) {
			lreach->traveltime = STARTWALKOFFLEDGE_TIME + height * 50 / aassettings.sv_gravity;

		} else {
			lreach->traveltime = STARTJUMP_TIME + VectorDistance( bestend, beststart ) * 240 / aassettings.sv_maxwalkvelocity;
		}

		//
		if( !AAS_AreaJumpPad( area2num ) ) {
			if( AAS_FallDelta( beststart[2] - bestend[2] ) > FALLDELTA_5DAMAGE ) {
				lreach->traveltime += FALLDAMAGE_5_TIME;

			} else if( AAS_FallDelta( beststart[2] - bestend[2] ) > FALLDELTA_10DAMAGE ) {
				lreach->traveltime += FALLDAMAGE_10_TIME;
			}
		}

		lreach->next			   = areareachability[area1num];
		areareachability[area1num] = lreach;

		//
		if( traveltype == TRAVEL_JUMP ) {
			reach_jump++;

		} else {
			reach_walkoffledge++;
		}
	}

	return qfalse;
}

/*!
	\brief Creates ladder reachability links between two areas if they share a ladder face.

	This function determines if two given areas can have ladder reachability between them by checking if they share a ladder face. It verifies that the shared face is vertical and that the ladder
   faces do not form a sharp corner. If the conditions are met, it allocates and sets up two reachability entries: one for moving from the first area to the second and another for the reverse
   direction. The function returns true if the reachability links are successfully created, otherwise false.

	\param area1num identifier of the first area
	\param area2num identifier of the second area
	\return qtrue if the ladder reachability was successfully created, otherwise qfalse
*/
int AAS_Reachability_Ladder( int area1num, int area2num )
{
	int					 i, j, k, l, edge1num, edge2num, sharededgenum, lowestedgenum;
	int					 face1num, face2num, ladderface1num, ladderface2num;
	int					 ladderface1vertical, ladderface2vertical, firstv;
	float				 face1area, face2area, bestface1area, bestface2area;
	float				 sv_jumpvel, maxjumpheight;
	vec3_t				 area1point, area2point, v1, v2, up = { 0, 0, 1 };
	vec3_t				 mid, lowestpoint, start, end, sharededgevec, dir;
	aas_area_t *		 area1, *area2;
	aas_face_t *		 face1, *face2, *ladderface1, *ladderface2;
	aas_plane_t *		 plane1, *plane2;
	aas_edge_t *		 sharededge, *edge1;
	aas_lreachability_t* lreach;
	aas_trace_t			 trace;

	if( !AAS_AreaLadder( area1num ) || !AAS_AreaLadder( area2num ) ) {
		return qfalse;
	}

	//
	sv_jumpvel = aassettings.sv_jumpvel;
	// maximum height a player can jump with the given initial z velocity
	maxjumpheight = AAS_MaxJumpHeight( sv_jumpvel );

	area1 = &( *aasworld ).areas[area1num];
	area2 = &( *aasworld ).areas[area2num];
	//
	ladderface1	   = NULL;
	ladderface2	   = NULL;
	ladderface1num = 0; // make compiler happy
	ladderface2num = 0; // make compiler happy
	bestface1area  = -9999;
	bestface2area  = -9999;
	sharededgenum  = 0; // make compiler happy
	lowestedgenum  = 0; // make compiler happy

	//
	for( i = 0; i < area1->numfaces; i++ ) {
		face1num = ( *aasworld ).faceindex[area1->firstface + i];
		face1	 = &( *aasworld ).faces[abs( face1num )];

		// if not a ladder face
		if( !( face1->faceflags & FACE_LADDER ) ) {
			continue;
		}

		//
		for( j = 0; j < area2->numfaces; j++ ) {
			face2num = ( *aasworld ).faceindex[area2->firstface + j];
			face2	 = &( *aasworld ).faces[abs( face2num )];

			// if not a ladder face
			if( !( face2->faceflags & FACE_LADDER ) ) {
				continue;
			}

			// check if the faces share an edge
			for( k = 0; k < face1->numedges; k++ ) {
				edge1num = ( *aasworld ).edgeindex[face1->firstedge + k];

				for( l = 0; l < face2->numedges; l++ ) {
					edge2num = ( *aasworld ).edgeindex[face2->firstedge + l];

					if( abs( edge1num ) == abs( edge2num ) ) {
						// get the face with the largest area
						face1area = AAS_FaceArea( face1 );
						face2area = AAS_FaceArea( face2 );

						if( face1area > bestface1area && face2area > bestface2area ) {
							bestface1area  = face1area;
							bestface2area  = face2area;
							ladderface1	   = face1;
							ladderface2	   = face2;
							ladderface1num = face1num;
							ladderface2num = face2num;
							sharededgenum  = edge1num;
						}

						break;
					}
				}

				if( l != face2->numedges ) {
					break;
				}
			}
		}
	}

	//
	if( ladderface1 && ladderface2 ) {
		// get the middle of the shared edge
		sharededge = &( *aasworld ).edges[abs( sharededgenum )];
		firstv	   = sharededgenum < 0;
		//
		VectorCopy( ( *aasworld ).vertexes[sharededge->v[firstv]], v1 );
		VectorCopy( ( *aasworld ).vertexes[sharededge->v[!firstv]], v2 );
		VectorAdd( v1, v2, area1point );
		VectorScale( area1point, 0.5, area1point );
		VectorCopy( area1point, area2point );
		//
		// if the face plane in area 1 is pretty much vertical
		plane1 = &( *aasworld ).planes[ladderface1->planenum ^ ( ladderface1num < 0 )];
		plane2 = &( *aasworld ).planes[ladderface2->planenum ^ ( ladderface2num < 0 )];
		//
		// get the points really into the areas
		VectorSubtract( v2, v1, sharededgevec );
		CrossProduct( plane1->normal, sharededgevec, dir );
		VectorNormalize( dir );
		// NOTE: 32 because that's larger than 16 (bot bbox x,y)
		VectorMA( area1point, -32, dir, area1point );
		VectorMA( area2point, 32, dir, area2point );
		//
		ladderface1vertical = fabs( DotProduct( plane1->normal, up ) ) < 0.1;
		ladderface2vertical = fabs( DotProduct( plane2->normal, up ) ) < 0.1;

		// there's only reachability between vertical ladder faces
		if( !ladderface1vertical && !ladderface2vertical ) {
			return qfalse;
		}

		// if both vertical ladder faces
		if( ladderface1vertical &&
			ladderface2vertical
			// and the ladder faces do not make a sharp corner
			&& DotProduct( plane1->normal, plane2->normal ) > 0.7
			// and the shared edge is not too vertical
			&& fabs( DotProduct( sharededgevec, up ) ) < 0.7 ) {
			// create a new reachability link
			lreach = AAS_AllocReachability();

			if( !lreach ) {
				return qfalse;
			}

			lreach->areanum = area2num;
			lreach->facenum = ladderface1num;
			lreach->edgenum = abs( sharededgenum );
			VectorCopy( area1point, lreach->start );
			// VectorCopy(area2point, lreach->end);
			VectorMA( area2point, -3, plane1->normal, lreach->end );
			lreach->traveltype		   = TRAVEL_LADDER;
			lreach->traveltime		   = 10;
			lreach->next			   = areareachability[area1num];
			areareachability[area1num] = lreach;
			//
			reach_ladder++;
			// create a new reachability link
			lreach = AAS_AllocReachability();

			if( !lreach ) {
				return qfalse;
			}

			lreach->areanum = area1num;
			lreach->facenum = ladderface2num;
			lreach->edgenum = abs( sharededgenum );
			VectorCopy( area2point, lreach->start );
			// VectorCopy(area1point, lreach->end);
			VectorMA( area1point, -3, plane2->normal, lreach->end );
			lreach->traveltype		   = TRAVEL_LADDER;
			lreach->traveltime		   = 10;
			lreach->next			   = areareachability[area2num];
			areareachability[area2num] = lreach;
			//
			reach_ladder++;
			//
			return qtrue;
		}

		// if the second ladder face is also a ground face
		// create ladder end (just ladder) reachability and
		// walk off a ladder (ledge) reachability
		if( ladderface1vertical && ( ladderface2->faceflags & FACE_GROUND ) ) {
			// create a new reachability link
			lreach = AAS_AllocReachability();

			if( !lreach ) {
				return qfalse;
			}

			lreach->areanum = area2num;
			lreach->facenum = ladderface1num;
			lreach->edgenum = abs( sharededgenum );
			VectorCopy( area1point, lreach->start );
			VectorCopy( area2point, lreach->end );
			lreach->end[2] += 16;
			VectorMA( lreach->end, -15, plane1->normal, lreach->end );
			lreach->traveltype		   = TRAVEL_LADDER;
			lreach->traveltime		   = 10;
			lreach->next			   = areareachability[area1num];
			areareachability[area1num] = lreach;
			//
			reach_ladder++;
			// create a new reachability link
			lreach = AAS_AllocReachability();

			if( !lreach ) {
				return qfalse;
			}

			lreach->areanum = area1num;
			lreach->facenum = ladderface2num;
			lreach->edgenum = abs( sharededgenum );
			VectorCopy( area2point, lreach->start );
			VectorCopy( area1point, lreach->end );
			lreach->traveltype		   = TRAVEL_WALKOFFLEDGE;
			lreach->traveltime		   = 10;
			lreach->next			   = areareachability[area2num];
			areareachability[area2num] = lreach;
			//
			reach_walkoffledge++;
			//
			return qtrue;
		}

		//
		if( ladderface1vertical ) {
			// find lowest edge of the ladder face
			lowestpoint[2] = 99999;

			for( i = 0; i < ladderface1->numedges; i++ ) {
				edge1num = abs( ( *aasworld ).edgeindex[ladderface1->firstedge + i] );
				edge1	 = &( *aasworld ).edges[edge1num];
				//
				VectorCopy( ( *aasworld ).vertexes[edge1->v[0]], v1 );
				VectorCopy( ( *aasworld ).vertexes[edge1->v[1]], v2 );
				//
				VectorAdd( v1, v2, mid );
				VectorScale( mid, 0.5, mid );

				//
				if( mid[2] < lowestpoint[2] ) {
					VectorCopy( mid, lowestpoint );
					lowestedgenum = edge1num;
				}
			}

			//
			plane1 = &( *aasworld ).planes[ladderface1->planenum];
			// trace down in the middle of this edge
			VectorMA( lowestpoint, 5, plane1->normal, start );
			VectorCopy( start, end );
			start[2] += 5;
			end[2] -= 100;
			// trace without entity collision
			trace = AAS_TraceClientBBox( start, end, PRESENCE_NORMAL, -1 );
			//
			//
#ifdef REACHDEBUG

			if( trace.startsolid ) {
				Log_Write( "trace from area %d started in solid\r\n", area1num );
			}

#endif // REACHDEBUG
	   //
			trace.endpos[2] += 1;
			area2num = AAS_PointAreaNum( trace.endpos );
			//
			area2 = &( *aasworld ).areas[area2num];

			for( i = 0; i < area2->numfaces; i++ ) {
				face2num = ( *aasworld ).faceindex[area2->firstface + i];
				face2	 = &( *aasworld ).faces[abs( face2num )];

				//
				if( face2->faceflags & FACE_LADDER ) {
					plane2 = &( *aasworld ).planes[face2->planenum];

					if( fabs( DotProduct( plane2->normal, up ) ) < 0.1 ) {
						break;
					}
				}
			}

			// if from another area without vertical ladder faces
			if( i >= area2->numfaces && area2num != area1num &&
				// the reachabilities shouldn't exist already
				!AAS_ReachabilityExists( area1num, area2num ) && !AAS_ReachabilityExists( area2num, area1num ) ) {
				// if the height is jumpable
				if( start[2] - trace.endpos[2] < maxjumpheight ) {
					// create a new reachability link
					lreach = AAS_AllocReachability();

					if( !lreach ) {
						return qfalse;
					}

					lreach->areanum = area2num;
					lreach->facenum = ladderface1num;
					lreach->edgenum = lowestedgenum;
					VectorCopy( lowestpoint, lreach->start );
					VectorCopy( trace.endpos, lreach->end );
					lreach->traveltype		   = TRAVEL_LADDER;
					lreach->traveltime		   = 10;
					lreach->next			   = areareachability[area1num];
					areareachability[area1num] = lreach;
					//
					reach_ladder++;
					// create a new reachability link
					lreach = AAS_AllocReachability();

					if( !lreach ) {
						return qfalse;
					}

					lreach->areanum = area1num;
					lreach->facenum = ladderface1num;
					lreach->edgenum = lowestedgenum;
					VectorCopy( trace.endpos, lreach->start );
					// get the end point a little bit into the ladder
					VectorMA( lowestpoint, -5, plane1->normal, lreach->end );
					// get the end point a little higher
					lreach->end[2] += 10;
					lreach->traveltype		   = TRAVEL_JUMP;
					lreach->traveltime		   = 10;
					lreach->next			   = areareachability[area2num];
					areareachability[area2num] = lreach;
					//
					reach_jump++;
					//
					return qtrue;
#ifdef REACHDEBUG
					Log_Write( "jump up to ladder reach between %d and %d\r\n", area2num, area1num );
#endif // REACHDEBUG
				}
#ifdef REACHDEBUG

				else {
					Log_Write( "jump too high between area %d and %d\r\n", area2num, area1num );
				}

#endif // REACHDEBUG
			}
		}
	}

	return qfalse;
}

/*!
	\brief Creates teleport reachability links between teleporter triggers and their destinations.

	The function scans all BSP entities for teleporter-related triggers – either trigger_multiple or trigger_teleport – and attempts to match them with corresponding target_teleporter and
   misc_teleporter_dest entities. For each matching pair it resolves the destination’s world position, verifies the destination is within a valid area (and adjusts for ground-level anomalies), and
   then creates a reachability record linking the source area to the destination area with travel type TRAVEL_TELEPORT and a constant travel time defined by TELEPORT_TIME. The reachability links are
   stored in the global area reachability list, and a global counter of teleport links is incremented. Errors such as missing targets or destinations are logged via botimport.Print.


*/
void AAS_Reachability_Teleport()
{
	int					 area1num, area2num;
	char				 target[MAX_EPAIRKEY], targetname[MAX_EPAIRKEY];
	char				 classname[MAX_EPAIRKEY], model[MAX_EPAIRKEY];
	int					 ent, dest;
	vec3_t				 origin, destorigin, mins, maxs, end, angles = { 0, 0, 0 };
	vec3_t				 mid;
	aas_lreachability_t* lreach;
	aas_trace_t			 trace;
	aas_link_t *		 areas, *link;

	for( ent = AAS_NextBSPEntity( 0 ); ent; ent = AAS_NextBSPEntity( ent ) ) {
		if( !AAS_ValueForBSPEpairKey( ent, "classname", classname, MAX_EPAIRKEY ) ) {
			continue;
		}

		if( !strcmp( classname, "trigger_multiple" ) ) {
			AAS_ValueForBSPEpairKey( ent, "model", model, MAX_EPAIRKEY );
			// #ifdef REACHDEBUG
			botimport.Print( PRT_MESSAGE, "trigger_multiple model = \"%s\"\n", model );
			// #endif REACHDEBUG
			AAS_BSPModelMinsMaxsOrigin( atoi( model + 1 ), angles, mins, maxs, origin );

			//
			if( !AAS_ValueForBSPEpairKey( ent, "target", target, MAX_EPAIRKEY ) ) {
				botimport.Print( PRT_ERROR, "trigger_multiple at %1.0f %1.0f %1.0f without target\n", origin[0], origin[1], origin[2] );
				continue;
			}

			for( dest = AAS_NextBSPEntity( 0 ); dest; dest = AAS_NextBSPEntity( dest ) ) {
				if( !AAS_ValueForBSPEpairKey( dest, "classname", classname, MAX_EPAIRKEY ) ) {
					continue;
				}

				if( !strcmp( classname, "target_teleporter" ) ) {
					if( !AAS_ValueForBSPEpairKey( dest, "targetname", targetname, MAX_EPAIRKEY ) ) {
						continue;
					}

					if( !strcmp( targetname, target ) ) {
						break;
					}
				}
			}

			if( !dest ) {
				continue;
			}

			if( !AAS_ValueForBSPEpairKey( dest, "target", target, MAX_EPAIRKEY ) ) {
				botimport.Print( PRT_ERROR, "target_teleporter without target\n" );
				continue;
			}

		} else if( !strcmp( classname, "trigger_teleport" ) ) {
			AAS_ValueForBSPEpairKey( ent, "model", model, MAX_EPAIRKEY );
			// #ifdef REACHDEBUG
			botimport.Print( PRT_MESSAGE, "trigger_teleport model = \"%s\"\n", model );
			// #endif REACHDEBUG
			AAS_BSPModelMinsMaxsOrigin( atoi( model + 1 ), angles, mins, maxs, origin );

			//
			if( !AAS_ValueForBSPEpairKey( ent, "target", target, MAX_EPAIRKEY ) ) {
				botimport.Print( PRT_ERROR, "trigger_teleport at %1.0f %1.0f %1.0f without target\n", origin[0], origin[1], origin[2] );
				continue;
			}

		} else {
			continue;
		}

		//
		for( dest = AAS_NextBSPEntity( 0 ); dest; dest = AAS_NextBSPEntity( dest ) ) {
			// classname should be misc_teleporter_dest
			// but I've also seen target_position and actually any
			// entity could be used... burp
			if( AAS_ValueForBSPEpairKey( dest, "targetname", targetname, MAX_EPAIRKEY ) ) {
				if( !strcmp( targetname, target ) ) {
					break;
				}
			}
		}

		if( !dest ) {
			botimport.Print( PRT_ERROR, "teleporter without misc_teleporter_dest (%s)\n", target );
			continue;
		}

		if( !AAS_VectorForBSPEpairKey( dest, "origin", destorigin ) ) {
			botimport.Print( PRT_ERROR, "teleporter destination (%s) without origin\n", target );
			continue;
		}

		//
		area2num = AAS_PointAreaNum( destorigin );

		// if not teleported into a teleporter or into a jumppad
		if( !AAS_AreaTeleporter( area2num ) && !AAS_AreaJumpPad( area2num ) ) {
			destorigin[2] += 24; // just for q2e1m2, the dork has put the telepads in the ground
			VectorCopy( destorigin, end );
			end[2] -= 100;
			trace = AAS_TraceClientBBox( destorigin, end, PRESENCE_CROUCH, -1 );

			if( trace.startsolid ) {
				botimport.Print( PRT_ERROR, "teleporter destination (%s) in solid\n", target );
				continue;
			}

			VectorCopy( trace.endpos, destorigin );
			area2num = AAS_PointAreaNum( destorigin );
		}

		//
		// botimport.Print(PRT_MESSAGE, "teleporter brush origin at %f %f %f\n", origin[0], origin[1], origin[2]);
		// botimport.Print(PRT_MESSAGE, "teleporter brush mins = %f %f %f\n", mins[0], mins[1], mins[2]);
		// botimport.Print(PRT_MESSAGE, "teleporter brush maxs = %f %f %f\n", maxs[0], maxs[1], maxs[2]);
		VectorAdd( origin, mins, mins );
		VectorAdd( origin, maxs, maxs );
		//
		VectorAdd( mins, maxs, mid );
		VectorScale( mid, 0.5, mid );
		// link an invalid (-1) entity
		areas = AAS_LinkEntityClientBBox( mins, maxs, -1, PRESENCE_CROUCH );

		if( !areas ) {
			botimport.Print( PRT_MESSAGE, "trigger_multiple not in any area\n" );
		}

		//
		for( link = areas; link; link = link->next_area ) {
			// if (!AAS_AreaGrounded(link->areanum)) continue;
			if( !AAS_AreaTeleporter( link->areanum ) ) {
				continue;
			}

			//
			area1num = link->areanum;
			// create a new reachability link
			lreach = AAS_AllocReachability();

			if( !lreach ) {
				break;
			}

			lreach->areanum = area2num;
			lreach->facenum = 0;
			lreach->edgenum = 0;
			VectorCopy( mid, lreach->start );
			VectorCopy( destorigin, lreach->end );
			lreach->traveltype		   = TRAVEL_TELEPORT;
			lreach->traveltime		   = TELEPORT_TIME;
			lreach->next			   = areareachability[area1num];
			areareachability[area1num] = lreach;
			//
			reach_teleport++;
		}

		// unlink the invalid entity
		AAS_UnlinkFromAreas( areas );
	}
}

#define REACHDEBUG

/*!
	\brief Calculates elevator reachability between different areas in the AAS navigation system

	This function processes elevator and platform entities in the BSP map to determine reachability between different navigation areas. It analyzes func_plat entities and calculates transitions
   between bottom and top positions of platforms. The function handles multiple check points around the platform edges and center to find valid areas that can be reached. It uses AAS trace functions
   to verify valid paths between platform positions and adjacent areas. The algorithm computes movement distances, speeds, and creates reachability links between areas based on platform movement
   patterns. It processes both the bottom and top positions of platforms, checking for grounded or swim areas around these positions to establish valid navigation transitions.

	\return None
	\throws None
*/
void AAS_Reachability_Elevator()
{
	int					 area1num, area2num, modelnum, i, j, k, l, n, p;
	float				 lip, height, speed;
	char				 model[MAX_EPAIRKEY], classname[MAX_EPAIRKEY];
	int					 ent;
	vec3_t				 mins, maxs, origin, angles = { 0, 0, 0 };
	vec3_t				 pos1, pos2, mids, platbottom, plattop;
	vec3_t				 bottomorg, toporg, start, end, dir;
	vec_t				 xvals[8], yvals[8], xvals_top[8], yvals_top[8];
	aas_lreachability_t* lreach;
	aas_trace_t			 trace;

#ifdef REACHDEBUG
	Log_Write( "AAS_Reachability_Elevator\r\n" );
#endif // REACHDEBUG

	for( ent = AAS_NextBSPEntity( 0 ); ent; ent = AAS_NextBSPEntity( ent ) ) {
		if( !AAS_ValueForBSPEpairKey( ent, "classname", classname, MAX_EPAIRKEY ) ) {
			continue;
		}

		if( !strcmp( classname, "func_plat" ) ) {
#ifdef REACHDEBUG
			Log_Write( "found func plat\r\n" );
#endif // REACHDEBUG

			if( !AAS_ValueForBSPEpairKey( ent, "model", model, MAX_EPAIRKEY ) ) {
				botimport.Print( PRT_ERROR, "func_plat without model\n" );
				continue;
			}

			// get the model number, and skip the leading *
			modelnum = atoi( model + 1 );

			if( modelnum <= 0 ) {
				botimport.Print( PRT_ERROR, "func_plat with invalid model number\n" );
				continue;
			}

			// get the mins, maxs and origin of the model
			// NOTE: the origin is usually (0,0,0) and the mins and maxs
			//       are the absolute mins and maxs
			AAS_BSPModelMinsMaxsOrigin( modelnum, angles, mins, maxs, origin );
			//
			AAS_VectorForBSPEpairKey( ent, "origin", origin );
			// pos1 is the top position, pos2 is the bottom
			VectorCopy( origin, pos1 );
			VectorCopy( origin, pos2 );
			// get the lip of the plat
			AAS_FloatForBSPEpairKey( ent, "lip", &lip );

			if( !lip ) {
				lip = 8;
			}

			// get the movement height of the plat
			AAS_FloatForBSPEpairKey( ent, "height", &height );

			if( !height ) {
				height = ( maxs[2] - mins[2] ) - lip;
			}

			// get the speed of the plat
			AAS_FloatForBSPEpairKey( ent, "speed", &speed );

			if( !speed ) {
				speed = 200;
			}

			// get bottom position below pos1
			pos2[2] -= height;
			//
			botimport.Print( PRT_MESSAGE, "pos2[2] = %1.1f pos1[2] = %1.1f\n", pos2[2], pos1[2] );
			// get a point just above the plat in the bottom position
			VectorAdd( mins, maxs, mids );
			VectorMA( pos2, 0.5, mids, platbottom );
			platbottom[2] = maxs[2] - ( pos1[2] - pos2[2] ) + 2;
			// get a point just above the plat in the top position
			VectorAdd( mins, maxs, mids );
			VectorMA( pos2, 0.5, mids, plattop );
			plattop[2] = maxs[2] + 2;

			//
			/*if (!area1num)
			{
				Log_Write("no grounded area near plat bottom\r\n");
				continue;
			} //end if*/
			// get the mins and maxs a little larger
			for( i = 0; i < 3; i++ ) {
				mins[i] -= 1;
				maxs[i] += 1;
			}

			//
			botimport.Print( PRT_MESSAGE, "platbottom[2] = %1.1f plattop[2] = %1.1f\n", platbottom[2], plattop[2] );
			//
			VectorAdd( mins, maxs, mids );
			VectorScale( mids, 0.5, mids );
			//
			xvals[0] = mins[0];
			xvals[1] = mids[0];
			xvals[2] = maxs[0];
			xvals[3] = mids[0];
			yvals[0] = mids[1];
			yvals[1] = maxs[1];
			yvals[2] = mids[1];
			yvals[3] = mins[1];
			//
			xvals[4] = mins[0];
			xvals[5] = maxs[0];
			xvals[6] = maxs[0];
			xvals[7] = mins[0];
			yvals[4] = maxs[1];
			yvals[5] = maxs[1];
			yvals[6] = mins[1];
			yvals[7] = mins[1];

			// find adjacent areas around the bottom of the plat
			for( i = 0; i < 9; i++ ) {
				if( i < 8 ) { // check at the sides of the plat
					bottomorg[0] = origin[0] + xvals[i];
					bottomorg[1] = origin[1] + yvals[i];
					bottomorg[2] = platbottom[2] + 16;
					// get a grounded or swim area near the plat in the bottom position
					area1num = AAS_PointAreaNum( bottomorg );

					for( k = 0; k < 16; k++ ) {
						if( area1num ) {
							if( AAS_AreaGrounded( area1num ) || AAS_AreaSwim( area1num ) ) {
								break;
							}
						}

						bottomorg[2] += 4;
						area1num = AAS_PointAreaNum( bottomorg );
					}

					// if in solid
					if( k >= 16 ) {
						continue;
					}

				} else { // at the middle of the plat
					VectorCopy( plattop, bottomorg );
					bottomorg[2] += 24;
					area1num = AAS_PointAreaNum( bottomorg );

					if( !area1num ) {
						continue;
					}

					VectorCopy( platbottom, bottomorg );
					bottomorg[2] += 24;
				}

				// look at adjacent areas around the top of the plat
				// make larger steps to outside the plat everytime
				for( n = 0; n < 3; n++ ) {
					for( k = 0; k < 3; k++ ) {
						mins[k] -= 4;
						maxs[k] += 4;
					}

					xvals_top[0] = mins[0];
					xvals_top[1] = mids[0];
					xvals_top[2] = maxs[0];
					xvals_top[3] = mids[0];
					yvals_top[0] = mids[1];
					yvals_top[1] = maxs[1];
					yvals_top[2] = mids[1];
					yvals_top[3] = mins[1];
					//
					xvals_top[4] = mins[0];
					xvals_top[5] = maxs[0];
					xvals_top[6] = maxs[0];
					xvals_top[7] = mins[0];
					yvals_top[4] = maxs[1];
					yvals_top[5] = maxs[1];
					yvals_top[6] = mins[1];
					yvals_top[7] = mins[1];

					//
					for( j = 0; j < 8; j++ ) {
						toporg[0] = origin[0] + xvals_top[j];
						toporg[1] = origin[1] + yvals_top[j];
						toporg[2] = plattop[2] + 16;
						// get a grounded or swim area near the plat in the top position
						area2num = AAS_PointAreaNum( toporg );

						for( l = 0; l < 16; l++ ) {
							if( area2num ) {
								if( AAS_AreaGrounded( area2num ) || AAS_AreaSwim( area2num ) ) {
									VectorCopy( plattop, start );
									start[2] += 32;
									VectorCopy( toporg, end );
									end[2] += 1;
									trace = AAS_TraceClientBBox( start, end, PRESENCE_CROUCH, -1 );

									if( trace.fraction >= 1 ) {
										break;
									}
								}
							}

							toporg[2] += 4;
							area2num = AAS_PointAreaNum( toporg );
						}

						// if in solid
						if( l >= 16 ) {
							continue;
						}

						// never create a reachability in the same area
						if( area2num == area1num ) {
							continue;
						}

						// if the area isn't grounded
						if( !AAS_AreaGrounded( area2num ) ) {
							continue;
						}

						// if there already exists reachability between the areas
						if( AAS_ReachabilityExists( area1num, area2num ) ) {
							continue;
						}

						// if the reachability start is within the elevator bounding box
						VectorSubtract( bottomorg, platbottom, dir );
						VectorNormalize( dir );
						dir[0] = bottomorg[0] + 24 * dir[0];
						dir[1] = bottomorg[1] + 24 * dir[1];
						dir[2] = bottomorg[2];

						//
						for( p = 0; p < 3; p++ )
							if( dir[p] < origin[p] + mins[p] || dir[p] > origin[p] + maxs[p] ) {
								break;
							}

						if( p >= 3 ) {
							continue;
						}

						// create a new reachability link
						lreach = AAS_AllocReachability();

						if( !lreach ) {
							continue;
						}

						lreach->areanum = area2num;
						// the facenum is the model number
						lreach->facenum = modelnum;
						// the edgenum is the height
						lreach->edgenum = ( int )height;
						//
						VectorCopy( dir, lreach->start );
						VectorCopy( toporg, lreach->end );
						lreach->traveltype = TRAVEL_ELEVATOR;
						lreach->traveltime = height * 100 / speed;

						if( !lreach->traveltime ) {
							lreach->traveltime = 50;
						}

						lreach->next			   = areareachability[area1num];
						areareachability[area1num] = lreach;
						// don't go any further to the outside
						n = 9999;
						//
#ifdef REACHDEBUG
						Log_Write( "elevator reach from %d to %d\r\n", area1num, area2num );
#endif // REACHDEBUG
	   //
						reach_elevator++;
					}
				}
			}
		}
	}
}

/*!
	\brief Determines reachable areas from a polygon of face points by evaluating ground faces and generating reachability structures.

	The function scans all ground faces in the AAS world, computing the minimal distance between the edges of a supplied polygon and those of each ground face. It keeps track of the best matching face
   and verifies several geometric constraints: horizontal distance must be within a jump limit, vertical differences are bounded, and the target face must be reachable in height. When a suitable face
   is found, the function creates a reachability entry with start and end positions derived from the closest edge pair, linking it into a returned list. If no valid reachability is found, or memory
   allocation fails, the function returns the list built so far (which may be empty).

	\param facepoints Array of vertices defining the polygon to evaluate; must form a closed loop
	\param numpoints Number of vertices in facepoints
	\param plane Plane of the original face for projecting test points
	\param towardsface If true, search is directed towards the face; if false, directed away
	\return Pointer to a linked list of reachability structures; NULL on failure or no matches
*/
aas_lreachability_t* AAS_FindFaceReachabilities( vec3_t* facepoints, int numpoints, aas_plane_t* plane, int towardsface )
{
	int					 i, j, k, l;
	int					 facenum, edgenum, bestfacenum;
	float *				 v1, *v2, *v3, *v4;
	float				 bestdist, speed, hordist, dist;
	vec3_t				 beststart, beststart2, bestend, bestend2, tmp, hordir, testpoint;
	aas_lreachability_t *lreach, *lreachabilities;
	aas_area_t*			 area;
	aas_face_t*			 face;
	aas_edge_t*			 edge;
	aas_plane_t *		 faceplane, *bestfaceplane;

	//
	lreachabilities = NULL;
	bestfacenum		= 0;
	bestfaceplane	= NULL;

	//
	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		area = &( *aasworld ).areas[i];
		// get the shortest distance between one of the func_bob start edges and
		// one of the face edges of area1
		bestdist = 999999;

		for( j = 0; j < area->numfaces; j++ ) {
			facenum = ( *aasworld ).faceindex[area->firstface + j];
			face	= &( *aasworld ).faces[abs( facenum )];

			// if not a ground face
			if( !( face->faceflags & FACE_GROUND ) ) {
				continue;
			}

			// get the ground planes
			faceplane = &( *aasworld ).planes[face->planenum];

			//
			for( k = 0; k < face->numedges; k++ ) {
				edgenum = abs( ( *aasworld ).edgeindex[face->firstedge + k] );
				edge	= &( *aasworld ).edges[edgenum];
				// calculate the minimum distance between the two edges
				v1 = ( *aasworld ).vertexes[edge->v[0]];
				v2 = ( *aasworld ).vertexes[edge->v[1]];

				//
				for( l = 0; l < numpoints; l++ ) {
					v3	 = facepoints[l];
					v4	 = facepoints[( l + 1 ) % numpoints];
					dist = AAS_ClosestEdgePoints( v1, v2, v3, v4, faceplane, plane, beststart, bestend, beststart2, bestend2, bestdist );

					if( dist < bestdist ) {
						bestfacenum	  = facenum;
						bestfaceplane = faceplane;
						bestdist	  = dist;
					}
				}
			}
		}

		//
		if( bestdist > 192 ) {
			continue;
		}

		//
		VectorMiddle( beststart, beststart2, beststart );
		VectorMiddle( bestend, bestend2, bestend );

		//
		if( !towardsface ) {
			VectorCopy( beststart, tmp );
			VectorCopy( bestend, beststart );
			VectorCopy( tmp, bestend );
		}

		//
		VectorSubtract( bestend, beststart, hordir );
		hordir[2] = 0;
		hordist	  = VectorLength( hordir );

		//
		if( hordist > 2 * AAS_MaxJumpDistance( aassettings.sv_jumpvel ) ) {
			continue;
		}

		// the end point should not be significantly higher than the start point
		if( bestend[2] - 32 > beststart[2] ) {
			continue;
		}

		// don't fall down too far
		if( bestend[2] < beststart[2] - 128 ) {
			continue;
		}

		// the distance should not be too far
		if( hordist > 32 ) {
			// check for walk off ledge
			if( !AAS_HorizontalVelocityForJump( 0, beststart, bestend, &speed ) ) {
				continue;
			}
		}

		//
		beststart[2] += 1;
		bestend[2] += 1;

		//
		if( towardsface ) {
			VectorCopy( bestend, testpoint );

		} else {
			VectorCopy( beststart, testpoint );
		}

		testpoint[2] = 0;
		testpoint[2] = ( bestfaceplane->dist - DotProduct( bestfaceplane->normal, testpoint ) ) / bestfaceplane->normal[2];

		//
		if( !AAS_PointInsideFace( bestfacenum, testpoint, 0.1 ) ) {
			// if the faces are not overlapping then only go down
			if( bestend[2] - 16 > beststart[2] ) {
				continue;
			}
		}

		lreach = AAS_AllocReachability();

		if( !lreach ) {
			return lreachabilities;
		}

		lreach->areanum = i;
		lreach->facenum = 0;
		lreach->edgenum = 0;
		VectorCopy( beststart, lreach->start );
		VectorCopy( bestend, lreach->end );
		lreach->traveltype = 0;
		lreach->traveltime = 0;
		lreach->next	   = lreachabilities;
		lreachabilities	   = lreach;
#ifndef BSPC

		if( towardsface ) {
			AAS_PermanentLine( lreach->start, lreach->end, 1 );

		} else {
			AAS_PermanentLine( lreach->start, lreach->end, 2 );
		}

#endif
	}

	return lreachabilities;
}

/*!
	\brief Processes func_bobbing entities to calculate and generate reachability information for navigation.

	This function iterates through all BSP entities to find func_bobbing objects, which represent moving platforms in the game environment. For each func_bobbing entity, it determines the bounding
   box, calculates the start and end positions of the platform's movement, and generates reachability links between areas that the platform can connect. It handles different axis of movement based on
   spawn flags and computes the reachability edges using face-based algorithms. The function logs the movement details and ensures that the generated reachabilities are valid by checking the connected
   areas.

*/
void AAS_Reachability_FuncBobbing()
{
	int					 ent, spawnflags, modelnum, axis;
	int					 i, numareas, areas[10];
	char				 classname[MAX_EPAIRKEY], model[MAX_EPAIRKEY];
	vec3_t				 origin, move_end, move_start, move_start_top, move_end_top;
	vec3_t				 mins, maxs, angles = { 0, 0, 0 };
	vec3_t				 start_edgeverts[4], end_edgeverts[4], mid;
	vec3_t				 org, start, end, dir, points[10];
	float				 height;
	aas_plane_t			 start_plane, end_plane;
	aas_lreachability_t *startreach, *endreach, *nextstartreach, *nextendreach, *lreach;
	aas_lreachability_t *firststartreach, *firstendreach;

	for( ent = AAS_NextBSPEntity( 0 ); ent; ent = AAS_NextBSPEntity( ent ) ) {
		if( !AAS_ValueForBSPEpairKey( ent, "classname", classname, MAX_EPAIRKEY ) ) {
			continue;
		}

		if( strcmp( classname, "func_bobbing" ) ) {
			continue;
		}

		AAS_FloatForBSPEpairKey( ent, "height", &height );

		if( !height ) {
			height = 32;
		}

		//
		if( !AAS_ValueForBSPEpairKey( ent, "model", model, MAX_EPAIRKEY ) ) {
			botimport.Print( PRT_ERROR, "func_bobbing without model\n" );
			continue;
		}

		// get the model number, and skip the leading *
		modelnum = atoi( model + 1 );

		if( modelnum <= 0 ) {
			botimport.Print( PRT_ERROR, "func_bobbing with invalid model number\n" );
			continue;
		}

		//
		AAS_BSPModelMinsMaxsOrigin( modelnum, angles, mins, maxs, NULL );
		//
		VectorAdd( mins, maxs, mid );
		VectorScale( mid, 0.5, mid );
		// VectorAdd(mid, origin, mid);
		VectorCopy( mid, origin );
		//
		VectorCopy( origin, move_end );
		VectorCopy( origin, move_start );
		//
		AAS_IntForBSPEpairKey( ent, "spawnflags", &spawnflags );

		// set the axis of bobbing
		if( spawnflags & 1 ) {
			axis = 0;

		} else if( spawnflags & 2 ) {
			axis = 1;

		} else {
			axis = 2;
		}

		//
		move_start[axis] -= height;
		move_end[axis] += height;
		//
		Log_Write( "funcbob model %d, start = {%1.1f, %1.1f, %1.1f} end = {%1.1f, %1.1f, %1.1f}\n", modelnum, move_start[0], move_start[1], move_start[2], move_end[0], move_end[1], move_end[2] );
		//
#ifndef BSPC
		/*
		AAS_DrawPermanentCross(move_start, 4, 1);
		AAS_DrawPermanentCross(move_end, 4, 2);
		*/
#endif

		//
		for( i = 0; i < 4; i++ ) {
			VectorCopy( move_start, start_edgeverts[i] );
			start_edgeverts[i][2] += maxs[2] - mid[2]; //+ bbox maxs z
			start_edgeverts[i][2] += 24;			   //+ player origin to ground dist
		}

		start_edgeverts[0][0] += maxs[0] - mid[0];
		start_edgeverts[0][1] += maxs[1] - mid[1];
		start_edgeverts[1][0] += maxs[0] - mid[0];
		start_edgeverts[1][1] += mins[1] - mid[1];
		start_edgeverts[2][0] += mins[0] - mid[0];
		start_edgeverts[2][1] += mins[1] - mid[1];
		start_edgeverts[3][0] += mins[0] - mid[0];
		start_edgeverts[3][1] += maxs[1] - mid[1];
		//
		start_plane.dist = start_edgeverts[0][2];
		VectorSet( start_plane.normal, 0, 0, 1 );

		//
		for( i = 0; i < 4; i++ ) {
			VectorCopy( move_end, end_edgeverts[i] );
			end_edgeverts[i][2] += maxs[2] - mid[2]; //+ bbox maxs z
			end_edgeverts[i][2] += 24;				 //+ player origin to ground dist
		}

		end_edgeverts[0][0] += maxs[0] - mid[0];
		end_edgeverts[0][1] += maxs[1] - mid[1];
		end_edgeverts[1][0] += maxs[0] - mid[0];
		end_edgeverts[1][1] += mins[1] - mid[1];
		end_edgeverts[2][0] += mins[0] - mid[0];
		end_edgeverts[2][1] += mins[1] - mid[1];
		end_edgeverts[3][0] += mins[0] - mid[0];
		end_edgeverts[3][1] += maxs[1] - mid[1];
		//
		end_plane.dist = end_edgeverts[0][2];
		VectorSet( end_plane.normal, 0, 0, 1 );
		//
#ifndef BSPC
		/*
		for (i = 0; i < 4; i++)
		{
			AAS_PermanentLine(start_edgeverts[i], start_edgeverts[(i+1)%4], 1);
			AAS_PermanentLine(end_edgeverts[i], end_edgeverts[(i+1)%4], 1);
		}
		*/
#endif
		VectorCopy( move_start, move_start_top );
		move_start_top[2] += maxs[2] - mid[2] + 24; //+ bbox maxs z
		VectorCopy( move_end, move_end_top );
		move_end_top[2] += maxs[2] - mid[2] + 24; //+ bbox maxs z

		//
		if( !AAS_PointAreaNum( move_start_top ) ) {
			continue;
		}

		if( !AAS_PointAreaNum( move_end_top ) ) {
			continue;
		}

		//
		for( i = 0; i < 2; i++ ) {
			firststartreach = firstendreach = NULL;

			//
			if( i == 0 ) {
				firststartreach = AAS_FindFaceReachabilities( start_edgeverts, 4, &start_plane, qtrue );
				firstendreach	= AAS_FindFaceReachabilities( end_edgeverts, 4, &end_plane, qfalse );

			} else {
				firststartreach = AAS_FindFaceReachabilities( end_edgeverts, 4, &end_plane, qtrue );
				firstendreach	= AAS_FindFaceReachabilities( start_edgeverts, 4, &start_plane, qfalse );
			}

			//
			// create reachabilities from start to end
			for( startreach = firststartreach; startreach; startreach = nextstartreach ) {
				nextstartreach = startreach->next;

				//
				// trace = AAS_TraceClientBBox(startreach->start, move_start_top, PRESENCE_NORMAL, -1);
				// if (trace.fraction < 1) continue;
				//
				for( endreach = firstendreach; endreach; endreach = nextendreach ) {
					nextendreach = endreach->next;
					//
					// trace = AAS_TraceClientBBox(endreach->end, move_end_top, PRESENCE_NORMAL, -1);
					// if (trace.fraction < 1) continue;
					//
					Log_Write( "funcbob reach from area %d to %d\n", startreach->areanum, endreach->areanum );

					//
					//
					if( i == 0 ) {
						VectorCopy( move_start_top, org );

					} else {
						VectorCopy( move_end_top, org );
					}

					VectorSubtract( startreach->start, org, dir );
					dir[2] = 0;
					VectorNormalize( dir );
					VectorCopy( startreach->start, start );
					VectorMA( startreach->start, 1, dir, start );
					start[2] += 1;
					VectorMA( startreach->start, 16, dir, end );
					end[2] += 1;
					//
					numareas = AAS_TraceAreas( start, end, areas, points, 10 );

					if( numareas <= 0 ) {
						continue;
					}

					if( numareas > 1 ) {
						VectorCopy( points[1], startreach->start );

					} else {
						VectorCopy( end, startreach->start );
					}

					//
					if( !AAS_PointAreaNum( startreach->start ) ) {
						continue;
					}

					if( !AAS_PointAreaNum( endreach->end ) ) {
						continue;
					}

					//
					lreach			= AAS_AllocReachability();
					lreach->areanum = endreach->areanum;

					if( i == 0 ) {
						lreach->edgenum = ( ( int )move_start[axis] << 16 ) | ( ( int )move_end[axis] & 0x0000ffff );

					} else {
						lreach->edgenum = ( ( int )move_end[axis] << 16 ) | ( ( int )move_start[axis] & 0x0000ffff );
					}

					lreach->facenum = ( spawnflags << 16 ) | modelnum;
					VectorCopy( startreach->start, lreach->start );
					VectorCopy( endreach->end, lreach->end );
#ifndef BSPC
					//					AAS_DrawArrow(lreach->start, lreach->end, LINECOLOR_BLUE, LINECOLOR_YELLOW);
					//					AAS_PermanentLine(lreach->start, lreach->end, 1);
#endif
					lreach->traveltype = TRAVEL_FUNCBOB;
					lreach->traveltime = 300;
					reach_funcbob++;
					lreach->next						  = areareachability[startreach->areanum];
					areareachability[startreach->areanum] = lreach;
					//
				}
			}

			for( startreach = firststartreach; startreach; startreach = nextstartreach ) {
				nextstartreach = startreach->next;
				AAS_FreeReachability( startreach );
			}

			for( endreach = firstendreach; endreach; endreach = nextendreach ) {
				nextendreach = endreach->next;
				AAS_FreeReachability( endreach );
			}

			// only go up with func_bobbing entities that go up and down
			if( !( spawnflags & 1 ) && !( spawnflags & 2 ) ) {
				break;
			}
		}
	}
}

/*!
	\brief Calculates and adds reachability information for jump pad trigger entities in the AAS navigation system

	This function processes trigger_push entities from the BSP file to determine jump pad reachability information. It analyzes the physics of the jump pad effects including gravity, velocity, and
   timing to calculate where players can reach from the jump pad area. The function creates jump pad reachability entries between areas that account for both horizontal and vertical movement during
   the jump pad effect.

	\return None
	\throws None
*/
void AAS_Reachability_JumpPad()
{
	int					 face2num, i, ret, modelnum, area2num, visualize;
	float				 speed, zvel, hordist, dist, time, height, gravity, forward;
	aas_face_t*			 face2;
	aas_area_t*			 area2;
	aas_lreachability_t* lreach;
	vec3_t				 areastart, facecenter, dir, cmdmove, teststart;
	vec3_t				 velocity, origin, ent2origin, angles, absmins, absmaxs;
	aas_clientmove_t	 move;
	aas_trace_t			 trace;
	int					 ent, ent2;
	aas_link_t *		 areas, *link;
	char				 target[MAX_EPAIRKEY], targetname[MAX_EPAIRKEY];
	char				 classname[MAX_EPAIRKEY], model[MAX_EPAIRKEY];

	for( ent = AAS_NextBSPEntity( 0 ); ent; ent = AAS_NextBSPEntity( ent ) ) {
		if( !AAS_ValueForBSPEpairKey( ent, "classname", classname, MAX_EPAIRKEY ) ) {
			continue;
		}

		if( strcmp( classname, "trigger_push" ) ) {
			continue;
		}

		//
		AAS_FloatForBSPEpairKey( ent, "speed", &speed );

		if( !speed ) {
			speed = 1000;
		}

		//		AAS_VectorForBSPEpairKey(ent, "angles", angles);
		//		AAS_SetMovedir(angles, velocity);
		//		VectorScale(velocity, speed, velocity);
		VectorClear( angles );
		// get the mins, maxs and origin of the model
		AAS_ValueForBSPEpairKey( ent, "model", model, MAX_EPAIRKEY );

		if( model[0] ) {
			modelnum = atoi( model + 1 );

		} else {
			modelnum = 0;
		}

		AAS_BSPModelMinsMaxsOrigin( modelnum, angles, absmins, absmaxs, origin );
		VectorAdd( origin, absmins, absmins );
		VectorAdd( origin, absmaxs, absmaxs );
		//
#ifdef REACHDEBUG
		botimport.Print( PRT_MESSAGE, "absmins = %f %f %f\n", absmins[0], absmins[1], absmins[2] );
		botimport.Print( PRT_MESSAGE, "absmaxs = %f %f %f\n", absmaxs[0], absmaxs[1], absmaxs[2] );
#endif // REACHDEBUG
		VectorAdd( absmins, absmaxs, origin );
		VectorScale( origin, 0.5, origin );

		// get the start areas
		VectorCopy( origin, teststart );
		teststart[2] += 64;
		trace = AAS_TraceClientBBox( teststart, origin, PRESENCE_CROUCH, -1 );

		if( trace.startsolid ) {
			botimport.Print( PRT_MESSAGE, "trigger_push start solid\n" );
			VectorCopy( origin, areastart );

		} else {
			VectorCopy( trace.endpos, areastart );
		}

		areastart[2] += 0.125;
		//
		// AAS_DrawPermanentCross(origin, 4, 4);
		// get the target entity
		AAS_ValueForBSPEpairKey( ent, "target", target, MAX_EPAIRKEY );

		for( ent2 = AAS_NextBSPEntity( 0 ); ent2; ent2 = AAS_NextBSPEntity( ent2 ) ) {
			if( !AAS_ValueForBSPEpairKey( ent2, "targetname", targetname, MAX_EPAIRKEY ) ) {
				continue;
			}

			if( !strcmp( targetname, target ) ) {
				break;
			}
		}

		if( !ent2 ) {
			botimport.Print( PRT_MESSAGE, "trigger_push without target entity %s\n", target );
			continue;
		}

		AAS_VectorForBSPEpairKey( ent2, "origin", ent2origin );
		//
		height	= ent2origin[2] - origin[2];
		gravity = aassettings.sv_gravity;
		time	= sqrt( height / ( 0.5 * gravity ) );

		if( !time ) {
			botimport.Print( PRT_MESSAGE, "trigger_push without time\n" );
			continue;
		}

		// set s.origin2 to the push velocity
		VectorSubtract( ent2origin, origin, velocity );
		dist	= VectorNormalize( velocity );
		forward = dist / time;
		// FIXME: why multiply by 1.1
		forward *= 1.1;
		VectorScale( velocity, forward, velocity );
		velocity[2] = time * gravity;
		// get the areas the jump pad brush is in
		areas = AAS_LinkEntityClientBBox( absmins, absmaxs, -1, PRESENCE_CROUCH );

		//*
		for( link = areas; link; link = link->next_area ) {
			if( link->areanum == 5772 ) {
				ret = qfalse;
			}
		} //*/

		for( link = areas; link; link = link->next_area ) {
			if( AAS_AreaJumpPad( link->areanum ) ) {
				break;
			}
		}

		if( !link ) {
			botimport.Print( PRT_MESSAGE, "trigger_multiple not in any jump pad area\n" );
			AAS_UnlinkFromAreas( areas );
			continue;
		}

		//
		botimport.Print( PRT_MESSAGE, "found a trigger_push with velocity %f %f %f\n", velocity[0], velocity[1], velocity[2] );

		// if there is a horizontal velocity check for a reachability without air control
		if( velocity[0] || velocity[1] ) {
			VectorSet( cmdmove, 0, 0, 0 );
			// VectorCopy(velocity, cmdmove);
			// cmdmove[2] = 0;
			memset( &move, 0, sizeof( aas_clientmove_t ) );
			area2num = 0;

			for( i = 0; i < 20; i++ ) {
				AAS_PredictClientMovement( &move,
					-1,
					areastart,
					PRESENCE_NORMAL,
					qfalse,
					velocity,
					cmdmove,
					0,
					30,
					0.1,
					SE_HITGROUND | SE_ENTERWATER | SE_ENTERSLIME | SE_ENTERLAVA | SE_HITGROUNDDAMAGE | SE_TOUCHJUMPPAD | SE_TOUCHTELEPORTER,
					0,
					qfalse ); // qtrue);
				area2num = AAS_PointAreaNum( move.endpos );

				for( link = areas; link; link = link->next_area ) {
					if( !AAS_AreaJumpPad( link->areanum ) ) {
						continue;
					}

					if( link->areanum == area2num ) {
						break;
					}
				}

				if( !link ) {
					break;
				}

				VectorCopy( move.endpos, areastart );
				VectorCopy( move.velocity, velocity );
			}

			if( area2num && i < 20 ) {
				for( link = areas; link; link = link->next_area ) {
					if( !AAS_AreaJumpPad( link->areanum ) ) {
						continue;
					}

					if( AAS_ReachabilityExists( link->areanum, area2num ) ) {
						continue;
					}

					// create a rocket or bfg jump reachability from area1 to area2
					lreach = AAS_AllocReachability();

					if( !lreach ) {
						AAS_UnlinkFromAreas( areas );
						return;
					}

					lreach->areanum = area2num;
					// NOTE: the facenum is the Z velocity
					lreach->facenum = velocity[2];
					// NOTE: the edgenum is the horizontal velocity
					lreach->edgenum = sqrt( velocity[0] * velocity[0] + velocity[1] * velocity[1] );
					VectorCopy( areastart, lreach->start );
					VectorCopy( move.endpos, lreach->end );
					lreach->traveltype				= TRAVEL_JUMPPAD;
					lreach->traveltime				= 200;
					lreach->next					= areareachability[link->areanum];
					areareachability[link->areanum] = lreach;
					//
					reach_jumppad++;
				}
			}
		}

		//
		if( fabs( velocity[0] ) > 100 || fabs( velocity[1] ) > 100 ) {
			continue;
		}

		// check for areas we can reach with air control
		for( area2num = 1; area2num < ( *aasworld ).numareas; area2num++ ) {
			visualize = qfalse;

			/*
			if (area2num == 3568)
			{
				for (link = areas; link; link = link->next_area)
				{
					if (link->areanum == 3380)
					{
						visualize = qtrue;
						botimport.Print(PRT_MESSAGE, "bah\n");
					}
				}
			} //end if*/
			// never try to go back to one of the original jumppad areas
			// and don't create reachabilities if they already exist
			for( link = areas; link; link = link->next_area ) {
				if( AAS_ReachabilityExists( link->areanum, area2num ) ) {
					break;
				}

				if( AAS_AreaJumpPad( link->areanum ) ) {
					if( link->areanum == area2num ) {
						break;
					}
				}
			}

			if( link ) {
				continue;
			}

			//
			area2 = &( *aasworld ).areas[area2num];

			for( i = 0; i < area2->numfaces; i++ ) {
				face2num = ( *aasworld ).faceindex[area2->firstface + i];
				face2	 = &( *aasworld ).faces[abs( face2num )];

				// if it is not a ground face
				if( !( face2->faceflags & FACE_GROUND ) ) {
					continue;
				}

				// get the center of the face
				AAS_FaceCenter( face2num, facecenter );

				// only go higher up
				if( facecenter[2] < areastart[2] ) {
					continue;
				}

				// get the jumppad jump z velocity
				zvel = velocity[2];
				// get the horizontal speed for the jump, if it isn't possible to calculate this
				// speed (the jump is not possible) then there's no jump reachability created
				ret = AAS_HorizontalVelocityForJump( zvel, areastart, facecenter, &speed );

				if( ret && speed < 150 ) {
					// direction towards the face center
					VectorSubtract( facecenter, areastart, dir );
					dir[2]	= 0;
					hordist = VectorNormalize( dir );
					// if (hordist < 1.6 * facecenter[2] - areastart[2])
					{
						// get command movement
						VectorScale( dir, speed, cmdmove );
						//
						AAS_PredictClientMovement( &move,
							-1,
							areastart,
							PRESENCE_NORMAL,
							qfalse,
							velocity,
							cmdmove,
							30,
							30,
							0.1,
							SE_ENTERWATER | SE_ENTERSLIME | SE_ENTERLAVA | SE_HITGROUNDDAMAGE | SE_TOUCHJUMPPAD | SE_TOUCHTELEPORTER | SE_HITGROUNDAREA,
							area2num,
							visualize );

						// if prediction time wasn't enough to fully predict the movement
						// don't enter slime or lava and don't fall from too high
						if( move.frames < 30 && !( move.stopevent & ( SE_ENTERSLIME | SE_ENTERLAVA | SE_HITGROUNDDAMAGE ) ) &&
							( move.stopevent & ( SE_HITGROUNDAREA | SE_TOUCHJUMPPAD | SE_TOUCHTELEPORTER ) ) ) {
							for( link = areas; link; link = link->next_area ) {
								if( !AAS_AreaJumpPad( link->areanum ) ) {
									continue;
								}

								if( AAS_ReachabilityExists( link->areanum, area2num ) ) {
									continue;
								}

								// create a jumppad reachability from area1 to area2
								lreach = AAS_AllocReachability();

								if( !lreach ) {
									AAS_UnlinkFromAreas( areas );
									return;
								}

								lreach->areanum = area2num;
								// NOTE: the facenum is the Z velocity
								lreach->facenum = velocity[2];
								// NOTE: the edgenum is the horizontal velocity
								lreach->edgenum = sqrt( cmdmove[0] * cmdmove[0] + cmdmove[1] * cmdmove[1] );
								VectorCopy( areastart, lreach->start );
								VectorCopy( facecenter, lreach->end );
								lreach->traveltype				= TRAVEL_JUMPPAD;
								lreach->traveltime				= 250;
								lreach->next					= areareachability[link->areanum];
								areareachability[link->areanum] = lreach;
								//
								reach_jumppad++;
							}
						}
					}
				}
			}
		}

		AAS_UnlinkFromAreas( areas );
	}
}

/*!
	\brief Adds a grapple reachability link between two areas when the path and geometry satisfy safety and movement constraints.

	The function first verifies that the source area is either grounded or swimming and not a crouch area, then ensures the target area is not significantly lower. It computes a starting position near
   the source area, optionally adjusting for swimming or solid checks. It iterates over the faces of the target area, filtering for solid, upward‑facing faces that can accommodate a grapple. For each
   suitable face, it performs several geometric checks: distance limits, minimum angle, skyline avoidance, and a trace to the face normal to ascertain a viable hook point. A second trace simulates the
   grapple’s path, ensuring the landing area is safe, not a lava source, and that it does not traverse cluster portals. If all tests pass and no existing reachability exists, a new reachability record
   is allocated and populated, updating the source area’s linked list. The function tracks successful creations in a counter. At the end, the function returns zero; the number of links added can be
   accessed via the counter.


	\param area1num Index of the source area for the grapple.
	\param area2num Index of the target area where a grapple might be attached.
	\return Non‑zero if a new reachability was allocated and added, zero otherwise.
*/
int AAS_Reachability_Grapple( int area1num, int area2num )
{
	int					 face2num, i, j, areanum, numareas, areas[20];
	float				 mingrappleangle, z, hordist;
	bsp_trace_t			 bsptrace;
	aas_trace_t			 trace;
	aas_face_t*			 face2;
	aas_area_t *		 area1, *area2;
	aas_lreachability_t* lreach;
	vec3_t				 areastart, facecenter, start, end, dir, down = { 0, 0, -1 };
	vec_t*				 v;

	// only grapple when on the ground or swimming
	if( !AAS_AreaGrounded( area1num ) && !AAS_AreaSwim( area1num ) ) {
		return qfalse;
	}

	// don't grapple from a crouch area
	if( !( AAS_AreaPresenceType( area1num ) & PRESENCE_NORMAL ) ) {
		return qfalse;
	}

	// NOTE: disabled area swim it doesn't work right
	if( AAS_AreaSwim( area1num ) ) {
		return qfalse;
	}

	//
	area1 = &( *aasworld ).areas[area1num];
	area2 = &( *aasworld ).areas[area2num];

	// don't grapple towards way lower areas
	if( area2->maxs[2] < area1->mins[2] ) {
		return qfalse;
	}

	//
	VectorCopy( ( *aasworld ).areas[area1num].center, start );

	// if not a swim area
	if( !AAS_AreaSwim( area1num ) ) {
		if( !AAS_PointAreaNum( start ) ) {
			Log_Write( "area %d center %f %f %f in solid?\r\n", area1num, start[0], start[1], start[2] );
		}

		VectorCopy( start, end );
		end[2] -= 1000;
		trace = AAS_TraceClientBBox( start, end, PRESENCE_CROUCH, -1 );

		if( trace.startsolid ) {
			return qfalse;
		}

		VectorCopy( trace.endpos, areastart );

	} else {
		if( !( AAS_PointContents( start ) & ( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER ) ) ) {
			return qfalse;
		}
	}

	//
	// start is now the start point
	//
	for( i = 0; i < area2->numfaces; i++ ) {
		face2num = ( *aasworld ).faceindex[area2->firstface + i];
		face2	 = &( *aasworld ).faces[abs( face2num )];

		// if it is not a solid face
		if( !( face2->faceflags & FACE_SOLID ) ) {
			continue;
		}

		// direction towards the first vertex of the face
		v = ( *aasworld ).vertexes[( *aasworld ).edges[abs( ( *aasworld ).edgeindex[face2->firstedge] )].v[0]];
		VectorSubtract( v, areastart, dir );

		// if the face plane is facing away
		if( DotProduct( ( *aasworld ).planes[face2->planenum].normal, dir ) > 0 ) {
			continue;
		}

		// get the center of the face
		AAS_FaceCenter( face2num, facecenter );

		// only go higher up with the grapple
		if( facecenter[2] < areastart[2] + 64 ) {
			continue;
		}

		// only use vertical faces or downward facing faces
		if( DotProduct( ( *aasworld ).planes[face2->planenum].normal, down ) < 0 ) {
			continue;
		}

		// direction towards the face center
		VectorSubtract( facecenter, areastart, dir );
		//
		z		= dir[2];
		dir[2]	= 0;
		hordist = VectorLength( dir );

		if( !hordist ) {
			continue;
		}

		// if too far
		if( hordist > 2000 ) {
			continue;
		}

		// check the minimal angle of the movement
		mingrappleangle = 15; // 15 degrees

		if( z / hordist < tan( 2 * M_PI * mingrappleangle / 360 ) ) {
			continue;
		}

		//
		VectorCopy( facecenter, start );
		VectorMA( facecenter, -500, ( *aasworld ).planes[face2->planenum].normal, end );
		//
		bsptrace = AAS_Trace( start, NULL, NULL, end, 0, CONTENTS_SOLID );

		// the grapple won't stick to the sky and the grapple point should be near the AAS wall
		if( ( bsptrace.surface.flags & SURF_SKY ) || ( bsptrace.fraction * 500 > 32 ) ) {
			continue;
		}

		// trace a full bounding box from the area center on the ground to
		// the center of the face
		VectorSubtract( facecenter, areastart, dir );
		VectorNormalize( dir );
		VectorMA( areastart, 4, dir, start );
		VectorCopy( bsptrace.endpos, end );
		trace = AAS_TraceClientBBox( start, end, PRESENCE_NORMAL, -1 );
		VectorSubtract( trace.endpos, facecenter, dir );

		if( VectorLength( dir ) > 24 ) {
			continue;
		}

		//
		VectorCopy( trace.endpos, start );
		VectorCopy( trace.endpos, end );
		end[2] -= AAS_FallDamageDistance();
		trace = AAS_TraceClientBBox( start, end, PRESENCE_NORMAL, -1 );

		if( trace.fraction >= 1 ) {
			continue;
		}

		// area to end in
		areanum = AAS_PointAreaNum( trace.endpos );

		// if not in lava or slime
		if( ( *aasworld ).areasettings[areanum].contents & AREACONTENTS_LAVA ) { //----(SA)	modified since slime is no longer deadly
			//		if ((*aasworld).areasettings[areanum].contents & (AREACONTENTS_SLIME|AREACONTENTS_LAVA))
			continue;
		}

		// do not go the the source area
		if( areanum == area1num ) {
			continue;
		}

		// don't create reachabilities if they already exist
		if( AAS_ReachabilityExists( area1num, areanum ) ) {
			continue;
		}

		// only end in areas we can stand
		if( !AAS_AreaGrounded( areanum ) ) {
			continue;
		}

		// never go through cluster portals!!
		numareas = AAS_TraceAreas( areastart, bsptrace.endpos, areas, NULL, 20 );

		if( numareas >= 20 ) {
			continue;
		}

		for( j = 0; j < numareas; j++ ) {
			if( ( *aasworld ).areasettings[areas[j]].contents & AREACONTENTS_CLUSTERPORTAL ) {
				break;
			}
		}

		if( j < numareas ) {
			continue;
		}

		// create a new reachability link
		lreach = AAS_AllocReachability();

		if( !lreach ) {
			return qfalse;
		}

		lreach->areanum = areanum;
		lreach->facenum = face2num;
		lreach->edgenum = 0;
		VectorCopy( areastart, lreach->start );
		// VectorCopy(facecenter, lreach->end);
		VectorCopy( bsptrace.endpos, lreach->end );
		lreach->traveltype = TRAVEL_GRAPPLEHOOK;
		VectorSubtract( lreach->end, lreach->start, dir );
		lreach->traveltime		   = STARTGRAPPLE_TIME + VectorLength( dir ) * 0.25;
		lreach->next			   = areareachability[area1num];
		areareachability[area1num] = lreach;
		//
		reach_grapple++;
	}

	//
	return qfalse;
}

/*!
	\brief Marks map areas that can be reached by a weapon jump based on special item items and jump pads

	It iterates over all map entities, checks whether they are special weapon, armor or power‑up items that can be used to launch a weapon jump, and for each such entity it finds a reachable area
   around the item’s origin. The AREA_WEAPONJUMP flag is then set for that area so that the bot AI recognises it as a weapon‑jump destination. Items that are not stationary are checked for solid
   placement and a message is printed if they lie inside solid geometry. The function also scans all area settings for jump‑pad contents and applies the same flag. Finally the total count of
   weapon‑jump areas is printed.

*/
void AAS_SetWeaponJumpAreaFlags()
{
	int	   ent, i;
	vec3_t mins = { -15, -15, -15 }, maxs = { 15, 15, 15 };
	vec3_t origin;
	int	   areanum, weaponjumpareas, spawnflags;
	char   classname[MAX_EPAIRKEY];

	weaponjumpareas = 0;

	for( ent = AAS_NextBSPEntity( 0 ); ent; ent = AAS_NextBSPEntity( ent ) ) {
		if( !AAS_ValueForBSPEpairKey( ent, "classname", classname, MAX_EPAIRKEY ) ) {
			continue;
		}

		if( !strcmp( classname, "item_armor_body" ) || !strcmp( classname, "item_armor_combat" ) || !strcmp( classname, "item_health_mega" ) || !strcmp( classname, "weapon_grenadelauncher" ) ||
			!strcmp( classname, "weapon_rocketlauncher" ) || !strcmp( classname, "weapon_lightning" ) || !strcmp( classname, "weapon_sp5" ) || !strcmp( classname, "weapon_railgun" ) ||
			!strcmp( classname, "weapon_bfg" ) || !strcmp( classname, "item_quad" ) || !strcmp( classname, "item_regen" ) || !strcmp( classname, "item_invulnerability" ) ) {
			if( AAS_VectorForBSPEpairKey( ent, "origin", origin ) ) {
				spawnflags = 0;
				AAS_IntForBSPEpairKey( ent, "spawnflags", &spawnflags );

				// if not a stationary item
				if( !( spawnflags & 1 ) ) {
					if( !AAS_DropToFloor( origin, mins, maxs ) ) {
						botimport.Print( PRT_MESSAGE, "%s in solid at (%1.1f %1.1f %1.1f)\n", classname, origin[0], origin[1], origin[2] );
					}
				}

				// areanum = AAS_PointAreaNum(origin);
				areanum = AAS_BestReachableArea( origin, mins, maxs, origin );
				// the bot may rocket jump towards this area
				( *aasworld ).areasettings[areanum].areaflags |= AREA_WEAPONJUMP;

				//
				if( !AAS_AreaGrounded( areanum ) ) {
					botimport.Print( PRT_MESSAGE, "area not grounded\n" );
				}

				//
				weaponjumpareas++;
			}
		}
	}

	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		if( ( *aasworld ).areasettings[i].contents & AREACONTENTS_JUMPPAD ) {
			( *aasworld ).areasettings[i].areaflags |= AREA_WEAPONJUMP;
			weaponjumpareas++;
		}
	}

	botimport.Print( PRT_MESSAGE, "%d weapon jump areas\n", weaponjumpareas );
}

/*!
	\brief Determines whether a weapon jump link exists between two areas and creates a reachability if one is possible

	The function first verifies that the source area is grounded and suitable for a jump (not a swim area) and that the destination area also has a ground surface. It requires the destination to be
   flagged for a weapon jump and higher than the source. From the source area's center it calculates a potential start point for the jump by tracing downward to the ground. It then iterates over all
   ground faces of the destination area that are above the start point, trying both a rocket‑jump and a BFG‑jump. For each face it computes the horizontal velocity needed to reach the face from the
   start position and, if the required speed is below a threshold, uses the prediction subsystem to ensure the motion would reach the face without falling into water, lava, slime or taking excessive
   damage. When a valid jump is found, it allocates a reachability structure, sets its fields to represent a rocket or BFG jump, links it into the source area's reachability list, and returns success.

	\param area1num index of the starting area to evaluate for a weapon jump
	\param area2num index of the destination area that might be reachable via a jump
	\return returns 1 if a jump reachability was created, 0 otherwise
*/
int AAS_Reachability_WeaponJump( int area1num, int area2num )
{
	int					 face2num, i, n, ret;
	float				 speed, zvel, hordist;
	aas_face_t*			 face2;
	aas_area_t *		 area1, *area2;
	aas_lreachability_t* lreach;
	vec3_t				 areastart, facecenter, start, end, dir, cmdmove; // teststart;
	vec3_t				 velocity;
	aas_clientmove_t	 move;
	aas_trace_t			 trace;

	if( !AAS_AreaGrounded( area1num ) || AAS_AreaSwim( area1num ) ) {
		return qfalse;
	}

	if( !AAS_AreaGrounded( area2num ) ) {
		return qfalse;
	}

	// NOTE: only weapon jump towards areas with an interesting item in it??
	if( !( ( *aasworld ).areasettings[area2num].areaflags & AREA_WEAPONJUMP ) ) {
		return qfalse;
	}

	//
	area1 = &( *aasworld ).areas[area1num];
	area2 = &( *aasworld ).areas[area2num];

	// don't weapon jump towards way lower areas
	if( area2->maxs[2] < area1->mins[2] ) {
		return qfalse;
	}

	//
	VectorCopy( ( *aasworld ).areas[area1num].center, start );

	// if not a swim area
	if( !AAS_PointAreaNum( start ) ) {
		Log_Write( "area %d center %f %f %f in solid?\r\n", area1num, start[0], start[1], start[2] );
	}

	VectorCopy( start, end );
	end[2] -= 1000;
	trace = AAS_TraceClientBBox( start, end, PRESENCE_CROUCH, -1 );

	if( trace.startsolid ) {
		return qfalse;
	}

	VectorCopy( trace.endpos, areastart );

	//
	// areastart is now the start point
	//
	for( i = 0; i < area2->numfaces; i++ ) {
		face2num = ( *aasworld ).faceindex[area2->firstface + i];
		face2	 = &( *aasworld ).faces[abs( face2num )];

		// if it is not a solid face
		if( !( face2->faceflags & FACE_GROUND ) ) {
			continue;
		}

		// get the center of the face
		AAS_FaceCenter( face2num, facecenter );

		// only go higher up with weapon jumps
		if( facecenter[2] < areastart[2] + 64 ) {
			continue;
		}

		// NOTE: set to 2 to allow bfg jump reachabilities
		for( n = 0; n < 1 /*2*/; n++ ) {
			// get the rocket jump z velocity
			if( n ) {
				zvel = AAS_BFGJumpZVelocity( areastart );

			} else {
				zvel = AAS_RocketJumpZVelocity( areastart );
			}

			// get the horizontal speed for the jump, if it isn't possible to calculate this
			// speed (the jump is not possible) then there's no jump reachability created
			ret = AAS_HorizontalVelocityForJump( zvel, areastart, facecenter, &speed );

			if( ret && speed < 270 ) {
				// direction towards the face center
				VectorSubtract( facecenter, areastart, dir );
				dir[2]	= 0;
				hordist = VectorNormalize( dir );
				// if (hordist < 1.6 * (facecenter[2] - areastart[2]))
				{
					// get command movement
					VectorScale( dir, speed, cmdmove );
					VectorSet( velocity, 0, 0, zvel );
					/*
					//get command movement
					VectorScale(dir, speed, velocity);
					velocity[2] = zvel;
					VectorSet(cmdmove, 0, 0, 0);
					*/
					//
					AAS_PredictClientMovement( &move,
						-1,
						areastart,
						PRESENCE_NORMAL,
						qtrue,
						velocity,
						cmdmove,
						30,
						30,
						0.1,
						SE_ENTERWATER | SE_ENTERSLIME | SE_ENTERLAVA | SE_HITGROUNDDAMAGE | SE_TOUCHJUMPPAD | SE_HITGROUNDAREA,
						area2num,
						qfalse );

					// if prediction time wasn't enough to fully predict the movement
					// don't enter slime or lava and don't fall from too high
					if( move.frames < 30 && !( move.stopevent & ( SE_ENTERSLIME | SE_ENTERLAVA | SE_HITGROUNDDAMAGE ) ) && ( move.stopevent & ( SE_HITGROUNDAREA | SE_TOUCHJUMPPAD ) ) ) {
						// create a rocket or bfg jump reachability from area1 to area2
						lreach = AAS_AllocReachability();

						if( !lreach ) {
							return qfalse;
						}

						lreach->areanum = area2num;
						lreach->facenum = 0;
						lreach->edgenum = 0;
						VectorCopy( areastart, lreach->start );
						VectorCopy( facecenter, lreach->end );

						if( n ) {
							lreach->traveltype = TRAVEL_BFGJUMP;

						} else {
							lreach->traveltype = TRAVEL_ROCKETJUMP;
						}

						lreach->traveltime		   = 300;
						lreach->next			   = areareachability[area1num];
						areareachability[area1num] = lreach;
						//
						reach_rocketjump++;
						return qtrue;
					}
				}
			}
		}
	}

	//
	return qfalse;
}

/*!
	\brief Adds walk‑off‑ledge reachabilities for a given area.

	The function inspects a grounded, non‑swim area and scans each of its ground faces and the adjoining non‑ground faces sharing an edge. It determines if a third face creates a gap large enough for
   a jump off ledge. For each suitable edge it builds a candidate trajectory by offsetting along the face normal, traces a client hit box from that point downward, and checks the resulting landing
   location. If the landing location is in a grounded area that does not contain lava or slime, a walk‑off‑ledge reachability is allocated, configured with travel type TRAVEL_WALKOFFLEDGE, travel time
   based on the height difference and gravity, and inserted into the global area reachability list. The process skips areas that already have a suitable reachability, or where the landing area is
   non‑grounded and non‑swim and does not qualify for a walk off ledge.

	\param areanum the numeric identifier of the area being processed
*/
void AAS_Reachability_WalkOffLedge( int areanum )
{
	int					 i, j, k, l, m, n;
	int					 face1num, face2num, face3num, edge1num, edge2num, edge3num;
	int					 otherareanum, gap, reachareanum, side;
	aas_area_t *		 area, *area2;
	aas_face_t *		 face1, *face2, *face3;
	aas_edge_t*			 edge;
	aas_plane_t*		 plane;
	vec_t *				 v1, *v2;
	vec3_t				 sharededgevec, mid, dir, testend;
	aas_lreachability_t* lreach;
	aas_trace_t			 trace;

	if( !AAS_AreaGrounded( areanum ) || AAS_AreaSwim( areanum ) ) {
		return;
	}

	//
	area = &( *aasworld ).areas[areanum];

	//
	for( i = 0; i < area->numfaces; i++ ) {
		face1num = ( *aasworld ).faceindex[area->firstface + i];
		face1	 = &( *aasworld ).faces[abs( face1num )];

		// face 1 must be a ground face
		if( !( face1->faceflags & FACE_GROUND ) ) {
			continue;
		}

		// go through all the edges of this ground face
		for( k = 0; k < face1->numedges; k++ ) {
			edge1num = ( *aasworld ).edgeindex[face1->firstedge + k];

			// find another not ground face using this same edge
			for( j = 0; j < area->numfaces; j++ ) {
				face2num = ( *aasworld ).faceindex[area->firstface + j];
				face2	 = &( *aasworld ).faces[abs( face2num )];

				// face 2 may not be a ground face
				if( face2->faceflags & FACE_GROUND ) {
					continue;
				}

				// compare all the edges
				for( l = 0; l < face2->numedges; l++ ) {
					edge2num = ( *aasworld ).edgeindex[face2->firstedge + l];

					if( abs( edge1num ) == abs( edge2num ) ) {
						// get the area at the other side of the face
						if( face2->frontarea == areanum ) {
							otherareanum = face2->backarea;

						} else {
							otherareanum = face2->frontarea;
						}

						//
						area2 = &( *aasworld ).areas[otherareanum];

						// if the other area is grounded!
						if( ( *aasworld ).areasettings[otherareanum].areaflags & AREA_GROUNDED ) {
							// check for a possible gap
							gap = qfalse;

							for( n = 0; n < area2->numfaces; n++ ) {
								face3num = ( *aasworld ).faceindex[area2->firstface + n];

								// may not be the shared face of the two areas
								if( abs( face3num ) == abs( face2num ) ) {
									continue;
								}

								//
								face3 = &( *aasworld ).faces[abs( face3num )];

								// find an edge shared by all three faces
								for( m = 0; m < face3->numedges; m++ ) {
									edge3num = ( *aasworld ).edgeindex[face3->firstedge + m];

									// but the edge should be shared by all three faces
									if( abs( edge3num ) == abs( edge1num ) ) {
										if( !( face3->faceflags & FACE_SOLID ) ) {
											gap = qtrue;
											break;
										}

										//
										if( face3->faceflags & FACE_GROUND ) {
											gap = qfalse;
											break;
										}

										// FIXME: there are more situations to be handled
										gap = qtrue;
										break;
									}
								}

								if( m < face3->numedges ) {
									break;
								}
							}

							if( !gap ) {
								break;
							}
						}

						// check for a walk off ledge reachability
						edge = &( *aasworld ).edges[abs( edge1num )];
						side = edge1num < 0;
						//
						v1 = ( *aasworld ).vertexes[edge->v[side]];
						v2 = ( *aasworld ).vertexes[edge->v[!side]];
						//
						plane = &( *aasworld ).planes[face1->planenum];
						// get the points really into the areas
						VectorSubtract( v2, v1, sharededgevec );
						CrossProduct( plane->normal, sharededgevec, dir );
						VectorNormalize( dir );
						//
						VectorAdd( v1, v2, mid );
						VectorScale( mid, 0.5, mid );
						VectorMA( mid, 8, dir, mid );
						//
						VectorCopy( mid, testend );
						testend[2] -= 1000;
						trace = AAS_TraceClientBBox( mid, testend, PRESENCE_CROUCH, -1 );

						//
						if( trace.startsolid ) {
							// Log_Write("area %d: trace.startsolid\r\n", areanum);
							break;
						}

						reachareanum = AAS_PointAreaNum( trace.endpos );

						if( reachareanum == areanum ) {
							// Log_Write("area %d: same area\r\n", areanum);
							break;
						}

						if( AAS_ReachabilityExists( areanum, reachareanum ) ) {
							// Log_Write("area %d: reachability already exists\r\n", areanum);
							break;
						}

						if( !AAS_AreaGrounded( reachareanum ) && !AAS_AreaSwim( reachareanum ) ) {
							// Log_Write("area %d, reach area %d: not grounded and not swim\r\n", areanum, reachareanum);
							break;
						}

						//
						if( ( *aasworld ).areasettings[reachareanum].contents & AREACONTENTS_LAVA ) { //----(SA)	modified since slime is no longer deadly
							//						if ((*aasworld).areasettings[reachareanum].contents & (AREACONTENTS_SLIME | AREACONTENTS_LAVA))
							// Log_Write("area %d, reach area %d: lava or slime\r\n", areanum, reachareanum);
							break;
						}

						lreach = AAS_AllocReachability();

						if( !lreach ) {
							break;
						}

						lreach->areanum = reachareanum;
						lreach->facenum = 0;
						lreach->edgenum = edge1num;
						VectorCopy( mid, lreach->start );
						VectorCopy( trace.endpos, lreach->end );
						lreach->traveltype = TRAVEL_WALKOFFLEDGE;
						lreach->traveltime = STARTWALKOFFLEDGE_TIME + fabs( mid[2] - trace.endpos[2] ) * 50 / aassettings.sv_gravity;

						if( !AAS_AreaSwim( reachareanum ) && !AAS_AreaJumpPad( reachareanum ) ) {
							if( AAS_FallDelta( mid[2] - trace.endpos[2] ) > FALLDELTA_5DAMAGE ) {
								lreach->traveltime += FALLDAMAGE_5_TIME;

							} else if( AAS_FallDelta( mid[2] - trace.endpos[2] ) > FALLDELTA_10DAMAGE ) {
								lreach->traveltime += FALLDAMAGE_10_TIME;
							}
						}

						lreach->next			  = areareachability[areanum];
						areareachability[areanum] = lreach;
						// we've got another walk off ledge reachability
						reach_walkoffledge++;
					}
				}
			}
		}
	}
}

/*!
	\brief Rebuilds and stores a global reachability table for all areas, allocating new memory, copying existing reachabilities and enforcing a minimum travel time.

	The function first frees any previously allocated reachability array in the global AAS world. It then allocates a new clear memory block large enough to hold all reachabilities plus a small
   margin. For each area, it records the index of the first reachable area in the new array and iterates through that area's linked-list of local reachabilities. Each entry is copied, the source and
   destination vectors are duplicated, and the travel type and time are transferred. If a travel time is less than the defined minimum, it is increased to that minimum. The counts of reachable areas
   for each area are updated and the global reachability size is adjusted accordingly.

*/
void AAS_StoreReachability()
{
	int					 i;
	aas_areasettings_t*	 areasettings;
	aas_lreachability_t* lreach;
	aas_reachability_t*	 reach;

	if( ( *aasworld ).reachability ) {
		FreeMemory( ( *aasworld ).reachability );
	}

	( *aasworld ).reachability	   = ( aas_reachability_t* )GetClearedMemory( ( numlreachabilities + 10 ) * sizeof( aas_reachability_t ) );
	( *aasworld ).reachabilitysize = 1;

	for( i = 0; i < ( *aasworld ).numareas; i++ ) {
		areasettings					 = &( *aasworld ).areasettings[i];
		areasettings->firstreachablearea = ( *aasworld ).reachabilitysize;
		areasettings->numreachableareas	 = 0;

		for( lreach = areareachability[i]; lreach; lreach = lreach->next ) {
			reach		   = &( *aasworld ).reachability[areasettings->firstreachablearea + areasettings->numreachableareas];
			reach->areanum = lreach->areanum;
			reach->facenum = lreach->facenum;
			reach->edgenum = lreach->edgenum;
			VectorCopy( lreach->start, reach->start );
			VectorCopy( lreach->end, reach->end );
			reach->traveltype = lreach->traveltype;
			reach->traveltime = lreach->traveltime;

			// RF, enforce the min reach time
			if( reach->traveltime < REACH_MIN_TIME ) {
				reach->traveltime = REACH_MIN_TIME;
			}

			//
			areasettings->numreachableareas++;
		}

		( *aasworld ).reachabilitysize += areasettings->numreachableareas;
	}
}

//===========================================================================
//
// TRAVEL_WALK					100%	equal floor height + steps
// TRAVEL_CROUCH				100%
// TRAVEL_BARRIERJUMP			100%
// TRAVEL_JUMP					 80%
// TRAVEL_LADDER				100%	+ fall down from ladder + jump up to ladder
// TRAVEL_WALKOFFLEDGE			 90%	walk off very steep walls?
// TRAVEL_SWIM					100%
// TRAVEL_WATERJUMP				100%
// TRAVEL_TELEPORT				100%
// TRAVEL_ELEVATOR				100%
// TRAVEL_GRAPPLEHOOK			100%
// TRAVEL_DOUBLEJUMP			  0%
// TRAVEL_RAMPJUMP				  0%
// TRAVEL_STRAFEJUMP			  0%
// TRAVEL_ROCKETJUMP			100%	(currently limited towards areas with items)
// TRAVEL_BFGJUMP				  0%	(currently disabled)
// TRAVEL_JUMPPAD				100%
// TRAVEL_FUNCBOB				100%
//
// Parameter:			-
// Returns:				true if NOT finished
// Changes Globals:		-
//===========================================================================

/*!
	\brief Calculates a portion of the AAS reachability data for the map and reports if further processing is required.

	This routine is called repeatedly during map initialization to build the reachability graph between map areas. It first verifies that a world file has been loaded and that reachability has not
   already been completed, then processes a batch of areas up to the specified time budget. Reachability edges are generated by a series of checks for swim, walk, step, ladder, jump, and other
   movement types, with special handling for teleporter, jumppad, and terrain content. Progress is printed to the console, and once all reachabilities for all areas have been computed, the data is
   stored, temporary memory freed, and clustering initialization begins. The function returns true while additional reachability work remains, and false when the graph is fully populated or when no
   work is needed.

	\param time Seconds allocated for the incremental reachability calculation in this call.
	\return Non‑zero value if more reachability work remains, zero otherwise.
*/
int AAS_ContinueInitReachability( float time )
{
	int			 i, j, todo, start_time;
	static float framereachability, reachability_delay;
	static int	 lastpercentage;

	if( !( *aasworld ).loaded ) {
		return qfalse;
	}

	// if reachability is calculated for all areas
	if( ( *aasworld ).reachabilityareas >= ( *aasworld ).numareas + 2 ) {
		return qfalse;
	}

	// if starting with area 1 (area 0 is a dummy)
	if( ( *aasworld ).reachabilityareas == 1 ) {
		botimport.Print( PRT_MESSAGE, "calculating reachability...\n" );
		lastpercentage	   = 0;
		framereachability  = 2000;
		reachability_delay = 1000;
	}

	// number of areas to calculate reachability for this cycle
	todo	   = ( *aasworld ).reachabilityareas + ( int )framereachability;
	start_time = Sys_MilliSeconds();

	// loop over the areas
	for( i = ( *aasworld ).reachabilityareas; i < ( *aasworld ).numareas && i < todo; i++ ) {
		( *aasworld ).reachabilityareas++;

		// only create jumppad reachabilities from jumppad areas
		if( ( *aasworld ).areasettings[i].contents & AREACONTENTS_JUMPPAD ) {
			continue;
		}

		// loop over the areas
		for( j = 1; j < ( *aasworld ).numareas; j++ ) {
			if( i == j ) {
				continue;
			}

			// never create reachabilities from teleporter or jumppad areas to regular areas
			if( ( *aasworld ).areasettings[i].contents & ( AREACONTENTS_TELEPORTER | AREACONTENTS_JUMPPAD ) ) {
				if( !( ( *aasworld ).areasettings[j].contents & ( AREACONTENTS_TELEPORTER | AREACONTENTS_JUMPPAD ) ) ) {
					continue;
				}
			}

			// if there already is a reachability link from area i to j
			if( AAS_ReachabilityExists( i, j ) ) {
				continue;
			}

			// check for a swim reachability
			if( AAS_Reachability_Swim( i, j ) ) {
				continue;
			}

			// check for a simple walk on equal floor height reachability
			if( AAS_Reachability_EqualFloorHeight( i, j ) ) {
				continue;
			}

			// check for step, barrier, waterjump and walk off ledge reachabilities
			if( AAS_Reachability_Step_Barrier_WaterJump_WalkOffLedge( i, j ) ) {
				continue;
			}

			// check for ladder reachabilities
			if( AAS_Reachability_Ladder( i, j ) ) {
				continue;
			}

			// check for a jump reachability
			if( AAS_Reachability_Jump( i, j ) ) {
				continue;
			}
		}

		// never create these reachabilities from teleporter or jumppad areas
		if( ( *aasworld ).areasettings[i].contents & ( AREACONTENTS_TELEPORTER | AREACONTENTS_JUMPPAD ) ) {
			continue;
		}

		// loop over the areas
		for( j = 1; j < ( *aasworld ).numareas; j++ ) {
			if( i == j ) {
				continue;
			}

			//
			if( AAS_ReachabilityExists( i, j ) ) {
				continue;
			}

			// check for a grapple hook reachability
			//  Ridah, no grapple
			//			AAS_Reachability_Grapple(i, j);
			// check for a weapon jump reachability
			//  Ridah, no weapon jumping
			//			AAS_Reachability_WeaponJump(i, j);
		}

		// if the calculation took more time than the max reachability delay
		if( Sys_MilliSeconds() - start_time > ( int )reachability_delay ) {
			break;
		}

		//
		if( ( *aasworld ).reachabilityareas * 1000 / ( *aasworld ).numareas > lastpercentage ) {
			break;
		}
	}

	//
	if( ( *aasworld ).reachabilityareas == ( *aasworld ).numareas ) {
		botimport.Print( PRT_MESSAGE, "\r%6.1f%%", ( float )100.0 );
		botimport.Print( PRT_MESSAGE, "\nplease wait while storing reachability...\n" );
		( *aasworld ).reachabilityareas++;
	}

	// if this is the last step in the reachability calculations
	else if( ( *aasworld ).reachabilityareas == ( *aasworld ).numareas + 1 ) {
		// create additional walk off ledge reachabilities for every area
		for( i = 1; i < ( *aasworld ).numareas; i++ ) {
			// only create jumppad reachabilities from jumppad areas
			if( ( *aasworld ).areasettings[i].contents & AREACONTENTS_JUMPPAD ) {
				continue;
			}

			AAS_Reachability_WalkOffLedge( i );
		}

		// create jump pad reachabilities
		AAS_Reachability_JumpPad();
		// create teleporter reachabilities
		AAS_Reachability_Teleport();
		// create elevator (func_plat) reachabilities
		AAS_Reachability_Elevator();
		// create func_bobbing reachabilities
		AAS_Reachability_FuncBobbing();
		//
		// #ifdef DEBUG
		botimport.Print( PRT_MESSAGE, "%6d reach swim\n", reach_swim );
		botimport.Print( PRT_MESSAGE, "%6d reach equal floor\n", reach_equalfloor );
		botimport.Print( PRT_MESSAGE, "%6d reach step\n", reach_step );
		botimport.Print( PRT_MESSAGE, "%6d reach barrier\n", reach_barrier );
		botimport.Print( PRT_MESSAGE, "%6d reach waterjump\n", reach_waterjump );
		botimport.Print( PRT_MESSAGE, "%6d reach walkoffledge\n", reach_walkoffledge );
		botimport.Print( PRT_MESSAGE, "%6d reach jump\n", reach_jump );
		botimport.Print( PRT_MESSAGE, "%6d reach ladder\n", reach_ladder );
		botimport.Print( PRT_MESSAGE, "%6d reach walk\n", reach_walk );
		botimport.Print( PRT_MESSAGE, "%6d reach teleport\n", reach_teleport );
		botimport.Print( PRT_MESSAGE, "%6d reach funcbob\n", reach_funcbob );
		botimport.Print( PRT_MESSAGE, "%6d reach elevator\n", reach_elevator );
		botimport.Print( PRT_MESSAGE, "%6d reach grapple\n", reach_grapple );
		botimport.Print( PRT_MESSAGE, "%6d reach rocketjump\n", reach_rocketjump );
		botimport.Print( PRT_MESSAGE, "%6d reach jumppad\n", reach_jumppad );
		// #endif
		//*/
		// store all the reachabilities
		AAS_StoreReachability();
		// free the reachability link heap
		AAS_ShutDownReachabilityHeap();
		//
		FreeMemory( areareachability );
		//
		( *aasworld ).reachabilityareas++;
		//
		botimport.Print( PRT_MESSAGE, "calculating clusters...\n" );

	} else {
		lastpercentage = ( *aasworld ).reachabilityareas * 1000 / ( *aasworld ).numareas;
		botimport.Print( PRT_MESSAGE, "\r%6.1f%%", ( float )lastpercentage / 10 );
	}

	// not yet finished
	return qtrue;
}

/*!
	\brief Initializes reachability data structures for the current AAS world.

	If the world is not loaded, the function exits immediately. For worlds that already have a reachability size set, the function optionally respects the “forcereachability” library variable: when
   not forced, it simply sets the number of reachable areas to the current area count plus two and returns. Otherwise it initializes the savefile flag, starts with area one, establishes a heap for
   reachability links, allocates memory for the per‑area reachability pointers, and configures flags for areas that support weapon‑jump actions.

*/
void AAS_InitReachability()
{
	if( !( *aasworld ).loaded ) {
		return;
	}

	if( ( *aasworld ).reachabilitysize ) {
#ifndef BSPC

		if( !( ( int )LibVarGetValue( "forcereachability" ) ) ) {
			( *aasworld ).reachabilityareas = ( *aasworld ).numareas + 2;
			return;
		}

#else
		( *aasworld ).reachabilityareas = ( *aasworld ).numareas + 2;
		return;
#endif // BSPC
	}

	( *aasworld ).savefile = qtrue;
	// start with area 1 because area zero is a dummy
	( *aasworld ).reachabilityareas = 1;
	// setup the heap with reachability links
	AAS_SetupReachabilityHeap();
	// allocate area reachability link array
	areareachability = ( aas_lreachability_t** )GetClearedMemory( ( *aasworld ).numareas * sizeof( aas_lreachability_t* ) );
	//
	AAS_SetWeaponJumpAreaFlags();
}
