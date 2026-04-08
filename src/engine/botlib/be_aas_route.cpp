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
 * name:		be_aas_route.c
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_utils.h"
#include "l_memory.h"
#include "l_log.h"
#include "l_crc.h"
#include "l_libvar.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "botshared/botlib.h"
#include "botshared/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"

#define ROUTING_DEBUG

// travel time in hundreths of a second = distance * 100 / speed
#define DISTANCEFACTOR_CROUCH	  1.3  // crouch speed = 100
#define DISTANCEFACTOR_SWIM		  1	   // should be 0.66, swim speed = 150
#define DISTANCEFACTOR_WALK		  0.33 // walk speed = 300

// Ridah, scale traveltimes with ground steepness of area
#define GROUNDSTEEPNESS_TIMESCALE 20 // this is the maximum scale, 1 being the usual for a flat ground

// cache refresh time
#define CACHE_REFRESHTIME		  15.0 // 15 seconds refresh time

// maximum number of routing updates each frame
#define MAX_FRAMEROUTINGUPDATES	  100

extern aas_t aasworlds[MAX_AAS_WORLDS];

/*

  area routing cache:
  stores the distances within one cluster to a specific goal area
  this goal area is in this same cluster and could be a cluster portal
  for every cluster there's a list with routing cache for every area
  in that cluster (including the portals of that cluster)
  area cache stores (*aasworld).clusters[?].numreachabilityareas travel times

  portal routing cache:
  stores the distances of all portals to a specific goal area
  this goal area could be in any cluster and could also be a cluster portal
  for every area ((*aasworld).numareas) the portal cache stores
  (*aasworld).numportals travel times

*/

#ifdef ROUTING_DEBUG
int numareacacheupdates;
int numportalcacheupdates;
#endif // ROUTING_DEBUG

int	  routingcachesize;
int	  max_routingcachesize;

/*!
	\brief Allocates and clears a block of memory for routing structures.

	The function forwards the requested size to an underlying allocation routine that returns a zero‑initialized buffer. This abstraction permits the routing code to switch between different memory
   pools, such as a hunk or a zone, without modifications.

	\param size Number of bytes to allocate.
	\return Pointer to the allocated memory, or nullptr if the allocation fails.
*/
void* AAS_RoutingGetMemory( int size )
{
	return GetClearedMemory( size );
}

/*!
	\brief Releases memory used by the AAS routing subsystem.

	This function frees a block of memory that was previously allocated by the AAS routing system. The pointer passed to it should refer to memory obtained through the routing allocator and must not
   have been freed already. Internally it simply forwards the request to the platform's FreeMemory routine, ensuring that the routing subsystem's custom allocation tracking is respected.

	TODO: clarify whether specific type of allocation or alignment is required.

	\param ptr Pointer to the memory block allocated by the AAS routing allocator that should be deallocated.
*/
void AAS_RoutingFreeMemory( void* ptr )
{
	FreeMemory( ptr );
}

// done.

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
#ifdef ROUTING_DEBUG

/*!
	\brief Prints the current counts of area cache, portal cache updates, and routing cache byte usage.

	This function outputs three statistics via botimport.Print: the number of area cache updates that have occurred, the number of portal cache updates, and the size, in bytes, of the routing cache.

*/
void AAS_RoutingInfo()
{
	botimport.Print( PRT_MESSAGE, "%d area cache updates\n", numareacacheupdates );
	botimport.Print( PRT_MESSAGE, "%d portal cache updates\n", numportalcacheupdates );
	botimport.Print( PRT_MESSAGE, "%d bytes routing cache\n", routingcachesize );
}

#endif // ROUTING_DEBUG

/*!
	\brief Returns the cluster‐relative number of a given area or portal within the specified cluster.

	The function retrieves the internal area index used within a cluster. If the supplied area number refers to a regular area whose cluster field is positive, the function returns its cluster
   area number from the area's settings. If the area number refers to a portal (represented by a negative cluster value), the portal’s front or back cluster is compared with the supplied cluster
   and the appropriate side’s cluster area number is returned.

	\param cluster The cluster index in which to locate the area.
	\param areanum A global area number; may refer to a normal area or a portal.
	\return An integer representing the area number relative to the cluster context.
*/
__inline int AAS_ClusterAreaNum( int cluster, int areanum )
{
	int side, areacluster;

	areacluster = ( *aasworld ).areasettings[areanum].cluster;

	if( areacluster > 0 ) {
		return ( *aasworld ).areasettings[areanum].clusterareanum;

	} else {
		/*#ifdef ROUTING_DEBUG
				if ((*aasworld).portals[-areacluster].frontcluster != cluster &&
						(*aasworld).portals[-areacluster].backcluster != cluster)
				{
					botimport.Print(PRT_ERROR, "portal %d: does not belong to cluster %d\n"
													, -areacluster, cluster);
				}
		#endif //ROUTING_DEBUG*/
		side = ( *aasworld ).portals[-areacluster].frontcluster != cluster;
		return ( *aasworld ).portals[-areacluster].clusterareanum[side];
	}
}

/*!
	\brief Initializes the travel flag lookup table in the AAS world.

	The function clears the entire travel flag array by setting each entry to an invalid value. It then assigns the correct travel flag for each defined travel type, such as walk, jump, ladder, and
   other movement actions. This prepared table is used later for pathfinding and movement logic.

*/
void AAS_InitTravelFlagFromType()
{
	int i;

	for( i = 0; i < MAX_TRAVELTYPES; i++ ) {
		( *aasworld ).travelflagfortype[i] = TFL_INVALID;
	}

	( *aasworld ).travelflagfortype[TRAVEL_INVALID]		 = TFL_INVALID;
	( *aasworld ).travelflagfortype[TRAVEL_WALK]		 = TFL_WALK;
	( *aasworld ).travelflagfortype[TRAVEL_CROUCH]		 = TFL_CROUCH;
	( *aasworld ).travelflagfortype[TRAVEL_BARRIERJUMP]	 = TFL_BARRIERJUMP;
	( *aasworld ).travelflagfortype[TRAVEL_JUMP]		 = TFL_JUMP;
	( *aasworld ).travelflagfortype[TRAVEL_LADDER]		 = TFL_LADDER;
	( *aasworld ).travelflagfortype[TRAVEL_WALKOFFLEDGE] = TFL_WALKOFFLEDGE;
	( *aasworld ).travelflagfortype[TRAVEL_SWIM]		 = TFL_SWIM;
	( *aasworld ).travelflagfortype[TRAVEL_WATERJUMP]	 = TFL_WATERJUMP;
	( *aasworld ).travelflagfortype[TRAVEL_TELEPORT]	 = TFL_TELEPORT;
	( *aasworld ).travelflagfortype[TRAVEL_ELEVATOR]	 = TFL_ELEVATOR;
	( *aasworld ).travelflagfortype[TRAVEL_ROCKETJUMP]	 = TFL_ROCKETJUMP;
	( *aasworld ).travelflagfortype[TRAVEL_BFGJUMP]		 = TFL_BFGJUMP;
	( *aasworld ).travelflagfortype[TRAVEL_GRAPPLEHOOK]	 = TFL_GRAPPLEHOOK;
	( *aasworld ).travelflagfortype[TRAVEL_DOUBLEJUMP]	 = TFL_DOUBLEJUMP;
	( *aasworld ).travelflagfortype[TRAVEL_RAMPJUMP]	 = TFL_RAMPJUMP;
	( *aasworld ).travelflagfortype[TRAVEL_STRAFEJUMP]	 = TFL_STRAFEJUMP;
	( *aasworld ).travelflagfortype[TRAVEL_JUMPPAD]		 = TFL_JUMPPAD;
	( *aasworld ).travelflagfortype[TRAVEL_FUNCBOB]		 = TFL_FUNCBOB;
}

int AAS_TravelFlagForType( int traveltype )
{
	if( traveltype < 0 || traveltype >= MAX_TRAVELTYPES ) {
		return TFL_INVALID;
	}

	return ( *aasworld ).travelflagfortype[traveltype];
}

/*!
	\brief Return the current AAS routing time.

	The function provides the current routing time reported by the system’s time function. It acts as a thin wrapper that forwards the value returned by AAS_Time, allowing callers to obtain the
   routing timestamp without knowing the underlying implementation. The returned value is a float in the same units used by AAS_Time, typically milliseconds or a game‑specific time unit.

	\return A float value representing the current AAS routing time.
*/
__inline float AAS_RoutingTime()
{
	return AAS_Time();
}

/*!
	\brief Release a routing cache and adjust the global cache size counter.

	The function reduces the global variable routingcachesize by the cache’s size and then frees the memory associated with the cache using AAS_RoutingFreeMemory. After this call, the caller must not
   access the cache pointer.

	\param cache Pointer to the routing cache structure to be freed.
*/
void AAS_FreeRoutingCache( aas_routingcache_t* cache )
{
	routingcachesize -= cache->size;
	AAS_RoutingFreeMemory( cache );
}

/*!
	\brief Removes all routing cache entries for the specified cluster

	If the global cluster area cache exists, this function iterates over all areas in the designated cluster and frees each routing cache node in the linked list. After freeing, the cache pointers for
   each area are set to null. The operation has no effect when the cluster area cache is not present. It performs no returning value and modifies the global aaworld cluster area cache structure.

	\param clusternum Index of the cluster whose routing cache should be removed; must be a valid cluster number in the current world data
*/
void AAS_RemoveRoutingCacheInCluster( int clusternum )
{
	int					i;
	aas_routingcache_t *cache, *nextcache;
	aas_cluster_t*		cluster;

	if( !( *aasworld ).clusterareacache ) {
		return;
	}

	cluster = &( *aasworld ).clusters[clusternum];

	for( i = 0; i < cluster->numareas; i++ ) {
		for( cache = ( *aasworld ).clusterareacache[clusternum][i]; cache; cache = nextcache ) {
			nextcache = cache->next;
			AAS_FreeRoutingCache( cache );
		}

		( *aasworld ).clusterareacache[clusternum][i] = NULL;
	}
}

/*!
	\brief Removes all routing cache entries that depend on a specified area.

	The function first determines the cluster that the area belongs to. If the area has a normal positive cluster number, the routing cache for that cluster is cleared. If the cluster number is
   negative, the area is considered a portal and the cache for both the front and back side clusters linked to the portal is cleared. After handling cluster‑specific cache, the function iterates
   through all portal cache lists and frees each cache element, effectively resetting the portal cache for the world.

	\param areanum The numeric identifier of the area whose routing cache should be removed.
*/
void AAS_RemoveRoutingCacheUsingArea( int areanum )
{
	int					i, clusternum;
	aas_routingcache_t *cache, *nextcache;

	clusternum = ( *aasworld ).areasettings[areanum].cluster;

	if( clusternum > 0 ) {
		// remove all the cache in the cluster the area is in
		AAS_RemoveRoutingCacheInCluster( clusternum );

	} else {
		// if this is a portal remove all cache in both the front and back cluster
		AAS_RemoveRoutingCacheInCluster( ( *aasworld ).portals[-clusternum].frontcluster );
		AAS_RemoveRoutingCacheInCluster( ( *aasworld ).portals[-clusternum].backcluster );
	}

	// remove all portal cache
	if( ( *aasworld ).portalcache ) {
		for( i = 0; i < ( *aasworld ).numareas; i++ ) {
			// refresh portal cache
			for( cache = ( *aasworld ).portalcache[i]; cache; cache = nextcache ) {
				nextcache = cache->next;
				AAS_FreeRoutingCache( cache );
			}

			( *aasworld ).portalcache[i] = NULL;
		}
	}
}

int AAS_EnableRoutingArea( int areanum, int enable )
{
	int flags;

	if( areanum <= 0 || areanum >= ( *aasworld ).numareas ) {
		if( bot_developer ) {
			botimport.Print( PRT_ERROR, "AAS_EnableRoutingArea: areanum %d out of range\n", areanum );
		}

		return 0;
	}

	flags = ( *aasworld ).areasettings[areanum].areaflags & AREA_DISABLED;

	if( enable < 0 ) {
		return !flags;
	}

	if( enable ) {
		( *aasworld ).areasettings[areanum].areaflags &= ~AREA_DISABLED;

	} else {
		( *aasworld ).areasettings[areanum].areaflags |= AREA_DISABLED;
	}

	// if the status of the area changed
	if( ( flags & AREA_DISABLED ) != ( ( *aasworld ).areasettings[areanum].areaflags & AREA_DISABLED ) ) {
		// remove all routing cache involving this area
		AAS_RemoveRoutingCacheUsingArea( areanum );
	}

	return !flags;
}

/*!
	\brief Builds reversed reachability links for every area by inverting the original reachability references.

	First, any previously allocated reversed reachability data is freed. Then a memory block large enough to hold an array of reversed reachability structures and a list of reversed link entries is
   obtained. The function iterates over all areas, inspecting each area's list of directly reachable areas. For every such link, a reversed link entry is created pointing back to the source area.
   These entries are inserted at the head of a linked list per destination area, and the link count for that area is incremented. When DEBUG mode is enabled the time taken for this process is printed.
   The resulting data structure allows quick determination of which areas can reach a given area.

*/
void AAS_CreateReversedReachability()
{
	int					i, n;
	aas_reversedlink_t* revlink;
	aas_reachability_t* reach;
	aas_areasettings_t* settings;
	char*				ptr;
#ifdef DEBUG
	int starttime;

	starttime = Sys_MilliSeconds();
#endif

	// free reversed links that have already been created
	if( ( *aasworld ).reversedreachability ) {
		AAS_RoutingFreeMemory( ( *aasworld ).reversedreachability );
	}

	// allocate memory for the reversed reachability links
	ptr = ( char* )AAS_RoutingGetMemory( ( *aasworld ).numareas * sizeof( aas_reversedreachability_t ) + ( *aasworld ).reachabilitysize * sizeof( aas_reversedlink_t ) );
	//
	( *aasworld ).reversedreachability = ( aas_reversedreachability_t* )ptr;
	// pointer to the memory for the reversed links
	ptr += ( *aasworld ).numareas * sizeof( aas_reversedreachability_t );

	// check all other areas for reachability links to the area
	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		// settings of the area
		settings = &( *aasworld ).areasettings[i];

		// check the reachability links
		for( n = 0; n < settings->numreachableareas; n++ ) {
			// reachability link
			reach = &( *aasworld ).reachability[settings->firstreachablearea + n];
			//
			revlink = ( aas_reversedlink_t* )ptr;
			ptr += sizeof( aas_reversedlink_t );
			//
			revlink->areanum										 = i;
			revlink->linknum										 = settings->firstreachablearea + n;
			revlink->next											 = ( *aasworld ).reversedreachability[reach->areanum].first;
			( *aasworld ).reversedreachability[reach->areanum].first = revlink;
			( *aasworld ).reversedreachability[reach->areanum].numlinks++;
		}
	}

#ifdef DEBUG
	botimport.Print( PRT_MESSAGE, "reversed reachability %d msec\n", Sys_MilliSeconds() - starttime );
#endif // DEBUG
}

/*!
	\brief Returns a scaling factor that adjusts movement distance based on the ground steepness of an area.

	The function looks up the area settings in the global world structure and multiplies the stored groundsteepness value by (GROUNDSTEEPNESS_TIMESCALE-1), then adds one. This factor is later used to
   modify travel distances or times for that area.

	\param areanum Area index whose ground steepness scaling is requested.
	\return A floating‑point scaling factor applied to distances traveled within the area.
*/
float AAS_AreaGroundSteepnessScale( int areanum )
{
	return ( 1.0 + ( *aasworld ).areasettings[areanum].groundsteepness * ( float )( GROUNDSTEEPNESS_TIMESCALE - 1 ) );
}

unsigned short int AAS_AreaTravelTime( int areanum, vec3_t start, vec3_t end )
{
	int	   intdist;
	float  dist;
	vec3_t dir;

	VectorSubtract( start, end, dir );
	dist = VectorLength( dir );
	// Ridah, factor in the groundsteepness now
	dist *= AAS_AreaGroundSteepnessScale( areanum );

	// if crouch only area
	if( AAS_AreaCrouch( areanum ) ) {
		dist *= DISTANCEFACTOR_CROUCH;
	}

	// if swim area
	else if( AAS_AreaSwim( areanum ) ) {
		dist *= DISTANCEFACTOR_SWIM;
	}

	// normal walk area
	else {
		dist *= DISTANCEFACTOR_WALK;
	}

	//
	intdist = ( int )ceil( dist );

	// make sure the distance isn't zero
	if( intdist <= 0 ) {
		intdist = 1;
	}

	return intdist;
}

/*!
	\brief Rebuilds the travel time matrix for all area reachabilities within the AAS world, allocating the required memory and storing the computed times.

	The function first releases any previously allocated travel time data. It then calculates the total amount of memory needed to hold a per‑area, per‑reachable‑area, per‑link travel time array.
   After allocating a contiguous block, it iterates over each area, sets up the pointer structure, and calculates the travel time between the area and each of its reachable areas using the distance
   from the area’s start to the link’s endpoint. These values are stored in a tri‑dimensional array that the AAS system uses for routing. A debug message reports the elapsed time for the operation.

*/
void AAS_CalculateAreaTravelTimes()
{
	int							i, l, n, size;
	char*						ptr;
	vec3_t						end;
	aas_reversedreachability_t* revreach;
	aas_reversedlink_t*			revlink;
	aas_reachability_t*			reach;
	aas_areasettings_t*			settings;
	int							starttime;

	starttime = Sys_MilliSeconds();

	// if there are still area travel times, free the memory
	if( ( *aasworld ).areatraveltimes ) {
		AAS_RoutingFreeMemory( ( *aasworld ).areatraveltimes );
	}

	// get the total size of all the area travel times
	size = ( *aasworld ).numareas * sizeof( unsigned short** );

	for( i = 0; i < ( *aasworld ).numareas; i++ ) {
		revreach = &( *aasworld ).reversedreachability[i];
		// settings of the area
		settings = &( *aasworld ).areasettings[i];
		//
		size += settings->numreachableareas * sizeof( unsigned short* );
		//
		size += settings->numreachableareas * revreach->numlinks * sizeof( unsigned short );
	}

	// allocate memory for the area travel times
	ptr							  = ( char* )AAS_RoutingGetMemory( size );
	( *aasworld ).areatraveltimes = ( unsigned short*** )ptr;
	ptr += ( *aasworld ).numareas * sizeof( unsigned short** );

	// calcluate the travel times for all the areas
	for( i = 0; i < ( *aasworld ).numareas; i++ ) {
		// reversed reachabilities of this area
		revreach = &( *aasworld ).reversedreachability[i];
		// settings of the area
		settings = &( *aasworld ).areasettings[i];
		//
		( *aasworld ).areatraveltimes[i] = ( unsigned short** )ptr;
		ptr += settings->numreachableareas * sizeof( unsigned short* );
		//
		reach = &( *aasworld ).reachability[settings->firstreachablearea];

		for( l = 0; l < settings->numreachableareas; l++, reach++ ) {
			( *aasworld ).areatraveltimes[i][l] = ( unsigned short* )ptr;
			ptr += revreach->numlinks * sizeof( unsigned short );

			// reachability link
			//
			for( n = 0, revlink = revreach->first; revlink; revlink = revlink->next, n++ ) {
				VectorCopy( ( *aasworld ).reachability[revlink->linknum].end, end );
				//
				( *aasworld ).areatraveltimes[i][l][n] = AAS_AreaTravelTime( i, end, reach->start );
			}
		}
	}

#ifdef DEBUG
	botimport.Print( PRT_MESSAGE, "area travel times %d msec\n", Sys_MilliSeconds() - starttime );
#endif // DEBUG
}

/*!
	\brief Returns the maximum travel time from a portal area to any reachable area via reversed reachabilities.

	The function obtains the portal’s area number, then iterates through all areas reachable from it. For each reachable area, it walks through all reversed link entries associated with that area. It
   looks up the travel time stored in the areatraveltimes array for the portal area and the reachable area, keeping track of the largest value encountered. The maximum travel time is then returned.

	\param portalnum index of the portal for which to calculate the maximum travel time.
	\return The maximum travel time value as an integer.
*/
int AAS_PortalMaxTravelTime( int portalnum )
{
	int							l, n, t, maxt;
	aas_portal_t*				portal;
	aas_reversedreachability_t* revreach;
	aas_reversedlink_t*			revlink;
	aas_areasettings_t*			settings;

	portal = &( *aasworld ).portals[portalnum];
	// reversed reachabilities of this portal area
	revreach = &( *aasworld ).reversedreachability[portal->areanum];
	// settings of the portal area
	settings = &( *aasworld ).areasettings[portal->areanum];
	//
	maxt = 0;

	for( l = 0; l < settings->numreachableareas; l++ ) {
		for( n = 0, revlink = revreach->first; revlink; revlink = revlink->next, n++ ) {
			t = ( *aasworld ).areatraveltimes[portal->areanum][l][n];

			if( t > maxt ) {
				maxt = t;
			}
		}
	}

	return maxt;
}

/*!
	\brief Reinitializes the portal maximum travel times table for the navigation system

	The function first frees any existing memory allocated for portal maximum travel times. It then allocates a new array sized by the current number of portals and stores the pointer in the global
   aasworld structure. For every portal, it calculates the maximum travel time using the helper AAS_PortalMaxTravelTime and populates the array accordingly. The resulting table is accessible via
   aasworld->portalmaxtraveltimes.

*/
void AAS_InitPortalMaxTravelTimes()
{
	int i;

	if( ( *aasworld ).portalmaxtraveltimes ) {
		AAS_RoutingFreeMemory( ( *aasworld ).portalmaxtraveltimes );
	}

	( *aasworld ).portalmaxtraveltimes = ( int* )AAS_RoutingGetMemory( ( *aasworld ).numportals * sizeof( int ) );

	for( i = 0; i < ( *aasworld ).numportals; i++ ) {
		( *aasworld ).portalmaxtraveltimes[i] = AAS_PortalMaxTravelTime( i );
		// botimport.Print(PRT_MESSAGE, "portal %d max tt = %d\n", i, (*aasworld).portalmaxtraveltimes[i]);
	}
}

/*!
	\brief Frees the oldest routing cache entry and returns whether a cache was freed

	It searches the cluster area caches first, looking for the oldest cache that does not lead to a portal. If found, it removes that cache from its linked list and frees its memory. If no suitable
   cluster cache is found, it looks through the portal caches for the oldest entry, removes it, and frees it. Finally it returns a flag indicating if a cache was freed.

	\return An integer value: 1 if a cache was freed, 0 otherwise
*/
int AAS_FreeOldestCache()
{
	int					i, j, bestcluster, bestarea, freed;
	float				besttime;
	aas_routingcache_t *cache, *bestcache;

	freed		= qfalse;
	besttime	= 999999999;
	bestcache	= NULL;
	bestcluster = 0;
	bestarea	= 0;

	// refresh cluster cache
	for( i = 0; i < ( *aasworld ).numclusters; i++ ) {
		for( j = 0; j < ( *aasworld ).clusters[i].numareas; j++ ) {
			for( cache = ( *aasworld ).clusterareacache[i][j]; cache; cache = cache->next ) {
				// never remove cache leading towards a portal
				if( ( *aasworld ).areasettings[cache->areanum].cluster < 0 ) {
					continue;
				}

				// if this cache is older than the cache we found so far
				if( cache->time < besttime ) {
					bestcache	= cache;
					bestcluster = i;
					bestarea	= j;
					besttime	= cache->time;
				}
			}
		}
	}

	if( bestcache ) {
		cache = bestcache;

		if( cache->prev ) {
			cache->prev->next = cache->next;

		} else {
			( *aasworld ).clusterareacache[bestcluster][bestarea] = cache->next;
		}

		if( cache->next ) {
			cache->next->prev = cache->prev;
		}

		AAS_FreeRoutingCache( cache );
		freed = qtrue;
	}

	besttime  = 999999999;
	bestcache = NULL;
	bestarea  = 0;

	for( i = 0; i < ( *aasworld ).numareas; i++ ) {
		// refresh portal cache
		for( cache = ( *aasworld ).portalcache[i]; cache; cache = cache->next ) {
			if( cache->time < besttime ) {
				bestcache = cache;
				bestarea  = i;
				besttime  = cache->time;
			}
		}
	}

	if( bestcache ) {
		cache = bestcache;

		if( cache->prev ) {
			cache->prev->next = cache->next;

		} else {
			( *aasworld ).portalcache[bestarea] = cache->next;
		}

		if( cache->next ) {
			cache->next->prev = cache->prev;
		}

		AAS_FreeRoutingCache( cache );
		freed = qtrue;
	}

	return freed;
}

/*!
	\brief Allocates a routing cache structure used to store travel times and reachability information.

	The function calculates the total amount of memory needed for a routing cache by adding the size of the aas_routingcache_t header to two arrays: one of unsigned short ints for travel times and one
   of unsigned chars for reachabilities. It increments the global counter that tracks the cumulative size of all routing caches. Then it obtains a block of the required size from AAS_RoutingGetMemory.
   The reachabilities pointer inside the header is set to immediately follow the travel time array, and the size field of the structure is written. The fully constructed cache is returned as a
   pointer. If the allocation routine fails, the returned pointer will be NULL.

	\param numtraveltimes Number of travel time buckets to store in the cache
	\return Pointer to the allocated aas_routingcache_t or NULL if the allocation fails.
*/
aas_routingcache_t* AAS_AllocRoutingCache( int numtraveltimes )
{
	aas_routingcache_t* cache;
	int					size;

	//
	size = sizeof( aas_routingcache_t ) + numtraveltimes * sizeof( unsigned short int ) + numtraveltimes * sizeof( unsigned char );
	//
	routingcachesize += size;
	//
	cache				  = ( aas_routingcache_t* )AAS_RoutingGetMemory( size );
	cache->reachabilities = ( unsigned char* )cache + sizeof( aas_routingcache_t ) + numtraveltimes * sizeof( unsigned short int );
	cache->size			  = size;
	return cache;
}

/*!
	\brief Releases all routing caches for every cluster area in the world data structure.

	The function checks whether a cluster area cache exists. If it does, it iterates over every cluster and each of its areas, freeing each routing cache via AAS_FreeRoutingCache. After all caches
   have been freed, the 2‑D array of cache pointers itself is deallocated with AAS_RoutingFreeMemory and the global pointer is cleared. The function performs no further error handling and guarantees
   that no cached routing data remains in memory.

*/
void AAS_FreeAllClusterAreaCache()
{
	int					i, j;
	aas_routingcache_t *cache, *nextcache;
	aas_cluster_t*		cluster;

	// free all cluster cache if existing
	if( !( *aasworld ).clusterareacache ) {
		return;
	}

	// free caches
	for( i = 0; i < ( *aasworld ).numclusters; i++ ) {
		cluster = &( *aasworld ).clusters[i];

		for( j = 0; j < cluster->numareas; j++ ) {
			for( cache = ( *aasworld ).clusterareacache[i][j]; cache; cache = nextcache ) {
				nextcache = cache->next;
				AAS_FreeRoutingCache( cache );
			}

			( *aasworld ).clusterareacache[i][j] = NULL;
		}
	}

	// free the cluster cache array
	AAS_RoutingFreeMemory( ( *aasworld ).clusterareacache );
	( *aasworld ).clusterareacache = NULL;
}

/*!
	\brief Initializes the cluster area cache for the AAS world structure.

	First the function counts how many area entries exist across all clusters in the world. It then allocates a single block of memory that holds two layers of pointers: an array of cluster pointers
   and, for each cluster, an array of area pointers. The allocations are done in one call to AAS_RoutingGetMemory, and afterwards the clusterareacache field of the world structure is set to point to
   this nested pointer table. After this function runs, clusterareacache[cluster][area] can be used for routing cache lookups.

*/
void AAS_InitClusterAreaCache()
{
	int	  i, size;
	char* ptr;

	//
	for( size = 0, i = 0; i < ( *aasworld ).numclusters; i++ ) {
		size += ( *aasworld ).clusters[i].numareas;
	}

	// two dimensional array with pointers for every cluster to routing cache
	// for every area in that cluster
	ptr							   = ( char* )AAS_RoutingGetMemory( ( *aasworld ).numclusters * sizeof( aas_routingcache_t** ) + size * sizeof( aas_routingcache_t* ) );
	( *aasworld ).clusterareacache = ( aas_routingcache_t*** )ptr;
	ptr += ( *aasworld ).numclusters * sizeof( aas_routingcache_t** );

	for( i = 0; i < ( *aasworld ).numclusters; i++ ) {
		( *aasworld ).clusterareacache[i] = ( aas_routingcache_t** )ptr;
		ptr += ( *aasworld ).clusters[i].numareas * sizeof( aas_routingcache_t* );
	}
}

/*!
	\brief Frees all portal routing caches that belong to the AAS world.

	The function first verifies that a portal cache exists. If present, it iterates over every area of the world, freeing each routing cache linked to that area. After all caches are released, the
   memory that held the array of pointers is freed and the global portal cache pointer is set to null.

*/
void AAS_FreeAllPortalCache()
{
	int					i;
	aas_routingcache_t *cache, *nextcache;

	// free all portal cache if existing
	if( !( *aasworld ).portalcache ) {
		return;
	}

	// free portal caches
	for( i = 0; i < ( *aasworld ).numareas; i++ ) {
		for( cache = ( *aasworld ).portalcache[i]; cache; cache = nextcache ) {
			nextcache = cache->next;
			AAS_FreeRoutingCache( cache );
		}

		( *aasworld ).portalcache[i] = NULL;
	}

	AAS_RoutingFreeMemory( ( *aasworld ).portalcache );
	( *aasworld ).portalcache = NULL;
}

/*!
	\brief Initializes the portal cache for the area system by allocating memory for routing cache pointers per area.

	The function obtains a pointer to the global area system structure and requests allocation from the routing memory manager. It multiplies the number of defined areas by the size of a routing cache
   pointer to compute the required block size, then stores the returned pointer in the portalcache field of the global structure. No per-area initialization occurs here, but the allocated block is
   ready for subsequent population by other functions.

*/
void AAS_InitPortalCache()
{
	//
	( *aasworld ).portalcache = ( aas_routingcache_t** )AAS_RoutingGetMemory( ( *aasworld ).numareas * sizeof( aas_routingcache_t* ) );
}

/*!
	\brief Releases memory allocated for area visibility structures in the global AAS world.

	If the global area visibility array is allocated, each per‑area visibility pointer is freed when present. After the per‑area data, the array itself is freed and the pointer cleared. The compressed
   visibility buffer is also freed if present. No errors are propagated.

*/
void AAS_FreeAreaVisibility()
{
	int i;

	if( ( *aasworld ).areavisibility ) {
		for( i = 0; i < ( *aasworld ).numareas; i++ ) {
			if( ( *aasworld ).areavisibility[i] ) {
				FreeMemory( ( *aasworld ).areavisibility[i] );
			}
		}
	}

	if( ( *aasworld ).areavisibility ) {
		FreeMemory( ( *aasworld ).areavisibility );
	}

	( *aasworld ).areavisibility = NULL;

	if( ( *aasworld ).decompressedvis ) {
		FreeMemory( ( *aasworld ).decompressedvis );
	}

	( *aasworld ).decompressedvis = NULL;
}

/*!
	\brief Initializes routing update structures by freeing existing memory and allocating fresh update arrays for areas and portals.

	This function operates on the global AAS world structure. It first checks for and frees any previously allocated routing update data for areas and portals. Then it allocates new buffers sized to
   the current number of areas and the number of portals plus one, using the custom memory allocation helper. The function has no return value and does not throw exceptions.

*/
void AAS_InitRoutingUpdate()
{
	//	int i, maxreachabilityareas;

	// free routing update fields if already existing
	if( ( *aasworld ).areaupdate ) {
		AAS_RoutingFreeMemory( ( *aasworld ).areaupdate );
	}

	//
	// Ridah, had to change it to numareas for hidepos checking
	/*
		maxreachabilityareas = 0;
		for (i = 0; i < (*aasworld).numclusters; i++)
		{
			if ((*aasworld).clusters[i].numreachabilityareas > maxreachabilityareas)
			{
				maxreachabilityareas = (*aasworld).clusters[i].numreachabilityareas;
			}
		}
		//allocate memory for the routing update fields
		(*aasworld).areaupdate = (aas_routingupdate_t *) AAS_RoutingGetMemory(
										maxreachabilityareas * sizeof(aas_routingupdate_t));
	*/
	( *aasworld ).areaupdate = ( aas_routingupdate_t* )AAS_RoutingGetMemory( ( *aasworld ).numareas * sizeof( aas_routingupdate_t ) );

	//
	if( ( *aasworld ).portalupdate ) {
		AAS_RoutingFreeMemory( ( *aasworld ).portalupdate );
	}

	// allocate memory for the portal update fields
	( *aasworld ).portalupdate = ( aas_routingupdate_t* )AAS_RoutingGetMemory( ( ( *aasworld ).numportals + 1 ) * sizeof( aas_routingupdate_t ) );
}

/*!
	\brief Creates routing caches for all navigable areas by marking eligible areas and precomputing travel times between them.

	The function iterates over each AAS world area, first checking whether the area has reachability information. It then examines the area’s settings to find a reachable area that satisfies the
   allowed travel flags. The travel flags are derived by taking the default flags and excluding various jumpable or hazardous terrain types, a change made to account for slime no longer being deadly.
   If such a reachable area is found, the area’s use‑for‑routing flag is set and a counter of routing areas is increased.

	After marking all eligible areas, the function loops again over every pair of different areas that are flagged for routing. For each pair, it calls AAS_AreaTravelTimeToGoalArea to compute how long
   it takes to travel between the two via the allowed travel types. The frame routing update counter is reset before each computation. This establishes a full routing cache that can be used by the AI
   for pathfinding.

	No value is returned; all effects are made through global state modifications and a log message emitted by botimport.Print.

*/
void AAS_CreateAllRoutingCache()
{
	int					i, j, k, t, tfl, numroutingareas;
	aas_areasettings_t* areasettings;
	aas_reachability_t* reach;

	numroutingareas = 0;
	tfl =
		TFL_DEFAULT & ~( TFL_JUMPPAD | TFL_ROCKETJUMP | TFL_BFGJUMP | TFL_GRAPPLEHOOK | TFL_DOUBLEJUMP | TFL_RAMPJUMP | TFL_STRAFEJUMP | TFL_LAVA ); //----(SA)	modified since slime is no longer deadly
	//	tfl = TFL_DEFAULT & ~(TFL_JUMPPAD|TFL_ROCKETJUMP|TFL_BFGJUMP|TFL_GRAPPLEHOOK|TFL_DOUBLEJUMP|TFL_RAMPJUMP|TFL_STRAFEJUMP|TFL_SLIME|TFL_LAVA);
	botimport.Print( PRT_MESSAGE, "AAS_CreateAllRoutingCache\n" );

	//
	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		if( !AAS_AreaReachability( i ) ) {
			continue;
		}

		areasettings = &( *aasworld ).areasettings[i];

		for( k = 0; k < areasettings->numreachableareas; k++ ) {
			reach = &( *aasworld ).reachability[areasettings->firstreachablearea + k];

			if( ( *aasworld ).travelflagfortype[reach->traveltype] & tfl ) {
				break;
			}
		}

		if( k >= areasettings->numreachableareas ) {
			continue;
		}

		( *aasworld ).areasettings[i].areaflags |= AREA_USEFORROUTING;
		numroutingareas++;
	}

	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		if( !( ( *aasworld ).areasettings[i].areaflags & AREA_USEFORROUTING ) ) {
			continue;
		}

		for( j = 1; j < ( *aasworld ).numareas; j++ ) {
			if( i == j ) {
				continue;
			}

			if( !( ( *aasworld ).areasettings[j].areaflags & AREA_USEFORROUTING ) ) {
				continue;
			}

			t								  = AAS_AreaTravelTimeToGoalArea( j, ( *aasworld ).areawaypoints[j], i, tfl );
			( *aasworld ).frameroutingupdates = 0;
			// if (t) break;
			// Log_Write("traveltime from %d to %d is %d", i, j, t);
		}
	}
}

unsigned short CRC_ProcessString( unsigned char* data, int length );

// the route cache header
// this header is followed by numportalcache + numareacache aas_routingcache_t
// structures that store routing cache
typedef struct routecacheheader_s {
	int ident;
	int version;
	int numareas;
	int numclusters;
	int areacrc;
	int clustercrc;
	int reachcrc;
	int numportalcache;
	int numareacache;
} routecacheheader_t;

#define RCID	  ( ( 'C' << 24 ) + ( 'R' << 16 ) + ( 'E' << 8 ) + 'M' )
#define RCVERSION 15

void AAS_DecompressVis( byte* in, int numareas, byte* decompressed );
int	 AAS_CompressVis( byte* vis, int numareas, byte* dest );

/*!
	\brief Creates and saves the current world’s routing cache to a file

	The function constructs a header containing identifiers, version, area and cluster counts, and CRC checksums of the world data. It then writes, in order, all portal cache entries, cluster area
   cache entries, visibility bitsets for each area (compressed), and the waypoint vectors for every area. The output file is named "maps/<mapname>.rcd" based on the world’s map name. After
   successfully writing, it outputs a confirmation message. The function interacts with global data such as the world structure, portal cache, cluster area cache, and visibility arrays.

*/
void AAS_WriteRouteCache()
{
	int					i, j, numportalcache, numareacache, size;
	aas_routingcache_t* cache;
	aas_cluster_t*		cluster;
	fileHandle_t		fp;
	char				filename[MAX_QPATH];
	routecacheheader_t	routecacheheader;
	byte*				buf;

	buf = ( byte* )GetClearedMemory( ( *aasworld ).numareas * 2 * sizeof( byte ) ); // in case it ends up bigger than the decompressedvis, which is rare but possible

	numportalcache = 0;

	for( i = 0; i < ( *aasworld ).numareas; i++ ) {
		for( cache = ( *aasworld ).portalcache[i]; cache; cache = cache->next ) {
			numportalcache++;
		}
	}

	numareacache = 0;

	for( i = 0; i < ( *aasworld ).numclusters; i++ ) {
		cluster = &( *aasworld ).clusters[i];

		for( j = 0; j < cluster->numareas; j++ ) {
			for( cache = ( *aasworld ).clusterareacache[i][j]; cache; cache = cache->next ) {
				numareacache++;
			}
		}
	}

	// open the file for writing
	Com_sprintf( filename, MAX_QPATH, "maps/%s.rcd", ( *aasworld ).mapname );
	botimport.FS_FOpenFile( filename, &fp, FS_WRITE );

	if( !fp ) {
		AAS_Error( "Unable to open file: %s\n", filename );
		return;
	}

	// create the header
	routecacheheader.ident			= RCID;
	routecacheheader.version		= RCVERSION;
	routecacheheader.numareas		= ( *aasworld ).numareas;
	routecacheheader.numclusters	= ( *aasworld ).numclusters;
	routecacheheader.areacrc		= CRC_ProcessString( ( unsigned char* )( *aasworld ).areas, sizeof( aas_area_t ) * ( *aasworld ).numareas );
	routecacheheader.clustercrc		= CRC_ProcessString( ( unsigned char* )( *aasworld ).clusters, sizeof( aas_cluster_t ) * ( *aasworld ).numclusters );
	routecacheheader.reachcrc		= CRC_ProcessString( ( unsigned char* )( *aasworld ).reachability, sizeof( aas_reachability_t ) * ( *aasworld ).reachabilitysize );
	routecacheheader.numportalcache = numportalcache;
	routecacheheader.numareacache	= numareacache;
	// write the header
	botimport.FS_Write( &routecacheheader, sizeof( routecacheheader_t ), fp );

	// write all the cache
	for( i = 0; i < ( *aasworld ).numareas; i++ ) {
		for( cache = ( *aasworld ).portalcache[i]; cache; cache = cache->next ) {
			botimport.FS_Write( cache, cache->size, fp );
		}
	}

	for( i = 0; i < ( *aasworld ).numclusters; i++ ) {
		cluster = &( *aasworld ).clusters[i];

		for( j = 0; j < cluster->numareas; j++ ) {
			for( cache = ( *aasworld ).clusterareacache[i][j]; cache; cache = cache->next ) {
				botimport.FS_Write( cache, cache->size, fp );
			}
		}
	}

	// write the visareas
	for( i = 0; i < ( *aasworld ).numareas; i++ ) {
		if( !( *aasworld ).areavisibility[i] ) {
			size = 0;
			botimport.FS_Write( &size, sizeof( int ), fp );
			continue;
		}

		AAS_DecompressVis( ( *aasworld ).areavisibility[i], ( *aasworld ).numareas, ( *aasworld ).decompressedvis );
		size = AAS_CompressVis( ( *aasworld ).decompressedvis, ( *aasworld ).numareas, buf );
		botimport.FS_Write( &size, sizeof( int ), fp );
		botimport.FS_Write( buf, size, fp );
	}

	// write the waypoints
	botimport.FS_Write( ( *aasworld ).areawaypoints, sizeof( vec3_t ) * ( *aasworld ).numareas, fp );
	//
	botimport.FS_FCloseFile( fp );
	botimport.Print( PRT_MESSAGE, "\nroute cache written to %s\n", filename );
}

/*!
	\brief Reads a routing cache from a file and converts it to a native format.

	The function begins by reading the cache size from the file and allocating a buffer to hold the serialized data. It reads the remaining bytes of the cache and calculates the number of travel time
   entries. On 32‑bit systems the raw data can be used directly; on 64‑bit or big‑endian systems the data is byte‑swapped into a newly allocated native structure. After copying the travel times and
   reachabilities into the native structure, any temporary buffer is freed before returning the fully populated cache object.

	\param fp file handle from which the cache data is read.
	\return Pointer to an allocated aas_routingcache_t containing the decoded cache data; the caller is responsible for freeing the memory.
*/
aas_routingcache_t* AAS_ReadCache( fileHandle_t fp )
{
	int						 i, size, numtraveltimes;
	aas_routingcache_t*		 nativecache;
	aas_routingcache_disc_t* cache;
	unsigned char*			 cache_reachabilities;

	botimport.FS_Read( &size, sizeof( size ), fp );
	cache		= ( aas_routingcache_disc_t* )AAS_RoutingGetMemory( size );
	cache->size = size;
	botimport.FS_Read( ( unsigned char* )cache + sizeof( size ), size - sizeof( size ), fp );

	numtraveltimes = ( size - sizeof( aas_routingcache_disc_t ) ) / 3;

	if( sizeof( intptr_t ) == 4 ) {
		nativecache = ( aas_routingcache_t* )cache;

	} else {
		int nativesize	  = size - sizeof( aas_routingcache_disc_t ) + sizeof( aas_routingcache_t );
		nativecache		  = ( aas_routingcache_t* )AAS_RoutingGetMemory( nativesize );
		nativecache->size = nativesize;
	}

	// copy to native structure and/or swap
	if( sizeof( intptr_t ) != 4 || 1 != LittleLong( 1 ) ) {
		nativecache->time			 = LittleFloat( cache->time );
		nativecache->cluster		 = LittleLong( cache->cluster );
		nativecache->areanum		 = LittleLong( cache->areanum );
		nativecache->origin[0]		 = LittleFloat( cache->origin[0] );
		nativecache->origin[1]		 = LittleFloat( cache->origin[1] );
		nativecache->origin[2]		 = LittleFloat( cache->origin[2] );
		nativecache->starttraveltime = LittleFloat( cache->starttraveltime );
		nativecache->travelflags	 = LittleLong( cache->travelflags );

		// DAJ BUGFIX for missing byteswaps for traveltimes
		for( i = 0; i < numtraveltimes; i++ ) {
			nativecache->traveltimes[i] = LittleShort( cache->traveltimes[i] );
		}
	}

	nativecache->reachabilities = ( unsigned char* )nativecache + sizeof( aas_routingcache_t ) + numtraveltimes * sizeof( nativecache->traveltimes[0] );

	// copy reachabilities to native structure, free original cache
	if( sizeof( intptr_t ) != 4 ) {
		cache_reachabilities = ( unsigned char* )cache + sizeof( aas_routingcache_disc_t ) + numtraveltimes * sizeof( cache->traveltimes[0] );

		for( i = 0; i < numtraveltimes; i++ ) {
			nativecache->reachabilities[i] = cache_reachabilities[i];
		}

		AAS_RoutingFreeMemory( cache );
	}

	return nativecache;
}

/*!
	\brief Loads and validates a route cache file for the current map, reconstructing routing data structures.

	The function attempts to open a file named "maps/<mapname>.rcd" and reads its header. It verifies the header’s identifier, version, and that area/cluster counts match the loaded AAS world. On
   Windows, CRC checks are performed for areas, clusters, and reachability data. If validation fails, the file is closed and the function returns false. On success, it reads and rebuilds portal
   caches, cluster area caches, visarea data, and area waypoints, allocating memory as necessary and storing the information in the global AAS world structure. The function finally closes the file and
   returns true to indicate a successful load.

	\return qtrue on success, qfalse on failure
*/
int AAS_ReadRouteCache()
{
	int					i, clusterareanum, size;
	fileHandle_t		fp = 0;
	char				filename[MAX_QPATH];
	routecacheheader_t	routecacheheader;
	aas_routingcache_t* cache;

	Com_sprintf( filename, MAX_QPATH, "maps/%s.rcd", ( *aasworld ).mapname );
	botimport.FS_FOpenFile( filename, &fp, FS_READ );

	if( !fp ) {
		return qfalse;
	}

	botimport.FS_Read( &routecacheheader, sizeof( routecacheheader_t ), fp );

	// GJD: route cache data MUST be written on a PC because I've not altered the writing code.

	routecacheheader.areacrc		= LittleLong( routecacheheader.areacrc );
	routecacheheader.clustercrc		= LittleLong( routecacheheader.clustercrc );
	routecacheheader.ident			= LittleLong( routecacheheader.ident );
	routecacheheader.numareacache	= LittleLong( routecacheheader.numareacache );
	routecacheheader.numareas		= LittleLong( routecacheheader.numareas );
	routecacheheader.numclusters	= LittleLong( routecacheheader.numclusters );
	routecacheheader.numportalcache = LittleLong( routecacheheader.numportalcache );
	routecacheheader.reachcrc		= LittleLong( routecacheheader.reachcrc );
	routecacheheader.version		= LittleLong( routecacheheader.version );

	if( routecacheheader.ident != RCID ) {
		botimport.FS_FCloseFile( fp );
		Com_Printf( "%s is not a route cache dump\n", filename ); // not an aas_error because we want to continue
		return qfalse;											  // and remake them by returning false here
	}

	if( routecacheheader.version != RCVERSION ) {
		botimport.FS_FCloseFile( fp );
		Com_Printf( "route cache dump has wrong version %d, should be %d", routecacheheader.version, RCVERSION );
		return qfalse;
	}

	if( routecacheheader.numareas != ( *aasworld ).numareas ) {
		botimport.FS_FCloseFile( fp );
		// AAS_Error("route cache dump has wrong number of areas\n");
		return qfalse;
	}

	if( routecacheheader.numclusters != ( *aasworld ).numclusters ) {
		botimport.FS_FCloseFile( fp );
		// AAS_Error("route cache dump has wrong number of clusters\n");
		return qfalse;
	}

#ifdef _WIN32 // crc code is only good on intel machines

	if( routecacheheader.areacrc != CRC_ProcessString( ( unsigned char* )( *aasworld ).areas, sizeof( aas_area_t ) * ( *aasworld ).numareas ) ) {
		botimport.FS_FCloseFile( fp );
		// AAS_Error("route cache dump area CRC incorrect\n");
		return qfalse;
	}

	if( routecacheheader.clustercrc != CRC_ProcessString( ( unsigned char* )( *aasworld ).clusters, sizeof( aas_cluster_t ) * ( *aasworld ).numclusters ) ) {
		botimport.FS_FCloseFile( fp );
		// AAS_Error("route cache dump cluster CRC incorrect\n");
		return qfalse;
	}

	if( routecacheheader.reachcrc != CRC_ProcessString( ( unsigned char* )( *aasworld ).reachability, sizeof( aas_reachability_t ) * ( *aasworld ).reachabilitysize ) ) {
		botimport.FS_FCloseFile( fp );
		// AAS_Error("route cache dump reachability CRC incorrect\n");
		return qfalse;
	}

#endif

	// read all the portal cache
	for( i = 0; i < routecacheheader.numportalcache; i++ ) {
		cache		= AAS_ReadCache( fp );
		cache->next = ( *aasworld ).portalcache[cache->areanum];
		cache->prev = NULL;

		if( ( *aasworld ).portalcache[cache->areanum] ) {
			( *aasworld ).portalcache[cache->areanum]->prev = cache;
		}

		( *aasworld ).portalcache[cache->areanum] = cache;
	}

	// read all the cluster area cache
	for( i = 0; i < routecacheheader.numareacache; i++ ) {
		cache		   = AAS_ReadCache( fp );
		clusterareanum = AAS_ClusterAreaNum( cache->cluster, cache->areanum );
		cache->next	   = ( *aasworld ).clusterareacache[cache->cluster][clusterareanum];
		cache->prev	   = NULL;

		if( ( *aasworld ).clusterareacache[cache->cluster][clusterareanum] ) {
			( *aasworld ).clusterareacache[cache->cluster][clusterareanum]->prev = cache;
		}

		( *aasworld ).clusterareacache[cache->cluster][clusterareanum] = cache;
	}

	// read the visareas
	( *aasworld ).areavisibility  = ( byte** )GetClearedMemory( ( *aasworld ).numareas * sizeof( byte* ) );
	( *aasworld ).decompressedvis = ( byte* )GetClearedMemory( ( *aasworld ).numareas * sizeof( byte ) );

	for( i = 0; i < ( *aasworld ).numareas; i++ ) {
		botimport.FS_Read( &size, sizeof( size ), fp );
		size = LittleLong( size );

		if( size ) {
			( *aasworld ).areavisibility[i] = ( byte* )GetMemory( size );
			botimport.FS_Read( ( *aasworld ).areavisibility[i], size, fp );
		}
	}

	// read the area waypoints
	( *aasworld ).areawaypoints = ( vec3_t* )GetClearedMemory( ( *aasworld ).numareas * sizeof( vec3_t ) );
	botimport.FS_Read( ( *aasworld ).areawaypoints, ( *aasworld ).numareas * sizeof( vec3_t ), fp );

	if( 1 != LittleLong( 1 ) ) {
		for( i = 0; i < ( *aasworld ).numareas; i++ ) {
			( *aasworld ).areawaypoints[i][0] = LittleFloat( ( *aasworld ).areawaypoints[i][0] );
			( *aasworld ).areawaypoints[i][1] = LittleFloat( ( *aasworld ).areawaypoints[i][1] );
			( *aasworld ).areawaypoints[i][2] = LittleFloat( ( *aasworld ).areawaypoints[i][2] );
		}
	}

	//
	botimport.FS_FCloseFile( fp );
	return qtrue;
}

void AAS_CreateVisibility();

/*!
	\brief Initializes routing by setting up travel flags, caches, reachability, and precomputing travel times

	AAS_InitRouting performs a comprehensive setup of the routing subsystem. The function initializes travel flags based on travel types, establishes routing update data structures, and creates
   reversed reachability links to support the update algorithm. It also builds cluster and portal caches and calculates area travel times as well as the maximum travel times through portals. If a
   route cache is unavailable, the function generates visibility data, constructs the full routing cache, and writes it to disk for future use. In debug builds, counters for area cache and portal
   cache updates are reset, and routing cache limits are set using configuration variables.

*/
void AAS_InitRouting()
{
	AAS_InitTravelFlagFromType();
	// initialize the routing update fields
	AAS_InitRoutingUpdate();
	// create reversed reachability links used by the routing update algorithm
	AAS_CreateReversedReachability();
	// initialize the cluster cache
	AAS_InitClusterAreaCache();
	// initialize portal cache
	AAS_InitPortalCache();
	// initialize the area travel times
	AAS_CalculateAreaTravelTimes();
	// calculate the maximum travel times through portals
	AAS_InitPortalMaxTravelTimes();
	//
#ifdef ROUTING_DEBUG
	numareacacheupdates	  = 0;
	numportalcacheupdates = 0;
#endif // ROUTING_DEBUG
	//
	routingcachesize	 = 0;
	max_routingcachesize = 1024 * ( int )LibVarValue( "max_routingcache", "4096" );

	//
	// Ridah, load or create the routing cache
	if( !AAS_ReadRouteCache() ) {
		( *aasworld ).initialized = qtrue; // Hack, so routing can compute traveltimes
		AAS_CreateVisibility();
		AAS_CreateAllRoutingCache();
		( *aasworld ).initialized = qfalse;

		AAS_WriteRouteCache(); // save it so we don't have to create it again
	}

	// done.
}

/*!
	\brief Frees all routing caches and related data used by the AI area system.

	This function deallocates cluster area, portal, and area visibility caches, releases memory for cached travel times within areas and maximum travel times through cluster portals, deletes reversed
   reachability links, frees routing algorithm memory for area and portal updates, and removes area waypoint data. After each deallocation it nulls the corresponding global pointer to prevent dangling
   references.

*/
void AAS_FreeRoutingCaches()
{
	// free all the existing cluster area cache
	AAS_FreeAllClusterAreaCache();
	// free all the existing portal cache
	AAS_FreeAllPortalCache();
	// free all the existing area visibility data
	AAS_FreeAreaVisibility();

	// free cached travel times within areas
	if( ( *aasworld ).areatraveltimes ) {
		AAS_RoutingFreeMemory( ( *aasworld ).areatraveltimes );
	}

	( *aasworld ).areatraveltimes = NULL;

	// free cached maximum travel time through cluster portals
	if( ( *aasworld ).portalmaxtraveltimes ) {
		AAS_RoutingFreeMemory( ( *aasworld ).portalmaxtraveltimes );
	}

	( *aasworld ).portalmaxtraveltimes = NULL;

	// free reversed reachability links
	if( ( *aasworld ).reversedreachability ) {
		AAS_RoutingFreeMemory( ( *aasworld ).reversedreachability );
	}

	( *aasworld ).reversedreachability = NULL;

	// free routing algorithm memory
	if( ( *aasworld ).areaupdate ) {
		AAS_RoutingFreeMemory( ( *aasworld ).areaupdate );
	}

	( *aasworld ).areaupdate = NULL;

	if( ( *aasworld ).portalupdate ) {
		AAS_RoutingFreeMemory( ( *aasworld ).portalupdate );
	}

	( *aasworld ).portalupdate = NULL;

	// free area waypoints
	if( ( *aasworld ).areawaypoints ) {
		FreeMemory( ( *aasworld ).areawaypoints );
	}

	( *aasworld ).areawaypoints = NULL;
}

/*!
	\brief Adds a routing update to the end of a doubly-linked list if it is not already present.

	When the supplied update structure is not flagged as being in the list, this function appends it to the tail referenced by the provided start and end pointers. If the list is empty, the new update
   becomes both the first and last node. The update’s previous pointer is set to the current tail, its next pointer to null, and the inlist flag is set to true. If the update is already in the list,
   the function does nothing.

	\param updateliststart Pointer to the variable holding the head of the list
	\param updatelistend Pointer to the variable holding the tail of the list
	\param update Pointer to the update node to be added
*/
__inline void AAS_AddUpdateToList( aas_routingupdate_t** updateliststart, aas_routingupdate_t** updatelistend, aas_routingupdate_t* update )
{
	if( !update->inlist ) {
		if( *updatelistend ) {
			( *updatelistend )->next = update;

		} else {
			*updateliststart = update;
		}

		update->prev   = *updatelistend;
		update->next   = NULL;
		*updatelistend = update;
		update->inlist = qtrue;
	}
}

int AAS_AreaContentsTravelFlag( int areanum )
{
	int contents, tfl;

	contents = ( *aasworld ).areasettings[areanum].contents;
	tfl		 = 0;

	if( contents & AREACONTENTS_WATER ) {
		return tfl |= TFL_WATER;

	} else if( contents & AREACONTENTS_SLIME ) {
		return tfl |= TFL_SLIME;

	} else if( contents & AREACONTENTS_LAVA ) {
		return tfl |= TFL_LAVA;

	} else {
		tfl |= TFL_AIR;
	}

	if( contents & AREACONTENTS_DONOTENTER_LARGE ) {
		tfl |= TFL_DONOTENTER_LARGE;
	}

	if( contents & AREACONTENTS_DONOTENTER ) {
		return tfl |= TFL_DONOTENTER;
	}

	return tfl;
}

/*!
	\brief updates the routing cache for a specific area within its cluster by computing minimal travel times to reachable areas

	The function expands the routing cache starting from the area specified in the cache.  It walks all reversed reachability links for the current area, filtering out undesired travel flags, disabled
   areas, and areas that would leave the cluster.  For each valid link it calculates the total travel time, consisting of the already travelled time, the time to move through the current area, and the
   travel time of the link itself, and updates the cache if the new time is shorter.  The algorithm proceeds breadth‑first, maintaining a list of areas whose travel time has been updated until the
   list is empty.  The cache holds per‑cluster travel times and indexes to the reachability links that provide the best route.


	\param areacache the routing cache entry for the area to be updated; must point to a valid aas_routingcache_t structure
*/
void AAS_UpdateAreaRoutingCache( aas_routingcache_t* areacache )
{
	int							i, nextareanum, cluster, badtravelflags, clusterareanum, linknum;
	int							numreachabilityareas;
	unsigned short int			t, startareatraveltimes[128];
	aas_routingupdate_t *		updateliststart, *updatelistend, *curupdate, *nextupdate;
	aas_reachability_t*			reach;
	aas_reversedreachability_t* revreach;
	aas_reversedlink_t*			revlink;

#ifdef ROUTING_DEBUG
	numareacacheupdates++;
#endif // ROUTING_DEBUG
	// number of reachability areas within this cluster
	numreachabilityareas = ( *aasworld ).clusters[areacache->cluster].numreachabilityareas;
	//
	// clear the routing update fields
	//	memset((*aasworld).areaupdate, 0, (*aasworld).numareas * sizeof(aas_routingupdate_t));
	//
	badtravelflags = ~areacache->travelflags;
	//
	clusterareanum = AAS_ClusterAreaNum( areacache->cluster, areacache->areanum );

	if( clusterareanum >= numreachabilityareas ) {
		return;
	}

	//
	memset( startareatraveltimes, 0, sizeof( startareatraveltimes ) );
	//
	curupdate		   = &( *aasworld ).areaupdate[clusterareanum];
	curupdate->areanum = areacache->areanum;
	// VectorCopy(areacache->origin, curupdate->start);
	curupdate->areatraveltimes = ( *aasworld ).areatraveltimes[areacache->areanum][0];
	curupdate->tmptraveltime   = areacache->starttraveltime;
	//
	areacache->traveltimes[clusterareanum] = areacache->starttraveltime;
	// put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	updateliststart = curupdate;
	updatelistend	= curupdate;

	// while there are updates in the current list, flip the lists
	while( updateliststart ) {
		curupdate = updateliststart;

		//
		if( curupdate->next ) {
			curupdate->next->prev = NULL;

		} else {
			updatelistend = NULL;
		}

		updateliststart = curupdate->next;
		//
		curupdate->inlist = qfalse;
		// check all reversed reachability links
		revreach = &( *aasworld ).reversedreachability[curupdate->areanum];

		//
		for( i = 0, revlink = revreach->first; revlink; revlink = revlink->next, i++ ) {
			linknum = revlink->linknum;
			reach	= &( *aasworld ).reachability[linknum];

			// if there is used an undesired travel type
			if( ( *aasworld ).travelflagfortype[reach->traveltype] & badtravelflags ) {
				continue;
			}

			// if not allowed to enter the next area
			if( ( *aasworld ).areasettings[reach->areanum].areaflags & AREA_DISABLED ) {
				continue;
			}

			// if the next area has a not allowed travel flag
			if( AAS_AreaContentsTravelFlag( reach->areanum ) & badtravelflags ) {
				continue;
			}

			// number of the area the reversed reachability leads to
			nextareanum = revlink->areanum;
			// get the cluster number of the area
			cluster = ( *aasworld ).areasettings[nextareanum].cluster;

			// don't leave the cluster
			if( cluster > 0 && cluster != areacache->cluster ) {
				continue;
			}

			// get the number of the area in the cluster
			clusterareanum = AAS_ClusterAreaNum( areacache->cluster, nextareanum );

			if( clusterareanum >= numreachabilityareas ) {
				continue;
			}

			// time already travelled plus the traveltime through
			// the current area plus the travel time from the reachability
			t = curupdate->tmptraveltime +
				// AAS_AreaTravelTime(curupdate->areanum, curupdate->start, reach->end) +
				curupdate->areatraveltimes[i] + reach->traveltime;
			//
			( *aasworld ).frameroutingupdates++;

			//
			if( !areacache->traveltimes[clusterareanum] || areacache->traveltimes[clusterareanum] > t ) {
				areacache->traveltimes[clusterareanum]	  = t;
				areacache->reachabilities[clusterareanum] = linknum - ( *aasworld ).areasettings[nextareanum].firstreachablearea;
				nextupdate								  = &( *aasworld ).areaupdate[clusterareanum];
				nextupdate->areanum						  = nextareanum;
				nextupdate->tmptraveltime				  = t;
				// VectorCopy(reach->start, nextupdate->start);
				nextupdate->areatraveltimes = ( *aasworld ).areatraveltimes[nextareanum][linknum - ( *aasworld ).areasettings[nextareanum].firstreachablearea];

				if( !nextupdate->inlist ) {
					nextupdate->next = NULL;
					nextupdate->prev = updatelistend;

					if( updatelistend ) {
						updatelistend->next = nextupdate;

					} else {
						updateliststart = nextupdate;
					}

					updatelistend	   = nextupdate;
					nextupdate->inlist = qtrue;
				}
			}
		}
	}
}

/*!
	\brief Retrieves or creates a routing cache entry for a specified area within a cluster, respecting travel flag constraints and per-frame update limits.

	The function first calculates the local area number inside the cluster and then looks up the existing cache list for that cluster-area pair. It searches the list for an entry whose travel flags
   match the requested flags; if found, that cache is reused. If no matching cache exists, the function checks whether the per-frame routing update count has exceeded the allowed maximum; if the
   caller has not forced an update and the limit has been hit, the function returns null. Otherwise, it allocates a new routing cache, initializes its fields—including cluster, area number, origin,
   start travel time, and flags—links it into the cluster’s cache list, stores it, and then calls a routine to populate the cache’s data. Finally, the cache’s access time is updated to the current
   routing time and the cache pointer is returned.

	\param clusternum The index of the cluster in which the area resides.
	\param areanum The global area index for which the cache is requested.
	\param travelflags Bitmask indicating the types of movement permitted for this cache lookup.
	\param forceUpdate If true, forces allocation and update of the cache regardless of the per-frame update limit.
	\return Pointer to the routing cache structure for the requested area and cluster; null if allocation is rejected due to update limits or an error occurs.
*/
aas_routingcache_t* AAS_GetAreaRoutingCache( int clusternum, int areanum, int travelflags, qboolean forceUpdate )
{
	int					clusterareanum;
	aas_routingcache_t *cache, *clustercache;

	// number of the area in the cluster
	clusterareanum = AAS_ClusterAreaNum( clusternum, areanum );
	// pointer to the cache for the area in the cluster
	clustercache = ( *aasworld ).clusterareacache[clusternum][clusterareanum];

	// find the cache without undesired travel flags
	for( cache = clustercache; cache; cache = cache->next ) {
		// if there aren't used any undesired travel types for the cache
		if( cache->travelflags == travelflags ) {
			break;
		}
	}

	// if there was no cache
	if( !cache ) {
		// NOTE: the number of routing updates is limited per frame
		if( !forceUpdate && ( ( *aasworld ).frameroutingupdates > MAX_FRAMEROUTINGUPDATES ) ) {
			return NULL;
		}

		cache		   = AAS_AllocRoutingCache( ( *aasworld ).clusters[clusternum].numreachabilityareas );
		cache->cluster = clusternum;
		cache->areanum = areanum;
		VectorCopy( ( *aasworld ).areas[areanum].center, cache->origin );
		cache->starttraveltime = 1;
		cache->travelflags	   = travelflags;
		cache->prev			   = NULL;
		cache->next			   = clustercache;

		if( clustercache ) {
			clustercache->prev = cache;
		}

		( *aasworld ).clusterareacache[clusternum][clusterareanum] = cache;
		AAS_UpdateAreaRoutingCache( cache );
	}

	// the cache has been accessed
	cache->time = AAS_RoutingTime();
	return cache;
}

/*!
	\brief Updates the portal routing cache by computing minimal travel times and reachabilities from a source area through connected portals

	Starting from the source area stored in the supplied portalcache, the function performs a traversal over all portals belonging to the same cluster. For each traversed portal, it calculates the
   total travel time via the portal’s reachability area, updating the cache only if a shorter route is found. It handles cluster‑portal start areas specially, records travel times into the cache’s
   traveltimes array, and stores the associated reachability data in the reachabilities array. The algorithm uses portalupdate structures as a work queue to manage frontier updates, iterating until no
   further improvements remain.

	\param portalcache Pointer to a routing cache structure to be updated; contains the start area, travel time, and arrays for traversal results.
*/
void AAS_UpdatePortalRoutingCache( aas_routingcache_t* portalcache )
{
	int					 i, portalnum, clusterareanum, clusternum;
	unsigned short int	 t;
	aas_portal_t*		 portal;
	aas_cluster_t*		 cluster;
	aas_routingcache_t*	 cache;
	aas_routingupdate_t *updateliststart, *updatelistend, *curupdate, *nextupdate;

#ifdef ROUTING_DEBUG
	numportalcacheupdates++;
#endif // ROUTING_DEBUG
	// clear the routing update fields
	//	memset((*aasworld).portalupdate, 0, ((*aasworld).numportals+1) * sizeof(aas_routingupdate_t));
	//
	curupdate				 = &( *aasworld ).portalupdate[( *aasworld ).numportals];
	curupdate->cluster		 = portalcache->cluster;
	curupdate->areanum		 = portalcache->areanum;
	curupdate->tmptraveltime = portalcache->starttraveltime;
	// if the start area is a cluster portal, store the travel time for that portal
	clusternum = ( *aasworld ).areasettings[portalcache->areanum].cluster;

	if( clusternum < 0 ) {
		portalcache->traveltimes[-clusternum] = portalcache->starttraveltime;
	}

	// put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	updateliststart = curupdate;
	updatelistend	= curupdate;

	// while there are updates in the current list, flip the lists
	while( updateliststart ) {
		curupdate = updateliststart;

		// remove the current update from the list
		if( curupdate->next ) {
			curupdate->next->prev = NULL;

		} else {
			updatelistend = NULL;
		}

		updateliststart = curupdate->next;
		// current update is removed from the list
		curupdate->inlist = qfalse;
		//
		cluster = &( *aasworld ).clusters[curupdate->cluster];
		//
		cache = AAS_GetAreaRoutingCache( curupdate->cluster, curupdate->areanum, portalcache->travelflags, qtrue );

		// take all portals of the cluster
		for( i = 0; i < cluster->numportals; i++ ) {
			portalnum = ( *aasworld ).portalindex[cluster->firstportal + i];
			portal	  = &( *aasworld ).portals[portalnum];

			// if this is the portal of the current update continue
			if( portal->areanum == curupdate->areanum ) {
				continue;
			}

			//
			clusterareanum = AAS_ClusterAreaNum( curupdate->cluster, portal->areanum );

			if( clusterareanum >= cluster->numreachabilityareas ) {
				continue;
			}

			//
			t = cache->traveltimes[clusterareanum];

			if( !t ) {
				continue;
			}

			t += curupdate->tmptraveltime;

			//
			if( !portalcache->traveltimes[portalnum] || portalcache->traveltimes[portalnum] > t ) {
				portalcache->traveltimes[portalnum]	   = t;
				portalcache->reachabilities[portalnum] = cache->reachabilities[clusterareanum];
				nextupdate							   = &( *aasworld ).portalupdate[portalnum];

				if( portal->frontcluster == curupdate->cluster ) {
					nextupdate->cluster = portal->backcluster;

				} else {
					nextupdate->cluster = portal->frontcluster;
				}

				nextupdate->areanum = portal->areanum;
				// add travel time through actual portal area for the next update
				nextupdate->tmptraveltime = t + ( *aasworld ).portalmaxtraveltimes[portalnum];

				if( !nextupdate->inlist ) {
					nextupdate->next = NULL;
					nextupdate->prev = updatelistend;

					if( updatelistend ) {
						updatelistend->next = nextupdate;

					} else {
						updateliststart = nextupdate;
					}

					updatelistend	   = nextupdate;
					nextupdate->inlist = qtrue;
				}
			}
		}
	}
}

/*!
	\brief Retrieves or allocates a portal routing cache for a specific cluster, area, and travel flags.

	The function searches the global portal cache list for the requested area and travel flags. If a matching cache is found, it is reused. If not, a new cache is allocated, initialized with the
   cluster number, area number, the center of the area as its origin, a starting travel time of one, and the specified travel flags. The new cache is inserted at the front of the area's cache list and
   updated by calling AAS_UpdatePortalRoutingCache. Finally, the cache’s timestamp is set to the current routing time and a pointer to the cache is returned.

	\param clusternum Cluster identifier that the cache will be associated with.
	\param areanum Area identifier whose portal cache list is inspected or extended.
	\param travelflags Flags describing the traversal characteristics for which the cache is relevant.
	\return Pointer to the portal routing cache entry matching the given cluster, area, and travel flags; may be an existing entry or a newly created one.
*/
aas_routingcache_t* AAS_GetPortalRoutingCache( int clusternum, int areanum, int travelflags )
{
	aas_routingcache_t* cache;

	// find the cached portal routing if existing
	for( cache = ( *aasworld ).portalcache[areanum]; cache; cache = cache->next ) {
		if( cache->travelflags == travelflags ) {
			break;
		}
	}

	// if the portal routing isn't cached
	if( !cache ) {
		cache		   = AAS_AllocRoutingCache( ( *aasworld ).numportals );
		cache->cluster = clusternum;
		cache->areanum = areanum;
		VectorCopy( ( *aasworld ).areas[areanum].center, cache->origin );
		cache->starttraveltime = 1;
		cache->travelflags	   = travelflags;
		// add the cache to the cache list
		cache->prev = NULL;
		cache->next = ( *aasworld ).portalcache[areanum];

		if( ( *aasworld ).portalcache[areanum] ) {
			( *aasworld ).portalcache[areanum]->prev = cache;
		}

		( *aasworld ).portalcache[areanum] = cache;
		// update the cache
		AAS_UpdatePortalRoutingCache( cache );
	}

	// the cache has been accessed
	cache->time = AAS_RoutingTime();
	return cache;
}

/*!
	\brief Calculates travel time and reachability from a source area to a goal area using AAS routing data.

	This function determines whether a route exists from a given source area to a target area, considering specified travel flags. It stores the computed travel time and the reachability number in the
   provided output parameters. The function handles various edge cases including invalid area numbers, disabled areas, and different cluster configurations. If the source and goal areas are the same,
   it returns immediately with a travel time of 1. Travel time is calculated considering the area's location and the reachability to the goal.

	\param areanum Source area number
	\param goalareanum Target goal area number
	\param origin Origin point within the source area (may be a vector or null for direct calculation)
	\param reachnum Pointer to an integer where the reachability index that leads into the goal area will be stored
	\param travelflags Bitmask controlling travel restrictions (e.g., non-enter, large non-enter)
	\param traveltime Pointer to an integer where the total travel time will be stored if a route is found
	\return 1 if a route exists, 0 otherwise
*/
int AAS_AreaRouteToGoalArea( int areanum, vec3_t origin, int goalareanum, int travelflags, int* traveltime, int* reachnum )
{
	int					clusternum, goalclusternum, portalnum, i, clusterareanum, bestreachnum;
	unsigned short int	t, besttime;
	aas_portal_t*		portal;
	aas_cluster_t*		cluster;
	aas_routingcache_t *areacache, *portalcache;
	aas_reachability_t* reach;
	aas_portalindex_t*	pPortalnum;

	if( !( *aasworld ).initialized ) {
		return qfalse;
	}

	if( areanum == goalareanum ) {
		*traveltime = 1;
		*reachnum	= 0;
		return qtrue;
	}

	//
	if( areanum <= 0 || areanum >= ( *aasworld ).numareas ) {
		if( bot_developer ) {
			botimport.Print( PRT_ERROR, "AAS_AreaTravelTimeToGoalArea: areanum %d out of range\n", areanum );
		}

		return qfalse;
	}

	if( goalareanum <= 0 || goalareanum >= ( *aasworld ).numareas ) {
		if( bot_developer ) {
			botimport.Print( PRT_ERROR, "AAS_AreaTravelTimeToGoalArea: goalareanum %d out of range\n", goalareanum );
		}

		return qfalse;
	}

	// make sure the routing cache doesn't grow to large
	while( routingcachesize > max_routingcachesize ) {
		if( !AAS_FreeOldestCache() ) {
			break;
		}
	}

	//
	if( AAS_AreaDoNotEnter( areanum ) || AAS_AreaDoNotEnter( goalareanum ) ) {
		travelflags |= TFL_DONOTENTER;
	}

	if( AAS_AreaDoNotEnterLarge( areanum ) || AAS_AreaDoNotEnterLarge( goalareanum ) ) {
		travelflags |= TFL_DONOTENTER_LARGE;
	}

	// NOTE: the number of routing updates is limited per frame
	/*
	if ((*aasworld).frameroutingupdates > MAX_FRAMEROUTINGUPDATES)
	{
	#ifdef DEBUG
	  //Log_Write("WARNING: AAS_AreaTravelTimeToGoalArea: frame routing updates overflowed");
	#endif
	  return 0;
	}
	*/
	//
	clusternum	   = ( *aasworld ).areasettings[areanum].cluster;
	goalclusternum = ( *aasworld ).areasettings[goalareanum].cluster;

	// check if the area is a portal of the goal area cluster
	if( clusternum < 0 && goalclusternum > 0 ) {
		portal = &( *aasworld ).portals[-clusternum];

		if( portal->frontcluster == goalclusternum || portal->backcluster == goalclusternum ) {
			clusternum = goalclusternum;
		}
	}

	// check if the goalarea is a portal of the area cluster
	else if( clusternum > 0 && goalclusternum < 0 ) {
		portal = &( *aasworld ).portals[-goalclusternum];

		if( portal->frontcluster == clusternum || portal->backcluster == clusternum ) {
			goalclusternum = clusternum;
		}
	}

	// if both areas are in the same cluster
	// NOTE: there might be a shorter route via another cluster!!! but we don't care
	if( clusternum > 0 && goalclusternum > 0 && clusternum == goalclusternum ) {
		//
		areacache = AAS_GetAreaRoutingCache( clusternum, goalareanum, travelflags, qfalse );

		// RF, note that the routing cache might be NULL now since we are restricting
		// the updates per frame, hopefully rejected cache's will be requested again
		// when things have settled down
		if( !areacache ) {
			return qfalse;
		}

		// the number of the area in the cluster
		clusterareanum = AAS_ClusterAreaNum( clusternum, areanum );
		// the cluster the area is in
		cluster = &( *aasworld ).clusters[clusternum];

		// if the area is NOT a reachability area
		if( clusterareanum >= cluster->numreachabilityareas ) {
			return qfalse;
		}

		// if it is possible to travel to the goal area through this cluster
		if( areacache->traveltimes[clusterareanum] != 0 ) {
			*reachnum = ( *aasworld ).areasettings[areanum].firstreachablearea + areacache->reachabilities[clusterareanum];

			//
			if( !origin ) {
				*traveltime = areacache->traveltimes[clusterareanum];
				return qtrue;
			}

			//
			reach		= &( *aasworld ).reachability[*reachnum];
			*traveltime = areacache->traveltimes[clusterareanum] + AAS_AreaTravelTime( areanum, origin, reach->start );
			return qtrue;
		}
	}

	//
	clusternum	   = ( *aasworld ).areasettings[areanum].cluster;
	goalclusternum = ( *aasworld ).areasettings[goalareanum].cluster;

	// if the goal area is a portal
	if( goalclusternum < 0 ) {
		// just assume the goal area is part of the front cluster
		portal		   = &( *aasworld ).portals[-goalclusternum];
		goalclusternum = portal->frontcluster;
	}

	// get the portal routing cache
	portalcache = AAS_GetPortalRoutingCache( goalclusternum, goalareanum, travelflags );

	// if the area is a cluster portal, read directly from the portal cache
	if( clusternum < 0 ) {
		*traveltime = portalcache->traveltimes[-clusternum];
		*reachnum	= ( *aasworld ).areasettings[areanum].firstreachablearea + portalcache->reachabilities[-clusternum];
		return qtrue;
	}

	//
	besttime	 = 0;
	bestreachnum = -1;
	// the cluster the area is in
	cluster = &( *aasworld ).clusters[clusternum];
	// current area inside the current cluster
	clusterareanum = AAS_ClusterAreaNum( clusternum, areanum );

	// if the area is NOT a reachability area
	if( clusterareanum >= cluster->numreachabilityareas ) {
		return qfalse;
	}

	//
	pPortalnum = ( *aasworld ).portalindex + cluster->firstportal;

	// find the portal of the area cluster leading towards the goal area
	for( i = 0; i < cluster->numportals; i++, pPortalnum++ ) {
		portalnum = *pPortalnum;

		// if the goal area isn't reachable from the portal
		if( !portalcache->traveltimes[portalnum] ) {
			continue;
		}

		//
		portal = ( *aasworld ).portals + portalnum;

		// if the area in disabled
		if( ( *aasworld ).areasettings[portal->areanum].areaflags & AREA_DISABLED ) {
			continue;
		}

		// get the cache of the portal area
		areacache = AAS_GetAreaRoutingCache( clusternum, portal->areanum, travelflags, qfalse );

		// RF, this may be NULL if we were unable to calculate the cache this frame
		if( !areacache ) {
			return qfalse;
		}

		// if the portal is NOT reachable from this area
		if( !areacache->traveltimes[clusterareanum] ) {
			continue;
		}

		// total travel time is the travel time the portal area is from
		// the goal area plus the travel time towards the portal area
		t = portalcache->traveltimes[portalnum] + areacache->traveltimes[clusterareanum];
		// FIXME: add the exact travel time through the actual portal area
		// NOTE: for now we just add the largest travel time through the area portal
		//		because we can't directly calculate the exact travel time
		//		to be more specific we don't know which reachability is used to travel
		//		into the portal area when coming from the current area
		t += ( *aasworld ).portalmaxtraveltimes[portalnum];
		//
		// Ridah, needs to be up here
		*reachnum = ( *aasworld ).areasettings[areanum].firstreachablearea + areacache->reachabilities[clusterareanum];

		// botimport.Print(PRT_MESSAGE, "portal reachability: %i\n", (int)areacache->reachabilities[clusterareanum] );

		if( origin ) {
			reach = ( *aasworld ).reachability + *reachnum;
			t += AAS_AreaTravelTime( areanum, origin, reach->start );
		}

		// if the time is better than the one already found
		if( !besttime || t < besttime ) {
			bestreachnum = *reachnum;
			besttime	 = t;
		}
	}

	// Ridah, check a route was found
	if( bestreachnum < 0 ) {
		return qfalse;
	}

	*reachnum	= bestreachnum;
	*traveltime = besttime;
	return qtrue;
}

int AAS_AreaTravelTimeToGoalArea( int areanum, vec3_t origin, int goalareanum, int travelflags )
{
	int traveltime, reachnum;

	if( AAS_AreaRouteToGoalArea( areanum, origin, goalareanum, travelflags, &traveltime, &reachnum ) ) {
		return traveltime;
	}

	return 0;
}

int AAS_AreaTravelTimeToGoalAreaCheckLoop( int areanum, vec3_t origin, int goalareanum, int travelflags, int loopareanum )
{
	int					traveltime, reachnum;
	aas_reachability_t* reach;

	if( AAS_AreaRouteToGoalArea( areanum, origin, goalareanum, travelflags, &traveltime, &reachnum ) ) {
		reach = &( *aasworld ).reachability[reachnum];

		if( loopareanum && reach->areanum == loopareanum ) {
			return 0; // going here will cause a looped route
		}

		return traveltime;
	}

	return 0;
}

/*!
	\brief Returns the reachability number between an area and a goal area or zero if no path exists

	This function attempts to find a route from a specified origin in area areanum to goalareanum using the traversal flags passed in travelflags. It delegates the calculation to
   AAS_AreaRouteToGoalArea, which calculates the travel time and reachability number. If a route is found, the reachability index is returned; otherwise zero is returned, indicating that no reachable
   path exists.

	\param areanum numeric identifier of the starting area
	\param origin 3‑dimensional starting position within the area
	\param goalareanum numeric identifier of the destination area
	\param travelflags flags controlling traversal options such as movement type or restrictions
	\return reachability index of the found route, or 0 if no reachable route could be determined
*/
int AAS_AreaReachabilityToGoalArea( int areanum, vec3_t origin, int goalareanum, int travelflags )
{
	int traveltime, reachnum;

	if( AAS_AreaRouteToGoalArea( areanum, origin, goalareanum, travelflags, &traveltime, &reachnum ) ) {
		return reachnum;
	}

	return 0;
}

void AAS_ReachabilityFromNum( int num, struct aas_reachability_s* reach )
{
	if( !( *aasworld ).initialized ) {
		memset( reach, 0, sizeof( aas_reachability_t ) );
		return;
	}

	if( num < 0 || num >= ( *aasworld ).reachabilitysize ) {
		memset( reach, 0, sizeof( aas_reachability_t ) );
		return;
	}

	memcpy( reach, &( *aasworld ).reachability[num], sizeof( aas_reachability_t ) );
	;
}

int AAS_NextAreaReachability( int areanum, int reachnum )
{
	aas_areasettings_t* settings;

	if( !( *aasworld ).initialized ) {
		return 0;
	}

	if( areanum <= 0 || areanum >= ( *aasworld ).numareas ) {
		botimport.Print( PRT_ERROR, "AAS_NextAreaReachability: areanum %d out of range\n", areanum );
		return 0;
	}

	settings = &( *aasworld ).areasettings[areanum];

	if( !reachnum ) {
		return settings->firstreachablearea;
	}

	if( reachnum < settings->firstreachablearea ) {
		botimport.Print( PRT_FATAL, "AAS_NextAreaReachability: reachnum < settings->firstreachableara" );
		return 0;
	}

	reachnum++;

	if( reachnum >= settings->firstreachablearea + settings->numreachableareas ) {
		return 0;
	}

	return reachnum;
}

int AAS_NextModelReachability( int num, int modelnum )
{
	int i;

	if( num <= 0 ) {
		num = 1;

	} else if( num >= ( *aasworld ).reachabilitysize ) {
		return 0;

	} else {
		num++;
	}

	//
	for( i = num; i < ( *aasworld ).reachabilitysize; i++ ) {
		if( ( *aasworld ).reachability[i].traveltype == TRAVEL_ELEVATOR ) {
			if( ( *aasworld ).reachability[i].facenum == modelnum ) {
				return i;
			}

		} else if( ( *aasworld ).reachability[i].traveltype == TRAVEL_FUNCBOB ) {
			if( ( ( *aasworld ).reachability[i].facenum & 0x0000FFFF ) == modelnum ) {
				return i;
			}
		}
	}

	return 0;
}

int AAS_RandomGoalArea( int areanum, int travelflags, int* goalareanum, vec3_t goalorigin )
{
	int			i, n, t;
	vec3_t		start, end;
	aas_trace_t trace;

	// if the area has no reachabilities
	if( !AAS_AreaReachability( areanum ) ) {
		return qfalse;
	}

	//
	n = ( *aasworld ).numareas * random();

	for( i = 0; i < ( *aasworld ).numareas; i++ ) {
		if( n <= 0 ) {
			n = 1;
		}

		if( n >= ( *aasworld ).numareas ) {
			n = 1;
		}

		if( AAS_AreaReachability( n ) ) {
			t = AAS_AreaTravelTimeToGoalArea( areanum, ( *aasworld ).areas[areanum].center, n, travelflags );

			// if the goal is reachable
			if( t > 0 ) {
				if( AAS_AreaSwim( n ) ) {
					*goalareanum = n;
					VectorCopy( ( *aasworld ).areas[n].center, goalorigin );
					// botimport.Print(PRT_MESSAGE, "found random goal area %d\n", *goalareanum);
					return qtrue;
				}

				VectorCopy( ( *aasworld ).areas[n].center, start );

				if( !AAS_PointAreaNum( start ) ) {
					Log_Write( "area %d center %f %f %f in solid?", n, start[0], start[1], start[2] );
				}

				VectorCopy( start, end );
				end[2] -= 300;
				trace = AAS_TraceClientBBox( start, end, PRESENCE_CROUCH, -1 );

				if( !trace.startsolid && AAS_PointAreaNum( trace.endpos ) == n ) {
					if( AAS_AreaGroundFaceArea( n ) > 300 ) {
						*goalareanum = n;
						VectorCopy( trace.endpos, goalorigin );
						// botimport.Print(PRT_MESSAGE, "found random goal area %d\n", *goalareanum);
						return qtrue;
					}
				}
			}
		}

		n++;
	}

	return qfalse;
}

/*!
	\brief Compresses a byte array using run‑length encoding.

	The function processes the input array of bytes and encodes consecutive identical values as a pair of the value and its run length. For each run it writes the byte value followed by a byte
   indicating how many times the value repeats (up to 255). The result is stored in the destination buffer. This provides a simple RLE compression for visibility data in the AAS subsystem.

	If the input is empty, zero bytes are written. The destination buffer must be large enough; the maximum possible size is twice the number of input bytes, since in the worst case every byte is a
   separate run.

	The function does not perform bounds checking or throw exceptions; callers must ensure that the dest array is large enough.

	\param vis Pointer to the source array of bytes to be compressed
	\param numareas Number of bytes in the source array
	\param dest Pointer to the buffer that will receive the compressed data
	\return The number of bytes written to the destination buffer
*/
int AAS_CompressVis( byte* vis, int numareas, byte* dest )
{
	int	  j;
	int	  rep;
	// int		visrow;
	byte* dest_p;
	byte  check;

	//
	dest_p = dest;
	// visrow = (numareas + 7)>>3;

	for( j = 0; j < numareas /*visrow*/; j++ ) {
		*dest_p++ = vis[j];
		check	  = vis[j];
		// if (vis[j])
		//	continue;

		rep = 1;

		for( j++; j < numareas /*visrow*/; j++ )
			if( vis[j] != check || rep == 255 ) {
				break;

			} else {
				rep++;
			}

		*dest_p++ = rep;
		j--;
	}

	return dest_p - dest;
}

/*!
	\brief Decompresses a run‑length encoded visibility map into a full byte array.

	The function initializes the output buffer to zero. It then iterates over the input,
	where each pair of bytes encodes a run length. The first byte is a flag indicating whether
	the run consists of visible (1) bits; if it is non‑zero, the following byte specifies the
	number of bits to set to 1 in the output. The function continues until the total number of
	bytes written equals numareas. A zero count causes an error via AAS_Error.

	\param decompressed Pointer to buffer that will receive the decompressed visibility data; must be
pre‑allocated to at least numareas bytes.
	\param in Pointer to compressed visibility data (each run encoded as two bytes: flag and count).
	\param numareas Total number of bytes expected in the decompressed visibility map.
*/
void AAS_DecompressVis( byte* in, int numareas, byte* decompressed )
{
	byte  c;
	byte* out;
	// int		row;
	byte* end;

	// initialize the vis data, only set those that are visible
	memset( decompressed, 0, numareas );

	// row = (numareas+7)>>3;
	out = decompressed;

	// RB: skip the int cast for 64 bit
	end = ( byte* )( decompressed + numareas );

	do {
		c = in[1];

		if( !c ) {
			AAS_Error( "DecompressVis: 0 repeat" );
		}

		if( *in ) { // we need to set these bits
			memset( out, 1, c );
		}

		in += 2;
		out += c;
	} while( out < end );
}

/*!
	\brief Check if a destination area is visible from a source area, decompressing visibility data if needed.

	The function operates on a global world structure that contains precomputed visibility lists for each area. If the cached decompressed visibility for the source area is not yet up to date, it will
   decompress that list and store it for future use. Once the visibility bit vector for the source area is available, the function simply checks the bit corresponding to the destination area and
   returns whether it is set. A missing visibility list for the source area results in an immediate non‑visible return. This process is used throughout the navigation code to determine line‑of‑sight
   and hiding opportunities.

	\param srcarea Index of the source area whose visibility is being queried.
	\param destarea Index of the destination area to test for visibility.
	\return An integer that evaluates to true (non‑zero) if the destination area is visible from the source area, and false (zero) otherwise.
*/
int AAS_AreaVisible( int srcarea, int destarea )
{
	if( srcarea != ( *aasworld ).decompressedvisarea ) {
		if( !( *aasworld ).areavisibility[srcarea] ) {
			return qfalse;
		}

		AAS_DecompressVis( ( *aasworld ).areavisibility[srcarea], ( *aasworld ).numareas, ( *aasworld ).decompressedvis );
		( *aasworld ).decompressedvisarea = srcarea;
	}

	return ( *aasworld ).decompressedvis[destarea];
}

/*!
	\brief Creates and stores a compressed visibility table for every valid area in the AAS world.

	For each area that is reachable, the function first finds a valid navigation waypoint by tracing from the center of the area down to a non‑solid point. It then tests visibility between every pair
   of valid areas: the function uses the potential‑visible set (PVS) for a quick pre‑check and, when necessary, performs a precise trace between their waypoints. If the trace is unobstructed or the
   areas are the same, the pair is marked visible. The visibility data for each area is stored in a temporary, decompressible array, compressed with AAS_CompressVis, and the resulting compressed
   buffer is copied into the AAS world’s visibility structures. The function cleans up temporary memory and reports the total compressed size. The computation avoids duplicate checks by keeping a
   symmetric area table and uses the compressed format to keep memory usage low.

*/
void AAS_CreateVisibility()
{
	int			i, j, size, totalsize;
	vec3_t		endpos, mins, maxs;
	bsp_trace_t trace;
	byte*		buf;
	byte*		validareas;
	int			numvalid  = 0;
	byte*		areaTable = NULL;
	int			numAreas, numAreaBits;

	numAreas	= ( *aasworld ).numareas;
	numAreaBits = ( ( numAreas + 8 ) >> 3 );
	areaTable	= ( byte* )GetClearedMemory( numAreas * numAreaBits * sizeof( byte ) );

	buf		   = ( byte* )GetClearedMemory( numAreas * 2 * sizeof( byte ) ); // in case it ends up bigger than the decompressedvis, which is rare but possible
	validareas = ( byte* )GetClearedMemory( numAreas * sizeof( byte ) );

	( *aasworld ).areavisibility  = ( byte** )GetClearedMemory( numAreas * sizeof( byte* ) );
	( *aasworld ).decompressedvis = ( byte* )GetClearedMemory( numAreas * sizeof( byte ) );
	( *aasworld ).areawaypoints	  = ( vec3_t* )GetClearedMemory( numAreas * sizeof( vec3_t ) );
	totalsize					  = numAreas * sizeof( byte* );

	for( i = 1; i < numAreas; i++ ) {
		if( !AAS_AreaReachability( i ) ) {
			continue;
		}

		// find the waypoint
		VectorCopy( ( *aasworld ).areas[i].center, endpos );
		endpos[2] -= 256;
		AAS_PresenceTypeBoundingBox( PRESENCE_NORMAL, mins, maxs );
		//		maxs[2] = 0;
		trace = AAS_Trace( ( *aasworld ).areas[i].center, mins, maxs, endpos, -1, CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_MONSTERCLIP );

		if( !trace.startsolid && trace.fraction < 1 && AAS_PointAreaNum( trace.endpos ) == i ) {
			VectorCopy( trace.endpos, ( *aasworld ).areawaypoints[i] );
			validareas[i] = 1;
			numvalid++;

		} else {
			continue;
		}
	}

	for( i = 1; i < numAreas; i++ ) {
		if( !validareas[i] ) {
			continue;
		}

		for( j = 1; j < numAreas; j++ ) {
			( *aasworld ).decompressedvis[j] = 0;

			if( i == j ) {
				( *aasworld ).decompressedvis[j] = 1;

				if( areaTable ) {
					areaTable[( i * numAreaBits ) + ( j >> 3 )] |= ( 1 << ( j & 7 ) );
				}

				continue;
			}

			if( !validareas[j] ) {
				continue;
			}

			// if we have already checked this combination, copy the result
			if( areaTable && ( i > j ) ) {
				// use the reverse result stored in the table
				if( areaTable[( j * numAreaBits ) + ( i >> 3 )] & ( 1 << ( i & 7 ) ) ) {
					( *aasworld ).decompressedvis[j] = 1;
				}

				// done, move to the next area
				continue;
			}

			// RF, check PVS first, since it's much faster
			if( !AAS_inPVS( ( *aasworld ).areawaypoints[i], ( *aasworld ).areawaypoints[j] ) ) {
				continue;
			}

			trace = AAS_Trace( ( *aasworld ).areawaypoints[i], NULL, NULL, ( *aasworld ).areawaypoints[j], -1, CONTENTS_SOLID );

			if( trace.fraction >= 1 ) {
				( *aasworld ).decompressedvis[j] = 1;
			}
		}

		size							= AAS_CompressVis( ( *aasworld ).decompressedvis, numAreas, buf );
		( *aasworld ).areavisibility[i] = ( byte* )GetMemory( size );
		memcpy( ( *aasworld ).areavisibility[i], buf, size );
		totalsize += size;
	}

	if( areaTable ) {
		FreeMemory( areaTable );
	}

	botimport.Print( PRT_MESSAGE, "AAS_CreateVisibility: compressed vis size = %i\n", totalsize );
}

float		VectorDistance( vec3_t v1, vec3_t v2 );
extern void ProjectPointOntoVector( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj );

/*!
	\brief Finds the nearest area that the given entity can use to hide from a specified enemy while respecting travel constraints.

	The routine performs a breadth‑first traversal of the navigation graph, starting from the origin in areanum and expanding to neighboring reachabilities.  It discards moves that use forbidden
   travel flags, pass through ladder or disabled areas, or result in a node that is in or adjacent to the enemy’s area.  Lines of sight to the enemy are checked using the visibility cache, and
   distances to the enemy are examined to avoid standing too close or in the enemy’s direct line.  The algorithm records the travel time to each area and keeps a per‑frame static limit to prevent
   excessive CPU usage, returning 0 if no suitable area is found or if the loop count exceeds the maximum.

	A negative srcnum forces the routine to run regardless of the frame counter, resetting the static timer.  The function returns the area number of the closest valid hiding area, or 0 if none can be
   found.

	\param srcnum Entity number of the searching bot or observer
	\param origin 3‑D position of the searching entity
	\param areanum Area number containing the origin
	\param enemynum Entity number of the opponent or threat
	\param enemyorigin 3‑D position of the opponent
	\param enemyareanum Area number containing the opponent; zero if unknown
	\param travelflags Bitmask of allowed travel types for movement
	\return The area number that represents the nearest valid hiding spot, or zero if no hiding area can be reached under the constraints.
*/
int			AAS_NearestHideArea( int srcnum, vec3_t origin, int areanum, int enemynum, vec3_t enemyorigin, int enemyareanum, int travelflags )
{
	int					 i, j, nextareanum, badtravelflags, numreach, bestarea;
	unsigned short int	 t, besttraveltime, enemytraveltime;
	aas_routingupdate_t *updateliststart, *updatelistend, *curupdate, *nextupdate;
	aas_reachability_t*	 reach;
	float				 dist1, dist2;
	float				 enemytraveldist;
	vec3_t				 enemyVec;
	qboolean			 startVisible;
	vec3_t				 v1, v2, p;
#define MAX_HIDEAREA_LOOPS 3000
	static float lastTime;
	static int	 loopCount;

	//
	if( srcnum < 0 ) { // hack to force run this call
		srcnum	 = -srcnum - 1;
		lastTime = 0;
	}

	// don't run this more than once per frame
	if( lastTime == AAS_Time() && loopCount >= MAX_HIDEAREA_LOOPS ) {
		return 0;
	}

	lastTime  = AAS_Time();
	loopCount = 0;

	//
	if( !( *aasworld ).hidetraveltimes ) {
		( *aasworld ).hidetraveltimes = ( unsigned short int* )GetClearedMemory( ( *aasworld ).numareas * sizeof( unsigned short int ) );

	} else {
		memset( ( *aasworld ).hidetraveltimes, 0, ( *aasworld ).numareas * sizeof( unsigned short int ) );
	}

	//
	if( !( *aasworld ).visCache ) {
		( *aasworld ).visCache = ( byte* )GetClearedMemory( ( *aasworld ).numareas * sizeof( byte ) );

	} else {
		memset( ( *aasworld ).visCache, 0, ( *aasworld ).numareas * sizeof( byte ) );
	}

	besttraveltime = 0;
	bestarea	   = 0;

	if( enemyareanum ) {
		enemytraveltime = AAS_AreaTravelTimeToGoalArea( areanum, origin, enemyareanum, travelflags );
	}

	VectorSubtract( enemyorigin, origin, enemyVec );
	enemytraveldist = VectorNormalize( enemyVec );
	startVisible	= botimport.AICast_VisibleFromPos( enemyorigin, enemynum, origin, srcnum, qfalse );
	//
	badtravelflags = ~travelflags;
	//
	curupdate		   = &( *aasworld ).areaupdate[areanum];
	curupdate->areanum = areanum;
	VectorCopy( origin, curupdate->start );
	curupdate->areatraveltimes = ( *aasworld ).areatraveltimes[areanum][0];
	curupdate->tmptraveltime   = 0;
	// put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	updateliststart = curupdate;
	updatelistend	= curupdate;

	// while there are updates in the current list, flip the lists
	while( updateliststart ) {
		curupdate = updateliststart;

		//
		if( curupdate->next ) {
			curupdate->next->prev = NULL;

		} else {
			updatelistend = NULL;
		}

		updateliststart = curupdate->next;
		//
		curupdate->inlist = qfalse;
		// check all reversed reachability links
		numreach = ( *aasworld ).areasettings[curupdate->areanum].numreachableareas;
		reach	 = &( *aasworld ).reachability[( *aasworld ).areasettings[curupdate->areanum].firstreachablearea];

		//
		for( i = 0; i < numreach; i++, reach++ ) {
			// if an undesired travel type is used
			if( ( *aasworld ).travelflagfortype[reach->traveltype] & badtravelflags ) {
				continue;
			}

			//
			if( AAS_AreaContentsTravelFlag( reach->areanum ) & badtravelflags ) {
				continue;
			}

			// dont pass through ladder areas
			if( ( *aasworld ).areasettings[reach->areanum].areaflags & AREA_LADDER ) {
				continue;
			}

			//
			if( ( *aasworld ).areasettings[reach->areanum].areaflags & AREA_DISABLED ) {
				continue;
			}

			// number of the area the reachability leads to
			nextareanum = reach->areanum;

			// if this moves us into the enemies area, skip it
			if( nextareanum == enemyareanum ) {
				continue;
			}

			// time already travelled plus the traveltime through
			// the current area plus the travel time from the reachability
			t = curupdate->tmptraveltime + AAS_AreaTravelTime( curupdate->areanum, curupdate->start, reach->start ) + reach->traveltime;
			// inc the loopCount, we are starting to use a bit of cpu time
			loopCount++;

			// if this isn't the fastest route to this area, ignore
			if( ( *aasworld ).hidetraveltimes[nextareanum] && ( *aasworld ).hidetraveltimes[nextareanum] < t ) {
				continue;
			}

			( *aasworld ).hidetraveltimes[nextareanum] = t;

			// if the bestarea is this area, then it must be a longer route, so ignore it
			if( bestarea == nextareanum ) {
				bestarea	   = 0;
				besttraveltime = 0;
			}

			// do this test now, so we can reject the route if it starts out too long
			if( besttraveltime && t >= besttraveltime ) {
				continue;
			}

			//
			// avoid going near the enemy
			ProjectPointOntoVector( enemyorigin, curupdate->start, reach->end, p );

			for( j = 0; j < 3; j++ ) {
				if( ( p[j] > curupdate->start[j] + 0.1 && p[j] > reach->end[j] + 0.1 ) || ( p[j] < curupdate->start[j] - 0.1 && p[j] < reach->end[j] - 0.1 ) ) {
					break;
				}
			}

			if( j < 3 ) {
				VectorSubtract( enemyorigin, reach->end, v2 );

			} else {
				VectorSubtract( enemyorigin, p, v2 );
			}

			dist2 = VectorLength( v2 );

			// never go through the enemy
			if( enemytraveldist > 32 && dist2 < enemytraveldist && dist2 < 256 ) {
				continue;
			}

			//
			VectorSubtract( reach->end, origin, v2 );

			if( enemytraveldist > 32 && DotProduct( v2, enemyVec ) > enemytraveldist / 2 ) {
				continue;
			}

			//
			VectorSubtract( enemyorigin, curupdate->start, v1 );
			dist1 = VectorLength( v1 );

			//
			if( enemytraveldist > 32 && dist2 < dist1 ) {
				t += ( dist1 - dist2 ) * 10;

				// test it again after modifying it
				if( besttraveltime && t >= besttraveltime ) {
					continue;
				}
			}

			// make sure the hide area doesn't have anyone else in it
			if( AAS_IsEntityInArea( srcnum, -1, nextareanum ) ) {
				t += 1000; // avoid this path/area
						   // continue;
			}

			//
			// if we weren't visible when starting, make sure we don't move into their view
			if( enemyareanum && !startVisible && AAS_AreaVisible( enemyareanum, nextareanum ) ) {
				continue;
				// t += 1000;
			}

			//
			if( !besttraveltime || besttraveltime > t ) {
				//
				// if this area doesn't have a vis list, ignore it
				if( ( *aasworld ).areavisibility[nextareanum] ) {
					// if the nextarea is not visible from the enemy area
					if( !AAS_AreaVisible( enemyareanum, nextareanum ) ) { // now last of all, check that this area is a safe hiding spot
						if( ( ( *aasworld ).visCache[nextareanum] == 2 ) ||
							( !( *aasworld ).visCache[nextareanum] && !botimport.AICast_VisibleFromPos( enemyorigin, enemynum, ( *aasworld ).areawaypoints[nextareanum], srcnum, qfalse ) ) ) {
							( *aasworld ).visCache[nextareanum] = 2;
							besttraveltime						= t;
							bestarea							= nextareanum;

						} else {
							( *aasworld ).visCache[nextareanum] = 1;
						}
					}
				}

				//
				// getting down to here is bad for cpu usage
				if( loopCount++ > MAX_HIDEAREA_LOOPS ) {
					// botimport.Print(PRT_MESSAGE, "AAS_NearestHideArea: exceeded max loops, aborting\n" );
					continue;
				}

				//
				// otherwise, add this to the list so we check is reachables
				// disabled, this should only store the raw traveltime, not the adjusted time
				//(*aasworld).hidetraveltimes[nextareanum] = t;
				nextupdate				  = &( *aasworld ).areaupdate[nextareanum];
				nextupdate->areanum		  = nextareanum;
				nextupdate->tmptraveltime = t;
				// remember where we entered this area
				VectorCopy( reach->end, nextupdate->start );

				// if this update is not in the list yet
				if( !nextupdate->inlist ) {
					// add the new update to the end of the list
					nextupdate->next = NULL;
					nextupdate->prev = updatelistend;

					if( updatelistend ) {
						updatelistend->next = nextupdate;

					} else {
						updateliststart = nextupdate;
					}

					updatelistend	   = nextupdate;
					nextupdate->inlist = qtrue;
				}
			}
		}
	}

	// botimport.Print(PRT_MESSAGE, "AAS_NearestHideArea: hidearea: %i, %i loops\n", bestarea, count );
	return bestarea;
}

/*!
	\brief Finds a suitable attack spot for an entity within a specified range from another entity while avoiding certain travel flags and enemy visibility constraints.

	This function performs a pathfinding search to identify a suitable area from which an entity can attack a target enemy. It considers the starting position of the entity, a reference point for
   range constraints, the target enemy, and a set of travel flags to determine valid travel routes. The function evaluates potential attack spots based on distance from the reference point, travel
   time from the starting entity to the spot, and visibility from the spot to the enemy. It uses AAS (Area Awareness System) data to compute travel times and visibility, and avoids areas that are too
   far from the range constraint or that don't meet visibility requirements for attacking the target. The search is limited to prevent excessive computation and returns the area number of the best
   found spot or 0 if none is found within the constraints.

	\param enemynum Identifier of the target entity (enemy) that the spot must be able to attack
	\param outpos Output parameter; on success receives 3-component world coordinates of the chosen attack spot
	\param rangedist Maximum distance from rangenum that a candidate spot may be located
	\param rangenum Identifier of an entity used to constrain the search radius from this entity
	\param srcnum Identifier of the entity performing the search (agent)
	\param travelflags Bitmask of allowed travel types; bits not set are prohibited
	\return Area number of the chosen attack spot (0 if no suitable spot was found)
*/
int AAS_FindAttackSpotWithinRange( int srcnum, int rangenum, int enemynum, float rangedist, int travelflags, float* outpos )
{
	int					 i, nextareanum, badtravelflags, numreach, bestarea;
	unsigned short int	 t, besttraveltime, enemytraveltime;
	aas_routingupdate_t *updateliststart, *updatelistend, *curupdate, *nextupdate;
	aas_reachability_t*	 reach;
	vec3_t				 srcorg, rangeorg, enemyorg;
	int					 srcarea, rangearea, enemyarea;
	unsigned short int	 srctraveltime;
	int					 count = 0;
#define MAX_ATTACKAREA_LOOPS 200
	static float lastTime;

	//
	// RF, currently doesn't work with multiple AAS worlds, so only enable for the default world
	// if (aasworld != aasworlds) return 0;
	//
	// don't run this more than once per frame
	if( lastTime == AAS_Time() ) {
		return 0;
	}

	lastTime = AAS_Time();

	//
	if( !( *aasworld ).hidetraveltimes ) {
		( *aasworld ).hidetraveltimes = ( unsigned short int* )GetClearedMemory( ( *aasworld ).numareas * sizeof( unsigned short int ) );

	} else {
		memset( ( *aasworld ).hidetraveltimes, 0, ( *aasworld ).numareas * sizeof( unsigned short int ) );
	}

	//
	if( !( *aasworld ).visCache ) {
		( *aasworld ).visCache = ( byte* )GetClearedMemory( ( *aasworld ).numareas * sizeof( byte ) );

	} else {
		memset( ( *aasworld ).visCache, 0, ( *aasworld ).numareas * sizeof( byte ) );
	}

	//
	AAS_EntityOrigin( srcnum, srcorg );
	AAS_EntityOrigin( rangenum, rangeorg );
	AAS_EntityOrigin( enemynum, enemyorg );
	//
	srcarea	  = BotFuzzyPointReachabilityArea( srcorg );
	rangearea = BotFuzzyPointReachabilityArea( rangeorg );
	enemyarea = BotFuzzyPointReachabilityArea( enemyorg );
	//
	besttraveltime	= 0;
	bestarea		= 0;
	enemytraveltime = AAS_AreaTravelTimeToGoalArea( srcarea, srcorg, enemyarea, travelflags );
	//
	badtravelflags = ~travelflags;
	//
	curupdate		   = &( *aasworld ).areaupdate[rangearea];
	curupdate->areanum = rangearea;
	VectorCopy( rangeorg, curupdate->start );
	curupdate->areatraveltimes = ( *aasworld ).areatraveltimes[srcarea][0];
	curupdate->tmptraveltime   = 0;
	// put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	updateliststart = curupdate;
	updatelistend	= curupdate;

	// while there are updates in the current list, flip the lists
	while( updateliststart ) {
		curupdate = updateliststart;

		//
		if( curupdate->next ) {
			curupdate->next->prev = NULL;

		} else {
			updatelistend = NULL;
		}

		updateliststart = curupdate->next;
		//
		curupdate->inlist = qfalse;
		// check all reversed reachability links
		numreach = ( *aasworld ).areasettings[curupdate->areanum].numreachableareas;
		reach	 = &( *aasworld ).reachability[( *aasworld ).areasettings[curupdate->areanum].firstreachablearea];

		//
		for( i = 0; i < numreach; i++, reach++ ) {
			// if an undesired travel type is used
			if( ( *aasworld ).travelflagfortype[reach->traveltype] & badtravelflags ) {
				continue;
			}

			//
			if( AAS_AreaContentsTravelFlag( reach->areanum ) & badtravelflags ) {
				continue;
			}

			// dont pass through ladder areas
			if( ( *aasworld ).areasettings[reach->areanum].areaflags & AREA_LADDER ) {
				continue;
			}

			//
			if( ( *aasworld ).areasettings[reach->areanum].areaflags & AREA_DISABLED ) {
				continue;
			}

			// number of the area the reachability leads to
			nextareanum = reach->areanum;

			// if this moves us into the enemies area, skip it
			if( nextareanum == enemyarea ) {
				continue;
			}

			// if we've already been to this area
			if( ( *aasworld ).hidetraveltimes[nextareanum] ) {
				continue;
			}

			// time already travelled plus the traveltime through
			// the current area plus the travel time from the reachability
			if( count++ > MAX_ATTACKAREA_LOOPS ) {
				// botimport.Print(PRT_MESSAGE, "AAS_FindAttackSpotWithinRange: exceeded max loops, aborting\n" );
				if( bestarea ) {
					VectorCopy( ( *aasworld ).areawaypoints[bestarea], outpos );
				}

				return bestarea;
			}

			t = curupdate->tmptraveltime + AAS_AreaTravelTime( curupdate->areanum, curupdate->start, reach->start ) + reach->traveltime;

			//
			// if it's too far from rangenum, ignore
			if( Distance( rangeorg, ( *aasworld ).areawaypoints[nextareanum] ) > rangedist ) {
				continue;
			}

			//
			// find the traveltime from srcnum
			srctraveltime = AAS_AreaTravelTimeToGoalArea( srcarea, srcorg, nextareanum, travelflags );

			// do this test now, so we can reject the route if it starts out too long
			if( besttraveltime && srctraveltime >= besttraveltime ) {
				continue;
			}

			//
			// if this area doesn't have a vis list, ignore it
			if( ( *aasworld ).areavisibility[nextareanum] ) {
				// if the nextarea can see the enemy area
				if( AAS_AreaVisible( enemyarea, nextareanum ) ) {
					// now last of all, check that this area is a good attacking spot
					if( ( ( *aasworld ).visCache[nextareanum] == 2 ) || ( !( *aasworld ).visCache[nextareanum] && ( count += 10 ) && // we are about to use lots of CPU time
																			botimport.AICast_CheckAttackAtPos( srcnum, enemynum, ( *aasworld ).areawaypoints[nextareanum], qfalse, qfalse ) ) ) {
						( *aasworld ).visCache[nextareanum] = 2;
						besttraveltime						= srctraveltime;
						bestarea							= nextareanum;

					} else {
						( *aasworld ).visCache[nextareanum] = 1;
					}
				}
			}

			( *aasworld ).hidetraveltimes[nextareanum] = t;
			nextupdate								   = &( *aasworld ).areaupdate[nextareanum];
			nextupdate->areanum						   = nextareanum;
			nextupdate->tmptraveltime				   = t;
			// remember where we entered this area
			VectorCopy( reach->end, nextupdate->start );

			// if this update is not in the list yet
			if( !nextupdate->inlist ) {
				// add the new update to the end of the list
				nextupdate->next = NULL;
				nextupdate->prev = updatelistend;

				if( updatelistend ) {
					updatelistend->next = nextupdate;

				} else {
					updateliststart = nextupdate;
				}

				updatelistend	   = nextupdate;
				nextupdate->inlist = qtrue;
			}
		}
	}

	// botimport.Print(PRT_MESSAGE, "AAS_NearestHideArea: hidearea: %i, %i loops\n", bestarea, count );
	if( bestarea ) {
		VectorCopy( ( *aasworld ).areawaypoints[bestarea], outpos );
	}

	return bestarea;
}

/*!
	\brief Finds the first point along a route from a source to a destination that can observe the destination

	The function determines the reachability areas containing the source and destination positions. If the source area is directly visible to the destination area, the source position is returned.
   Otherwise it iteratively follows the route produced by AAS_AreaRouteToGoalArea, examining each consecutive reachability segment. At each step it checks whether the current area or its reachability
   end point can see the destination area; if found, that point is returned. The process stops when a loop is detected, the route exhausts, or the maximum iteration count is reached. In such cases the
   function returns false and the output position is unchanged.

	\param srcpos Source position in world coordinates
	\param destpos Destination position in world coordinates
	\param travelflags Flags controlling traversal behavior for routing
	\param retpos Output vector that receives the first visible position on the route
	\return Returns true if a visible position was found; otherwise returns false.
*/
qboolean AAS_GetRouteFirstVisPos( vec3_t srcpos, vec3_t destpos, int travelflags, vec3_t retpos )
{
	int				   srcarea, destarea, travarea;
	vec3_t			   travpos;
	int				   ftraveltime, freachnum, lasttraveltime;
	aas_reachability_t reach;
	int				   loops = 0;
#define MAX_GETROUTE_VISPOS_LOOPS 200
	//
	// SRCPOS: enemy
	// DESTPOS: self
	// RETPOS: first area that is visible from destpos, in route from srcpos to destpos
	srcarea = BotFuzzyPointReachabilityArea( srcpos );

	if( !srcarea ) {
		return qfalse;
	}

	destarea = BotFuzzyPointReachabilityArea( destpos );

	if( !destarea ) {
		return qfalse;
	}

	if( destarea == srcarea ) {
		VectorCopy( srcpos, retpos );
		return qtrue;
	}

	//
	// if the srcarea can see the destarea
	if( AAS_AreaVisible( srcarea, destarea ) ) {
		VectorCopy( srcpos, retpos );
		return qtrue;
	}

	// if this area doesn't have a vis list, ignore it
	if( !( *aasworld ).areavisibility[destarea] ) {
		return qfalse;
	}

	//
	travarea = srcarea;
	VectorCopy( srcpos, travpos );
	lasttraveltime = -1;

	while( ( loops++ < MAX_GETROUTE_VISPOS_LOOPS ) && AAS_AreaRouteToGoalArea( travarea, travpos, destarea, travelflags, &ftraveltime, &freachnum ) ) {
		if( lasttraveltime >= 0 && ftraveltime >= lasttraveltime ) {
			return qfalse; // we may be in a loop
		}

		lasttraveltime = ftraveltime;
		//
		AAS_ReachabilityFromNum( freachnum, &reach );

		if( reach.areanum == destarea ) {
			VectorCopy( travpos, retpos );
			return qtrue;
		}

		// if the reach area can see the destarea
		if( AAS_AreaVisible( reach.areanum, destarea ) ) {
			VectorCopy( reach.end, retpos );
			return qtrue;
		}

		//
		travarea = reach.areanum;
		VectorCopy( reach.end, travpos );
	}

	//
	// unsuccessful
	return qfalse;
}
