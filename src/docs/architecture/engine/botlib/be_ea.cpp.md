<!-- doxygenix: sha256=e5a36591f68d65d35d70f206a08c97b49d5f173181daaf2b1e608f4f3c8196a9 -->

# engine/botlib/be_ea.cpp

## File Purpose
- Implements the elementary‑action layer of the bot AI, translating bot decisions into engine‑friendly commands.
- Acts as the bridge between high‑level bot logic and the per‑client input state used by the server.

## Core Responsibilities
- Provide a compact EA_ API for bot actions such as chat, movement, combat, inventory, and weapon handling.
- Translate high‑level bot intentions into low‑level bot_input_t representations and engine command strings.
- Maintain per‑client input state in a globally accessible array of bot_input_t.
- Offer lifecycle hooks (setup, shutdown, reset) that integrate with the bot library’s memory subsystem.

## Key Types and Functions
- EA_Say(int client, char* str) — sends a public chat command.
- EA_SayTeam(int client, char* str) — sends a team‑chat command.
- EA_UseItem(int client, char* it) — executes a "use" command.
- EA_DropItem(int client, char* it) — executes a "drop" command.
- EA_UseInv(int client, char* inv) — inventory‑use command.
- EA_DropInv(int client, char* inv) — inventory‑drop command.
- EA_Command(int client, char* command) — forwards an arbitrary console command.
- EA_SelectWeapon(int client, int weapon) — sets weapon in bot_input_t.
- EA_Attack(int client) — sets ACTION_ATTACK flag.
- EA_Reload(int client) — sets ACTION_RELOAD flag.
- EA_Talk(int client) — sets ACTION_TALK flag.
- EA_Use(int client) — sets ACTION_USE flag.
- EA_Respawn(int client) — sets ACTION_RESPAWN flag.
- EA_Jump(int client) — toggles ACTION_JUMP while preventing consecutive jumps.
- EA_DelayedJump(int client) — toggles ACTION_DELAYEDJUMP flag.
- EA_Gesture(int client) — sets ACTION_GESTURE flag.
- EA_Crouch(int client) — sets ACTION_CROUCH flag.
- EA_Walk(int client) — sets ACTION_WALK flag.
- EA_MoveUp(int client) — sets ACTION_MOVEUP flag.
- EA_MoveDown(int client) — sets ACTION_MOVEDOWN flag.
- EA_MoveForward(int client) — sets ACTION_MOVEFORWARD flag.
- EA_MoveBack(int client) — sets ACTION_MOVEBACK flag.
- EA_MoveLeft(int client) — sets ACTION_MOVELEFT flag.
- EA_MoveRight(int client) — sets ACTION_MOVERIGHT flag.
- EA_Move(int client, vec3_t dir, float speed) — copies direction, clamps speed to ±MAX_USERMOVE.
- EA_View(int client, vec3_t viewangles) — copies view angles into bot_input_t.
- EA_EndRegular(int client, float thinktime) — placeholder for post‑think cleanup.
- EA_GetInput(int client, float thinktime, bot_input_t* input) — copies current input to caller buffer.
- EA_ResetInput(int client, bot_input_t* init) — clears all flags, preserves jump state, copies initializer if provided.
- EA_Setup() — allocates botinputs array for maxclients using GetClearedHunkMemory.
- EA_Shutdown() — frees botinputs with FreeMemory and null‑sets pointer.

## Important Control Flow
- Commands are sent to clients using botimport.BotClientCommand by formatting strings like "say %s" or "invuse %s".
- EA_ functions update the global botinputs[client] entry: setting or clearing actionflags, modifying dir, speed, viewangles, weapon, etc.
- EA_Setup allocates the botinputs array once using GetClearedHunkMemory.
- EA_Shutdown frees that memory with FreeMemory.
- EA_ResetInput clears a client’s input state while preserving ACTION_JUMPEDLASTFRAME and may copy an initializer.
- EA_GetInput copies the current input for a client into a caller‑supplied structure after setting thinktime.
- EA_EndRegular (commented) would normally package the input, send it to the engine, and reset flags.

## External Dependencies
- q_shared.h – core types, va, VectorClear/Copy, qfalse.
- l_memory.h – GetClearedHunkMemory, FreeMemory.
- l_script.h, l_precomp.h, l_struct.h – legacy script/struct utilities.
- botlib.h – definitions of bot_input_t, botlibglobals, and the botimport interface.
- be_interface.h – public API contract for elementary actions.
- Implicit use of global botimport for BotClientCommand and BotInput.

## How It Fits
- Functions as the low‑level command dispatcher within the engine: bots generate intentions → EA_ functions → bot_input_t / CLI → submission to the server.
- Relies on EA_Setup‑allocated botinputs array referenced by other botlib modules such as planners and pathfinders.
- Uses the engine’s botimport interface to treat bot actions like normal client commands.
