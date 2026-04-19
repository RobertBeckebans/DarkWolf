<!-- doxygenix: sha256=bd8b625e6c083d8cbed417501991e0123d8ac5b37c181fb5f225473ffbbaca0e -->

# engine/botlib/be_aas_optimize.h

## File Purpose
- Declares the interface for the AAS optimization routine, isolating the implementation in a corresponding .cpp file and offering a clean, single-public function for callers.

## Core Responsibilities
- * Provides a single entry point for optimizing the AAS (Area Awareness System) data structures used by the bot navigation system.
- * Triggers optimization routines that potentially reclaim memory, merge redundant nodes, or adjust internal caches for efficient runtime operation.

## Key Types and Functions
- AAS_Optimize() — initiates optimization of the AAS map data structures created during navigation graph generation.

## Important Control Flow
- Runtime execution calls the AAS_Optimize function during the AAS subsystem initialization phase, after the AAS data has been populated from map contents. The function traverses the internally built graph structures, performing cleanup, compression, or other optimization passes to improve pathfinding performance.

## External Dependencies
- No explicit header dependencies are included in this file. The implementation is expected to rely on internal AAS data structures defined elsewhere in the botlib, e.g., be_aas.h and related translation files.

## How It Fits
- * Forms part of the botlib’s navigation subsystem, offering a maintenance step post-AAS data loading.
- * Allows other engine components (e.g., map startup, hot-reload) to invoke optimization without needing to access internal AAS internals directly.
