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
 * name:		be_aas_file.c
 *
 * desc:		AAS file loading/writing
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_libvar.h"
#include "l_utils.h"
#include "aasfile.h"
#include "botshared/botlib.h"
#include "botshared/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"

/*!
	\brief Swaps byte order of AAS data if the system architecture is big-endian.

	This function performs byte swapping on all AAS (Area Awareness System) data structures to ensure compatibility when the data is loaded on a system with different endianness. It checks whether the
   system is already in little-endian format using LittleLong, and if so, returns immediately. Otherwise, it iterates through all AAS data structures including bounding boxes, vertexes, planes, edges,
   faces, areas, area settings, reachability information, nodes, cluster portals, and clusters, applying appropriate byte swapping functions to each member field. The function handles different data
   types such as integers, floats, and shorts using LittleLong, LittleFloat, and LittleShort macros.

	\return None
	\throws None
*/
void AAS_SwapAASData()
{
	int i, j;

	// Ridah, no need to do anything if this OS doesn't need byte swapping
	if( LittleLong( 1 ) == 1 ) {
		return;
	}

	// done.

	// bounding boxes
	for( i = 0; i < ( *aasworld ).numbboxes; i++ ) {
		( *aasworld ).bboxes[i].presencetype = LittleLong( ( *aasworld ).bboxes[i].presencetype );
		( *aasworld ).bboxes[i].flags		 = LittleLong( ( *aasworld ).bboxes[i].flags );

		for( j = 0; j < 3; j++ ) {
			( *aasworld ).bboxes[i].mins[j] = LittleFloat( ( *aasworld ).bboxes[i].mins[j] );
			( *aasworld ).bboxes[i].maxs[j] = LittleFloat( ( *aasworld ).bboxes[i].maxs[j] );
		}
	}

	// vertexes
	for( i = 0; i < ( *aasworld ).numvertexes; i++ ) {
		for( j = 0; j < 3; j++ ) {
			( *aasworld ).vertexes[i][j] = LittleFloat( ( *aasworld ).vertexes[i][j] );
		}
	}

	// planes
	for( i = 0; i < ( *aasworld ).numplanes; i++ ) {
		for( j = 0; j < 3; j++ ) {
			( *aasworld ).planes[i].normal[j] = LittleFloat( ( *aasworld ).planes[i].normal[j] );
		}

		( *aasworld ).planes[i].dist = LittleFloat( ( *aasworld ).planes[i].dist );
		( *aasworld ).planes[i].type = LittleLong( ( *aasworld ).planes[i].type );
	}

	// edges
	for( i = 0; i < ( *aasworld ).numedges; i++ ) {
		( *aasworld ).edges[i].v[0] = LittleLong( ( *aasworld ).edges[i].v[0] );
		( *aasworld ).edges[i].v[1] = LittleLong( ( *aasworld ).edges[i].v[1] );
	}

	// edgeindex
	for( i = 0; i < ( *aasworld ).edgeindexsize; i++ ) {
		( *aasworld ).edgeindex[i] = LittleLong( ( *aasworld ).edgeindex[i] );
	}

	// faces
	for( i = 0; i < ( *aasworld ).numfaces; i++ ) {
		( *aasworld ).faces[i].planenum	 = LittleLong( ( *aasworld ).faces[i].planenum );
		( *aasworld ).faces[i].faceflags = LittleLong( ( *aasworld ).faces[i].faceflags );
		( *aasworld ).faces[i].numedges	 = LittleLong( ( *aasworld ).faces[i].numedges );
		( *aasworld ).faces[i].firstedge = LittleLong( ( *aasworld ).faces[i].firstedge );
		( *aasworld ).faces[i].frontarea = LittleLong( ( *aasworld ).faces[i].frontarea );
		( *aasworld ).faces[i].backarea	 = LittleLong( ( *aasworld ).faces[i].backarea );
	}

	// face index
	for( i = 0; i < ( *aasworld ).faceindexsize; i++ ) {
		( *aasworld ).faceindex[i] = LittleLong( ( *aasworld ).faceindex[i] );
	}

	// convex areas
	for( i = 0; i < ( *aasworld ).numareas; i++ ) {
		( *aasworld ).areas[i].areanum	 = LittleLong( ( *aasworld ).areas[i].areanum );
		( *aasworld ).areas[i].numfaces	 = LittleLong( ( *aasworld ).areas[i].numfaces );
		( *aasworld ).areas[i].firstface = LittleLong( ( *aasworld ).areas[i].firstface );

		for( j = 0; j < 3; j++ ) {
			( *aasworld ).areas[i].mins[j]	 = LittleFloat( ( *aasworld ).areas[i].mins[j] );
			( *aasworld ).areas[i].maxs[j]	 = LittleFloat( ( *aasworld ).areas[i].maxs[j] );
			( *aasworld ).areas[i].center[j] = LittleFloat( ( *aasworld ).areas[i].center[j] );
		}
	}

	// area settings
	for( i = 0; i < ( *aasworld ).numareasettings; i++ ) {
		( *aasworld ).areasettings[i].contents			 = LittleLong( ( *aasworld ).areasettings[i].contents );
		( *aasworld ).areasettings[i].areaflags			 = LittleLong( ( *aasworld ).areasettings[i].areaflags );
		( *aasworld ).areasettings[i].presencetype		 = LittleLong( ( *aasworld ).areasettings[i].presencetype );
		( *aasworld ).areasettings[i].cluster			 = LittleLong( ( *aasworld ).areasettings[i].cluster );
		( *aasworld ).areasettings[i].clusterareanum	 = LittleLong( ( *aasworld ).areasettings[i].clusterareanum );
		( *aasworld ).areasettings[i].numreachableareas	 = LittleLong( ( *aasworld ).areasettings[i].numreachableareas );
		( *aasworld ).areasettings[i].firstreachablearea = LittleLong( ( *aasworld ).areasettings[i].firstreachablearea );
		( *aasworld ).areasettings[i].groundsteepness	 = LittleLong( ( *aasworld ).areasettings[i].groundsteepness );
	}

	// area reachability
	for( i = 0; i < ( *aasworld ).reachabilitysize; i++ ) {
		( *aasworld ).reachability[i].areanum = LittleLong( ( *aasworld ).reachability[i].areanum );
		( *aasworld ).reachability[i].facenum = LittleLong( ( *aasworld ).reachability[i].facenum );
		( *aasworld ).reachability[i].edgenum = LittleLong( ( *aasworld ).reachability[i].edgenum );

		for( j = 0; j < 3; j++ ) {
			( *aasworld ).reachability[i].start[j] = LittleFloat( ( *aasworld ).reachability[i].start[j] );
			( *aasworld ).reachability[i].end[j]   = LittleFloat( ( *aasworld ).reachability[i].end[j] );
		}

		( *aasworld ).reachability[i].traveltype = LittleLong( ( *aasworld ).reachability[i].traveltype );
		( *aasworld ).reachability[i].traveltime = LittleShort( ( *aasworld ).reachability[i].traveltime );
	}

	// nodes
	for( i = 0; i < ( *aasworld ).numnodes; i++ ) {
		( *aasworld ).nodes[i].planenum	   = LittleLong( ( *aasworld ).nodes[i].planenum );
		( *aasworld ).nodes[i].children[0] = LittleLong( ( *aasworld ).nodes[i].children[0] );
		( *aasworld ).nodes[i].children[1] = LittleLong( ( *aasworld ).nodes[i].children[1] );
	}

	// cluster portals
	for( i = 0; i < ( *aasworld ).numportals; i++ ) {
		( *aasworld ).portals[i].areanum		   = LittleLong( ( *aasworld ).portals[i].areanum );
		( *aasworld ).portals[i].frontcluster	   = LittleLong( ( *aasworld ).portals[i].frontcluster );
		( *aasworld ).portals[i].backcluster	   = LittleLong( ( *aasworld ).portals[i].backcluster );
		( *aasworld ).portals[i].clusterareanum[0] = LittleLong( ( *aasworld ).portals[i].clusterareanum[0] );
		( *aasworld ).portals[i].clusterareanum[1] = LittleLong( ( *aasworld ).portals[i].clusterareanum[1] );
	}

	// cluster portal index
	for( i = 0; i < ( *aasworld ).portalindexsize; i++ ) {
		( *aasworld ).portalindex[i] = LittleLong( ( *aasworld ).portalindex[i] );
	}

	// cluster
	for( i = 0; i < ( *aasworld ).numclusters; i++ ) {
		( *aasworld ).clusters[i].numareas			   = LittleLong( ( *aasworld ).clusters[i].numareas );
		( *aasworld ).clusters[i].numreachabilityareas = LittleLong( ( *aasworld ).clusters[i].numreachabilityareas );
		( *aasworld ).clusters[i].numportals		   = LittleLong( ( *aasworld ).clusters[i].numportals );
		( *aasworld ).clusters[i].firstportal		   = LittleLong( ( *aasworld ).clusters[i].firstportal );
	}
}

/*!
	\brief Releases all memory allocated for the current AAS world and resets the global state to an unloaded configuration.

	The function iterates over each member of the global AAS data structure, freeing any allocated memory blocks and zeroing associated counters. It clears arrays for boxes, vertexes, planes, edges,
   faces, areas, area settings, reachability, nodes, portals, portal indices, and clusters. After deallocation, it null‑terminates the corresponding pointers and resets all size fields to zero.
   Finally, status flags indicating whether the world is loaded, initialized, or saved are set to false, ensuring the structure is returned to an empty, uninitialized state.

*/
void AAS_DumpAASData()
{
	( *aasworld ).numbboxes = 0;

	if( ( *aasworld ).bboxes ) {
		FreeMemory( ( *aasworld ).bboxes );
	}

	( *aasworld ).bboxes	  = NULL;
	( *aasworld ).numvertexes = 0;

	if( ( *aasworld ).vertexes ) {
		FreeMemory( ( *aasworld ).vertexes );
	}

	( *aasworld ).vertexes	= NULL;
	( *aasworld ).numplanes = 0;

	if( ( *aasworld ).planes ) {
		FreeMemory( ( *aasworld ).planes );
	}

	( *aasworld ).planes   = NULL;
	( *aasworld ).numedges = 0;

	if( ( *aasworld ).edges ) {
		FreeMemory( ( *aasworld ).edges );
	}

	( *aasworld ).edges			= NULL;
	( *aasworld ).edgeindexsize = 0;

	if( ( *aasworld ).edgeindex ) {
		FreeMemory( ( *aasworld ).edgeindex );
	}

	( *aasworld ).edgeindex = NULL;
	( *aasworld ).numfaces	= 0;

	if( ( *aasworld ).faces ) {
		FreeMemory( ( *aasworld ).faces );
	}

	( *aasworld ).faces			= NULL;
	( *aasworld ).faceindexsize = 0;

	if( ( *aasworld ).faceindex ) {
		FreeMemory( ( *aasworld ).faceindex );
	}

	( *aasworld ).faceindex = NULL;
	( *aasworld ).numareas	= 0;

	if( ( *aasworld ).areas ) {
		FreeMemory( ( *aasworld ).areas );
	}

	( *aasworld ).areas			  = NULL;
	( *aasworld ).numareasettings = 0;

	if( ( *aasworld ).areasettings ) {
		FreeMemory( ( *aasworld ).areasettings );
	}

	( *aasworld ).areasettings	   = NULL;
	( *aasworld ).reachabilitysize = 0;

	if( ( *aasworld ).reachability ) {
		FreeMemory( ( *aasworld ).reachability );
	}

	( *aasworld ).reachability = NULL;
	( *aasworld ).numnodes	   = 0;

	if( ( *aasworld ).nodes ) {
		FreeMemory( ( *aasworld ).nodes );
	}

	( *aasworld ).nodes		 = NULL;
	( *aasworld ).numportals = 0;

	if( ( *aasworld ).portals ) {
		FreeMemory( ( *aasworld ).portals );
	}

	( *aasworld ).portals	 = NULL;
	( *aasworld ).numportals = 0;

	if( ( *aasworld ).portalindex ) {
		FreeMemory( ( *aasworld ).portalindex );
	}

	( *aasworld ).portalindex	  = NULL;
	( *aasworld ).portalindexsize = 0;

	if( ( *aasworld ).clusters ) {
		FreeMemory( ( *aasworld ).clusters );
	}

	( *aasworld ).clusters	  = NULL;
	( *aasworld ).numclusters = 0;
	//
	( *aasworld ).loaded	  = qfalse;
	( *aasworld ).initialized = qfalse;
	( *aasworld ).savefile	  = qfalse;
}

//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
#ifdef AASFILEDEBUG
void AAS_FileInfo()
{
	int i, n, optimized;

	botimport.Print( PRT_MESSAGE, "version = %d\n", AASVERSION );
	botimport.Print( PRT_MESSAGE, "numvertexes = %d\n", ( *aasworld ).numvertexes );
	botimport.Print( PRT_MESSAGE, "numplanes = %d\n", ( *aasworld ).numplanes );
	botimport.Print( PRT_MESSAGE, "numedges = %d\n", ( *aasworld ).numedges );
	botimport.Print( PRT_MESSAGE, "edgeindexsize = %d\n", ( *aasworld ).edgeindexsize );
	botimport.Print( PRT_MESSAGE, "numfaces = %d\n", ( *aasworld ).numfaces );
	botimport.Print( PRT_MESSAGE, "faceindexsize = %d\n", ( *aasworld ).faceindexsize );
	botimport.Print( PRT_MESSAGE, "numareas = %d\n", ( *aasworld ).numareas );
	botimport.Print( PRT_MESSAGE, "numareasettings = %d\n", ( *aasworld ).numareasettings );
	botimport.Print( PRT_MESSAGE, "reachabilitysize = %d\n", ( *aasworld ).reachabilitysize );
	botimport.Print( PRT_MESSAGE, "numnodes = %d\n", ( *aasworld ).numnodes );
	botimport.Print( PRT_MESSAGE, "numportals = %d\n", ( *aasworld ).numportals );
	botimport.Print( PRT_MESSAGE, "portalindexsize = %d\n", ( *aasworld ).portalindexsize );
	botimport.Print( PRT_MESSAGE, "numclusters = %d\n", ( *aasworld ).numclusters );

	//
	for( n = 0, i = 0; i < ( *aasworld ).numareasettings; i++ ) {
		if( ( *aasworld ).areasettings[i].areaflags & AREA_GROUNDED ) {
			n++;
		}
	}

	botimport.Print( PRT_MESSAGE, "num grounded areas = %d\n", n );
	//
	botimport.Print( PRT_MESSAGE, "planes size %d bytes\n", ( *aasworld ).numplanes * sizeof( aas_plane_t ) );
	botimport.Print( PRT_MESSAGE, "areas size %d bytes\n", ( *aasworld ).numareas * sizeof( aas_area_t ) );
	botimport.Print( PRT_MESSAGE, "areasettings size %d bytes\n", ( *aasworld ).numareasettings * sizeof( aas_areasettings_t ) );
	botimport.Print( PRT_MESSAGE, "nodes size %d bytes\n", ( *aasworld ).numnodes * sizeof( aas_node_t ) );
	botimport.Print( PRT_MESSAGE, "reachability size %d bytes\n", ( *aasworld ).reachabilitysize * sizeof( aas_reachability_t ) );
	botimport.Print( PRT_MESSAGE, "portals size %d bytes\n", ( *aasworld ).numportals * sizeof( aas_portal_t ) );
	botimport.Print( PRT_MESSAGE, "clusters size %d bytes\n", ( *aasworld ).numclusters * sizeof( aas_cluster_t ) );

	optimized = ( *aasworld ).numplanes * sizeof( aas_plane_t ) + ( *aasworld ).numareas * sizeof( aas_area_t ) + ( *aasworld ).numareasettings * sizeof( aas_areasettings_t ) +
				( *aasworld ).numnodes * sizeof( aas_node_t ) + ( *aasworld ).reachabilitysize * sizeof( aas_reachability_t ) + ( *aasworld ).numportals * sizeof( aas_portal_t ) +
				( *aasworld ).numclusters * sizeof( aas_cluster_t );
	botimport.Print( PRT_MESSAGE, "optimzed size %d KB\n", optimized >> 10 );
}

#endif // AASFILEDEBUG

/*!
	\brief Loads a specified lump from an AAS file into newly allocated memory.

	If the requested lump size is zero the function immediately returns NULL.
	When the caller's expected next offset does not match the supplied offset a warning is issued. The function attempts to seek to the correct position; if that fails an error is logged,
   diagnostic dumps are output, and the file is closed before returning NULL. Memory for the lump is obtained from GetClearedHunkMemory and the data is read directly into this buffer. The supplied
   lastoffset pointer is updated to reflect the position following the read. The returned buffer occupies persistent hunk memory and is returned as a raw byte array; the caller is responsible for
   interpreting it as the appropriate structure type.

	\param fp file handle from which the lump is read
	\param offset file position where the lump begins
	\param length number of bytes to read
	\param lastoffset pointer to the last read offset, updated after the function executes
	\return pointer to the allocated buffer containing the lump data, or NULL if the length is zero or an error occurs
*/
char* AAS_LoadAASLump( fileHandle_t fp, int offset, int length, int* lastoffset )
{
	char* buf;

	//
	if( !length ) {
		return NULL;
	}

	// seek to the data
	if( offset != *lastoffset ) {
		botimport.Print( PRT_WARNING, "AAS file not sequentially read\n" );

		if( botimport.FS_Seek( fp, offset, FS_SEEK_SET ) ) {
			AAS_Error( "can't seek to aas lump\n" );
			AAS_DumpAASData();
			botimport.FS_FCloseFile( fp );
			return 0;
		}
	}

	// allocate memory
	buf = ( char* )GetClearedHunkMemory( length + 1 );

	// read the data
	if( length ) {
		botimport.FS_Read( buf, length, fp );
		*lastoffset += length;
	}

	return buf;
}

/*!
	\brief XORs each byte of the buffer with a value derived from its index multiplied by 119

	The function iterates through the provided buffer of the given size.
	For each element, it XORs the byte with a byte value computed from the element's index multiplied by 119 and then cast to unsigned char.
	This operation effectively scrambles or de‑scrambles the data, depending on whether the same function is applied again.
	The function does not allocate additional memory and works in-place on the supplied array.

	NOTE: The multiplier 119 is hard‑coded and therefore the result is deterministic.
	No bounds checking is performed beyond the size parameter, so callers must ensure the buffer has at least size bytes.

	\param data pointer to the byte array to be processed
	\param size number of bytes in the array to be processed
*/
void AAS_DData( unsigned char* data, int size )
{
	int i;

	for( i = 0; i < size; i++ ) {
		data[i] ^= ( unsigned char )i * 119;
	}
}

/*!
	\brief Loads an AAS file from the specified filename and initializes the AAS world data structures.

	This function loads an AAS (Automated Adjacency Search) file for use in navigation and pathfinding. It opens the file, validates the header information including the file ID and version, and then
   loads all the individual data lumps contained in the AAS file. The function performs checksum validation against the current BSP file to ensure compatibility. It also handles error conditions by
   returning appropriate error codes when file operations fail or when the AAS file is incompatible.

	\param filename Name of the .aas file to load, relative to the maps directory
	\return An integer error code indicating success or the type of failure encountered. Returns BLERR_NOERROR on success, or one of several BLERR_ constants indicating specific types of errors such
   as file opening failures, incorrect file IDs, version mismatches, or loading errors. \throws TODO: clarify whether this function throws exceptions or uses assertions, as the provided implementation
   does not show explicit throws or assertions.
*/
int AAS_LoadAASFile( char* filename )
{
	fileHandle_t fp;
	aas_header_t header;
	int			 offset, length, lastoffset;

	botimport.Print( PRT_MESSAGE, "trying to load %s\n", filename );
	// dump current loaded aas file
	AAS_DumpAASData();
	// open the file
	botimport.FS_FOpenFile( filename, &fp, FS_READ );

	if( !fp ) {
		AAS_Error( "can't open %s\n", filename );
		return BLERR_CANNOTOPENAASFILE;
	}

	// read the header
	botimport.FS_Read( &header, sizeof( aas_header_t ), fp );
	lastoffset = sizeof( aas_header_t );
	// check header identification
	header.ident = LittleLong( header.ident );

	if( header.ident != AASID ) {
		AAS_Error( "%s is not an AAS file\n", filename );
		botimport.FS_FCloseFile( fp );
		return BLERR_WRONGAASFILEID;
	}

	// check the version
	header.version = LittleLong( header.version );

	//
	if( header.version != AASVERSION ) {
		AAS_Error( "aas file %s is version %i, not %i\n", filename, header.version, AASVERSION );
		botimport.FS_FCloseFile( fp );
		return BLERR_WRONGAASFILEVERSION;
	}

	//
	if( header.version == AASVERSION ) {
		AAS_DData( ( unsigned char* )&header + 8, sizeof( aas_header_t ) - 8 );
	}

	//
	( *aasworld ).bspchecksum = atoi( LibVarGetString( "sv_mapChecksum" ) );

	if( LittleLong( header.bspchecksum ) != ( *aasworld ).bspchecksum ) {
		AAS_Error( "aas file %s is out of date\n", filename );
		botimport.FS_FCloseFile( fp );
		return BLERR_WRONGAASFILEVERSION;
	}

	// load the lumps:
	// bounding boxes
	offset					= LittleLong( header.lumps[AASLUMP_BBOXES].fileofs );
	length					= LittleLong( header.lumps[AASLUMP_BBOXES].filelen );
	( *aasworld ).bboxes	= ( aas_bbox_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset );
	( *aasworld ).numbboxes = length / sizeof( aas_bbox_t );

	if( ( *aasworld ).numbboxes && !( *aasworld ).bboxes ) {
		return BLERR_CANNOTREADAASLUMP;
	}

	// vertexes
	offset					  = LittleLong( header.lumps[AASLUMP_VERTEXES].fileofs );
	length					  = LittleLong( header.lumps[AASLUMP_VERTEXES].filelen );
	( *aasworld ).vertexes	  = ( aas_vertex_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset );
	( *aasworld ).numvertexes = length / sizeof( aas_vertex_t );

	if( ( *aasworld ).numvertexes && !( *aasworld ).vertexes ) {
		return BLERR_CANNOTREADAASLUMP;
	}

	// planes
	offset					= LittleLong( header.lumps[AASLUMP_PLANES].fileofs );
	length					= LittleLong( header.lumps[AASLUMP_PLANES].filelen );
	( *aasworld ).planes	= ( aas_plane_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset );
	( *aasworld ).numplanes = length / sizeof( aas_plane_t );

	if( ( *aasworld ).numplanes && !( *aasworld ).planes ) {
		return BLERR_CANNOTREADAASLUMP;
	}

	// edges
	offset				   = LittleLong( header.lumps[AASLUMP_EDGES].fileofs );
	length				   = LittleLong( header.lumps[AASLUMP_EDGES].filelen );
	( *aasworld ).edges	   = ( aas_edge_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset );
	( *aasworld ).numedges = length / sizeof( aas_edge_t );

	if( ( *aasworld ).numedges && !( *aasworld ).edges ) {
		return BLERR_CANNOTREADAASLUMP;
	}

	// edgeindex
	offset						= LittleLong( header.lumps[AASLUMP_EDGEINDEX].fileofs );
	length						= LittleLong( header.lumps[AASLUMP_EDGEINDEX].filelen );
	( *aasworld ).edgeindex		= ( aas_edgeindex_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset );
	( *aasworld ).edgeindexsize = length / sizeof( aas_edgeindex_t );

	if( ( *aasworld ).edgeindexsize && !( *aasworld ).edgeindex ) {
		return BLERR_CANNOTREADAASLUMP;
	}

	// faces
	offset				   = LittleLong( header.lumps[AASLUMP_FACES].fileofs );
	length				   = LittleLong( header.lumps[AASLUMP_FACES].filelen );
	( *aasworld ).faces	   = ( aas_face_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset );
	( *aasworld ).numfaces = length / sizeof( aas_face_t );

	if( ( *aasworld ).numfaces && !( *aasworld ).faces ) {
		return BLERR_CANNOTREADAASLUMP;
	}

	// faceindex
	offset						= LittleLong( header.lumps[AASLUMP_FACEINDEX].fileofs );
	length						= LittleLong( header.lumps[AASLUMP_FACEINDEX].filelen );
	( *aasworld ).faceindex		= ( aas_faceindex_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset );
	( *aasworld ).faceindexsize = length / sizeof( int );

	if( ( *aasworld ).faceindexsize && !( *aasworld ).faceindex ) {
		return BLERR_CANNOTREADAASLUMP;
	}

	// convex areas
	offset				   = LittleLong( header.lumps[AASLUMP_AREAS].fileofs );
	length				   = LittleLong( header.lumps[AASLUMP_AREAS].filelen );
	( *aasworld ).areas	   = ( aas_area_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset );
	( *aasworld ).numareas = length / sizeof( aas_area_t );

	if( ( *aasworld ).numareas && !( *aasworld ).areas ) {
		return BLERR_CANNOTREADAASLUMP;
	}

	// area settings
	offset						  = LittleLong( header.lumps[AASLUMP_AREASETTINGS].fileofs );
	length						  = LittleLong( header.lumps[AASLUMP_AREASETTINGS].filelen );
	( *aasworld ).areasettings	  = ( aas_areasettings_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset );
	( *aasworld ).numareasettings = length / sizeof( aas_areasettings_t );

	if( ( *aasworld ).numareasettings && !( *aasworld ).areasettings ) {
		return BLERR_CANNOTREADAASLUMP;
	}

	// reachability list
	offset						   = LittleLong( header.lumps[AASLUMP_REACHABILITY].fileofs );
	length						   = LittleLong( header.lumps[AASLUMP_REACHABILITY].filelen );
	( *aasworld ).reachability	   = ( aas_reachability_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset );
	( *aasworld ).reachabilitysize = length / sizeof( aas_reachability_t );

	if( ( *aasworld ).reachabilitysize && !( *aasworld ).reachability ) {
		return BLERR_CANNOTREADAASLUMP;
	}

	// nodes
	offset				   = LittleLong( header.lumps[AASLUMP_NODES].fileofs );
	length				   = LittleLong( header.lumps[AASLUMP_NODES].filelen );
	( *aasworld ).nodes	   = ( aas_node_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset );
	( *aasworld ).numnodes = length / sizeof( aas_node_t );

	if( ( *aasworld ).numnodes && !( *aasworld ).nodes ) {
		return BLERR_CANNOTREADAASLUMP;
	}

	// cluster portals
	offset					 = LittleLong( header.lumps[AASLUMP_PORTALS].fileofs );
	length					 = LittleLong( header.lumps[AASLUMP_PORTALS].filelen );
	( *aasworld ).portals	 = ( aas_portal_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset );
	( *aasworld ).numportals = length / sizeof( aas_portal_t );

	if( ( *aasworld ).numportals && !( *aasworld ).portals ) {
		return BLERR_CANNOTREADAASLUMP;
	}

	// cluster portal index
	offset						  = LittleLong( header.lumps[AASLUMP_PORTALINDEX].fileofs );
	length						  = LittleLong( header.lumps[AASLUMP_PORTALINDEX].filelen );
	( *aasworld ).portalindex	  = ( aas_portalindex_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset );
	( *aasworld ).portalindexsize = length / sizeof( aas_portalindex_t );

	if( ( *aasworld ).portalindexsize && !( *aasworld ).portalindex ) {
		return BLERR_CANNOTREADAASLUMP;
	}

	// clusters
	offset					  = LittleLong( header.lumps[AASLUMP_CLUSTERS].fileofs );
	length					  = LittleLong( header.lumps[AASLUMP_CLUSTERS].filelen );
	( *aasworld ).clusters	  = ( aas_cluster_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset );
	( *aasworld ).numclusters = length / sizeof( aas_cluster_t );

	if( ( *aasworld ).numclusters && !( *aasworld ).clusters ) {
		return BLERR_CANNOTREADAASLUMP;
	}

	// swap everything
	AAS_SwapAASData();
	// aas file is loaded
	( *aasworld ).loaded = qtrue;
	// close the file
	botimport.FS_FCloseFile( fp );
	//
#ifdef AASFILEDEBUG
	AAS_FileInfo();
#endif // AASFILEDEBUG
	//
	return BLERR_NOERROR;
}

static int AAS_WriteAASLump_offset;

/*!
	\brief Writes a lump to an AAS file and updates the corresponding header entry.

	The function records the file offset and length for a specified lump in the AAS header, writes the lump data to the file if any, advances the write offset, and returns a success flag. File offsets
   and lengths are converted to little‑endian format before storage. The lump information is stored in the header’s lumps array at the index supplied by lumpnum.

	\param fp File handle to the open AAS file
	\param h Pointer to the AAS header structure that contains lump metadata
	\param lumpnum Index of the lump entry within the header to update
	\param data Pointer to the data buffer to be written to the file
	\param length Size of the data buffer in bytes
	\return qtrue (non‑zero) on success, qfalse (zero) otherwise
*/
int		   AAS_WriteAASLump( fileHandle_t fp, aas_header_t* h, int lumpnum, void* data, int length )
{
	aas_lump_t* lump;

	lump = &h->lumps[lumpnum];

	lump->fileofs = LittleLong( AAS_WriteAASLump_offset ); // LittleLong(ftell(fp));
	lump->filelen = LittleLong( length );

	if( length > 0 ) {
		botimport.FS_Write( data, length, fp );
	}

	AAS_WriteAASLump_offset += length;

	return qtrue;
}

qboolean AAS_WriteAASFile( char* filename )
{
	aas_header_t header;
	fileHandle_t fp;

	botimport.Print( PRT_MESSAGE, "writing %s\n", filename );
	// swap the aas data
	AAS_SwapAASData();
	// initialize the file header
	memset( &header, 0, sizeof( aas_header_t ) );
	header.ident	   = LittleLong( AASID );
	header.version	   = LittleLong( AASVERSION );
	header.bspchecksum = LittleLong( ( *aasworld ).bspchecksum );
	// open a new file
	botimport.FS_FOpenFile( filename, &fp, FS_WRITE );

	if( !fp ) {
		botimport.Print( PRT_ERROR, "error opening %s\n", filename );
		return qfalse;
	}

	// write the header
	botimport.FS_Write( &header, sizeof( aas_header_t ), fp );
	AAS_WriteAASLump_offset = sizeof( aas_header_t );

	// add the data lumps to the file
	if( !AAS_WriteAASLump( fp, &header, AASLUMP_BBOXES, ( *aasworld ).bboxes, ( *aasworld ).numbboxes * sizeof( aas_bbox_t ) ) ) {
		return qfalse;
	}

	if( !AAS_WriteAASLump( fp, &header, AASLUMP_VERTEXES, ( *aasworld ).vertexes, ( *aasworld ).numvertexes * sizeof( aas_vertex_t ) ) ) {
		return qfalse;
	}

	if( !AAS_WriteAASLump( fp, &header, AASLUMP_PLANES, ( *aasworld ).planes, ( *aasworld ).numplanes * sizeof( aas_plane_t ) ) ) {
		return qfalse;
	}

	if( !AAS_WriteAASLump( fp, &header, AASLUMP_EDGES, ( *aasworld ).edges, ( *aasworld ).numedges * sizeof( aas_edge_t ) ) ) {
		return qfalse;
	}

	if( !AAS_WriteAASLump( fp, &header, AASLUMP_EDGEINDEX, ( *aasworld ).edgeindex, ( *aasworld ).edgeindexsize * sizeof( aas_edgeindex_t ) ) ) {
		return qfalse;
	}

	if( !AAS_WriteAASLump( fp, &header, AASLUMP_FACES, ( *aasworld ).faces, ( *aasworld ).numfaces * sizeof( aas_face_t ) ) ) {
		return qfalse;
	}

	if( !AAS_WriteAASLump( fp, &header, AASLUMP_FACEINDEX, ( *aasworld ).faceindex, ( *aasworld ).faceindexsize * sizeof( aas_faceindex_t ) ) ) {
		return qfalse;
	}

	if( !AAS_WriteAASLump( fp, &header, AASLUMP_AREAS, ( *aasworld ).areas, ( *aasworld ).numareas * sizeof( aas_area_t ) ) ) {
		return qfalse;
	}

	if( !AAS_WriteAASLump( fp, &header, AASLUMP_AREASETTINGS, ( *aasworld ).areasettings, ( *aasworld ).numareasettings * sizeof( aas_areasettings_t ) ) ) {
		return qfalse;
	}

	if( !AAS_WriteAASLump( fp, &header, AASLUMP_REACHABILITY, ( *aasworld ).reachability, ( *aasworld ).reachabilitysize * sizeof( aas_reachability_t ) ) ) {
		return qfalse;
	}

	if( !AAS_WriteAASLump( fp, &header, AASLUMP_NODES, ( *aasworld ).nodes, ( *aasworld ).numnodes * sizeof( aas_node_t ) ) ) {
		return qfalse;
	}

	if( !AAS_WriteAASLump( fp, &header, AASLUMP_PORTALS, ( *aasworld ).portals, ( *aasworld ).numportals * sizeof( aas_portal_t ) ) ) {
		return qfalse;
	}

	if( !AAS_WriteAASLump( fp, &header, AASLUMP_PORTALINDEX, ( *aasworld ).portalindex, ( *aasworld ).portalindexsize * sizeof( aas_portalindex_t ) ) ) {
		return qfalse;
	}

	if( !AAS_WriteAASLump( fp, &header, AASLUMP_CLUSTERS, ( *aasworld ).clusters, ( *aasworld ).numclusters * sizeof( aas_cluster_t ) ) ) {
		return qfalse;
	}

	// rewrite the header with the added lumps
	botimport.FS_Seek( fp, 0, FS_SEEK_SET );
	botimport.FS_Write( &header, sizeof( aas_header_t ), fp );
	// close the file
	botimport.FS_FCloseFile( fp );
	return qtrue;
}
