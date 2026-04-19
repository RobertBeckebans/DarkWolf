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
	\file engine/botlib/l_memory.cpp
	\brief This file implements the memory allocation subsystem used by the DarkWolf bot library, offering both minimal wrappers and an optional comprehensive memory manager for debugging and leakage detection.
	\note doxygenix: sha256=a5c58765d5bbc7647fe58a69339acd03e83f6bc94957baa944deca4e3f774135

	\par File Purpose
	- This file implements the memory allocation subsystem used by the DarkWolf bot library, offering both minimal wrappers and an optional comprehensive memory manager for debugging and leakage detection.

	\par Core Responsibilities
	- Serve as a thin wrapper around botimport’s raw memory APIs to provide a consistent allocation interface for the bot library.
	- Optional detailed memory tracking when MEMORYMANEGER/DIGMDEBUG is defined, including block size, source location, and label information.
	- Support separate allocation pools: general heap allocations and hunk allocations.
	- Expose debugging utilities that print current memory consumption, list all live blocks, and detect invalid frees.
	- Ensure that each allocated block carries a verification token (MEM_ID or HUNK_ID) to guard against double‑free or corrupt pointer errors.

	\par Key Types and Functions
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

	\par Control Flow
	- For each allocation request, the appropriate Get* function obtains a block from the bot import’s memory or hunk allocator, optionally prefixes it with a MEM_ID or HUNK_ID header, and then returns a pointer to the usable data region.
	- When MEMORYMANEGER is enabled, a linked-list of memoryblock_t structures is maintained; each new block is linked at the head and the global counters (allocatedmemory, totalmemorysize, numblocks) are updated.
	- FreeMemory first locates the metadata block using BlockFromPointer, validates the header, then unlinks it from the list, updates the counters, and finally frees the underlying memory via botimport.FreeMemory.
	- Diagnostic functions such as PrintUsedMemorySize and PrintMemoryLabels traverse the list to report current usage and detailed allocation records.
	- DumpMemory repeatedly frees every block until the list is empty, resetting counters to zero.

	\par Dependencies
	- "q_shared.h" – Core Quake-style types and constants.
	- "botshared/botlib.h" – Declaration of the bot import interface used for allocation.
	- "l_log.h" – Logging facilities for error and message output.
	- "be_interface.h" – Provides the botimport structure with memory functions.

	\par How It Fits
	- Integrated into the bot library layer, providing the sole mechanism by which bots request and release dynamic memory.
	- Works in tandem with the engine’s hunk allocator (via botimport.HunkAlloc) and the garbage‑collection system of the underlying RTW engine.
	- When DEBUG is enabled, the memory manager helps developers spot leaks and corruption during development builds.
	- All functions are thin and have little runtime overhead, ensuring that bot performance is not impacted outside debug builds.
*/

/*****************************************************************************
 * name:		l_memory.c
 *
 * desc:		memory allocation
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "botshared/botlib.h"
#include "l_log.h"
#include "be_interface.h"

#ifdef _DEBUG
	#define MEMDEBUG
	#define MEMORYMANEGER
#endif

#define MEM_ID	0x12345678l
#define HUNK_ID 0x87654321l

int allocatedmemory;
int totalmemorysize;
int numblocks;

#ifdef MEMORYMANEGER

typedef struct memoryblock_s {
	unsigned long int id;
	void*			  ptr;
	int				  size;
	#ifdef MEMDEBUG
	char* label;
	char* file;
	int	  line;
	#endif // MEMDEBUG
	struct memoryblock_s *prev, *next;
} memoryblock_t;

memoryblock_t* memory;

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void		   LinkMemoryBlock( memoryblock_t* block )
{
	block->prev = NULL;
	block->next = memory;

	if( memory ) {
		memory->prev = block;
	}

	memory = block;
}

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void UnlinkMemoryBlock( memoryblock_t* block )
{
	if( block->prev ) {
		block->prev->next = block->next;

	} else {
		memory = block->next;
	}

	if( block->next ) {
		block->next->prev = block->prev;
	}
}

	//===========================================================================
	//
	// Parameter:			-
	// Returns:				-
	// Changes Globals:		-
	//===========================================================================
	#ifdef MEMDEBUG
void* GetMemoryDebug( unsigned long size, char* label, char* file, int line )
	#else
void* GetMemory( unsigned long size )
	#endif // MEMDEBUG
{
	void*		   ptr;
	memoryblock_t* block;

	ptr			= botimport.GetMemory( size + sizeof( memoryblock_t ) );
	block		= ( memoryblock_t* )ptr;
	block->id	= MEM_ID;
	block->ptr	= ( char* )ptr + sizeof( memoryblock_t );
	block->size = size + sizeof( memoryblock_t );
	#ifdef MEMDEBUG
	block->label = label;
	block->file	 = file;
	block->line	 = line;
	#endif // MEMDEBUG
	LinkMemoryBlock( block );
	allocatedmemory += block->size;
	totalmemorysize += block->size + sizeof( memoryblock_t );
	numblocks++;
	return block->ptr;
}

	//===========================================================================
	//
	// Parameter:			-
	// Returns:				-
	// Changes Globals:		-
	//===========================================================================
	#ifdef MEMDEBUG
void* GetClearedMemoryDebug( unsigned long size, char* label, char* file, int line )
	#else
void* GetClearedMemory( unsigned long size )
	#endif // MEMDEBUG
{
	void* ptr;
	#ifdef MEMDEBUG
	ptr = GetMemoryDebug( size, label, file, line );
	#else
	ptr = GetMemory( size );
	#endif // MEMDEBUG
	memset( ptr, 0, size );
	return ptr;
}

	//===========================================================================
	//
	// Parameter:			-
	// Returns:				-
	// Changes Globals:		-
	//===========================================================================
	#ifdef MEMDEBUG
void* GetHunkMemoryDebug( unsigned long size, char* label, char* file, int line )
	#else
void* GetHunkMemory( unsigned long size )
	#endif // MEMDEBUG
{
	void*		   ptr;
	memoryblock_t* block;

	ptr			= botimport.HunkAlloc( size + sizeof( memoryblock_t ) );
	block		= ( memoryblock_t* )ptr;
	block->id	= HUNK_ID;
	block->ptr	= ( char* )ptr + sizeof( memoryblock_t );
	block->size = size + sizeof( memoryblock_t );
	#ifdef MEMDEBUG
	block->label = label;
	block->file	 = file;
	block->line	 = line;
	#endif // MEMDEBUG
	LinkMemoryBlock( block );
	allocatedmemory += block->size;
	totalmemorysize += block->size + sizeof( memoryblock_t );
	numblocks++;
	return block->ptr;
}

	//===========================================================================
	//
	// Parameter:			-
	// Returns:				-
	// Changes Globals:		-
	//===========================================================================
	#ifdef MEMDEBUG
void* GetClearedHunkMemoryDebug( unsigned long size, char* label, char* file, int line )
	#else
void* GetClearedHunkMemory( unsigned long size )
	#endif // MEMDEBUG
{
	void* ptr;
	#ifdef MEMDEBUG
	ptr = GetHunkMemoryDebug( size, label, file, line );
	#else
	ptr = GetHunkMemory( size );
	#endif // MEMDEBUG
	memset( ptr, 0, size );
	return ptr;
}

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
memoryblock_t* BlockFromPointer( void* ptr, char* str )
{
	memoryblock_t* block;

	if( !ptr ) {
	#ifdef MEMDEBUG
		// char *crash = (char *) NULL;
		// crash[0] = 1;
		botimport.Print( PRT_FATAL, "%s: NULL pointer\n", str );
	#endif // MEMDEBUG
		return NULL;
	}

	block = ( memoryblock_t* )( ( char* )ptr - sizeof( memoryblock_t ) );

	if( block->id != MEM_ID && block->id != HUNK_ID ) {
		botimport.Print( PRT_FATAL, "%s: invalid memory block\n", str );
		return NULL;
	}

	if( block->ptr != ptr ) {
		botimport.Print( PRT_FATAL, "%s: memory block pointer invalid\n", str );
		return NULL;
	}

	return block;
}

void FreeMemory( void* ptr )
{
	memoryblock_t* block;

	block = BlockFromPointer( ptr, "FreeMemory" );

	if( !block ) {
		return;
	}

	UnlinkMemoryBlock( block );
	allocatedmemory -= block->size;
	totalmemorysize -= block->size + sizeof( memoryblock_t );
	numblocks--;

	//
	if( block->id == MEM_ID ) {
		botimport.FreeMemory( block );
	}
}

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int MemoryByteSize( void* ptr )
{
	memoryblock_t* block;

	block = BlockFromPointer( ptr, "MemoryByteSize" );

	if( !block ) {
		return 0;
	}

	return block->size;
}

void PrintUsedMemorySize()
{
	botimport.Print( PRT_MESSAGE, "total allocated memory: %d KB\n", allocatedmemory >> 10 );
	botimport.Print( PRT_MESSAGE, "total botlib memory: %d KB\n", totalmemorysize >> 10 );
	botimport.Print( PRT_MESSAGE, "total memory blocks: %d\n", numblocks );
}

void PrintMemoryLabels()
{
	memoryblock_t* block;
	int			   i;

	PrintUsedMemorySize();
	i = 0;
	Log_Write( "\r\n" );

	for( block = memory; block; block = block->next ) {
	#ifdef MEMDEBUG

		if( block->id == HUNK_ID ) {
			Log_Write( "%6d, hunk %p, %8d: %24s line %6d: %s\r\n", i, block->ptr, block->size, block->file, block->line, block->label );

		} else {
			Log_Write( "%6d,      %p, %8d: %24s line %6d: %s\r\n", i, block->ptr, block->size, block->file, block->line, block->label );
		}

	#endif // MEMDEBUG
		i++;
	}
}

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void DumpMemory()
{
	memoryblock_t* block;

	for( block = memory; block; block = memory ) {
		FreeMemory( block->ptr );
	}

	totalmemorysize = 0;
	allocatedmemory = 0;
}

#else

	//===========================================================================
	//
	// Parameter:			-
	// Returns:				-
	// Changes Globals:		-
	//===========================================================================
	#ifdef MEMDEBUG
void* GetMemoryDebug( unsigned long size, char* label, char* file, int line )
	#else

/*!
	\brief Allocates a block of memory of the specified size, adding overhead for memory tracking

	The function allocates a block of memory of the specified size, including space for a memory identifier. It returns a pointer to the allocated memory block, excluding the space reserved for the memory identifier. The allocated memory is tracked using a MEM_ID marker for debugging purposes

	\param size The size of the memory block to allocate in bytes
	\return A pointer to the allocated memory block, or NULL if allocation fails
	\throws NULL is returned if the memory allocation fails
*/
void* GetMemory( unsigned long size )
	#endif // MEMDEBUG
{
	void*			   ptr;
	unsigned long int* memid;

	ptr = botimport.GetMemory( size + sizeof( unsigned long int ) );

	if( !ptr ) {
		return NULL;
	}

	memid  = ( unsigned long int* )ptr;
	*memid = MEM_ID;
	return ( unsigned long int* )( ( char* )ptr + sizeof( unsigned long int ) );
}

	//===========================================================================
	//
	// Parameter:			-
	// Returns:				-
	// Changes Globals:		-
	//===========================================================================
	#ifdef MEMDEBUG
void* GetClearedMemoryDebug( unsigned long size, char* label, char* file, int line )
	#else

/*!
	\brief Allocates memory of the specified size and initializes it to zero

	This function allocates a block of memory of the given size and initializes all bytes in the block to zero. It is used in the AAS (Area Awareness System) for allocating memory for portal, portal index, and cluster data structures. The function handles both debug and non-debug memory allocation paths. The allocated memory is guaranteed to be zero-initialized which is useful for preventing undefined behavior when working with data structures that rely on zeroed memory.

	\param size The size in bytes of the memory block to allocate
	\return A pointer to the newly allocated and zero-initialized memory block
*/
void* GetClearedMemory( unsigned long size )
	#endif // MEMDEBUG
{
	void* ptr;
	#ifdef MEMDEBUG
	ptr = GetMemoryDebug( size, label, file, line );
	#else
	ptr = GetMemory( size );
	#endif // MEMDEBUG
	memset( ptr, 0, size );
	return ptr;
}

	//===========================================================================
	//
	// Parameter:			-
	// Returns:				-
	// Changes Globals:		-
	//===========================================================================
	#ifdef MEMDEBUG
void* GetHunkMemoryDebug( unsigned long size, char* label, char* file, int line )
	#else

/*!
	\brief Allocates a block of memory from the hunk allocator with the specified size.

	This function allocates memory using the hunk allocator, adding extra space to store a memory ID for tracking. The allocated memory block includes a header that stores the HUNK_ID, which is used for memory validation and tracking. If the allocation fails, the function returns NULL. The returned pointer points to the memory block after the header.

	\param size The size in bytes of the memory block to allocate.
	\return A pointer to the allocated memory block, or NULL if allocation failed.
*/
void* GetHunkMemory( unsigned long size )
	#endif // MEMDEBUG
{
	void*			   ptr;
	unsigned long int* memid;

	ptr = botimport.HunkAlloc( size + sizeof( unsigned long int ) );

	if( !ptr ) {
		return NULL;
	}

	memid  = ( unsigned long int* )ptr;
	*memid = HUNK_ID;
	return ( unsigned long int* )( ( char* )ptr + sizeof( unsigned long int ) );
}

	//===========================================================================
	//
	// Parameter:			-
	// Returns:				-
	// Changes Globals:		-
	//===========================================================================
	#ifdef MEMDEBUG
void* GetClearedHunkMemoryDebug( unsigned long size, char* label, char* file, int line )
	#else

/*!
	\brief Allocates memory from the hunk and initializes it to zero

	This function allocates a block of memory of the specified size from the hunk allocator. The allocated memory is initialized to zero before being returned. It supports debug tracking when MEMDEBUG is defined, which records allocation details such as label, file, and line number. The function is typically used to allocate cleared memory blocks for data structures that need to be initialized to zero before use.

	\param size The size in bytes of the memory block to allocate
	\return A pointer to the allocated and cleared memory block
*/
void* GetClearedHunkMemory( unsigned long size )
	#endif // MEMDEBUG
{
	void* ptr;
	#ifdef MEMDEBUG
	ptr = GetHunkMemoryDebug( size, label, file, line );
	#else
	ptr = GetHunkMemory( size );
	#endif // MEMDEBUG
	memset( ptr, 0, size );
	return ptr;
}

/*!
	\brief Frees memory that was allocated with a corresponding AllocMemory call, checking for valid memory identifier before freeing.

	This function is used to free memory that was previously allocated using a matching AllocMemory function. It performs a validity check by examining a memory identifier stored just before the actual memory block. If the identifier matches the expected MEM_ID, it proceeds to free the memory using the botimport.FreeMemory function. This approach helps prevent double-free errors and ensures memory integrity.

	\param ptr Pointer to the memory block to be freed
*/
void FreeMemory( void* ptr )
{
	unsigned long int* memid;

	memid = ( unsigned long int* )( ( char* )ptr - sizeof( unsigned long int ) );

	if( *memid == MEM_ID ) {
		botimport.FreeMemory( memid );
	}
}

/*!
	\brief Prints the current amount of memory used by the application

	This function outputs to the console the total amount of memory currently allocated by the application. It is typically used for debugging or performance monitoring purposes to track memory consumption over time. The function does not take any parameters and does not return any value.

*/
void PrintUsedMemorySize()
{
}

/*!
	\brief Prints memory allocation labels to the console

	This function outputs memory allocation labels that have been registered with the memory manager. It is typically used for debugging purposes to track memory usage and identify potential memory leaks. The function does not take any parameters and does not return any value. It is designed to be called when detailed memory information is needed during development or debugging sessions.

*/
void PrintMemoryLabels()
{
}

#endif
