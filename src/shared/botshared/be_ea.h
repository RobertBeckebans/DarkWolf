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
 * name:		be_ea.h
 *
 * desc:		elementary actions
 *
 *
 *****************************************************************************/

/*!
	\brief Executes a say command for a client with the specified message.

	This function sends a say command to the specified client, allowing the client to broadcast a message to all players in the game. The function uses the botimport interface to execute the command,
   formatting the input string into a say command before sending it.

	\param client The client identifier for whom the say command is executed
	\param str The message string to be sent as a say command
*/
void EA_Say( int client, char* str );

/*!
	\brief Sends a team chat message from a bot client.

	This function constructs a chat command for the specified client to send a team message. It uses the va function to format the message and then calls the bot client command function to execute the
   command.

	\param client The index of the bot client sending the message
	\param str The team message to be sent
*/
void EA_SayTeam( int client, char* str );

/*!
	\brief Executes a use command for a specified item by a client

	This function sends a use command to the game engine for a specific client to use a given item. It constructs the command string by formatting "use %s" with the item name and then dispatches it
   through the bot import interface

	\param client The client identifier for which the command is being executed
	\param it The item to be used
*/
void EA_UseItem( int client, char* it );

/*!
	\brief Drops the specified item for the given client.

	This function sends a drop command to the bot client to drop the specified item. It uses the bot import interface to execute the command.

	\param client The client identifier for which the item should be dropped
	\param it The item to be dropped
*/
void EA_DropItem( int client, char* it );

/*!
	\brief Executes the use inventory command for a bot client

	This function sends an inventory use command to a specified bot client. It constructs a command string in the format "invuse <item>" using the provided inventory item name and passes it to the bot
   client command interface. The function is part of the elementary actions interface for bot behavior control.

	\param client The client identifier for the bot that will execute the inventory use command
	\param inv The inventory item name to be used
*/
void EA_UseInv( int client, char* inv );

/*!
	\brief Performs an elementary action to drop a specified inventory item for a client.

	This function sends a command to the bot client to drop a specific inventory item. It constructs a command string using the va function format, which includes the client identifier and the item to
   be dropped. The actual execution of the command is delegated to the botimport.BotClientCommand function.

	\param client The identifier of the client for whom the inventory item should be dropped
	\param inv The name or identifier of the inventory item to be dropped
*/
void EA_DropInv( int client, char* inv );

/*!
	\brief Executes a command for a specific client using the bot library interface

	This function serves as a wrapper to execute commands for a specified client through the bot library interface. It takes a client identifier and a command string and forwards them to the bot
   library's client command handler. The function is typically used in game engine contexts where bots need to perform specific actions or commands.

	\param client Identifier for the client (typically a bot) for which the command is executed
	\param command Command string to be executed for the specified client
*/
void EA_Command( int client, char* command );

/*!
	\brief Sets the weapon for a specified client in the bot input system.

	This function updates the weapon selection for a given client by storing the weapon ID in the corresponding bot input structure. It is part of the elementary actions interface used by the game's
   bot system to control various player actions.

	\param client The client identifier for which the weapon is being selected
	\param weapon The weapon identifier to be selected for the client
*/
void EA_SelectWeapon( int client, int weapon );

/*!
	\brief Sets the attack action flag for the specified client.

	This function sets the ACTION_ATTACK flag for a given client, indicating that the client should perform an attack action. The function accesses the bot input structure for the specified client and
   modifies its action flags to include the attack flag.

	\param client The identifier of the client for which to set the attack action
*/
void EA_Attack( int client );

/*!
	\brief Sets the reload action flag for the specified client.

	This function marks the client input state to indicate that a reload action should be performed. It accesses the bot input structure for the given client and sets the ACTION_RELOAD flag within the
   action flags. This is typically used by the bot library to communicate that a weapon reload should be initiated.

	\param client The identifier for the client or bot that should perform the reload action
*/
void EA_Reload( int client );

/*!
	\brief Sets the respawn action flag for the specified client.

	This function marks a client for respawn by setting the ACTION_RESPAWN flag in the client's input structure. It is part of the bot library's elementary actions interface and is typically called
   when a bot needs to respawn in the game.

	\param client The index of the client (bot) for which to set the respawn action
*/
void EA_Respawn( int client );

/*!
	\brief Sets the talk action flag for a specified client.

	This function is used to mark a client as performing a talk action. It accesses the bot input structure for the given client and sets the ACTION_TALK flag in the action flags. The function is part
   of the elementary actions interface used by the game engine for bot control.

	\param client The index of the client for whom to set the talk action
*/
void EA_Talk( int client );

/*!
	\brief Sets the gesture action flag for a specified client.

	This function is used to mark a client as performing a gesture action. It accesses the bot input structure for the given client and sets the ACTION_GESTURE flag in the action flags. The function
   is part of the elementary actions interface used by the game engine for bot control.

	\param client The index of the client for whom to set the gesture action
*/
void EA_Gesture( int client );

/*!
	\brief Sets the use action flag for a specified bot client

	This function marks a bot client as needing to perform a use action. It accesses the bot input structure for the given client and sets the ACTION_USE flag in the action flags. This is typically
   used by the bot library to indicate that a specific bot should execute a use command at the next available opportunity.

	\param client The index of the bot client for which to set the use action
*/
void EA_Use( int client );

/*!
	\brief Toggles the jump action for a bot client, preventing consecutive jumps.

	This function manages the jump action for a bot client by checking if the bot already jumped on the previous frame. If so, it clears the jump action flag; otherwise, it sets the jump action flag.
   This prevents bots from continuously jumping in succession.

	\param client The index of the bot client for which to toggle the jump action
*/
void EA_Jump( int client );

/*!
	\brief Toggles a delayed jump action for a specified bot client.

	This function manages the delayed jump state for a bot client. It checks if the bot previously jumped on the last frame, and if so, it clears the delayed jump flag. Otherwise, it sets the delayed
   jump flag. This mechanism allows for controlled jump behavior in bot AI, potentially preventing consecutive jumps or managing jump timing.

	\param client The index of the bot client for which to toggle the delayed jump action.
*/
void EA_DelayedJump( int client );

/*!
	\brief Sets the crouch action flag for the specified bot client.

	This function is used to instruct a bot client to crouch by setting the ACTION_CROUCH flag in the bot's input structure. It is part of the bot library's input system and is called by the game
   engine to control bot behavior. The function operates on a global array of bot input structures indexed by the client number.

	\param client The index of the bot client for which to set the crouch action.
*/
void EA_Crouch( int client );

/*!
	\brief Sets the walk action flag for the specified client.

	This function enables the walk action for a bot client by modifying the action flags in the bot input structure. It is typically used to instruct a bot to start walking.

	\param client The index of the client bot for which to set the walk action
*/
void EA_Walk( int client );

/*!
	\brief Sets the move up action flag for the specified bot client.

	This function is used to mark that a bot client should perform a move up action. It accesses the bot input structure for the given client and sets the ACTION_MOVEUP flag in the action flags. This
   is typically called as part of the bot's input processing to indicate movement intentions.

	\param client The client ID of the bot for which to set the move up action
*/
void EA_MoveUp( int client );

/*!
	\brief Sets the move down action flag for the specified client.

	This function is used to indicate that the client should move downward. It modifies the action flags of the bot input structure for the given client by setting the ACTION_MOVEDOWN flag. This is
   part of the bot input handling system used in the game engine.

	\param client The client identifier for which to set the move down action
*/
void EA_MoveDown( int client );

/*!
	\brief Sets the forward movement action flag for the specified client bot.

	This function activates the forward movement action for a bot identified by the client index. It accesses the bot input structure for the given client and sets the ACTION_MOVEFORWARD flag, which
   indicates that the bot should move forward in the game world. The function is part of the bot library's input handling system and is typically called by bot AI routines to control bot movement.

	\param client Index of the client bot for which to set the forward movement flag
*/
void EA_MoveForward( int client );

/*!
	\brief Sets the move back action flag for the specified client

	This function is used to indicate that the client should move backwards. It accesses the bot input structure for the given client and sets the ACTION_MOVEBACK flag in the action flags. This is
   part of the bot input system that handles various movement and action commands for AI-controlled characters in the game.

	\param client The client identifier for which to set the move back action
*/
void EA_MoveBack( int client );

/*!
	\brief Sets the move left action flag for the specified client bot.

	This function is used to indicate that the bot associated with the given client should move left. It modifies the action flags of the bot's input structure to include the move left action. The
   function is typically called as part of the bot's input handling system in the game engine.

	\param client The client identifier for the bot that should move left
*/
void EA_MoveLeft( int client );

/*!
	\brief Sets the right movement action flag for the specified bot client.

	This function is used to indicate that the bot client should move to the right. It modifies the action flags of the bot's input structure to include the ACTION_MOVERIGHT flag. The function is part
   of the bot library's input handling system and is typically called by the game engine or bot AI logic to control bot movement. The client parameter identifies which bot client's input state is
   being modified.

	\param client The identifier of the bot client for which to set the right movement flag
*/
void EA_MoveRight( int client );

/*!
	\brief Sets the movement direction and speed for a bot client

	This function configures the movement parameters for a specified bot client by setting the direction vector and speed. The direction is copied into the bot's input structure, and the speed is
   clamped between negative and positive MAX_USERMOVE values to ensure it stays within valid bounds. This function is part of the bot input system and is used to control how bots move within the game
   environment.

	\param client The index of the bot client for which to set movement
	\param dir The 3D direction vector specifying the movement direction
	\param speed The speed value for the movement, which will be clamped to the range [-MAX_USERMOVE, MAX_USERMOVE]
*/
void EA_Move( int client, vec3_t dir, float speed );

/*!
	\brief Sets the view angles for a specified bot client.

	This function updates the view angles for a given bot client by copying the provided viewangles vector into the corresponding bot input structure. It is used to control the direction the bot is
   looking in the game world. The function accesses the botinputs array which stores input data for each bot client, and updates the viewangles member of the bot_input_t structure for the specified
   client.

	\param client The index of the bot client whose view angles are to be set
	\param viewangles The new view angles to be applied to the bot client
*/
void EA_View( int client, vec3_t viewangles );

/*!
	\brief Sends regular input to the server for a bot client.

	This function prepares and sends regular input data to the server for a specified bot client. It sets up the bot's input state with the provided think time, processes the input through the bot
   library, and then resets the input state. The function is part of the bot AI interface and is used to end a regular input cycle for a bot entity.

	\param client The client identifier for the bot
	\param thinktime The think time to be set for the bot input
*/
void EA_EndRegular( int client, float thinktime );

/*!
	\brief Retrieves the input state for a specified bot client and copies it to the provided input structure.

	This function fetches the input state for a given bot client identified by the client index. It updates the think time for the bot and then copies the complete input state from an internal buffer
   to the provided input structure. The function is typically used in the game engine's bot framework to retrieve the current input state for bot entities before they process their actions. The input
   structure contains movement direction, speed, and action flags that represent the bot's current state and intended actions.

	\param client Index of the bot client whose input state is to be retrieved
	\param thinktime Time interval for the bot to think before taking actions
	\param input Pointer to the structure where the bot's input state will be copied
*/
void EA_GetInput( int client, float thinktime, bot_input_t* input );

/*!
	\brief Resets the input state for a bot client, optionally initializing it with provided values.

	This function resets the input state of a bot client identified by the client index. It clears the current input state and optionally copies initial input values from the provided init structure.
   The function preserves the jump state between resets to maintain accurate movement behavior.

	\param client The index of the bot client whose input state is to be reset
	\param init Pointer to an optional initial input state structure to copy values from
*/
void EA_ResetInput( int client, bot_input_t* init );

/*!
	\brief Initializes bot inputs for the bot library

	Sets up the bot input structure memory for the maximum number of clients. This function allocates memory for bot inputs and returns a status code indicating success or failure of the
   initialization process.

	\return Error code indicating the result of the initialization process, where BLERR_NOERROR indicates success
*/
int	 EA_Setup();

/*!
	\brief Frees the memory allocated for bot inputs and sets the botinputs pointer to NULL.

	This function is responsible for cleaning up the memory previously allocated for bot input data. It first calls FreeMemory to release the memory pointed to by the botinputs variable, and then sets
   the botinputs pointer to NULL to prevent dangling pointer issues.

*/
void EA_Shutdown();
