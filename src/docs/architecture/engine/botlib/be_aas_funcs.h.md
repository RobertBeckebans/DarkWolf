<!-- doxygenix: sha256=ee4c78f6377083b67cb3df1f1695a8470c2b63793b0df118a6f623a6fab84485 -->

# engine/botlib/be_aas_funcs.h

## File Purpose
- Provide a unified header that bundles together the full API surface of the bot AAS subsystem for other engine modules.
- Reduce header clutter and prevent duplicate includes across the codebase.

## Core Responsibilities
- Act as a single inclusion point for all AAS–related components of the bot library.
- Aggregate the interfaces of individual AAS modules such as clustering, routing, sampling, movement, optimization, and debugging.
- Facilitate modular compilation by controlling the order and availability of these components.

## Key Types and Functions
- TODO: this header declares no new types or functions; it merely includes other headers.

## Important Control Flow
- At compile time, this header expands to include a series of bot AAS subsystem headers, ensuring all necessary declarations are visible to any translation unit that includes it.

## External Dependencies
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

## How It Fits
- Located in `engine/botlib`, it is part of the bot navigation system, enabling bots to navigate complex level geometry.
- Other bot or gameplay code includes this file to gain access to AAS functions without managing individual headers.
- It acts as an interface layer between high‑level bot logic and lower‑level AAS implementation details.
