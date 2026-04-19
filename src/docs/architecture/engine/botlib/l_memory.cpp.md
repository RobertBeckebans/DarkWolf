<!-- doxygenix: sha256=a5c58765d5bbc7647fe58a69339acd03e83f6bc94957baa944deca4e3f774135 -->

# engine/botlib/l_memory.cpp

## File Purpose
- This file implements the memory allocation subsystem used by the DarkWolf bot library, offering both minimal wrappers and an optional comprehensive memory manager for debugging and leakage detection.

## Core Responsibilities
- Serve as a thin wrapper around botimport’s raw memory APIs to provide a consistent allocation interface for the bot library.
- Optional detailed memory tracking when MEMORYMANEGER/DIGMDEBUG is defined, including block size, source location, and label information.
- Support separate allocation pools: general heap allocations and hunk allocations.
- Expose debugging utilities that print current memory consumption, list all live blocks, and detect invalid frees.
- Ensure that each allocated block carries a verification token (MEM_ID or HUNK_ID) to guard against double‑free or corrupt pointer errors.

## Key Types and Functions
- memoryblock_t — structure used to track each allocation when MEMORYMANEGER is defined, containing an id, pointer, size, optional debug info, and linked‑list pointers.
- LinkMemoryBlock(memoryblock_t* block) — inserts a block at the head of the global list.
- UnlinkMemoryBlock(memoryblock_t* block) — removes a block from the global list.
- GetMemoryDebug/ GetMemory — allocate memory from botimport.GetMemory, add a MEM_ID header, and optionally store debug metadata.
- GetClearedMemoryDebug/ GetClearedMemory — allocate and zero‑initialize memory using GetMemory or GetMemoryDebug.
- GetHunkMemoryDebug/ GetHunkMemory — allocate from botimport.HunkAlloc, prefix with HUNK_ID, and optionally store debug metadata.
- GetClearedHunkMemoryDebug/ GetClearedHunkMemory — allocate a hunk block and zero it.
- BlockFromPointer(void* ptr, char* str) — computes the preceding memoryblock_t and validates the id; used by FreeMemory and MemoryByteSize.
- FreeMemory(void* ptr) — validates a pointer, unlinks its block, updates counters, and frees the underlying memory.
- MemoryByteSize(void* ptr) — returns the size field of the block corresponding to ptr.
- PrintUsedMemorySize() — reports total allocated, total botlib, and number of blocks to the console.
- PrintMemoryLabels() — iterates the list and logs detailed per‑block information (label, file, line).
- DumpMemory() — frees all remaining blocks and resets counters.

## Important Control Flow
- For each allocation request, the appropriate Get* function obtains a block from the bot import’s memory or hunk allocator, optionally prefixes it with a MEM_ID or HUNK_ID header, and then returns a pointer to the usable data region.
- When MEMORYMANEGER is enabled, a linked-list of memoryblock_t structures is maintained; each new block is linked at the head and the global counters (allocatedmemory, totalmemorysize, numblocks) are updated.
- FreeMemory first locates the metadata block using BlockFromPointer, validates the header, then unlinks it from the list, updates the counters, and finally frees the underlying memory via botimport.FreeMemory.
- Diagnostic functions such as PrintUsedMemorySize and PrintMemoryLabels traverse the list to report current usage and detailed allocation records.
- DumpMemory repeatedly frees every block until the list is empty, resetting counters to zero.

## External Dependencies
- "q_shared.h" – Core Quake-style types and constants.
- "botshared/botlib.h" – Declaration of the bot import interface used for allocation.
- "l_log.h" – Logging facilities for error and message output.
- "be_interface.h" – Provides the botimport structure with memory functions.

## How It Fits
- Integrated into the bot library layer, providing the sole mechanism by which bots request and release dynamic memory.
- Works in tandem with the engine’s hunk allocator (via botimport.HunkAlloc) and the garbage‑collection system of the underlying RTW engine.
- When DEBUG is enabled, the memory manager helps developers spot leaks and corruption during development builds.
- All functions are thin and have little runtime overhead, ensuring that bot performance is not impacted outside debug builds.
