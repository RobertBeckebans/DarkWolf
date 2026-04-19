<!-- doxygenix: sha256=1aad9171335b10b541868724c3ead7f63bba9d13edbd7104cd8c5645e34c7f04 -->

# engine/botlib/be_interface.cpp

## File Purpose
- Implements the bot library interface, providing exported functions for initialization, shutdown, entity management, and debugging.

## Core Responsibilities
- Initializes and exposes bot library API to the engine.
- Configures AAS, EA, and AI subsystem export tables.

## Key Types and Functions
- botlib_globals_t botlibglobals — structure holding global bot state such as maxclients, maxentities, and setup flag.
- botlib_export_t be_botlib_export — the exported API table.
- botlib_import_t botimport — imported engine callbacks used by bot library.
- int botlibsetup — flag marking whether library has been initialized.
- int Sys_MilliSeconds() — platform‑agnostic millisecond counter.
- qboolean ValidClientNumber(int num, char* str) — checks client index against botlibglobals.maxclients.
- qboolean ValidEntityNumber(int num, char* str) — checks entity index against botlibglobals.maxentities.
- qboolean BotLibSetup(char* str) — verifies that bot library is currently set up.
- int Export_BotLibSetup() — performs full bot library initialization, configuring debug flags, setting up AAS, EA, movement AI, and global variables.
- int Export_BotLibShutdown() — orderly de‑initializes subsystems, cleans globals, and handles recursion guard.
- int Export_BotLibVarSet(char* var_name, char* value) — sets a libvar.
- int Export_BotLibVarGet(char* var_name, char* value, int size) — retrieves a libvar string.
- int Export_BotLibStartFrame(float time) — begins a frame after verifying setup and delegates to AAS_StartFrame.
- int Export_BotLibLoadMap(const char* mapname) — loads AAS data for a map and initializes level items.
- int Export_BotLibUpdateEntity(int ent, bot_entitystate_t* state) — updates an entity state in AAS after validation.
- int BotExportTest(int parm0, char* parm1, vec3_t parm2, vec3_t parm3) — debug routine that visualizes and logs navigation data; active only in DEBUG builds.
- static void Init_AAS_Export(aas_export_t* aas) — populates AAS export table with function pointers to AAS subsystem implementations.
- static void Init_EA_Export(ea_export_t* ea) — populates EA export table with function pointers to EA subsystem implementations.
- static void Init_AI_Export(ai_export_t* ai) — populates AI export table with function pointers to AI subsystem implementations.
- botlib_export_t* GetBotLibAPI(int apiVersion, botlib_import_t* import) — validates API version, sets up export tables, and returns pointer to be_botlib_export.

## Important Control Flow
- botimport is set from import argument in GetBotLibAPI; be_botlib_export table zeroed and filled using Init_*Export functions; botlibglobals initialized during Export_BotLibSetup; subsequent bot operations call exported functions, which check BotLibSetup and delegate to AAS, EA, AI subsystems; Export_BotLibShutdown reverse order cleans up; BotExportTest provides debug diagnostics

## External Dependencies
- q_shared.h
- l_memory.h
- l_log.h
- l_libvar.h
- l_script.h
- l_precomp.h
- l_struct.h
- aasfile.h
- botshared/botlib.h
- botshared/be_aas.h
- be_aas_funcs.h
- be_aas_def.h
- be_interface.h
- botshared/be_ea.h
- be_ai_weight.h
- botshared/be_ai_goal.h
- botshared/be_ai_move.h
- botshared/be_ai_weap.h
- botshared/be_ai_chat.h
- botshared/be_ai_char.h
- botshared/be_ai_gen.h

## How It Fits
- This file is the central entry point for the bot subsystem; it constructs the exported function table used by the game engine to interact with bot logic. It wires together lower‑level AAS, EA, and AI components and exposes configuration functions.
