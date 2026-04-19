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
	\file engine/botlib/l_utils.h
	\brief This header supplies lightweight macro definitions for use throughout the bot library, facilitating cross‑module consistency.
	\note doxygenix: sha256=e37b7787f3db07b0afacd10015ea89564d8e2f59201cbfd12ef14f00db7c9fae

	\par File Purpose
	- This header supplies lightweight macro definitions for use throughout the bot library, facilitating cross‑module consistency.
	- It replaces repeated manual function calls and common inline calculations with descriptive names, aiding readability and maintenance.

	\par Core Responsibilities
	- Defines basic utility macros for bot library, providing a way to convert vectors to angles, and simple inline maximum and minimum calculations.
	- Provides a macro to standardize path string length limits, aliasing to MAX_QPATH constant from engine core.
	- Encapsulates the call to the engine's vectoangles function through a macro for convenience.

	\par Key Types and Functions
	- Vector2Angles(v, a) — expands to *vectoangles(v, a)*, a wrapper for converting a directional vector to Euler angles.
MAX_PATH — expands to the engine's path limit constant *MAX_QPATH*, used for directory/file string bounds.
Maximum(x, y) — expands to the greater of *x* or *y* using a ternary expression.
Minimum(x, y) — expands to the lesser of *x* or *y* using a ternary expression.

	\par Control Flow
	- TODO: clarify details.

	\par Dependencies
	- Depends on the definition of *MAX_QPATH* from the engine's path handling headers (e.g. *q_shared.h* or *file_util.h*).
	- Relies on the function *vectoangles* which is declared in the engine's math utilities, typically in *trigonometry.h* or similar.

	\par How It Fits
	- *l_utils.h* is included by various bot source files to provide common utilities before compiling the bot behavior code.
	- It ensures that bot compilation is independent of the core engine definitions by mapping to engine constants and functions.
*/

/*****************************************************************************
 * name:		l_util.h
 *
 * desc:		utils
 *
 *
 *****************************************************************************/

#define Vector2Angles( v, a ) vectoangles( v, a )
#define MAX_PATH			  MAX_QPATH
#define Maximum( x, y )		  ( x > y ? x : y )
#define Minimum( x, y )		  ( x < y ? x : y )
