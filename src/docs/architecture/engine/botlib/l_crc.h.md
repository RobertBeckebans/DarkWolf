<!-- doxygenix: sha256=cc5d17c7504f31a36679e2fa68a8718d6d08ccda53bc387b5401e35276afb0ab -->

# engine/botlib/l_crc.h

## File Purpose
- Header for bot library CRC functionality, offering deterministic 16‑bit hashing for data integrity and lightweight checks.
- Defines a typedef (crc_t) and declares six functions that collectively implement the CRC lifecycle.

## Core Responsibilities
- Provide a lightweight, 16‑bit CRC computation mechanism used by bot logic and other engine subsystems.
- Expose a clear API for initializing, updating, and finalizing CRC values.
- Encapsulate table‑driven CRC mathematics so callers need not handle polynomial calculations directly.

## Key Types and Functions
- crc_t — typedef of unsigned short representing a 16‑bit CRC value.
- CRC_Init(unsigned short* crcvalue) — initializes the CRC accumulator to a predefined start value.
- CRC_ProcessByte(unsigned short* crcvalue, byte data) — updates the CRC with a single byte using the lookup table.
- CRC_Value(unsigned short crcvalue) — finalizes and returns the CRC by XORing with a constant.
- CRC_ProcessString(unsigned char* data, int length) — computes the CRC for an arbitrary byte array.
- CRC_ContinueProcessString(unsigned short* crc, char* data, int length) — continues a running CRC over an additional data buffer.

## Important Control Flow
- The CRC calculation starts by initializing a 16‑bit accumulator with CRC_Init.
- CRC_ProcessString optionally calls CRC_Init and then iterates over each byte in the supplied data buffer.
- For each byte, the high byte of the current CRC accumulator is XOR‑ed with the byte to form an index into a static lookup table.
- The CRC accumulator is left‑shifted by eight bits and XOR‑ed with the retrieved table entry, producing the new accumulator value.
- After all bytes are processed, CRC_Value XORs the final accumulator with a predefined constant to produce the finished CRC.
- CRC_ContinueProcessString allows resuming a computation by applying the same per‑byte update logic to an existing CRC value.
- CRC_ProcessByte can be used for single‑byte updates, performing the same table lookup and accumulator update as inside the loops.

## External Dependencies
- Requires the type 'byte', typically defined as unsigned char elsewhere in the codebase.
- Relies on a statically‑defined lookup table (crctable) implemented in the corresponding .cpp file.
- No explicit header includes are present; callers must include the surrounding engine headers that define dependent types.

## How It Fits
- Central to file integrity verification within bots, e.g., for validating network packets, model data, or configuration structures.
- Provides a reusable component for all engine modules that need CRC calculations without duplicating logic.
- Fits into the botlib layer, typically called from state serialization, command parsing, or networking code.
