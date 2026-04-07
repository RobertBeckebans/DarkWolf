# AGENTS

This file provides instructions for AI agents to understand the layout of the DarkWolf repository, run builds/tests, and follow project conventions.
DarkWolf is a fork of the Return to Castle Wolfenstein id Tech 3 engine by id Software.

## Repository Structure

- `/build/`                : Build outputs and intermediates (bin, obj, VS projects)
  - `/build/bin/`          : Built binaries by platform/config
  - `/build/obj/`          : Object files and build logs by target/config
  - `/build/vs2022/`       : Visual Studio 2022 project files
- `/chats/`                : AI Chat/session artifacts
- `/diags/`                : Visual Studio Diagnostic outputs and logs
- `/doxygen-xml/`          : Doxygen XML output which serves as input for doxygenix.py agentic comment generator
- `/engine/`               : Engine source code
  - `/engine/botlib/`      : Bot library
  - `/engine/client/`      : Client code
  - `/engine/collision/`   : Collision system
  - `/engine/qcommon/`     : Core shared engine code
  - `/engine/renderer/`    : Rendering system
  - `/engine/server/`      : Server code
  - `/engine/sound/`       : Audio system
  - `/engine/splines/`     : Spline utilities
  - `/engine/win32/`       : Windows-specific code
- `/games/`                : Game logic modules
  - `/games/default/cgame/`: Client game module
  - `/games/default/game/` : Game module (server-side)
  - `/games/default/ui/`   : UI module
- `/libs/`                 : Third-party libraries
  - `/libs/ft2/`           : FreeType
  - `/libs/jpeg-6/`        : JPEG library
  - `/libs/openal/`        : OpenAL SDK and libs
  - `/libs/opengl/`        : OpenGL to DX12 wrapper IceBridge
  - `/libs/zlib/`          : zlib
- `/premake/`              : Premake scripts/config
- `/shared/`               : Shared code between engine and game modules
  - `/shared/botshared/`   : Shared bot logic
- `/tools/`                : Tooling and utilities
  - `/tools/bspc/`         : AAS compiler
  - `/tools/extractfuncs/` : Utility tool to generate games/default/game/g_funcs.h for save games

## Conventions

- Keep engine code in `/engine/` and game modules in `/games/`.
- Place third-party dependencies under `/libs/`.
- Build artifacts must stay under `/build/` and should not be checked in if avoidable.
- Documentation outputs (Doxygen) go to `/doxygen-xml/`.

## Notes for Agents

- Avoid editing build outputs under `/build/`.
- Prefer edits in `/engine/`, `/games/`, `/shared/`, or `/tools/` depending on the task.
- Check formatting config in the repository root (`.clang-format-*`, `astyle-*.ini`).
