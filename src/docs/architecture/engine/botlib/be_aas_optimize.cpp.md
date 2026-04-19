<!-- doxygenix: sha256=128ef17d090cf9c1bbc43f0888f34633df2008376cd049a7ddf89bf1695b568c -->

# engine/botlib/be_aas_optimize.cpp

## File Purpose
- Provide a lightweight optimization pass for the AAS (Area Awareness System) data structure.
- Reduce the memory footprint of the map by retaining only ladder faces and necessary edges.
- Prepare the data for efficient runtime navigation by bots.

## Core Responsibilities
- Create an optimized subset of AAS map geometry that retains only ladder faces and required edges.
- Map original vertices, edges, and faces to new compact arrays while preserving connectivity.
- Update reachability data to reference the newly optimized geometry.
- Replace the global AAS world data with the optimized geometry and clean up intermediate memory.

## Key Types and Functions
- optimized_t — Structure holding optimized vertices, edges, indices, faces, and area data plus mapping tables.
- AAS_KeepEdge(aas_edge_t* edge) — Currently always returns 1; placeholder for future edge‑filtering logic.
- AAS_OptimizeEdge(optimized_t* optimized, int edgenum) — Maps an original edge to an optimized edge, adding required vertices and storing a signed edge index.
- AAS_KeepFace(aas_face_t* face) — Returns 1 when the face has the FACE_LADDER flag; determines which faces are kept.
- AAS_OptimizeFace(optimized_t* optimized, int facenum) — Copies and optimizes a face, optimizes its constituent edges, and stores a signed face index.
- AAS_OptimizeArea(optimized_t* optimized, int areanum) — Copies area metadata, optimizes each face in the area, and updates the face index.
- AAS_OptimizeAlloc(optimized_t* optimized) — Allocates cleared memory for all optimized arrays and initializes counters (vertexes, edges, faceindex, etc.).
- AAS_OptimizeStore(optimized_t* optimized) — Swaps global `aasworld` arrays with the optimized ones and frees temporary mapping arrays.
- AAS_Optimize() — Main driver that orchestrates allocation, area optimization, reachability index updates, storing of data, and logs completion.

## Important Control Flow
- The entry point `AAS_Optimize()` allocates an `optimized_t` structure and iterates over all non‑core areas to build optimized geometry by calling `AAS_OptimizeArea`.
- Within each area, `AAS_OptimizeArea` copies the area record, then iterates its faces, passing each to `AAS_OptimizeFace`.
- `AAS_OptimizeFace` copies the face, optimizes each edge via `AAS_OptimizeEdge`, and records the resulting edge indices in the optimized structure.
- `AAS_OptimizeEdge` retains an edge only if `AAS_KeepEdge` permits it (currently always true), re‑maps its vertex indices, creates a new optimized edge, and tracks the mapping via the `edgeoptimizeindex` array.
- After geometry is processed, `AAS_Optimize()` updates all reachability entries that reference edges or faces, substituting the optimized indices while preserving numeric sign.
- Finally `AAS_OptimizeStore()` swaps the global `aasworld` arrays with the optimized ones, frees temporary indexing arrays, and prints an optimization confirmation.

## External Dependencies
- "q_shared.h"
- "l_libvar.h"
- "l_memory.h"
- "l_script.h"
- "l_precomp.h"
- "l_struct.h"
- "aasfile.h"
- "botshared/botlib.h"
- "botshared/be_aas.h"
- "be_aas_funcs.h"
- "be_interface.h"
- "be_aas_def.h"

## How It Fits
- This module plugs into the bot pre‑processing pipeline, invoked after full area connectivity has been computed.
- It transforms the raw world data (`aasworld`) into a compact representation suitable for real‑time pathfinding.
- By eliminating unnecessary geometry, it improves caching and traversal performance for AI navigation.
