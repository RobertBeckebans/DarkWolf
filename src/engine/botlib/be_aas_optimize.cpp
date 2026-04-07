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
 * name:		be_aas_optimize.c
 *
 * desc:		decreases the .aas file size after the reachabilities have
 *				been calculated, just dumps all the faces, edges and vertexes
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_libvar.h"
// #include "l_utils.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "botshared/botlib.h"
#include "botshared/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"

typedef struct optimized_s {
	// vertixes
	int				 numvertexes;
	aas_vertex_t*	 vertexes;
	// edges
	int				 numedges;
	aas_edge_t*		 edges;
	// edge index
	int				 edgeindexsize;
	aas_edgeindex_t* edgeindex;
	// faces
	int				 numfaces;
	aas_face_t*		 faces;
	// face index
	int				 faceindexsize;
	aas_faceindex_t* faceindex;
	// convex areas
	int				 numareas;
	aas_area_t*		 areas;
	//
	int*			 vertexoptimizeindex;
	int*			 edgeoptimizeindex;
	int*			 faceoptimizeindex;
} optimized_t;

/*!
	\brief Always indicates that a given edge must be kept.

	Determines whether an edge should be retained during optimization. The current implementation always returns 1, so every edge is kept. Future revisions may add criteria based on edge properties.

	\param edge pointer to the edge to evaluate for retention
	\return non‑zero if the edge should be kept, zero otherwise
*/
int AAS_KeepEdge( aas_edge_t* edge )
{
	return 1;
}

/*!
	\brief Returns the optimized index of the specified edge, or 0 if the edge is not kept.

	The function first queries the global world structure to obtain the edge and checks whether it should be retained by calling AAS_KeepEdge. If the edge is not kept, the function immediately returns
   0. If the edge has already been optimized previously, it returns the stored optimized index, preserving the sign of the original edge number. Otherwise a new optimized edge is created: the function
   ensures each vertex of the original edge is recorded in the optimized vertex list (adding new entries if necessary), and the edge itself is added to the optimized edge list. The corresponding index
   maps and counters are updated, and finally the function returns the new edge index with the same sign as the input parameter.


	\param optimized pointer to the structure holding the optimized mesh data
	\param edgenum index of the edge in the global world map, sign indicates direction
	\return The optimized edge index, positive or negative to preserve the original direction, or 0 if the edge is not kept by AAS_KeepEdge.
*/
int AAS_OptimizeEdge( optimized_t* optimized, int edgenum )
{
	int			i, optedgenum;
	aas_edge_t *edge, *optedge;

	edge = &( *aasworld ).edges[abs( edgenum )];

	if( !AAS_KeepEdge( edge ) ) {
		return 0;
	}

	optedgenum = optimized->edgeoptimizeindex[abs( edgenum )];

	if( optedgenum ) {
		// keep the edge reversed sign
		if( edgenum > 0 ) {
			return optedgenum;

		} else {
			return -optedgenum;
		}
	}

	optedge = &optimized->edges[optimized->numedges];

	for( i = 0; i < 2; i++ ) {
		if( optimized->vertexoptimizeindex[edge->v[i]] ) {
			optedge->v[i] = optimized->vertexoptimizeindex[edge->v[i]];

		} else {
			VectorCopy( ( *aasworld ).vertexes[edge->v[i]], optimized->vertexes[optimized->numvertexes] );
			optedge->v[i]							   = optimized->numvertexes;
			optimized->vertexoptimizeindex[edge->v[i]] = optimized->numvertexes;
			optimized->numvertexes++;
		}
	}

	optimized->edgeoptimizeindex[abs( edgenum )] = optimized->numedges;
	optedgenum									 = optimized->numedges;
	optimized->numedges++;

	// keep the edge reversed sign
	if( edgenum > 0 ) {
		return optedgenum;

	} else {
		return -optedgenum;
	}
}

/*!
	\brief Returns 1 if the face has the ladder flag, otherwise returns 0.

	The function examines the face's faceflags field for the FACE_LADDER bit. If the bit is not set, it returns 0, indicating the face should not be kept for further processing. If the bit is set, it
   returns 1, meaning the face qualifies for optimization.

	\param face Pointer to a face structure whose flags are inspected.
	\return 1 if the FACE_LADDER flag is set in the face’s flags, otherwise 0.
*/
int AAS_KeepFace( aas_face_t* face )
{
	if( !( face->faceflags & FACE_LADDER ) ) {
		return 0;

	} else {
		return 1;
	}
}

/*!
	\brief Optimize an individual face for later area optimization, returning its new index or zero if it is excluded.

	The function examines the face indicated by facenum in the global world data. If the face should not be kept, it returns zero. If the face has already been optimized, it returns the stored index,
   preserving the sign of facenum to indicate face side. Otherwise, a copy of the face is created in the optimized structure, its edges are individually optimized, and mappings for edges and faces are
   updated. The face’s new index is stored for future lookups and returned with the original sign.


	\param optimized structure accumulating optimized area data, including face and edge records
	\param facenum index of the face in the global world structure; the sign indicates the side to keep
	\return Index of the face within the optimized structure, or zero if the face is not kept.
*/
int AAS_OptimizeFace( optimized_t* optimized, int facenum )
{
	int			i, edgenum, optedgenum, optfacenum;
	aas_face_t *face, *optface;

	face = &( *aasworld ).faces[abs( facenum )];

	if( !AAS_KeepFace( face ) ) {
		return 0;
	}

	optfacenum = optimized->faceoptimizeindex[abs( facenum )];

	if( optfacenum ) {
		// keep the face side sign
		if( facenum > 0 ) {
			return optfacenum;

		} else {
			return -optfacenum;
		}
	}

	optface = &optimized->faces[optimized->numfaces];
	memcpy( optface, face, sizeof( aas_face_t ) );

	optface->numedges  = 0;
	optface->firstedge = optimized->edgeindexsize;

	for( i = 0; i < face->numedges; i++ ) {
		edgenum	   = ( *aasworld ).edgeindex[face->firstedge + i];
		optedgenum = AAS_OptimizeEdge( optimized, edgenum );

		if( optedgenum ) {
			optimized->edgeindex[optface->firstedge + optface->numedges] = optedgenum;
			optface->numedges++;
			optimized->edgeindexsize++;
		}
	}

	optimized->faceoptimizeindex[abs( facenum )] = optimized->numfaces;
	optfacenum									 = optimized->numfaces;
	optimized->numfaces++;

	// keep the face side sign
	if( facenum > 0 ) {
		return optfacenum;

	} else {
		return -optfacenum;
	}
}

/*!
	\brief Processes the faces of a specified area and stores the resulting optimized data in the supplied structure.

	The function copies the original area data from the global world into the optimized structure. It then resets the face count for the optimized area and iterates over each face in the original
   area, calling AAS_OptimizeFace to optimize the individual face. Optimized face indices are written into the optimized face index array, updating the area’s face count and the overall face index
   size counter.

	\param optimized Pointer to the optimized data structure where the processed area will be stored.
	\param areanum Index of the area to optimize.
*/
void AAS_OptimizeArea( optimized_t* optimized, int areanum )
{
	int			i, facenum, optfacenum;
	aas_area_t *area, *optarea;

	area	= &( *aasworld ).areas[areanum];
	optarea = &optimized->areas[areanum];
	memcpy( optarea, area, sizeof( aas_area_t ) );

	optarea->numfaces  = 0;
	optarea->firstface = optimized->faceindexsize;

	for( i = 0; i < area->numfaces; i++ ) {
		facenum	   = ( *aasworld ).faceindex[area->firstface + i];
		optfacenum = AAS_OptimizeFace( optimized, facenum );

		if( optfacenum ) {
			optimized->faceindex[optarea->firstface + optarea->numfaces] = optfacenum;
			optarea->numfaces++;
			optimized->faceindexsize++;
		}
	}
}

/*!
	\brief Initializes the optimized data structures for AAS processing by allocating and clearing memory.

	The function allocates memory blocks for vertices, edges, edge indices, faces, face indices, areas, and optimization indices using GetClearedMemory. It sets the count of used vertices to zero, and
   initializes the count of used edges and faces to one, reserving index zero as a dummy placeholder. The sizes of the index structures are reset to zero, and the total number of areas is copied from
   the global world data.

	\param optimized pointer to an optimized_t structure that will receive the allocated arrays and initialized counters and sizes
*/
void AAS_OptimizeAlloc( optimized_t* optimized )
{
	optimized->vertexes		 = ( aas_vertex_t* )GetClearedMemory( ( *aasworld ).numvertexes * sizeof( aas_vertex_t ) );
	optimized->numvertexes	 = 0;
	optimized->edges		 = ( aas_edge_t* )GetClearedMemory( ( *aasworld ).numedges * sizeof( aas_edge_t ) );
	optimized->numedges		 = 1; // edge zero is a dummy
	optimized->edgeindex	 = ( aas_edgeindex_t* )GetClearedMemory( ( *aasworld ).edgeindexsize * sizeof( aas_edgeindex_t ) );
	optimized->edgeindexsize = 0;
	optimized->faces		 = ( aas_face_t* )GetClearedMemory( ( *aasworld ).numfaces * sizeof( aas_face_t ) );
	optimized->numfaces		 = 1; // face zero is a dummy
	optimized->faceindex	 = ( aas_faceindex_t* )GetClearedMemory( ( *aasworld ).faceindexsize * sizeof( aas_faceindex_t ) );
	optimized->faceindexsize = 0;
	optimized->areas		 = ( aas_area_t* )GetClearedMemory( ( *aasworld ).numareas * sizeof( aas_area_t ) );
	optimized->numareas		 = ( *aasworld ).numareas;
	//
	optimized->vertexoptimizeindex = ( int* )GetClearedMemory( ( *aasworld ).numvertexes * sizeof( int ) );
	optimized->edgeoptimizeindex   = ( int* )GetClearedMemory( ( *aasworld ).numedges * sizeof( int ) );
	optimized->faceoptimizeindex   = ( int* )GetClearedMemory( ( *aasworld ).numfaces * sizeof( int ) );
}

/*!
	\brief Updates the global AAS world with optimized geometry data and frees temporary optimization indices.

	The function first frees any existing vertex, edge, face, area, and index arrays stored in the global aasworld structure, then replaces them with the corresponding pointers from the supplied
   optimized structure. After the data has been stored, the temporary arrays used for calculating the optimizations (vertexoptimizeindex, edgeoptimizeindex, faceoptimizeindex) are freed. This
   effectively swaps the old geometry with the optimized version and cleans up intermediate allocations.

	\param optimized pointer to a structure containing precomputed optimized geometry arrays and index counts to be imported into the global world state
*/
void AAS_OptimizeStore( optimized_t* optimized )
{
	// store the optimized vertexes
	if( ( *aasworld ).vertexes ) {
		FreeMemory( ( *aasworld ).vertexes );
	}

	( *aasworld ).vertexes	  = optimized->vertexes;
	( *aasworld ).numvertexes = optimized->numvertexes;

	// store the optimized edges
	if( ( *aasworld ).edges ) {
		FreeMemory( ( *aasworld ).edges );
	}

	( *aasworld ).edges	   = optimized->edges;
	( *aasworld ).numedges = optimized->numedges;

	// store the optimized edge index
	if( ( *aasworld ).edgeindex ) {
		FreeMemory( ( *aasworld ).edgeindex );
	}

	( *aasworld ).edgeindex		= optimized->edgeindex;
	( *aasworld ).edgeindexsize = optimized->edgeindexsize;

	// store the optimized faces
	if( ( *aasworld ).faces ) {
		FreeMemory( ( *aasworld ).faces );
	}

	( *aasworld ).faces	   = optimized->faces;
	( *aasworld ).numfaces = optimized->numfaces;

	// store the optimized face index
	if( ( *aasworld ).faceindex ) {
		FreeMemory( ( *aasworld ).faceindex );
	}

	( *aasworld ).faceindex		= optimized->faceindex;
	( *aasworld ).faceindexsize = optimized->faceindexsize;

	// store the optimized areas
	if( ( *aasworld ).areas ) {
		FreeMemory( ( *aasworld ).areas );
	}

	( *aasworld ).areas	   = optimized->areas;
	( *aasworld ).numareas = optimized->numareas;
	// free optimize indexes
	FreeMemory( optimized->vertexoptimizeindex );
	FreeMemory( optimized->edgeoptimizeindex );
	FreeMemory( optimized->faceoptimizeindex );
}

void AAS_Optimize()
{
	int			i, sign;
	optimized_t optimized;

	AAS_OptimizeAlloc( &optimized );

	for( i = 1; i < ( *aasworld ).numareas; i++ ) {
		AAS_OptimizeArea( &optimized, i );
	}

	// reset the reachability face pointers
	for( i = 0; i < ( *aasworld ).reachabilitysize; i++ ) {
		// NOTE: for TRAVEL_ELEVATOR the facenum is the model number of
		//		the elevator
		if( ( *aasworld ).reachability[i].traveltype == TRAVEL_ELEVATOR ) {
			continue;
		}

		// NOTE: for TRAVEL_JUMPPAD the facenum is the Z velocity and the edgenum is the hor velocity
		if( ( *aasworld ).reachability[i].traveltype == TRAVEL_JUMPPAD ) {
			continue;
		}

		// NOTE: for TRAVEL_FUNCBOB the facenum and edgenum contain other coded information
		if( ( *aasworld ).reachability[i].traveltype == TRAVEL_FUNCBOB ) {
			continue;
		}

		//
		sign								  = ( *aasworld ).reachability[i].facenum;
		( *aasworld ).reachability[i].facenum = optimized.faceoptimizeindex[abs( ( *aasworld ).reachability[i].facenum )];

		if( sign < 0 ) {
			( *aasworld ).reachability[i].facenum = -( *aasworld ).reachability[i].facenum;
		}

		sign								  = ( *aasworld ).reachability[i].edgenum;
		( *aasworld ).reachability[i].edgenum = optimized.edgeoptimizeindex[abs( ( *aasworld ).reachability[i].edgenum )];

		if( sign < 0 ) {
			( *aasworld ).reachability[i].edgenum = -( *aasworld ).reachability[i].edgenum;
		}
	}

	// store the optimized AAS data into (*aasworld)
	AAS_OptimizeStore( &optimized );
	// print some nice stuff :)
	botimport.Print( PRT_MESSAGE, "AAS data optimized.\n" );
}
