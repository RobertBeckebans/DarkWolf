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
	\file engine/botlib/be_aas_debug.cpp
	\brief Implements visualization support for the AAS (Area A* Search) system, enabling developers to see polygons, lines, crosses, arrows, and area reachabilities during debugging sessions.
	\note doxygenix: sha256=0be8bea3c45b21e7c5183625c97637f513873d08e95b7fb28b94c67936411111

	\par File Purpose
	- Implements visualization support for the AAS (Area A* Search) system, enabling developers to see polygons, lines, crosses, arrows, and area reachabilities during debugging sessions.

	\par Core Responsibilities
	- Provide runtime visualization helpers for AAS debugging.
	- Manage creation, reuse, and deletion of debug line and polygon handles.
	- Draw geometric primitives (cross, arrow, plane cross, bounding boxes) to aid developers in understanding navigation data.
	- Translate travel‑type constants into human‑readable messages.
	- Compute and display predicted client movement for reachabilities.

	\par Key Types and Functions
	- debuglines — array of active debug‑line identifiers.
	- debuglinevisible — parallel flags marking whether a line handle is currently shown.
	- debugpolygons — array of active debug‑polygon identifiers.
	- AAS_ClearShownPolygons() — deletes all active polygons and clears the handle array.
	- AAS_ShowPolygon(int color, int numpoints, vec3_t* points) — creates the first free polygon, sets its vertices, and displays it.
	- AAS_ClearShownDebugLines() — deletes all active lines and marks them invisible.
	- AAS_DebugLine(vec3_t start, vec3_t end, int color) — reuses or creates a line, makes it visible, and draws a segment.
	- AAS_PermanentLine(vec3_t start, vec3_t end, int color) — creates a line that remains until explicitly deleted.
	- AAS_DrawArrow(vec3_t start, vec3_t end, int linecolor, int arrowcolor) — renders an arrow by drawing shaft and head lines.
	- AAS_DrawCross(vec3_t origin, float size, int color) — draws a 3‑D cross at the origin.
	- AAS_DrawPlaneCross(vec3_t point, vec3_t normal, float dist, int type, int color) — draws a cross in an arbitrary plane.
	- AAS_DrawPermanentCross(vec3_t origin, float size, int color) — permanent 3‑D cross using `AAS_DebugLine`.
	- AAS_ShowAreaPolygons(int areanum, int color, int groundfacesonly) — visualizes face polygons belonging to an area.
	- AAS_ShowArea(int areanum, int groundfacesonly) — draws edges of all faces in an area.
	- AAS_ShowFace(int facenum) — visualizes the edges and normal of a single face.
	- AAS_ShowFacePolygon(int facenum, int color, int flip) — collects face vertices and passes them to `AAS_ShowPolygon`.
	- AAS_ShowBoundingBox(vec3_t origin, vec3_t mins, vec3_t maxs) — draws the edges of an axis‑aligned bounding box.
	- AAS_PrintTravelType(int traveltype) — logs a readable name for a travel‑type constant (no‑op in non‑DEBUG builds).
	- AAS_ShowReachability(aas_reachability_t* reach) — displays reachability polygons, arrows, and predicted movement.
	- AAS_ShowReachableAreas(int areanum) — iterates over all reachable areas, throttles display updates, prints travel type/time, and calls `AAS_ShowReachability`.

	\par Control Flow
	- Global arrays hold temporary line and polygon handles (`debuglines`, `debuglinevisible`, `debugpolygons`).
- `AAS_ClearShownPolygons` / `AAS_ClearShownDebugLines` iterate these arrays to delete existing handles via `botimport` API and reset the entries.
- `AAS_ShowPolygon`, `AAS_ShowFacePolygon`, `AAS_ShowAreaPolygons`, etc., locate a free index, create a handle with `botimport.DebugPolygonCreate`/`DebugLineCreate`, and immediately show it with `botimport.DebugPolygonShow`/`DebugLineShow`.
- `AAS_DrawArrow`, `AAS_DrawCross`, `AAS_DrawPlaneCross`, `AAS_DrawPermanentCross`, and `AAS_ShowBoundingBox` compute geometry, then use the debug line utilities to render static visual markers.
- `AAS_ShowReachability` uses travel type logic to compute velocities, calls `AAS_PredictClientMovement`, and renders arrows/crosses to illustrate the expected movement for each reachability.
- `AAS_ShowReachableAreas` maintains a static index over a area's reachable list, throttles updates to every 1.5 s via `AAS_Time()`, and drives `AAS_ShowReachability`.
- All functions indirectly rely on the external `aasworld` data structure and bot imports.

	\par Dependencies
	- "q_shared.h" – core engine types and definitions.
	- "l_memory.h", "l_script.h", "l_precomp.h", "l_struct.h", "l_libvar.h" – low‑level engine utilities.
	- "aasfile.h", "botshared/botlib.h", "botshared/be_aas.h" – AAS data structures and bot library interfaces.
	- "be_interface.h", "be_aas_funcs.h", "be_aas_def.h" – bot engine specific interfaces.

	\par How It Fits
	- Acts as a debugging extension within the bot library; it does not alter navigation logic, only provides visual markers.
	- The functions are invoked from debugging or profiling tools, or from other parts of the bot engine when stepping through reachabilities.
	- Keeps the core bot navigation code isolated from rendering concerns by interfacing with the `botimport` debug API.
*/

/*****************************************************************************
 * name:		be_aas_debug.c
 *
 * desc:		AAS debug code
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_libvar.h"
#include "aasfile.h"
#include "botshared/botlib.h"
#include "botshared/be_aas.h"
#include "be_interface.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

#define MAX_DEBUGLINES	  1024
#define MAX_DEBUGPOLYGONS 128

int		   debuglines[MAX_DEBUGLINES];
int		   debuglinevisible[MAX_DEBUGLINES];
int		   numdebuglines;

static int debugpolygons[MAX_DEBUGPOLYGONS];

void	   AAS_ClearShownPolygons()
{
	int i;

	//*
	for( i = 0; i < MAX_DEBUGPOLYGONS; i++ ) {
		if( debugpolygons[i] ) {
			botimport.DebugPolygonDelete( debugpolygons[i] );
		}

		debugpolygons[i] = 0;
	}

	//*/
	/*
		for (i = 0; i < MAX_DEBUGPOLYGONS; i++)
		{
			botimport.DebugPolygonDelete(i);
			debugpolygons[i] = 0;
		}
	*/
}

/*!
	\brief Creates a debug polygon using the given color and vertices.

	Iterates through the global debug polygon array and, when it finds a free slot, calls botimport.DebugPolygonCreate with the specified color, number of points, and vertex list. It only creates a
   polygon in the first available slot and then stops searching.

	\param color The color index for the new debug polygon
	\param numpoints The number of vertices that define the polygon
	\param points A pointer to an array of vec3_t structures containing the vertex coordinates
*/
void AAS_ShowPolygon( int color, int numpoints, vec3_t* points )
{
	int i;

	for( i = 0; i < MAX_DEBUGPOLYGONS; i++ ) {
		if( !debugpolygons[i] ) {
			debugpolygons[i] = botimport.DebugPolygonCreate( color, numpoints, points );
			break;
		}
	}
}

void AAS_ClearShownDebugLines()
{
	int i;

	// make all lines invisible
	for( i = 0; i < MAX_DEBUGLINES; i++ ) {
		if( debuglines[i] ) {
			// botimport.DebugLineShow(debuglines[i], NULL, NULL, LINECOLOR_NONE);
			botimport.DebugLineDelete( debuglines[i] );
			debuglines[i]		= 0;
			debuglinevisible[i] = qfalse;
		}
	}
}

void AAS_DebugLine( vec3_t start, vec3_t end, int color )
{
	int line;

	for( line = 0; line < MAX_DEBUGLINES; line++ ) {
		if( !debuglines[line] ) {
			debuglines[line]	   = botimport.DebugLineCreate();
			debuglinevisible[line] = qfalse;
			numdebuglines++;
		}

		if( !debuglinevisible[line] ) {
			botimport.DebugLineShow( debuglines[line], start, end, color );
			debuglinevisible[line] = qtrue;
			return;
		}
	}
}

void AAS_PermanentLine( vec3_t start, vec3_t end, int color )
{
	int line;

	line = botimport.DebugLineCreate();
	botimport.DebugLineShow( line, start, end, color );
}

void AAS_DrawPermanentCross( vec3_t origin, float size, int color )
{
	int	   i, debugline;
	vec3_t start, end;

	for( i = 0; i < 3; i++ ) {
		VectorCopy( origin, start );
		start[i] += size;
		VectorCopy( origin, end );
		end[i] -= size;
		AAS_DebugLine( start, end, color );
		debugline = botimport.DebugLineCreate();
		botimport.DebugLineShow( debugline, start, end, color );
	}
}

void AAS_DrawPlaneCross( vec3_t point, vec3_t normal, float dist, int type, int color )
{
	int	   n0, n1, n2, j, line, lines[2];
	vec3_t start1, end1, start2, end2;

	// make a cross in the hit plane at the hit point
	VectorCopy( point, start1 );
	VectorCopy( point, end1 );
	VectorCopy( point, start2 );
	VectorCopy( point, end2 );

	n0 = type % 3;
	n1 = ( type + 1 ) % 3;
	n2 = ( type + 2 ) % 3;
	start1[n1] -= 6;
	start1[n2] -= 6;
	end1[n1] += 6;
	end1[n2] += 6;
	start2[n1] += 6;
	start2[n2] -= 6;
	end2[n1] -= 6;
	end2[n2] += 6;

	start1[n0] = ( dist - ( start1[n1] * normal[n1] + start1[n2] * normal[n2] ) ) / normal[n0];
	end1[n0]   = ( dist - ( end1[n1] * normal[n1] + end1[n2] * normal[n2] ) ) / normal[n0];
	start2[n0] = ( dist - ( start2[n1] * normal[n1] + start2[n2] * normal[n2] ) ) / normal[n0];
	end2[n0]   = ( dist - ( end2[n1] * normal[n1] + end2[n2] * normal[n2] ) ) / normal[n0];

	for( j = 0, line = 0; j < 2 && line < MAX_DEBUGLINES; line++ ) {
		if( !debuglines[line] ) {
			debuglines[line]	   = botimport.DebugLineCreate();
			lines[j++]			   = debuglines[line];
			debuglinevisible[line] = qtrue;
			numdebuglines++;

		} else if( !debuglinevisible[line] ) {
			lines[j++]			   = debuglines[line];
			debuglinevisible[line] = qtrue;
		}
	}

	botimport.DebugLineShow( lines[0], start1, end1, color );
	botimport.DebugLineShow( lines[1], start2, end2, color );
}

void AAS_ShowBoundingBox( vec3_t origin, vec3_t mins, vec3_t maxs )
{
	vec3_t bboxcorners[8];
	int	   lines[3];
	int	   i, j, line;

	// upper corners
	bboxcorners[0][0] = origin[0] + maxs[0];
	bboxcorners[0][1] = origin[1] + maxs[1];
	bboxcorners[0][2] = origin[2] + maxs[2];
	//
	bboxcorners[1][0] = origin[0] + mins[0];
	bboxcorners[1][1] = origin[1] + maxs[1];
	bboxcorners[1][2] = origin[2] + maxs[2];
	//
	bboxcorners[2][0] = origin[0] + mins[0];
	bboxcorners[2][1] = origin[1] + mins[1];
	bboxcorners[2][2] = origin[2] + maxs[2];
	//
	bboxcorners[3][0] = origin[0] + maxs[0];
	bboxcorners[3][1] = origin[1] + mins[1];
	bboxcorners[3][2] = origin[2] + maxs[2];
	// lower corners
	memcpy( bboxcorners[4], bboxcorners[0], sizeof( vec3_t ) * 4 );

	for( i = 0; i < 4; i++ ) {
		bboxcorners[4 + i][2] = origin[2] + mins[2];
	}

	// draw bounding box
	for( i = 0; i < 4; i++ ) {
		for( j = 0, line = 0; j < 3 && line < MAX_DEBUGLINES; line++ ) {
			if( !debuglines[line] ) {
				debuglines[line]	   = botimport.DebugLineCreate();
				lines[j++]			   = debuglines[line];
				debuglinevisible[line] = qtrue;
				numdebuglines++;

			} else if( !debuglinevisible[line] ) {
				lines[j++]			   = debuglines[line];
				debuglinevisible[line] = qtrue;
			}
		}

		// top plane
		botimport.DebugLineShow( lines[0], bboxcorners[i], bboxcorners[( i + 1 ) & 3], LINECOLOR_RED );
		// bottom plane
		botimport.DebugLineShow( lines[1], bboxcorners[4 + i], bboxcorners[4 + ( ( i + 1 ) & 3 )], LINECOLOR_RED );
		// vertical lines
		botimport.DebugLineShow( lines[2], bboxcorners[i], bboxcorners[4 + i], LINECOLOR_RED );
	}
}

void AAS_ShowFace( int facenum )
{
	int			 i, color, edgenum;
	aas_edge_t*	 edge;
	aas_face_t*	 face;
	aas_plane_t* plane;
	vec3_t		 start, end;

	color = LINECOLOR_YELLOW;

	// check if face number is in range
	if( facenum >= ( *aasworld ).numfaces ) {
		botimport.Print( PRT_ERROR, "facenum %d out of range\n", facenum );
	}

	face = &( *aasworld ).faces[facenum];

	// walk through the edges of the face
	for( i = 0; i < face->numedges; i++ ) {
		// edge number
		edgenum = abs( ( *aasworld ).edgeindex[face->firstedge + i] );

		// check if edge number is in range
		if( edgenum >= ( *aasworld ).numedges ) {
			botimport.Print( PRT_ERROR, "edgenum %d out of range\n", edgenum );
		}

		edge = &( *aasworld ).edges[edgenum];

		if( color == LINECOLOR_RED ) {
			color = LINECOLOR_GREEN;

		} else if( color == LINECOLOR_GREEN ) {
			color = LINECOLOR_BLUE;

		} else if( color == LINECOLOR_BLUE ) {
			color = LINECOLOR_YELLOW;

		} else {
			color = LINECOLOR_RED;
		}

		AAS_DebugLine( ( *aasworld ).vertexes[edge->v[0]], ( *aasworld ).vertexes[edge->v[1]], color );
	}

	plane	= &( *aasworld ).planes[face->planenum];
	edgenum = abs( ( *aasworld ).edgeindex[face->firstedge] );
	edge	= &( *aasworld ).edges[edgenum];
	VectorCopy( ( *aasworld ).vertexes[edge->v[0]], start );
	VectorMA( start, 20, plane->normal, end );
	AAS_DebugLine( start, end, LINECOLOR_RED );
}

/*!
	\brief Displays a face polygon in the debug view, optionally reversing the vertex order

	This function retrieves a face by its index from the AAS world data. It first verifies that the index is within bounds, printing an error if not. It then walks through the edges of the face,
   collecting the vertex positions for each edge into an array. When the flip flag is set, the edges are processed in reverse order to reverse the winding of the polygon. Finally, the collected points
   are passed to AAS_ShowPolygon together with the desired color to render the polygon.

	\param facenum Index of the face to display
	\param color Color value used to draw the polygon
	\param flip If non-zero, the vertices are processed in reverse order
*/
void AAS_ShowFacePolygon( int facenum, int color, int flip )
{
	int			i, edgenum, numpoints;
	vec3_t		points[128];
	aas_edge_t* edge;
	aas_face_t* face;

	// check if face number is in range
	if( facenum >= ( *aasworld ).numfaces ) {
		botimport.Print( PRT_ERROR, "facenum %d out of range\n", facenum );
	}

	face = &( *aasworld ).faces[facenum];
	// walk through the edges of the face
	numpoints = 0;

	if( flip ) {
		for( i = face->numedges - 1; i >= 0; i-- ) {
			// edge number
			edgenum = ( *aasworld ).edgeindex[face->firstedge + i];
			edge	= &( *aasworld ).edges[abs( edgenum )];
			VectorCopy( ( *aasworld ).vertexes[edge->v[edgenum < 0]], points[numpoints] );
			numpoints++;
		}

	} else {
		for( i = 0; i < face->numedges; i++ ) {
			// edge number
			edgenum = ( *aasworld ).edgeindex[face->firstedge + i];
			edge	= &( *aasworld ).edges[abs( edgenum )];
			VectorCopy( ( *aasworld ).vertexes[edge->v[edgenum < 0]], points[numpoints] );
			numpoints++;
		}
	}

	AAS_ShowPolygon( color, numpoints, points );
}

void AAS_ShowArea( int areanum, int groundfacesonly )
{
	int			areaedges[MAX_DEBUGLINES];
	int			numareaedges, i, j, n, color = 0, line;
	int			facenum, edgenum;
	aas_area_t* area;
	aas_face_t* face;
	aas_edge_t* edge;

	//
	numareaedges = 0;

	//
	if( areanum < 0 || areanum >= ( *aasworld ).numareas ) {
		botimport.Print( PRT_ERROR, "area %d out of range [0, %d]\n", areanum, ( *aasworld ).numareas );
		return;
	}

	// pointer to the convex area
	area = &( *aasworld ).areas[areanum];

	// walk through the faces of the area
	for( i = 0; i < area->numfaces; i++ ) {
		facenum = abs( ( *aasworld ).faceindex[area->firstface + i] );

		// check if face number is in range
		if( facenum >= ( *aasworld ).numfaces ) {
			botimport.Print( PRT_ERROR, "facenum %d out of range\n", facenum );
		}

		face = &( *aasworld ).faces[facenum];

		// ground faces only
		if( groundfacesonly ) {
			if( !( face->faceflags & ( FACE_GROUND | FACE_LADDER ) ) ) {
				continue;
			}
		}

		// walk through the edges of the face
		for( j = 0; j < face->numedges; j++ ) {
			// edge number
			edgenum = abs( ( *aasworld ).edgeindex[face->firstedge + j] );

			// check if edge number is in range
			if( edgenum >= ( *aasworld ).numedges ) {
				botimport.Print( PRT_ERROR, "edgenum %d out of range\n", edgenum );
			}

			// check if the edge is stored already
			for( n = 0; n < numareaedges; n++ ) {
				if( areaedges[n] == edgenum ) {
					break;
				}
			}

			if( n == numareaedges && numareaedges < MAX_DEBUGLINES ) {
				areaedges[numareaedges++] = edgenum;
			}
		}

		// AAS_ShowFace(facenum);
	}

	// draw all the edges
	for( n = 0; n < numareaedges; n++ ) {
		for( line = 0; line < MAX_DEBUGLINES; line++ ) {
			if( !debuglines[line] ) {
				debuglines[line]	   = botimport.DebugLineCreate();
				debuglinevisible[line] = qfalse;
				numdebuglines++;
			}

			if( !debuglinevisible[line] ) {
				break;
			}
		}

		if( line >= MAX_DEBUGLINES ) {
			return;
		}

		edge = &( *aasworld ).edges[areaedges[n]];

		if( color == LINECOLOR_RED ) {
			color = LINECOLOR_BLUE;

		} else if( color == LINECOLOR_BLUE ) {
			color = LINECOLOR_GREEN;

		} else if( color == LINECOLOR_GREEN ) {
			color = LINECOLOR_YELLOW;

		} else {
			color = LINECOLOR_RED;
		}

		botimport.DebugLineShow( debuglines[line], ( *aasworld ).vertexes[edge->v[0]], ( *aasworld ).vertexes[edge->v[1]], color );
		debuglinevisible[line] = qtrue;
	} // end for*/
}

void AAS_ShowAreaPolygons( int areanum, int color, int groundfacesonly )
{
	int			i, facenum;
	aas_area_t* area;
	aas_face_t* face;

	//
	if( areanum < 0 || areanum >= ( *aasworld ).numareas ) {
		botimport.Print( PRT_ERROR, "area %d out of range [0, %d]\n", areanum, ( *aasworld ).numareas );
		return;
	}

	// pointer to the convex area
	area = &( *aasworld ).areas[areanum];

	// walk through the faces of the area
	for( i = 0; i < area->numfaces; i++ ) {
		facenum = abs( ( *aasworld ).faceindex[area->firstface + i] );

		// check if face number is in range
		if( facenum >= ( *aasworld ).numfaces ) {
			botimport.Print( PRT_ERROR, "facenum %d out of range\n", facenum );
		}

		face = &( *aasworld ).faces[facenum];

		// ground faces only
		if( groundfacesonly ) {
			if( !( face->faceflags & ( FACE_GROUND | FACE_LADDER ) ) ) {
				continue;
			}
		}

		AAS_ShowFacePolygon( facenum, color, face->frontarea != areanum );
	}
}

void AAS_DrawCross( vec3_t origin, float size, int color )
{
	int	   i;
	vec3_t start, end;

	for( i = 0; i < 3; i++ ) {
		VectorCopy( origin, start );
		start[i] += size;
		VectorCopy( origin, end );
		end[i] -= size;
		AAS_DebugLine( start, end, color );
	}
}

void AAS_PrintTravelType( int traveltype )
{
#ifdef DEBUG
	char* str;

	//
	switch( traveltype ) {
		case TRAVEL_INVALID:
			str = "TRAVEL_INVALID";
			break;

		case TRAVEL_WALK:
			str = "TRAVEL_WALK";
			break;

		case TRAVEL_CROUCH:
			str = "TRAVEL_CROUCH";
			break;

		case TRAVEL_BARRIERJUMP:
			str = "TRAVEL_BARRIERJUMP";
			break;

		case TRAVEL_JUMP:
			str = "TRAVEL_JUMP";
			break;

		case TRAVEL_LADDER:
			str = "TRAVEL_LADDER";
			break;

		case TRAVEL_WALKOFFLEDGE:
			str = "TRAVEL_WALKOFFLEDGE";
			break;

		case TRAVEL_SWIM:
			str = "TRAVEL_SWIM";
			break;

		case TRAVEL_WATERJUMP:
			str = "TRAVEL_WATERJUMP";
			break;

		case TRAVEL_TELEPORT:
			str = "TRAVEL_TELEPORT";
			break;

		case TRAVEL_ELEVATOR:
			str = "TRAVEL_ELEVATOR";
			break;

		case TRAVEL_ROCKETJUMP:
			str = "TRAVEL_ROCKETJUMP";
			break;

		case TRAVEL_BFGJUMP:
			str = "TRAVEL_BFGJUMP";
			break;

		case TRAVEL_GRAPPLEHOOK:
			str = "TRAVEL_GRAPPLEHOOK";
			break;

		case TRAVEL_JUMPPAD:
			str = "TRAVEL_JUMPPAD";
			break;

		case TRAVEL_FUNCBOB:
			str = "TRAVEL_FUNCBOB";
			break;

		default:
			str = "UNKNOWN TRAVEL TYPE";
			break;
	}

	botimport.Print( PRT_MESSAGE, "%s", str );
#endif // DEBUG
}

void AAS_DrawArrow( vec3_t start, vec3_t end, int linecolor, int arrowcolor )
{
	vec3_t dir, cross, p1, p2, up = { 0, 0, 1 };
	float  dot;

	VectorSubtract( end, start, dir );
	VectorNormalize( dir );
	dot = DotProduct( dir, up );

	if( dot > 0.99 || dot < -0.99 ) {
		VectorSet( cross, 1, 0, 0 );

	} else {
		CrossProduct( dir, up, cross );
	}

	VectorMA( end, -6, dir, p1 );
	VectorCopy( p1, p2 );
	VectorMA( p1, 6, cross, p1 );
	VectorMA( p2, -6, cross, p2 );

	AAS_DebugLine( start, end, linecolor );
	AAS_DebugLine( p1, end, arrowcolor );
	AAS_DebugLine( p2, end, arrowcolor );
}

/*!
	\brief Displays visual information about a reachability including area polygons, directional arrows, and predicted client movement based on its travel type.

	The function begins by drawing the polygons of the area related to the reachability and an arrow from its start point to its end point. It then handles several travel types: for jumps or walking
   off edges, it computes a horizontal velocity and predicts movement, optionally drawing a cross if the travel type is a normal jump. For rocket jumps it determines the vertical component of the jump
   velocity, calculates the horizontal velocity, and predicts the resulting movement. For jump pads it sets velocity components from struct fields and also predicts movement. The predictions use
   AAS_PredictClientMovement with appropriate flags for entering water, slime, lava, hitting ground damage and other surface transitions.

	\param reach a pointer to an aas_reachability_s structure that describes the reachability, including its start and end points, travel type, area number, and edge or face data used for velocity
   calculations.
*/
void AAS_ShowReachability( aas_reachability_t* reach )
{
	vec3_t			 dir, cmdmove, velocity;
	float			 speed, zvel;
	aas_clientmove_t move;

	AAS_ShowAreaPolygons( reach->areanum, 5, qtrue );
	// AAS_ShowArea(reach->areanum, qtrue);
	AAS_DrawArrow( reach->start, reach->end, LINECOLOR_BLUE, LINECOLOR_YELLOW );

	//
	if( reach->traveltype == TRAVEL_JUMP || reach->traveltype == TRAVEL_WALKOFFLEDGE ) {
		AAS_HorizontalVelocityForJump( aassettings.sv_jumpvel, reach->start, reach->end, &speed );
		//
		VectorSubtract( reach->end, reach->start, dir );
		dir[2] = 0;
		VectorNormalize( dir );
		// set the velocity
		VectorScale( dir, speed, velocity );
		// set the command movement
		VectorClear( cmdmove );
		cmdmove[2] = aassettings.sv_jumpvel;
		//
		AAS_PredictClientMovement(
			&move, -1, reach->start, PRESENCE_NORMAL, qtrue, velocity, cmdmove, 3, 30, 0.1, SE_HITGROUND | SE_ENTERWATER | SE_ENTERSLIME | SE_ENTERLAVA | SE_HITGROUNDDAMAGE, 0, qtrue );

		//
		if( reach->traveltype == TRAVEL_JUMP ) {
			AAS_JumpReachRunStart( reach, dir );
			AAS_DrawCross( dir, 4, LINECOLOR_BLUE );
		}

	} else if( reach->traveltype == TRAVEL_ROCKETJUMP ) {
		zvel = AAS_RocketJumpZVelocity( reach->start );
		AAS_HorizontalVelocityForJump( zvel, reach->start, reach->end, &speed );
		//
		VectorSubtract( reach->end, reach->start, dir );
		dir[2] = 0;
		VectorNormalize( dir );
		// get command movement
		VectorScale( dir, speed, cmdmove );
		VectorSet( velocity, 0, 0, zvel );
		//
		AAS_PredictClientMovement( &move,
			-1,
			reach->start,
			PRESENCE_NORMAL,
			qtrue,
			velocity,
			cmdmove,
			30,
			30,
			0.1,
			SE_ENTERWATER | SE_ENTERSLIME | SE_ENTERLAVA | SE_HITGROUNDDAMAGE | SE_TOUCHJUMPPAD | SE_HITGROUNDAREA,
			reach->areanum,
			qtrue );

	} else if( reach->traveltype == TRAVEL_JUMPPAD ) {
		VectorSet( cmdmove, 0, 0, 0 );
		//
		VectorSubtract( reach->end, reach->start, dir );
		dir[2] = 0;
		VectorNormalize( dir );
		// set the velocity
		// NOTE: the edgenum is the horizontal velocity
		VectorScale( dir, reach->edgenum, velocity );
		// NOTE: the facenum is the Z velocity
		velocity[2] = reach->facenum;
		//
		AAS_PredictClientMovement( &move,
			-1,
			reach->start,
			PRESENCE_NORMAL,
			qtrue,
			velocity,
			cmdmove,
			30,
			30,
			0.1,
			SE_ENTERWATER | SE_ENTERSLIME | SE_ENTERLAVA | SE_HITGROUNDDAMAGE | SE_TOUCHJUMPPAD | SE_HITGROUNDAREA,
			reach->areanum,
			qtrue );
	}
}

void AAS_ShowReachableAreas( int areanum )
{
	aas_areasettings_t*		  settings;
	static aas_reachability_t reach;
	static int				  index, lastareanum;
	static float			  lasttime;

	if( areanum != lastareanum ) {
		index		= 0;
		lastareanum = areanum;
	}

	settings = &( *aasworld ).areasettings[areanum];

	//
	if( !settings->numreachableareas ) {
		return;
	}

	//
	if( index >= settings->numreachableareas ) {
		index = 0;
	}

	//
	if( AAS_Time() - lasttime > 1.5 ) {
		memcpy( &reach, &( *aasworld ).reachability[settings->firstreachablearea + index], sizeof( aas_reachability_t ) );
		index++;
		lasttime = AAS_Time();
		AAS_PrintTravelType( reach.traveltype );
		botimport.Print( PRT_MESSAGE, "(traveltime: %i)\n", reach.traveltime );
	}

	AAS_ShowReachability( &reach );
}
