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
 * name:		be_aas_cluster.c
 *
 * desc:		area clustering
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_log.h"
#include "l_memory.h"
#include "l_libvar.h"
#include "aasfile.h"
#include "botshared/botlib.h"
#include "botshared/be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

extern botlib_import_t botimport;

#define AAS_MAX_PORTALS			65536
#define AAS_MAX_PORTALINDEXSIZE 65536
#define AAS_MAX_CLUSTERS		65536
//
#define MAX_PORTALAREAS			1024

// do not flood through area faces, only use reachabilities
int	 nofaceflood = qtrue;

/*!
	\brief Resets the cluster identifiers for all defined AAS areas to zero.

	The function iterates over every area in the AAS world whose index is greater than zero and assigns a cluster value of zero. This removes any cluster association that might have been previously
   assigned to those areas. Area zero is left untouched, presumably because it has a special role or is not intended to belong to a cluster. After execution, no area will be associated with a cluster.

*/
void AAS_RemoveClusterAreas()
{
	int i;

	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		( *aasworld ).areasettings[i].cluster = 0;
	}
}

/*!
	\brief Resets all area cluster assignments equal to a specified cluster number to zero.

	The function iterates over all areas in the global world except the first, checks each area's cluster setting, and if it matches the provided cluster number, the cluster is cleared by setting it
   to zero. This effectively removes the specified cluster from all areas that were using it.

	\param clusternum The cluster number whose assignments should be cleared.
*/
void AAS_ClearCluster( int clusternum )
{
	int i;

	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		if( ( *aasworld ).areasettings[i].cluster == clusternum ) {
			( *aasworld ).areasettings[i].cluster = 0;
		}
	}
}

/*!
	\brief Clears references to the specified cluster in the AAS world portal list.

	The function loops over all portals in the global AAS world data structure, starting at index 1. For each portal, it checks whether either the front or back cluster matches the supplied cluster
   number. If a match is found, the corresponding cluster field is set to 0, effectively removing the connection to that cluster. This is typically used when a cluster is being deleted or detached
   from the navigation graph.

	\param clusternum The cluster ID whose portal references are to be cleared.
*/
void AAS_RemovePortalsClusterReference( int clusternum )
{
	int portalnum;

	for( portalnum = 1; portalnum < ( *aasworld ).numportals; portalnum++ ) {
		if( ( *aasworld ).portals[portalnum].frontcluster == clusternum ) {
			( *aasworld ).portals[portalnum].frontcluster = 0;
		}

		if( ( *aasworld ).portals[portalnum].backcluster == clusternum ) {
			( *aasworld ).portals[portalnum].backcluster = 0;
		}
	}
}

/*!
	\brief Associates an area with a portal and updates the portal's cluster links, returning success status

	The function iterates through all portals to find one belonging to the supplied area. If the portal already has either front or back cluster matching the provided cluster number, the function
   reports success. If the portal has no clusters yet, it assigns the missing side; otherwise, if both sides are occupied it reports that the portal separates more than two clusters and clears the
   cluster portal flag for the area. It then records the portal in the cluster's index, increments cluster portal counts, and records the portal's negative number as the area's cluster value. Errors
   are logged when the portal cannot be found or when the portal index size limit is exceeded.

	\param areanum the index of the area whose portal is being updated
	\param clusternum the index of the cluster to associate with the portal side
	\return 1 if the portal is already updated or has been successfully updated, 0 if an error occurs (portal belongs to more than two clusters or portal not found)
*/
int AAS_UpdatePortal( int areanum, int clusternum )
{
	int			   portalnum;
	aas_portal_t*  portal;
	aas_cluster_t* cluster;

	// find the portal of the area
	for( portalnum = 1; portalnum < ( *aasworld ).numportals; portalnum++ ) {
		if( ( *aasworld ).portals[portalnum].areanum == areanum ) {
			break;
		}
	}

	//
	if( portalnum == ( *aasworld ).numportals ) {
		AAS_Error( "no portal of area %d", areanum );
		return qtrue;
	}

	//
	portal = &( *aasworld ).portals[portalnum];

	// if the portal is already fully updated
	if( portal->frontcluster == clusternum ) {
		return qtrue;
	}

	if( portal->backcluster == clusternum ) {
		return qtrue;
	}

	// if the portal has no front cluster yet
	if( !portal->frontcluster ) {
		portal->frontcluster = clusternum;
	}

	// if the portal has no back cluster yet
	else if( !portal->backcluster ) {
		portal->backcluster = clusternum;

	} else {
		Log_Write( "portal using area %d is seperating more than two clusters\r\n", areanum );
		// remove the cluster portal flag contents
		( *aasworld ).areasettings[areanum].contents &= ~AREACONTENTS_CLUSTERPORTAL;
		return qfalse;
	}

	if( ( *aasworld ).portalindexsize >= AAS_MAX_PORTALINDEXSIZE ) {
		AAS_Error( "AAS_MAX_PORTALINDEXSIZE" );
		return qtrue;
	}

	// set the area cluster number to the negative portal number
	( *aasworld ).areasettings[areanum].cluster = -portalnum;
	// add the portal to the cluster using the portal index
	cluster																  = &( *aasworld ).clusters[clusternum];
	( *aasworld ).portalindex[cluster->firstportal + cluster->numportals] = portalnum;
	( *aasworld ).portalindexsize++;
	cluster->numportals++;
	return qtrue;
}

/*!
	\brief Recursively adds an area and all areas connected through faces or reachabilities to a cluster, updating cluster information and handling portal adjustments

	The function verifies that the supplied area number is within bounds and that the area has not already been assigned to a different cluster. If the area is part of a cluster portal it delegates to
   AAS_UpdatePortal. Otherwise it records the cluster number and the index within the cluster for the area, increments the cluster’s area count, and then propagates the assignment recursively.

	For non‑portal areas the flood proceeds in two stages: first, through adjacent faces unless the global nofaceflood flag suppresses this step; second, through the reachability list for the area. In
   either stage the function calls itself recursively for each target area, returning false if any recursion fails. If all recursive calls succeed, it returns true.

	An error is logged when an area is found to belong to a cluster different from the target cluster, indicating an unintended cross‑cluster reachability, and the function then returns false.

	\param areanum The index of the area to start the flood from
	\param clusternum The cluster identifier to which the area and its neighbors should be assigned
	\return Non‑zero if the flood succeeded and the area was added to the cluster; zero if an error occurred or the area was already linked to a different cluster.
*/
int AAS_FloodClusterAreas_r( int areanum, int clusternum )
{
	aas_area_t* area;
	aas_face_t* face;
	int			facenum, i;

	//
	if( areanum <= 0 || areanum >= ( *aasworld ).numareas ) {
		AAS_Error( "AAS_FloodClusterAreas_r: areanum out of range" );
		return qfalse;
	}

	// if the area is already part of a cluster
	if( ( *aasworld ).areasettings[areanum].cluster > 0 ) {
		if( ( *aasworld ).areasettings[areanum].cluster == clusternum ) {
			return qtrue;
		}

		//
		// there's a reachability going from one cluster to another only in one direction
		//
		AAS_Error( "cluster %d touched cluster %d at area %d\r\n", clusternum, ( *aasworld ).areasettings[areanum].cluster, areanum );
		return qfalse;
	}

	// don't add the cluster portal areas to the clusters
	if( ( *aasworld ).areasettings[areanum].contents & AREACONTENTS_CLUSTERPORTAL ) {
		return AAS_UpdatePortal( areanum, clusternum );
	}

	// set the area cluster number
	( *aasworld ).areasettings[areanum].cluster		   = clusternum;
	( *aasworld ).areasettings[areanum].clusterareanum = ( *aasworld ).clusters[clusternum].numareas;
	// the cluster has an extra area
	( *aasworld ).clusters[clusternum].numareas++;

	area = &( *aasworld ).areas[areanum];

	// use area faces to flood into adjacent areas
	if( !nofaceflood ) {
		for( i = 0; i < area->numfaces; i++ ) {
			facenum = abs( ( *aasworld ).faceindex[area->firstface + i] );
			face	= &( *aasworld ).faces[facenum];

			if( face->frontarea == areanum ) {
				if( face->backarea ) {
					if( !AAS_FloodClusterAreas_r( face->backarea, clusternum ) ) {
						return qfalse;
					}
				}

			} else {
				if( face->frontarea ) {
					if( !AAS_FloodClusterAreas_r( face->frontarea, clusternum ) ) {
						return qfalse;
					}
				}
			}
		}
	}

	// use the reachabilities to flood into other areas
	for( i = 0; i < ( *aasworld ).areasettings[areanum].numreachableareas; i++ ) {
		if( !( *aasworld ).reachability[( *aasworld ).areasettings[areanum].firstreachablearea + i].areanum ) {
			continue;
		}

		if( !AAS_FloodClusterAreas_r( ( *aasworld ).reachability[( *aasworld ).areasettings[areanum].firstreachablearea + i].areanum, clusternum ) ) {
			return qfalse;
		}
	}

	return qtrue;
}

/*!
	\brief Floods unclustered areas by propagating cluster assignments from neighboring clustered areas via reachability data.

	The function iterates over all areas in the world except the first one.  For each area that has no cluster set and is not itself a cluster portal, it scans the list of reachable areas.  If a
   reachable area is a cluster portal it is ignored; otherwise, if that area already has a cluster set, the function calls the recursive flood routine to propagate the current cluster to the area in
   question.  Failure of any recursive call causes the function to return qfalse immediately; otherwise it continues.  After all areas have been processed, the function returns qtrue.

	\param clusternum The cluster index that will be assigned to unclustered areas when the flood proceeds
*/
int AAS_FloodClusterAreasUsingReachabilities( int clusternum )
{
	int i, j, areanum;

	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		// if this area already has a cluster set
		if( ( *aasworld ).areasettings[i].cluster ) {
			continue;
		}

		// if this area is a cluster portal
		if( ( *aasworld ).areasettings[i].contents & AREACONTENTS_CLUSTERPORTAL ) {
			continue;
		}

		// loop over the reachable areas from this area
		for( j = 0; j < ( *aasworld ).areasettings[i].numreachableareas; j++ ) {
			// the reachable area
			areanum = ( *aasworld ).reachability[( *aasworld ).areasettings[i].firstreachablearea + j].areanum;

			// if this area is a cluster portal
			if( ( *aasworld ).areasettings[areanum].contents & AREACONTENTS_CLUSTERPORTAL ) {
				continue;
			}

			// if this area has a cluster set
			if( ( *aasworld ).areasettings[areanum].cluster ) {
				if( !AAS_FloodClusterAreas_r( i, clusternum ) ) {
					return qfalse;
				}

				i = 0;
				break;
			}
		}
	}

	return qtrue;
}

/*!
	\brief Assigns unique area numbers to each portal of a specified cluster.

	For the cluster identified by clusternum, the function walks through its list of portal indices. For each portal, it determines whether the given cluster occupies the portal’s front or back side.
   Depending on that side, the portal’s clusterareanum field is set to the current numareas value of the cluster, and then the cluster’s numareas counter is incremented. This establishes a sequential
   numbering of portal sides within the cluster.

	\param clusternum Index of the cluster whose portals should be numbered
*/
void AAS_NumberClusterPortals( int clusternum )
{
	int			   i, portalnum;
	aas_cluster_t* cluster;
	aas_portal_t*  portal;

	cluster = &( *aasworld ).clusters[clusternum];

	for( i = 0; i < cluster->numportals; i++ ) {
		portalnum = ( *aasworld ).portalindex[cluster->firstportal + i];
		portal	  = &( *aasworld ).portals[portalnum];

		if( portal->frontcluster == clusternum ) {
			portal->clusterareanum[0] = cluster->numareas++;

		} else {
			portal->clusterareanum[1] = cluster->numareas++;
		}
	}
}

/*!
	\brief Numbers all areas and portals in a cluster, assigning internal cluster area numbers and differentiating reachable from unreachable ones.

	The function first resets the cluster’s area counters. It then scans every area; if an area belongs to the cluster and is reachable, it receives a cluster‑specific number and contributes to both
   the total area count and the reachability count. Next, all portals of the cluster that lead to a reachable area are numbered and the reachability count is incremented. After that, any areas that
   are not reachable are numbered, increasing only the total area count. Finally, all portals that lead to unreachable areas are numbered without affecting the reachability count. The numbering is
   stored in the world’s data structures: each area’s clusterareanum field and each portal’s clusterareanum entries.

	The routine assumes a valid cluster index and no bounds checking is performed. The numbering scheme is used later for pathfinding and navigation within the cluster.

	\param clusternum The index of the cluster to number its areas and portals.
*/
void AAS_NumberClusterAreas( int clusternum )
{
	int			   i, portalnum;
	aas_cluster_t* cluster;
	aas_portal_t*  portal;

	( *aasworld ).clusters[clusternum].numareas				= 0;
	( *aasworld ).clusters[clusternum].numreachabilityareas = 0;

	// number all areas in this cluster WITH reachabilities
	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		//
		if( ( *aasworld ).areasettings[i].cluster != clusternum ) {
			continue;
		}

		//
		if( !AAS_AreaReachability( i ) ) {
			continue;
		}

		//
		( *aasworld ).areasettings[i].clusterareanum = ( *aasworld ).clusters[clusternum].numareas;
		// the cluster has an extra area
		( *aasworld ).clusters[clusternum].numareas++;
		( *aasworld ).clusters[clusternum].numreachabilityareas++;
	}

	// number all portals in this cluster WITH reachabilities
	cluster = &( *aasworld ).clusters[clusternum];

	for( i = 0; i < cluster->numportals; i++ ) {
		portalnum = ( *aasworld ).portalindex[cluster->firstportal + i];
		portal	  = &( *aasworld ).portals[portalnum];

		if( !AAS_AreaReachability( portal->areanum ) ) {
			continue;
		}

		if( portal->frontcluster == clusternum ) {
			portal->clusterareanum[0] = cluster->numareas++;
			( *aasworld ).clusters[clusternum].numreachabilityareas++;

		} else {
			portal->clusterareanum[1] = cluster->numareas++;
			( *aasworld ).clusters[clusternum].numreachabilityareas++;
		}
	}

	// number all areas in this cluster WITHOUT reachabilities
	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		//
		if( ( *aasworld ).areasettings[i].cluster != clusternum ) {
			continue;
		}

		//
		if( AAS_AreaReachability( i ) ) {
			continue;
		}

		//
		( *aasworld ).areasettings[i].clusterareanum = ( *aasworld ).clusters[clusternum].numareas;
		// the cluster has an extra area
		( *aasworld ).clusters[clusternum].numareas++;
	}

	// number all portals in this cluster WITHOUT reachabilities
	cluster = &( *aasworld ).clusters[clusternum];

	for( i = 0; i < cluster->numportals; i++ ) {
		portalnum = ( *aasworld ).portalindex[cluster->firstportal + i];
		portal	  = &( *aasworld ).portals[portalnum];

		if( AAS_AreaReachability( portal->areanum ) ) {
			continue;
		}

		if( portal->frontcluster == clusternum ) {
			portal->clusterareanum[0] = cluster->numareas++;

		} else {
			portal->clusterareanum[1] = cluster->numareas++;
		}
	}
}

/*!
	\brief Builds connected area clusters by flooding the world and recording their boundaries

	The function first removes any previously stored cluster data, then iterates over all non‑dummy areas in the world. For each area that is not already assigned to a cluster, does not contain a
   portal, and satisfies reachability conditions when nofaceflood is enabled, it starts a new cluster. It initializes a fresh cluster structure, performs a recursive flood to collect all reachable
   areas, augments that list using reachabilities, numbers the cluster areas, and increments the global cluster count. If the maximum cluster limit is exceeded or a flood operation fails, the function
   returns "qfalse" and logs an error.

	\return qtrue if all clusters were successfully created, otherwise qfalse
*/
int AAS_FindClusters()
{
	int			   i;
	aas_cluster_t* cluster;

	AAS_RemoveClusterAreas();

	//
	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		// if the area is already part of a cluster
		if( ( *aasworld ).areasettings[i].cluster ) {
			continue;
		}

		// if not flooding through faces only use areas that have reachabilities
		if( nofaceflood ) {
			if( !( *aasworld ).areasettings[i].numreachableareas ) {
				continue;
			}
		}

		// if the area is a cluster portal
		if( ( *aasworld ).areasettings[i].contents & AREACONTENTS_CLUSTERPORTAL ) {
			continue;
		}

		if( ( *aasworld ).numclusters >= AAS_MAX_CLUSTERS ) {
			AAS_Error( "AAS_MAX_CLUSTERS" );
			return qfalse;
		}

		cluster						  = &( *aasworld ).clusters[( *aasworld ).numclusters];
		cluster->numareas			  = 0;
		cluster->numreachabilityareas = 0;
		cluster->firstportal		  = ( *aasworld ).portalindexsize;
		cluster->numportals			  = 0;

		// flood the areas in this cluster
		if( !AAS_FloodClusterAreas_r( i, ( *aasworld ).numclusters ) ) {
			return qfalse;
		}

		if( !AAS_FloodClusterAreasUsingReachabilities( ( *aasworld ).numclusters ) ) {
			return qfalse;
		}

		// number the cluster areas
		// AAS_NumberClusterPortals((*aasworld).numclusters);
		AAS_NumberClusterAreas( ( *aasworld ).numclusters );
		// Log_Write("cluster %d has %d areas\r\n", (*aasworld).numclusters, cluster->numareas);
		( *aasworld ).numclusters++;
	}

	return qtrue;
}

/*!
	\brief Creates portal entries for all cluster portal areas and logs each portal.

	Iterates over all areas starting from index 1 and for any area flagged as a cluster portal, creates a portal record in the global AAS world structure. Each portal record is initialized with the
   area number, and front and back cluster identifiers set to 0. If the total number of portals would exceed the maximum allowed, the function calls AAS_Error and aborts. Each created portal is logged
   and the global portal count is incremented.

*/
void AAS_CreatePortals()
{
	int			  i;
	aas_portal_t* portal;

	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		// if the area is a cluster portal
		if( ( *aasworld ).areasettings[i].contents & AREACONTENTS_CLUSTERPORTAL ) {
			if( ( *aasworld ).numportals >= AAS_MAX_PORTALS ) {
				AAS_Error( "AAS_MAX_PORTALS" );
				return;
			}

			portal				 = &( *aasworld ).portals[( *aasworld ).numportals];
			portal->areanum		 = i;
			portal->frontcluster = 0;
			portal->backcluster	 = 0;
			Log_Write( "portal %d: area %d\r\n", ( *aasworld ).numportals, portal->areanum );
			( *aasworld ).numportals++;
		}
	}
}

/*!
	\brief Recursively marks all connected areas from a starting area through non‑solid faces.

	The function takes a list of area identifiers and, starting from a specified index within that list, traverses each face of the current area. Non‑solid faces are inspected to find the adjacent
   area on the other side of the face. If that adjacent area is also in the supplied list and has not yet been marked, the function recurses with that adjacent area as the new starting point. As a
   side effect, the "connectedareas" array is used both as a visited set and as the result, storing a boolean value for each index in the area list. After the function returns, "connectedareas[i]" is
   true for every area in the list that can be reached from the original area via a path of non‑solid faces.

	The algorithm assumes that "areanums" and "connectedareas" are of length "numareas" and that the global AAS world data structures are correctly initialized.

	\param areanums array of area identifiers to consider in the search
	\param numareas number of elements in the areanums and connectedareas arrays
	\param connectedareas boolean array marking whether each area has been reached; used as both visitation flag and result
	\param curarea index into areanums indicating the area currently being expanded
*/
void AAS_ConnectedAreas_r( int* areanums, int numareas, int* connectedareas, int curarea )
{
	int			i, j, otherareanum, facenum;
	aas_area_t* area;
	aas_face_t* face;

	connectedareas[curarea] = qtrue;
	area					= &( *aasworld ).areas[areanums[curarea]];

	for( i = 0; i < area->numfaces; i++ ) {
		facenum = abs( ( *aasworld ).faceindex[area->firstface + i] );
		face	= &( *aasworld ).faces[facenum];

		// if the face is solid
		if( face->faceflags & FACE_SOLID ) {
			continue;
		}

		// get the area at the other side of the face
		if( face->frontarea != areanums[curarea] ) {
			otherareanum = face->frontarea;

		} else {
			otherareanum = face->backarea;
		}

		// check if the face is leading to one of the other areas
		for( j = 0; j < numareas; j++ ) {
			if( areanums[j] == otherareanum ) {
				break;
			}
		}

		// if the face isn't leading to one of the other areas
		if( j == numareas ) {
			continue;
		}

		// if the other area is already connected
		if( connectedareas[j] ) {
			continue;
		}

		// recursively proceed with the other area
		AAS_ConnectedAreas_r( areanums, numareas, connectedareas, j );
	}
}

/*!
	\brief Determine whether a set of area numbers is fully connected through portals.

	The function starts by clearing an array that will record which areas are reachable. If the number of areas is less than 1, it immediately returns false. For a single area it returns true. For
   multiple areas it calls the recursive helper to mark all areas reachable from the first one. After traversal, it verifies that every supplied area was marked; if any area was left unreachable, it
   returns false; otherwise it returns true.

	\param areanums An array of area identifiers to test for connectivity.
	\param numareas The number of identifiers in the areanums array.
	\return True if every area can reach every other, false otherwise.
*/
qboolean AAS_ConnectedAreas( int* areanums, int numareas )
{
	int connectedareas[MAX_PORTALAREAS], i;

	memset( connectedareas, 0, sizeof( connectedareas ) );

	if( numareas < 1 ) {
		return qfalse;
	}

	if( numareas == 1 ) {
		return qtrue;
	}

	AAS_ConnectedAreas_r( areanums, numareas, connectedareas, 0 );

	for( i = 0; i < numareas; i++ ) {
		if( !connectedareas[i] ) {
			return qfalse;
		}
	}

	return qtrue;
}

/*!
	\brief Recursively collects adjacent areas that have a subset of presence types, storing their identifiers in a supplied array.

	The function begins by adding the current area to the list and then iterates over each face of that area. Faces marked as solid are skipped. For each non‑solid face it determines the area on the
   opposite side of the face. If that neighboring area has a presencetype that is a proper subset of the current area's presencetype (i.e., the current area contains all the presence bits of the
   neighbor but no additional bits) and it is not already in the list, the function recurses on that neighbor. The recursion stops when there are no further qualifying adjacent areas or when the
   number of collected areas would exceed the defined limit of MAX_PORTALAREAS. In that case AAS_Error is invoked and the current count is returned. This mechanism is used to gather connected areas
   that share the same or a smaller set of presence types for further processing in cluster creation.

	\param areanums array that receives the area numbers identified during the recursion
	\param numareas current count of areas already stored in the array, incremented as new areas are added
	\param curareanum area identifier for the area being processed in the current recursion step
	\return The total number of areas found and stored in the array after the recursion completes
*/
int AAS_GetAdjacentAreasWithLessPresenceTypes_r( int* areanums, int numareas, int curareanum )
{
	int			i, j, presencetype, otherpresencetype, otherareanum, facenum;
	aas_area_t* area;
	aas_face_t* face;

	areanums[numareas++] = curareanum;
	area				 = &( *aasworld ).areas[curareanum];
	presencetype		 = ( *aasworld ).areasettings[curareanum].presencetype;

	for( i = 0; i < area->numfaces; i++ ) {
		facenum = abs( ( *aasworld ).faceindex[area->firstface + i] );
		face	= &( *aasworld ).faces[facenum];

		// if the face is solid
		if( face->faceflags & FACE_SOLID ) {
			continue;
		}

		// the area at the other side of the face
		if( face->frontarea != curareanum ) {
			otherareanum = face->frontarea;

		} else {
			otherareanum = face->backarea;
		}

		//
		otherpresencetype = ( *aasworld ).areasettings[otherareanum].presencetype;

		// if the other area has less presence types
		if( ( presencetype & ~otherpresencetype ) && !( otherpresencetype & ~presencetype ) ) {
			// check if the other area isn't already in the list
			for( j = 0; j < numareas; j++ ) {
				if( otherareanum == areanums[j] ) {
					break;
				}
			}

			// if the other area isn't already in the list
			if( j == numareas ) {
				if( numareas >= MAX_PORTALAREAS ) {
					AAS_Error( "MAX_PORTALAREAS" );
					return numareas;
				}

				numareas = AAS_GetAdjacentAreasWithLessPresenceTypes_r( areanums, numareas, otherareanum );
			}
		}
	}

	return numareas;
}

/*!
	\brief Determines whether the specified area can form part of a cluster portal and, if so, marks the area and its adjacent areas as portal and route areas.

	The function first confirms the area is not already flagged as a portal and is ground‑connected. It collects all adjacent areas that share faces with fewer presence types. For each of these
   neighboring areas it examines their shared faces to separate them into front and back groups, ensuring each group contains at least one face and that all front faces share a common plane and all
   back faces share another. It checks connectivity of the front and back groups, then guarantees that no edge of a front face is shared by a back face. If all conditions are met, the function flags
   each area in the set with the cluster portal and route portal bits, logs the action, and returns the number of areas added. If any check fails the function returns 0 and makes no changes.

	\param areanum Index of the area to evaluate for potential portal inclusion.
	\return The number of areas that were successfully marked as part of a new cluster portal; returns 0 if the area cannot serve as a portal.
*/
int AAS_CheckAreaForPossiblePortals( int areanum )
{
	int			i, j, k, fen, ben, frontedgenum, backedgenum, facenum;
	int			areanums[MAX_PORTALAREAS], numareas, otherareanum;
	int			numareafrontfaces[MAX_PORTALAREAS], numareabackfaces[MAX_PORTALAREAS];
	int			frontfacenums[MAX_PORTALAREAS], backfacenums[MAX_PORTALAREAS];
	int			numfrontfaces, numbackfaces;
	int			frontareanums[MAX_PORTALAREAS], backareanums[MAX_PORTALAREAS];
	int			numfrontareas, numbackareas;
	int			frontplanenum, backplanenum, faceplanenum;
	aas_area_t* area;
	aas_face_t *frontface, *backface, *face;

	// if it isn't already a portal
	if( ( *aasworld ).areasettings[areanum].contents & AREACONTENTS_CLUSTERPORTAL ) {
		return 0;
	}

	// it must be a grounded area
	if( !( ( *aasworld ).areasettings[areanum].areaflags & AREA_GROUNDED ) ) {
		return 0;
	}

	//
	memset( numareafrontfaces, 0, sizeof( numareafrontfaces ) );
	memset( numareabackfaces, 0, sizeof( numareabackfaces ) );
	numareas = numfrontfaces = numbackfaces = 0;
	numfrontareas = numbackareas = 0;
	frontplanenum = backplanenum = -1;
	// add any adjacent areas with less presence types
	numareas = AAS_GetAdjacentAreasWithLessPresenceTypes_r( areanums, 0, areanum );

	//
	for( i = 0; i < numareas; i++ ) {
		area = &( *aasworld ).areas[areanums[i]];

		for( j = 0; j < area->numfaces; j++ ) {
			facenum = abs( ( *aasworld ).faceindex[area->firstface + j] );
			face	= &( *aasworld ).faces[facenum];

			// if the face is solid
			if( face->faceflags & FACE_SOLID ) {
				continue;
			}

			// check if the face is shared with one of the other areas
			for( k = 0; k < numareas; k++ ) {
				if( k == i ) {
					continue;
				}

				if( face->frontarea == areanums[k] || face->backarea == areanums[k] ) {
					break;
				}
			}

			// if the face is shared
			if( k != numareas ) {
				continue;
			}

			// the number of the area at the other side of the face
			if( face->frontarea == areanums[i] ) {
				otherareanum = face->backarea;

			} else {
				otherareanum = face->frontarea;
			}

			// if the other area already is a cluter portal
			if( ( *aasworld ).areasettings[otherareanum].contents & AREACONTENTS_CLUSTERPORTAL ) {
				return 0;
			}

			// number of the plane of the area
			faceplanenum = face->planenum & ~1;

			//
			if( frontplanenum < 0 || faceplanenum == frontplanenum ) {
				frontplanenum				   = faceplanenum;
				frontfacenums[numfrontfaces++] = facenum;

				for( k = 0; k < numfrontareas; k++ ) {
					if( frontareanums[k] == otherareanum ) {
						break;
					}
				}

				if( k == numfrontareas ) {
					frontareanums[numfrontareas++] = otherareanum;
				}

				numareafrontfaces[i]++;

			} else if( backplanenum < 0 || faceplanenum == backplanenum ) {
				backplanenum				 = faceplanenum;
				backfacenums[numbackfaces++] = facenum;

				for( k = 0; k < numbackareas; k++ ) {
					if( backareanums[k] == otherareanum ) {
						break;
					}
				}

				if( k == numbackareas ) {
					backareanums[numbackareas++] = otherareanum;
				}

				numareabackfaces[i]++;

			} else {
				return 0;
			}
		}
	}

	// every area should have at least one front face and one back face
	for( i = 0; i < numareas; i++ ) {
		if( !numareafrontfaces[i] || !numareabackfaces[i] ) {
			return 0;
		}
	}

	// the front areas should all be connected
	if( !AAS_ConnectedAreas( frontareanums, numfrontareas ) ) {
		return 0;
	}

	// the back areas should all be connected
	if( !AAS_ConnectedAreas( backareanums, numbackareas ) ) {
		return 0;
	}

	// none of the front faces should have a shared edge with a back face
	for( i = 0; i < numfrontfaces; i++ ) {
		frontface = &( *aasworld ).faces[frontfacenums[i]];

		for( fen = 0; fen < frontface->numedges; fen++ ) {
			frontedgenum = abs( ( *aasworld ).edgeindex[frontface->firstedge + fen] );

			for( j = 0; j < numbackfaces; j++ ) {
				backface = &( *aasworld ).faces[backfacenums[j]];

				for( ben = 0; ben < backface->numedges; ben++ ) {
					backedgenum = abs( ( *aasworld ).edgeindex[backface->firstedge + ben] );

					if( frontedgenum == backedgenum ) {
						break;
					}
				}

				if( ben != backface->numedges ) {
					break;
				}
			}

			if( j != numbackfaces ) {
				break;
			}
		}

		if( fen != frontface->numedges ) {
			break;
		}
	}

	if( i != numfrontfaces ) {
		return 0;
	}

	// set the cluster portal contents
	for( i = 0; i < numareas; i++ ) {
		( *aasworld ).areasettings[areanums[i]].contents |= AREACONTENTS_CLUSTERPORTAL;
		// this area can be used as a route portal
		( *aasworld ).areasettings[areanums[i]].contents |= AREACONTENTS_ROUTEPORTAL;
		Log_Write( "possible portal: %d\r\n", areanums[i] );
	}

	//
	return numareas;
}

/*!
	\brief Counts all potential portals in the current AAS world and reports the total.

	Iterates through each area of the AAS world (starting from area 1) and calls AAS_CheckAreaForPossiblePortals to determine how many portals that area can form. Sums those counts and prints the
   total number of possible portals to the bot output stream.

*/
void AAS_FindPossiblePortals()
{
	int i, numpossibleportals;

	numpossibleportals = 0;

	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		numpossibleportals += AAS_CheckAreaForPossiblePortals( i );
	}

	botimport.Print( PRT_MESSAGE, "\r%6d possible portals\n", numpossibleportals );
}

/*!
	\brief Clears the cluster portal flag for all areas in the current AAS world.

	Iterates over all area entries in the AAS world, starting from index 1 up to but not including the total number of areas, and removes the AREACONTENTS_CLUSTERPORTAL flag from each area's contents
   field.

*/
void AAS_RemoveAllPortals()
{
	int i;

	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		( *aasworld ).areasettings[i].contents &= ~AREACONTENTS_CLUSTERPORTAL;
	}
}

/*!
	\brief Validates that every non‑dummy portal has both a front and a back cluster, cleaning up area flags and logging missing links as it does so.

	This function walks through the portal array starting after the placeholder at index 0. For each portal it checks whether both the frontcluster and backcluster members are non‑zero. If one of them
   is missing the function removes the AREACONTENTS_CLUSTERPORTAL flag from the area settings of that portal, logs a message describing the missing cluster, and immediately returns with a false
   result. If all portals have both connections, the function returns a true value. The function performs no external side‑effects besides the area flag manipulation and log output.

	\return Non‑zero value if every portal has connected front and back clusters; zero otherwise.
*/
int AAS_TestPortals()
{
	int			  i;
	aas_portal_t* portal;

	for( i = 1; i < ( *aasworld ).numportals; i++ ) {
		portal = &( *aasworld ).portals[i];

		if( !portal->frontcluster ) {
			( *aasworld ).areasettings[portal->areanum].contents &= ~AREACONTENTS_CLUSTERPORTAL;
			Log_Write( "portal area %d has no front cluster\r\n", portal->areanum );
			return qfalse;
		}

		if( !portal->backcluster ) {
			( *aasworld ).areasettings[portal->areanum].contents &= ~AREACONTENTS_CLUSTERPORTAL;
			Log_Write( "portal area %d has no back cluster\r\n", portal->areanum );
			return qfalse;
		}
	}

	return qtrue;
}

/*!
	\brief Counts the forced cluster portals in the world and prints the total.

	Iterates over all areas defined in the global world and increments a counter for each area that has the cluster portal flag set. After counting, the value is output using botimport.Print with a
   formatted message.

*/
void AAS_CountForcedClusterPortals()
{
	int num, i;

	num = 0;

	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		if( ( *aasworld ).areasettings[i].contents & AREACONTENTS_CLUSTERPORTAL ) {
			num++;
		}
	}

	botimport.Print( PRT_MESSAGE, "%6d forced portals\n", num );
}

/*!
	\brief Sets the AREACONTENTS_VIEWPORTAL flag on all area settings that currently have the AREACONTENTS_CLUSTERPORTAL flag.

	The function iterates over each area setting in the global aasworld structure except the first entry. For each area, it checks whether the AREACONTENTS_CLUSTERPORTAL bit is set in the contents
   field. If that bit is present, it sets the AREACONTENTS_VIEWPORTAL bit in the same field, enabling the area to be treated as a viewport portal for rendering purposes.

*/
void AAS_CreateViewPortals()
{
	int i;

	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		if( ( *aasworld ).areasettings[i].contents & AREACONTENTS_CLUSTERPORTAL ) {
			( *aasworld ).areasettings[i].contents |= AREACONTENTS_VIEWPORTAL;
		}
	}
}

/*!
	\brief Adds the cluster portal flag to any area that is already marked as a viewportal.

	Iterates through all areas in the AAS world, and for each area whose settings contain the view portal flag, it also sets the cluster portal flag in that area's content bitmap.

*/
void AAS_SetViewPortalsAsClusterPortals()
{
	int i;

	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		if( ( *aasworld ).areasettings[i].contents & AREACONTENTS_VIEWPORTAL ) {
			( *aasworld ).areasettings[i].contents |= AREACONTENTS_CLUSTERPORTAL;
		}
	}
}

/*!
	\brief Initializes portal and cluster structures for the AAS system using the current world geometry and configuration settings.

	If the world has not been loaded it exits immediately.  When clustering already exists it will skip initialization unless the configuration variables "forceclustering" or "forcereachability"
   demand it; for the BSPC build the function always returns early.

	The routine counts the forced cluster portals then removes any existing portal and cluster markers, and searches for possible portals, building view portals for the bot.  Memory for portals,
   portal indices and clusters is cleared and allocated fresh.

	In a loop it repeatedly creates portals, finds clusters, tests the portals and continues until a successful configuration is found.  Each iteration increments a counter that tracks how many times
   the portal area was removed.

	After a valid configuration is achieved it marks the AAS file as needing to be saved, dumps statistics of portals, clusters and reachability areas, and reports an efficiency metric based on the
   number of reachability areas and portals.  The function does not return any value.

	TODO: clarify the purpose of the removedPortalAreas counter that is printed during the loop.

*/
void AAS_InitClustering()
{
	int i, removedPortalAreas;
	int n, total, numreachabilityareas;

	if( !( *aasworld ).loaded ) {
		return;
	}

	// if there are clusters
	if( ( *aasworld ).numclusters >= 1 ) {
#ifndef BSPC

		// if clustering isn't forced
		if( !( ( int )LibVarGetValue( "forceclustering" ) ) && !( ( int )LibVarGetValue( "forcereachability" ) ) ) {
			return;
		}

#else
		return;
#endif
	}

	//
	AAS_CountForcedClusterPortals();
	// remove all the existing portals
	// AAS_RemoveAllPortals();
	// remove all area cluster marks
	AAS_RemoveClusterAreas();
	// find possible cluster portals
	AAS_FindPossiblePortals();
	// craete portals to for the bot view
	AAS_CreateViewPortals();

	// remove all portals that are not closing a cluster
	// AAS_RemoveNotClusterClosingPortals();
	// initialize portal memory
	if( ( *aasworld ).portals ) {
		FreeMemory( ( *aasworld ).portals );
	}

	( *aasworld ).portals = ( aas_portal_t* )GetClearedMemory( AAS_MAX_PORTALS * sizeof( aas_portal_t ) );

	// initialize portal index memory
	if( ( *aasworld ).portalindex ) {
		FreeMemory( ( *aasworld ).portalindex );
	}

	( *aasworld ).portalindex = ( aas_portalindex_t* )GetClearedMemory( AAS_MAX_PORTALINDEXSIZE * sizeof( aas_portalindex_t ) );

	// initialize cluster memory
	if( ( *aasworld ).clusters ) {
		FreeMemory( ( *aasworld ).clusters );
	}

	( *aasworld ).clusters = ( aas_cluster_t* )GetClearedMemory( AAS_MAX_CLUSTERS * sizeof( aas_cluster_t ) );
	//
	removedPortalAreas = 0;
	botimport.Print( PRT_MESSAGE, "\r%6d removed portal areas", removedPortalAreas );

	while( 1 ) {
		botimport.Print( PRT_MESSAGE, "\r%6d", removedPortalAreas );
		// initialize the number of portals and clusters
		( *aasworld ).numportals	  = 1; // portal 0 is a dummy
		( *aasworld ).portalindexsize = 0;
		( *aasworld ).numclusters	  = 1; // cluster 0 is a dummy
		// create the portals from the portal areas
		AAS_CreatePortals();
		//
		removedPortalAreas++;

		// find the clusters
		if( !AAS_FindClusters() ) {
			continue;
		}

		// test the portals
		if( !AAS_TestPortals() ) {
			continue;
		}

		//
		break;
	}

	botimport.Print( PRT_MESSAGE, "\n" );
	// the AAS file should be saved
	( *aasworld ).savefile = qtrue;
	// report cluster info
	botimport.Print( PRT_MESSAGE, "%6d portals created\n", ( *aasworld ).numportals );
	botimport.Print( PRT_MESSAGE, "%6d clusters created\n", ( *aasworld ).numclusters );

	for( i = 1; i < ( *aasworld ).numclusters; i++ ) {
		botimport.Print( PRT_MESSAGE, "cluster %d has %d reachability areas\n", i, ( *aasworld ).clusters[i].numreachabilityareas );
	}

	// report AAS file efficiency
	numreachabilityareas = 0;
	total				 = 0;

	for( i = 0; i < ( *aasworld ).numclusters; i++ ) {
		n = ( *aasworld ).clusters[i].numreachabilityareas;
		numreachabilityareas += n;
		total += n * n;
	}

	total += numreachabilityareas * ( *aasworld ).numportals;
	//
	botimport.Print( PRT_MESSAGE, "%6i total reachability areas\n", numreachabilityareas );
	botimport.Print( PRT_MESSAGE, "%6i AAS memory/CPU usage (the lower the better)\n", total * 3 );
}