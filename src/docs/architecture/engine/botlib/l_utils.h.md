<!-- doxygenix: sha256=e37b7787f3db07b0afacd10015ea89564d8e2f59201cbfd12ef14f00db7c9fae -->

# engine/botlib/l_utils.h

## File Purpose
- This header supplies lightweight macro definitions for use throughout the bot library, facilitating cross‑module consistency.
- It replaces repeated manual function calls and common inline calculations with descriptive names, aiding readability and maintenance.

## Core Responsibilities
- Defines basic utility macros for bot library, providing a way to convert vectors to angles, and simple inline maximum and minimum calculations.
- Provides a macro to standardize path string length limits, aliasing to MAX_QPATH constant from engine core.
- Encapsulates the call to the engine's vectoangles function through a macro for convenience.

## Key Types and Functions
- Vector2Angles(v, a) — expands to *vectoangles(v, a)*, a wrapper for converting a directional vector to Euler angles.
MAX_PATH — expands to the engine's path limit constant *MAX_QPATH*, used for directory/file string bounds.
Maximum(x, y) — expands to the greater of *x* or *y* using a ternary expression.
Minimum(x, y) — expands to the lesser of *x* or *y* using a ternary expression.

## Important Control Flow

## External Dependencies
- Depends on the definition of *MAX_QPATH* from the engine's path handling headers (e.g. *q_shared.h* or *file_util.h*).
- Relies on the function *vectoangles* which is declared in the engine's math utilities, typically in *trigonometry.h* or similar.

## How It Fits
- *l_utils.h* is included by various bot source files to provide common utilities before compiling the bot behavior code.
- It ensures that bot compilation is independent of the core engine definitions by mapping to engine constants and functions.
