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

/*****************************************************************************
 * name:		l_memory.h
 *
 * desc:		memory management
 *
 *
 *****************************************************************************/

#ifdef _DEBUG
	#ifndef BSPC
		#define MEMDEBUG
	#endif
#endif

#ifdef MEMDEBUG
	#define GetMemory( size )		 GetMemoryDebug( size, #size, __FILE__, __LINE__ );
	#define GetClearedMemory( size ) GetClearedMemoryDebug( size, #size, __FILE__, __LINE__ );
// allocate a memory block of the given size
void* GetMemoryDebug( unsigned long size, char* label, char* file, int line );
// allocate a memory block of the given size and clear it
void* GetClearedMemoryDebug( unsigned long size, char* label, char* file, int line );
	//
	#define GetHunkMemory( size )		 GetHunkMemoryDebug( size, #size, __FILE__, __LINE__ );
	#define GetClearedHunkMemory( size ) GetClearedHunkMemoryDebug( size, #size, __FILE__, __LINE__ );
// allocate a memory block of the given size
void* GetHunkMemoryDebug( unsigned long size, char* label, char* file, int line );
// allocate a memory block of the given size and clear it
void* GetClearedHunkMemoryDebug( unsigned long size, char* label, char* file, int line );
#else

/*!
\brief Allocates a block of memory for the requested size and records a debug identifier.

The function calls botimport.GetMemory with size plus space for an unsigned long header that holds the constant MEM_ID. If the allocation succeeds, MEM_ID is written into this header and the
function returns a pointer to the payload just after the header; if allocation fails, NULL is returned. This wrapper preserves the original allocation while adding a debug header for memory
tracking.

\param size Requested number of bytes for the caller.
\return Pointer to the allocated memory payload, or NULL if allocation fails.
*/
void* GetMemory( unsigned long size );

/*!
\brief Allocates a block of the specified size, zeroes it, and returns a pointer to it.

In normal builds the function obtains memory via a call to GetMemory before zeroing the newly allocated buffer. When MEMDEBUG is enabled it calls GetMemoryDebug, which records allocation metadata
such as filename and line number along with a label; the buffer is then cleared with memset. The caller receives a pointer to the zeroed memory.

The function does not perform any error checking on the allocation result and simply forwards whatever pointer GetMemory/GetMemoryDebug returns.

This utility is typically used during initialization of large data structures, for example the bot engine’s portal, index or cluster arrays.

\param size The number of bytes to allocate and clear.
\return A pointer to the allocated and zeroed memory block, or the null pointer if the underlying allocation failed.
*/
void* GetClearedMemory( unsigned long size );
	//
	#ifdef BSPC
		#define GetHunkMemory		 GetMemory
		#define GetClearedHunkMemory GetClearedMemory
	#else

/*!
\brief Allocates a block of hunk memory of the requested size and returns a pointer to it.

The function requests memory from the hunk allocator, allocating an extra space to store a distinguishing header value (HUNK_ID). It first calls botimport.HunkAlloc with the size increased by the
header size. If that fails, it returns NULL. Otherwise, it writes the HUNK_ID marker at the beginning of the block and returns a pointer that skips over the header, giving the caller a clean block
of the requested size. This header can be used by debugging tools to identify blocks allocated by this function.

\param size number of bytes requested for allocation, excluding the header that the function adds internally
\return Pointer to the usable memory region immediately following the header, or NULL if the allocation failed.
*/
void* GetHunkMemory( unsigned long size );

/*!
\brief Allocates a block of memory of the requested size from the hunk allocator, zeroes its contents, and returns the pointer.

When MEMDEBUG is defined, the allocation uses GetHunkMemoryDebug to record debug information such as label, file, and line.
Otherwise, GetHunkMemory is used. After allocation the new memory is cleared to zero with memset. The function does not perform any error handling, so the returned pointer may be null if the
allocation fails.

\param size number of bytes to allocate and clear
\return pointer to the zero‑initialized memory block
*/
void* GetClearedHunkMemory( unsigned long size );
	#endif
#endif

/*!
	\brief Releases a memory block previously allocated by the Bot import module if it is identified as a valid block.

	The function first calculates a pointer to the unsigned long integer that precedes the requested block. That integer is expected to contain the MEM_ID marker placed during allocation. If the
   marker matches the predefined MEM_ID value, the function delegates the actual deallocation to the bot import's FreeMemory routine, passing the full block address. This safeguards against freeing
   memory that was not allocated through the corresponding allocation function.

	\param ptr Pointer to the memory block that was returned by an allocation routine that prefixes the block with a MEM_ID marker.
*/
void FreeMemory( void* ptr );

/*!
	\brief Prints the total used memory size

	This function outputs the cumulative amount of memory currently allocated by the system, providing insight into memory consumption for debugging or profiling purposes

*/
void PrintUsedMemorySize();

/*!
	\brief Prints all memory blocks with labels.

	This function is intended to iterate over the memory manager’s internal list of blocks and output diagnostic information for each block, such as its label and size. It serves as a debugging aid to
   verify memory allocation and labeling. Currently the implementation is empty, leaving the functionality to be provided.


*/
void PrintMemoryLabels();
// returns the size of the memory block in bytes
int	 MemoryByteSize( void* ptr );
// free all allocated memory
void DumpMemory();
