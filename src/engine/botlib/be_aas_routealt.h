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
 * name:		be_aas_routealt.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#ifdef AASINTERN
void AAS_InitAlternativeRouting();
void AAS_ShutdownAlternativeRouting();
#endif // AASINTERN

/*!
	\brief Finds alternative route goals between a start and goal position using AAS data structures

	This function computes alternative route goals by identifying mid-range areas that connect the start and goal positions through valid travel paths. It filters areas based on travel time constraints relative to the shortest path and then clusters these areas to find optimal alternative goal positions. The function returns the number of alternative goals found, up to the maximum allowed, and populates the provided array with the details of each alternative goal including area number, travel times, and position coordinates.

	\param start Starting position in the world
	\param goal Target position in the world
	\param travelflags Travel flags to determine valid paths
	\param altroutegoals Output array to store alternative route goals
	\param maxaltroutegoals Maximum number of alternative goals to return
	\param color Debug color for visualization (if enabled)
	\return The number of alternative route goals found and stored in the altroutegoals array
*/
int AAS_AlternativeRouteGoals( vec3_t start, vec3_t goal, int travelflags, aas_altroutegoal_t* altroutegoals, int maxaltroutegoals, int color );
