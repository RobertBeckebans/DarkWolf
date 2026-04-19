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
	\file engine/botlib/be_aas_routealt.cpp
	\brief Implement the optional alternative routing logic for the AAS-based bot pathfinding engine, enabling bots to discover and use alternate paths when the optimal path is unsuitable or blocked.
	\note doxygenix: sha256=22115d730e35ba3cb7e300ab23f1826cc30947d268fbbe941d53f739c854ca99

	\par File Purpose
	- Implement the optional alternative routing logic for the AAS-based bot pathfinding engine, enabling bots to discover and use alternate paths when the optimal path is unsuitable or blocked.
	- Provide data structures, initialization, shutdown, and the core algorithm that selects valid, near‑optimal alternative route goals.

	\par Core Responsibilities
	- Maintain global arrays used to store mid‑range area candidates and temporary cluster lists.
	- Provide an alternative route goal selection algorithm that expands from a set of mid‑range area nodes, clusters them, and chooses representative goal positions.
	- Offer initialization and shutdown helpers that allocate and free the data structures, ensuring proper resource management in the AAS subsystem.

	\par Key Types and Functions
	- midrangearea_t — structure holding validity flag and travel times to start/goal for mid‑range area nodes.
	- midrangeareas — pointer to dynamic array of midrangearea_t objects sized by the world’s area count.
	- clusterareas — pointer to dynamic array of integers used to collect connected mid‑range area numbers during flood fill.
	- numclusterareas — integer counter of current cluster size during flooding.
	- AAS_AltRoutingFloodCluster_r(int areanum) — recursively marks an area as part of the current cluster and propagates through all reachable neighboring mid‑range areas.
	- AAS_AlternativeRouteGoals(vec3_t start, vec3_t goal, int travelflags, aas_altroutegoal_t* altroutegoals, int maxaltroutegoals, int color) — main routine that evaluates all potential mid‑range areas, clusters them, and fills the output array with alternative route goals.
	- AAS_InitAlternativeRouting() — allocates or reallocates the global midrangeareas and clusterareas arrays when alternative routing is enabled.
	- AAS_ShutdownAlternativeRouting() — frees the allocated arrays, clears global pointers, and resets cluster count when alternative routing is disabled.

	\par Control Flow
	- When enabled by the ENABLE_ALTROUTING macro, AAS_AlternativeRouteGoals is the entry point. It first validates the start and goal positions, retrieves their area numbers, and computes the fastest travel time from start to goal.
	- It then iterates over all areas, selecting those that contain the AREACONTENTS_ROUTEPORTAL flag, have reachable paths, and whose travel time to/from the start and goal areas is within 1.5× the fastest travel time; such areas are marked as mid‑range candidates in the midrangeareas array.
	- For each valid mid‑range area, the recursive function AAS_AltRoutingFloodCluster_r flood‑fills through reachable faces, building a cluster of connected mid‑range areas in the clusterareas array and marking processed areas as invalid.
	- The cluster’s centroid is computed, then the area within the cluster closest to that centroid is chosen as an alternative route goal. Its position, travel times, and extra time are written into the caller‑supplied altroutegoals buffer.
	- The process stops when either all clusters are processed or the caller’s maximum goal count is reached, after which a diagnostic message reports the number of goals produced.

	\par Dependencies
	- "q_shared.h", "l_utils.h", "l_memory.h", "l_log.h", "l_script.h", "l_precomp.h", "l_struct.h" (standard core headers for utilities and memory management)
	- "aasfile.h", "botshared/botlib.h", "botshared/be_aas.h", "be_aas_funcs.h", "be_interface.h", "be_aas_def.h" (AAS engine and bot library interfaces)
	- Global symbols AAS_PointAreaNum, AAS_AreaTravelTimeToGoalArea, AAS_AreaReachability, and several vector operations (VectorClear, VectorAdd, VectorScale, VectorSubtract, VectorLength, VectorCopy).
	- Bot import functions for diagnostics (botimport.Print) and drawing primitives (AAS_DrawPermanentCross).

	\par How It Fits
	- This component sits within the bot AAS module, extending core travel‑time calculations to support strategic route diversification.
	- It is invoked by caller code that requires a set of fallback goal positions, typically when a primary path is compromised or when multiple concurrent bot paths need spreading.
	- By grouping mid‑range nodes into clusters, it reduces per‑bot decision overhead and encourages spatially distinct alternative goals.
*/

/*****************************************************************************
 * name:		be_aas_routealt.c
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_utils.h"
#include "l_memory.h"
#include "l_log.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "botshared/botlib.h"
#include "botshared/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"

// #define ENABLE_ALTROUTING

typedef struct midrangearea_s {
	int			   valid;
	unsigned short starttime;
	unsigned short goaltime;
} midrangearea_t;

midrangearea_t* midrangeareas;
int*			clusterareas;
int				numclusterareas;

/*!
	\brief Recursively adds all connected mid‑range areas to the current cluster.

	Starting at the specified area number, this function records the area as part of the current flood cluster and disables it as a mid‑range candidate. It then examines every face of the area,
   determining the neighbor on the other side of each face. If that neighboring area is still valid a mid‑range area, the function recursively processes that neighbor, propagating the flood throughout
   the connected component. Areas with no neighbor or already removed from the mid‑range list are ignored. The recursion terminates when no further valid neighboring areas remain.

	\param areanum the area number from which to begin the flood
*/
void			AAS_AltRoutingFloodCluster_r( int areanum )
{
	int			i, otherareanum;
	aas_area_t* area;
	aas_face_t* face;

	// add the current area to the areas of the current cluster
	clusterareas[numclusterareas] = areanum;
	numclusterareas++;
	// remove the area from the mid range areas
	midrangeareas[areanum].valid = qfalse;
	// flood to other areas through the faces of this area
	area = &( *aasworld ).areas[areanum];

	for( i = 0; i < area->numfaces; i++ ) {
		face = &( *aasworld ).faces[abs( ( *aasworld ).faceindex[area->firstface + i] )];

		// get the area at the other side of the face
		if( face->frontarea == areanum ) {
			otherareanum = face->backarea;

		} else {
			otherareanum = face->frontarea;
		}

		// if there is an area at the other side of this face
		if( !otherareanum ) {
			continue;
		}

		// if the other area is not a midrange area
		if( !midrangeareas[otherareanum].valid ) {
			continue;
		}

		//
		AAS_AltRoutingFloodCluster_r( otherareanum );
	}
}

int AAS_AlternativeRouteGoals( vec3_t start, vec3_t goal, int travelflags, aas_altroutegoal_t* altroutegoals, int maxaltroutegoals, int color )
{
#ifndef ENABLE_ALTROUTING
	return 0;
#else
	int	   i, j, startareanum, goalareanum, bestareanum;
	int	   numaltroutegoals, nummidrangeareas;
	int	   starttime, goaltime, goaltraveltime;
	float  dist, bestdist;
	vec3_t mid, dir;
	#ifdef DEBUG
	int startmillisecs;

	startmillisecs = Sys_MilliSeconds();
	#endif

	startareanum = AAS_PointAreaNum( start );

	if( !startareanum ) {
		return 0;
	}

	goalareanum = AAS_PointAreaNum( goal );

	if( !goalareanum ) {
		return 0;
	}

	// travel time towards the goal area
	goaltraveltime = AAS_AreaTravelTimeToGoalArea( startareanum, start, goalareanum, travelflags );
	// clear the midrange areas
	memset( midrangeareas, 0, ( *aasworld ).numareas * sizeof( midrangearea_t ) );
	numaltroutegoals = 0;
	//
	nummidrangeareas = 0;

	//
	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		//
		if( !( ( *aasworld ).areasettings[i].contents & AREACONTENTS_ROUTEPORTAL ) ) {
			continue;
		}

		// if the area has no reachabilities
		if( !AAS_AreaReachability( i ) ) {
			continue;
		}

		// tavel time from the area to the start area
		starttime = AAS_AreaTravelTimeToGoalArea( startareanum, start, i, travelflags );

		if( !starttime ) {
			continue;
		}

		// if the travel time from the start to the area is greater than the shortest goal travel time
		if( starttime > 1.5 * goaltraveltime ) {
			continue;
		}

		// travel time from the area to the goal area
		goaltime = AAS_AreaTravelTimeToGoalArea( i, NULL, goalareanum, travelflags );

		if( !goaltime ) {
			continue;
		}

		// if the travel time from the area to the goal is greater than the shortest goal travel time
		if( goaltime > 1.5 * goaltraveltime ) {
			continue;
		}

		// this is a mid range area
		midrangeareas[i].valid	   = qtrue;
		midrangeareas[i].starttime = starttime;
		midrangeareas[i].goaltime  = goaltime;
		Log_Write( "%d midrange area %d", nummidrangeareas, i );
		nummidrangeareas++;
	}

	//
	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		if( !midrangeareas[i].valid ) {
			continue;
		}

		// get the areas in one cluster
		numclusterareas = 0;
		AAS_AltRoutingFloodCluster_r( i );
		// now we've got a cluster with areas through which an alternative route could go
		// get the 'center' of the cluster
		VectorClear( mid );

		for( j = 0; j < numclusterareas; j++ ) {
			VectorAdd( mid, ( *aasworld ).areas[clusterareas[j]].center, mid );
		}

		VectorScale( mid, 1.0 / numclusterareas, mid );
		// get the area closest to the center of the cluster
		bestdist	= 999999;
		bestareanum = 0;

		for( j = 0; j < numclusterareas; j++ ) {
			VectorSubtract( mid, ( *aasworld ).areas[clusterareas[j]].center, dir );
			dist = VectorLength( dir );

			if( dist < bestdist ) {
				bestdist	= dist;
				bestareanum = clusterareas[j];
			}
		}

		// now we've got an area for an alternative route
		// FIXME: add alternative goal origin
		VectorCopy( ( *aasworld ).areas[bestareanum].center, altroutegoals[numaltroutegoals].origin );
		altroutegoals[numaltroutegoals].areanum			= bestareanum;
		altroutegoals[numaltroutegoals].starttraveltime = midrangeareas[bestareanum].starttime;
		altroutegoals[numaltroutegoals].goaltraveltime	= midrangeareas[bestareanum].goaltime;
		altroutegoals[numaltroutegoals].extratraveltime = ( midrangeareas[bestareanum].starttime + midrangeareas[bestareanum].goaltime ) - goaltraveltime;
		numaltroutegoals++;
		//
	#ifdef DEBUG
		botimport.Print( PRT_MESSAGE, "alternative route goal area %d, numclusterareas = %d\n", bestareanum, numclusterareas );

		if( color ) {
			AAS_DrawPermanentCross( ( *aasworld ).areas[bestareanum].center, 10, color );
		}

			// AAS_ShowArea(bestarea, qtrue);
	#endif

		// don't return more than the maximum alternative route goals
		if( numaltroutegoals >= maxaltroutegoals ) {
			break;
		}
	}

	botimport.Print( PRT_MESSAGE, "%d alternative route goals\n", numaltroutegoals );
	#ifdef DEBUG
	botimport.Print( PRT_MESSAGE, "alternative route goals in %d msec\n", Sys_MilliSeconds() - startmillisecs );
	#endif
	return numaltroutegoals;
#endif
}

/*!
	\brief Initializes the data structures used for alternative routing, allocating new memory for mid‑range and cluster area arrays and freeing any existing ones.

	This function is compiled only when alternative routing support is enabled via ENABLE_ALTROUTING. It frees previously allocated memory for the global midrangeareas and clusterareas arrays if they
   exist, then allocates fresh memory sized according to the current world’s area count. The new allocations are stored back into these global pointers, preparing them for use by the pathfinding
   subsystem.

*/
void AAS_InitAlternativeRouting()
{
#ifdef ENABLE_ALTROUTING

	if( midrangeareas ) {
		FreeMemory( midrangeareas );
	}

	midrangeareas = ( midrangearea_t* )GetMemory( ( *aasworld ).numareas * sizeof( midrangearea_t ) );

	if( clusterareas ) {
		FreeMemory( clusterareas );
	}

	clusterareas = ( int* )GetMemory( aasworld.numareas * sizeof( int ) );
#endif
}

/*!
	\brief Releases resources used for alternative routing and resets related globals when available.

	When the ENABLE_ALTROUTING flag is set, the function frees the memory allocated for the midrange and cluster routing areas if they exist, clears the pointers to those structures, and resets the
   cluster area count to zero. This cleanup ensures that all temporary routing data is properly discarded before the application shuts down or reinitializes the routing system.

*/
void AAS_ShutdownAlternativeRouting()
{
#ifdef ENABLE_ALTROUTING

	if( midrangeareas ) {
		FreeMemory( midrangeareas );
	}

	midrangeareas = NULL;

	if( clusterareas ) {
		FreeMemory( clusterareas );
	}

	clusterareas	= NULL;
	numclusterareas = 0;
#endif
}
