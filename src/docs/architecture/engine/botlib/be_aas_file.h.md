<!-- doxygenix: sha256=7c3d4ad6ca6ed6fc1626526225a36d85bd6229aae016912ec2bd08065b501b16 -->

# engine/botlib/be_aas_file.h

## File Purpose
- Declare the runtime API for manipulating AAS navigation files under the AASINTERN configuration.

## Core Responsibilities
- Defines the interface for loading, writing, dumping, and inspecting AAS navigation files used by the bot navigation system.
- Provides diagnostic utilities to validate AAS integrity and performance during development and debugging.

## Key Types and Functions
- AAS_LoadAASFile(char* filename) – reads and parses an AAS file into internal structures.
- AAS_WriteAASFile(char* filename) – writes the current AAS data to a file, typically used for debug output.
- AAS_DumpAASData() – prints detailed internal AAS statistics to stdout or a logfile.
- AAS_FileInfo() – displays summary information about the currently loaded AAS file.

## Important Control Flow
- AAS_LoadAASFile reads a binary AAS file, parses its header and node data, and populates internal structures for bot pathfinding.
- AAS_WriteAASFile serializes the current in‑memory AAS representation back to disk, ensuring consistency with the engine's pathfinding data.
- AAS_DumpAASData iterates over the loaded AAS model and emits diagnostic information (node counts, edges, metrics) to the console or log for debugging.
- AAS_FileInfo outputs a concise summary of the loaded AAS file—name, size, version, and key statistics—to aid verification.

## External Dependencies
- qboolean type (defined elsewhere in the engine, e.g., engine/common/qboolean.h)
- Standard C headers for file I/O (stdio.h, stdlib.h)

## How It Fits
- Acts as the low‑level interface between the bot navigation module and persistent AAS data stored on disk.
- Enables the engine to load precomputed navigation meshes, modify them, and verify their correctness during development.
