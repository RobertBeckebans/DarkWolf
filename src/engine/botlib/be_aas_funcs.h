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
	\file engine/botlib/be_aas_funcs.h
	\brief Provide a unified header that bundles together the full API surface of the bot AAS subsystem for other engine modules.
	\note doxygenix: sha256=ee4c78f6377083b67cb3df1f1695a8470c2b63793b0df118a6f623a6fab84485

	\par File Purpose
	- Provide a unified header that bundles together the full API surface of the bot AAS subsystem for other engine modules.
	- Reduce header clutter and prevent duplicate includes across the codebase.

	\par Core Responsibilities
	- Act as a single inclusion point for all AAS–related components of the bot library.
	- Aggregate the interfaces of individual AAS modules such as clustering, routing, sampling, movement, optimization, and debugging.
	- Facilitate modular compilation by controlling the order and availability of these components.

	\par Key Types and Functions
	- TODO: this header declares no new types or functions; it merely includes other headers.

	\par Control Flow
	- At compile time, this header expands to include a series of bot AAS subsystem headers, ensuring all necessary declarations are visible to any translation unit that includes it.

	\par Dependencies
	- "be_aas_main.h"
	- "be_aas_entity.h"
	- "be_aas_sample.h"
	- "be_aas_cluster.h"
	- "be_aas_reach.h"
	- "be_aas_route.h"
	- "be_aas_routealt.h"
	- "be_aas_debug.h"
	- "be_aas_file.h"
	- "be_aas_optimize.h"
	- "be_aas_bsp.h"
	- "be_aas_move.h"
	- "be_aas_routetable.h"

	\par How It Fits
	- Located in `engine/botlib`, it is part of the bot navigation system, enabling bots to navigate complex level geometry.
	- Other bot or gameplay code includes this file to gain access to AAS functions without managing individual headers.
	- It acts as an interface layer between high‑level bot logic and lower‑level AAS implementation details.
*/

/*****************************************************************************
 * name:		be_aas_funcs.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#ifndef BSPCINCLUDE

	#include "be_aas_main.h"
	#include "be_aas_entity.h"
	#include "be_aas_sample.h"
	#include "be_aas_cluster.h"
	#include "be_aas_reach.h"
	#include "be_aas_route.h"
	#include "be_aas_routealt.h"
	#include "be_aas_debug.h"
	#include "be_aas_file.h"
	#include "be_aas_optimize.h"
	#include "be_aas_bsp.h"
	#include "be_aas_move.h"

	// Ridah, route-tables
	#include "be_aas_routetable.h"

#endif // BSPCINCLUDE
