<!-- doxygenix: sha256=082b67635494ec28f4640bd7f4e501cd72a5884abffe375939ba57d2a78c2133 -->

# engine/botlib/l_crc.cpp

## File Purpose
- Implements a small, efficient CRC algorithm used by the bot library to hash strings, packet payloads or identifiers.

## Core Responsibilities
- Provide a deterministic 16‑bit CRC (CCITT XModem) implementation for bot and engine modules.
- Offer lifecycle functions to initialise, update, finalise, and process arbitrary byte arrays in a single pass.
- Expose a static lookup table (crctable) for fast polynomial evaluation.

## Key Types and Functions
- crctable[257] – static array holding pre‑computed 16‑bit CRC table entries for polynomial 0x1021.
- CRC_Init(unsigned short* crcvalue) – sets *crcvalue to 0xFFFF.
- CRC_ProcessByte(unsigned short* crcvalue, byte data) – updates *crcvalue by incorporating a single byte.
- CRC_Value(unsigned short crcvalue) – XORs crcvalue with 0x0000 and returns the result.
- CRC_ProcessString(unsigned char* data, int length) – computes the CRC of a byte buffer and returns the final value.
- CRC_ContinueProcessString(unsigned short* crc, char* data, int length) – appends the CRC of a buffer to an existing CRC value.

## Important Control Flow
- A CRC calculation begins with CRC_Init setting the accumulator to CRC_INIT_VALUE (0xFFFF).
- Data bytes are processed either one by one with CRC_ProcessByte or in bulk by CRC_ProcessString or CRC_ContinueProcessString.
- Each byte is integrated by shifting the current CRC value left 8 bits, XOR‑ing the high byte with the data byte to index the 257‑entry crctable, and XOR‑ing the table entry with the shifted value.
- Upon completion of the byte stream, CRC_Value applies a final XOR with CRC_XOR_VALUE (0x0000) to produce the final checksum.

## External Dependencies
- "q_shared.h"
- "botshared/botlib.h"
- "be_interface.h" // used for botimport.Print (unused in current file)

## How It Fits
- The botlib relies on CRC for verification of command strings, caching of parsed data, or lightweight integrity checks during gameplay.
- CRC functions are called by higher‑level bot utilities and game logic where fast, portable checksums are sufficient.
- This file encapsulates CRC logic so other engine components can remain agnostic of the polynomial details.
