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
	\file engine/botlib/be_aas_cluster.h
	\brief Provide internal function prototypes for AAS clustering initialization within the bot navigation system.
	\note doxygenix: sha256=f7cb60e12a8d0961618d95846dc270a291c190c667a183372054f93088b5f322

	\par File Purpose
	- Provide internal function prototypes for AAS clustering initialization within the bot navigation system.

	\par Core Responsibilities
	- Declare internal functions for initializing AAS clustering and setting up cluster portals; enforce compile-time conditional inclusion via AASINTERN.

	\par Key Types and Functions
	- AAS_InitClustering() — initializes clustering structures for AAS.
	- AAS_SetViewPortalsAsClusterPortals() — configures view portals as cluster portals.

	\par Control Flow
	- During engine startup, if AASINTERN is defined, AAS_InitClustering() is invoked to initialize clustering data structures; subsequently AAS_SetViewPortalsAsClusterPortals() configures portal-to-cluster relationships for visibility calculations.

	\par Dependencies
	- This header has no explicit includes; it relies on the surrounding AAS implementation (likely other botlib headers) for definitions of AAS data structures.

	\par How It Fits
	- Part of the bot navigation subsystem (AAS) that handles spatial partitioning; these functions are called during navigation graph preparation to establish clusters and portal visibility.
*/

/*****************************************************************************
 * name:		be_aas_cluster.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#ifdef AASINTERN
// initialize the AAS clustering
void AAS_InitClustering();
void AAS_SetViewPortalsAsClusterPortals();
#endif // AASINTERN
