<!-- doxygenix: sha256=3943a70bd320cc488917a1579655c4758afb6d884dd517866726a7b9f089a4c4 -->

# engine/botlib/be_aas_file.cpp

## File Purpose
- Provides the file IO subsystem for the AAS (Area Awareness System) data used by the bot AI, including loading, writing, swapping, and cleanup operations.

## Core Responsibilities
- Maintain the global AAS world state used by the bot navigation system.
- Serialize and deserialize AAS data to and from disk files.
- Handle endianness conversion for cross‑platform compatibility.
- Manage memory allocation and deallocation for all AAS data structures.

## Key Types and Functions
- aas_header_t — structure representing the file header containing identification, version, checksum, and lump offsets.
- aas_world_t — global data structure holding all loaded AAS data arrays and state flags.
- AAS_SwapAASData() — iterates over all AAS data structures and converts each numeric field to little‑endian if necessary.
- AAS_DumpAASData() — frees memory for all AAS data arrays and resets the global state.
- AAS_LoadAASLump(fileHandle_t fp, int offset, int length, int* lastoffset) → char* — reads a lump from the file into a hunk buffer, returning the raw data.
- AAS_DData(unsigned char* data, int size) — bit‑wise XORs each byte of the buffer with its index multiplied by 119.
- AAS_LoadAASFile(char* filename) → int — orchestrates full file loading: header validation, data lump loading, endian swap, and state flag updates.
- AAS_WriteAASLump_offset (static int) — tracks the current write position when serialising lumps.
- AAS_WriteAASLump(fileHandle_t fp, aas_header_t* h, int lumpnum, void* data, int length) → int — writes a lump to the file, records its offset and length in the header structure.
- AAS_WriteAASFile(char* filename) → qboolean — writes the entire AAS world to disk, serialising all lumps and updating the header.

## Important Control Flow
- AAS_LoadAASFile opens the specified .aas file, reads and validates the header, then loads each data lump sequentially via AAS_LoadAASLump, performing byte‑order swapping with AAS_SwapAASData before setting the world as loaded and closing the file.
- AAS_DumpAASData frees every allocated memory block in the global aasworld, resets all size counters to zero, and clears loaded/initialized flags.
- AAS_WriteAASFile swaps the world data, writes an initial zero header, serialises each lump with AAS_WriteAASLump while updating the header’s offsets and lengths, rewrites the header at the file start, and closes the file.
- AAS_SwapAASData iterates over all data arrays—boxes, vertices, planes, edges, faces, areas, settings, reachability, nodes, portals, and clusters—applying LittleLong, LittleFloat, and LittleShort to convert from machine byte order to little‑endian.

## External Dependencies
- "q_shared.h"
- "l_memory.h"
- "l_script.h"
- "l_precomp.h"
- "l_struct.h"
- "l_libvar.h"
- "l_utils.h"
- "aasfile.h"
- "botshared/botlib.h"
- "botshared/be_aas.h"
- "be_aas_funcs.h"
- "be_interface.h"
- "be_aas_def.h"

## How It Fits
- Integrates with the bot shared library to expose AAS file handling to higher‑level navigation modules.
- Serves as the persistence layer for the navigation graph that informs pathfinding, reachability, and area checks performed by bots during gameplay.
- Ensures the AAS data can be produced by external tools and consumed at runtime on any platform by performing endianness conversion.
