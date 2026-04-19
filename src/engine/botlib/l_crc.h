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
	\file engine/botlib/l_crc.h
	\brief Header for bot library CRC functionality, offering deterministic 16‑bit hashing for data integrity and lightweight checks.
	\note doxygenix: sha256=cc5d17c7504f31a36679e2fa68a8718d6d08ccda53bc387b5401e35276afb0ab

	\par File Purpose
	- Header for bot library CRC functionality, offering deterministic 16‑bit hashing for data integrity and lightweight checks.
	- Defines a typedef (crc_t) and declares six functions that collectively implement the CRC lifecycle.

	\par Core Responsibilities
	- Provide a lightweight, 16‑bit CRC computation mechanism used by bot logic and other engine subsystems.
	- Expose a clear API for initializing, updating, and finalizing CRC values.
	- Encapsulate table‑driven CRC mathematics so callers need not handle polynomial calculations directly.

	\par Key Types and Functions
	- crc_t — typedef of unsigned short representing a 16‑bit CRC value.
	- CRC_Init(unsigned short* crcvalue) — initializes the CRC accumulator to a predefined start value.
	- CRC_ProcessByte(unsigned short* crcvalue, byte data) — updates the CRC with a single byte using the lookup table.
	- CRC_Value(unsigned short crcvalue) — finalizes and returns the CRC by XORing with a constant.
	- CRC_ProcessString(unsigned char* data, int length) — computes the CRC for an arbitrary byte array.
	- CRC_ContinueProcessString(unsigned short* crc, char* data, int length) — continues a running CRC over an additional data buffer.

	\par Control Flow
	- The CRC calculation starts by initializing a 16‑bit accumulator with CRC_Init.
	- CRC_ProcessString optionally calls CRC_Init and then iterates over each byte in the supplied data buffer.
	- For each byte, the high byte of the current CRC accumulator is XOR‑ed with the byte to form an index into a static lookup table.
	- The CRC accumulator is left‑shifted by eight bits and XOR‑ed with the retrieved table entry, producing the new accumulator value.
	- After all bytes are processed, CRC_Value XORs the final accumulator with a predefined constant to produce the finished CRC.
	- CRC_ContinueProcessString allows resuming a computation by applying the same per‑byte update logic to an existing CRC value.
	- CRC_ProcessByte can be used for single‑byte updates, performing the same table lookup and accumulator update as inside the loops.

	\par Dependencies
	- Requires the type 'byte', typically defined as unsigned char elsewhere in the codebase.
	- Relies on a statically‑defined lookup table (crctable) implemented in the corresponding .cpp file.
	- No explicit header includes are present; callers must include the surrounding engine headers that define dependent types.

	\par How It Fits
	- Central to file integrity verification within bots, e.g., for validating network packets, model data, or configuration structures.
	- Provides a reusable component for all engine modules that need CRC calculations without duplicating logic.
	- Fits into the botlib layer, typically called from state serialization, command parsing, or networking code.
*/

//===========================================================================
//
// Name:				l_crc.h
// Function:		for CRC checks
// Programmer:		Mr Elusive (MrElusive@demigod.demon.nl)
// Last update:	1997-12-31
// Tab Size:		3
//===========================================================================

typedef unsigned short crc_t;

/*!
	\brief Sets the initial CRC value.

	The CRC value pointed to by the parameter is assigned the predefined initial value used to start CRC computation.

	\param crcvalue Pointer to the variable that will receive the initial CRC value.
*/
void				   CRC_Init( unsigned short* crcvalue );

/*!
	\brief Updates the CRC value with the supplied byte using a lookup table

	The function shifts the current CRC value left by eight bits and then XORs the result with an entry from the static lookup table. The table index is computed from the high byte of the current CRC
   value XORed with the data byte. The resulting 16‑bit value becomes the new CRC. This operation is performed in place through the pointer passed in the first parameter.

	\param crcvalue Pointer to the CRC value to be updated in place
	\param data Byte to incorporate into the CRC calculation
*/
void				   CRC_ProcessByte( unsigned short* crcvalue, byte data );

/*!
	\brief XORs the supplied CRC value with a predefined constant.

	This function applies a bitwise exclusive-or between the input CRC value and CRC_XOR_VALUE, which finalizes the CRC calculation used elsewhere in the library.

	\param crcvalue Initial CRC value that will be XORed with the constant.
	\return Resulting unsigned short CRC after XOR operation.
*/
unsigned short		   CRC_Value( unsigned short crcvalue );

/*!
	\brief Calculates the 16‑bit CRC value for a byte array.

	The function initializes a CRC accumulator, then iterates over the supplied data buffer byte by byte. For each byte, it computes an index into a prefilled table based on the current high byte of
   the CRC combined with the data byte. The CRC accumulator is updated by shifting and XORing with the table entry. After all bytes are processed, the final CRC value is returned via CRC_Value. A
   guard ensures that an out‑of‑range index defaults to zero, although a correct table should never produce such a value.

	\param data pointer to the data buffer to hash
	\param length number of bytes in the buffer
	\return the calculated 16‑bit CRC value
*/
unsigned short		   CRC_ProcessString( unsigned char* data, int length );

/*!
	\brief Continues a running CRC computation by processing the supplied byte sequence into the provided CRC value.

	The routine iterates through each byte of the data buffer, updating the 16‑bit CRC in place. For each character it left‑shifts the current CRC, performs a table lookup into crctable using the XOR
   of the high byte of the current CRC with the data byte, and stores the result back into the CRC accumulator. The function assumes that crctable is a precomputed table for the chosen polynomial and
   that the data pointer refers to a valid buffer of the given length.

	\param crc pointer to the current CRC value that will be updated in place
	\param data pointer to the array of characters to process
	\param length number of characters in data to include in the computation
*/
void				   CRC_ContinueProcessString( unsigned short* crc, char* data, int length );
