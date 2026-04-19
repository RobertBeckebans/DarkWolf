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
	\file engine/botlib/be_aas_file.h
	\brief Declare the runtime API for manipulating AAS navigation files under the AASINTERN configuration.
	\note doxygenix: sha256=7c3d4ad6ca6ed6fc1626526225a36d85bd6229aae016912ec2bd08065b501b16

	\par File Purpose
	- Declare the runtime API for manipulating AAS navigation files under the AASINTERN configuration.

	\par Core Responsibilities
	- Defines the interface for loading, writing, dumping, and inspecting AAS navigation files used by the bot navigation system.
	- Provides diagnostic utilities to validate AAS integrity and performance during development and debugging.

	\par Key Types and Functions
	- AAS_LoadAASFile(char* filename) – reads and parses an AAS file into internal structures.
	- AAS_WriteAASFile(char* filename) – writes the current AAS data to a file, typically used for debug output.
	- AAS_DumpAASData() – prints detailed internal AAS statistics to stdout or a logfile.
	- AAS_FileInfo() – displays summary information about the currently loaded AAS file.

	\par Control Flow
	- AAS_LoadAASFile reads a binary AAS file, parses its header and node data, and populates internal structures for bot pathfinding.
	- AAS_WriteAASFile serializes the current in‑memory AAS representation back to disk, ensuring consistency with the engine's pathfinding data.
	- AAS_DumpAASData iterates over the loaded AAS model and emits diagnostic information (node counts, edges, metrics) to the console or log for debugging.
	- AAS_FileInfo outputs a concise summary of the loaded AAS file—name, size, version, and key statistics—to aid verification.

	\par Dependencies
	- qboolean type (defined elsewhere in the engine, e.g., engine/common/qboolean.h)
	- Standard C headers for file I/O (stdio.h, stdlib.h)

	\par How It Fits
	- Acts as the low‑level interface between the bot navigation module and persistent AAS data stored on disk.
	- Enables the engine to load precomputed navigation meshes, modify them, and verify their correctness during development.
*/

/*****************************************************************************
 * name:		be_aas_file.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#ifdef AASINTERN
// loads the AAS file with the given name
int		 AAS_LoadAASFile( char* filename );
// writes an AAS file with the given name
qboolean AAS_WriteAASFile( char* filename );
// dumps the loaded AAS data
void	 AAS_DumpAASData();
// print AAS file information
void	 AAS_FileInfo();
#endif // AASINTERN
