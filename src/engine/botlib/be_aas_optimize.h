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
	\file engine/botlib/be_aas_optimize.h
	\brief Declares the interface for the AAS optimization routine, isolating the implementation in a corresponding .cpp file and offering a clean, single-public function for callers.
	\note doxygenix: sha256=bd8b625e6c083d8cbed417501991e0123d8ac5b37c181fb5f225473ffbbaca0e

	\par File Purpose
	- Declares the interface for the AAS optimization routine, isolating the implementation in a corresponding .cpp file and offering a clean, single-public function for callers.

	\par Core Responsibilities
	- * Provides a single entry point for optimizing the AAS (Area Awareness System) data structures used by the bot navigation system.
	- * Triggers optimization routines that potentially reclaim memory, merge redundant nodes, or adjust internal caches for efficient runtime operation.

	\par Key Types and Functions
	- AAS_Optimize() — initiates optimization of the AAS map data structures created during navigation graph generation.

	\par Control Flow
	- Runtime execution calls the AAS_Optimize function during the AAS subsystem initialization phase, after the AAS data has been populated from map contents. The function traverses the internally built graph structures, performing cleanup, compression, or other optimization passes to improve pathfinding performance.

	\par Dependencies
	- No explicit header dependencies are included in this file. The implementation is expected to rely on internal AAS data structures defined elsewhere in the botlib, e.g., be_aas.h and related translation files.

	\par How It Fits
	- * Forms part of the botlib’s navigation subsystem, offering a maintenance step post-AAS data loading.
	- * Allows other engine components (e.g., map startup, hot-reload) to invoke optimization without needing to access internal AAS internals directly.
*/

/*****************************************************************************
 * name:		be_aas_optimize.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

void AAS_Optimize();
