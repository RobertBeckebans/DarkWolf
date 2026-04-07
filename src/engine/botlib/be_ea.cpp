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
 * name:		be_ea.c
 *
 * desc:		elementary actions
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "botlib.h"
#include "be_interface.h"

#define MAX_USERMOVE		   400
#define MAX_COMMANDARGUMENTS   10
#define ACTION_JUMPEDLASTFRAME 128

bot_input_t* botinputs;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void		 EA_Say( int client, char* str )
{
	botimport.BotClientCommand( client, va( "say %s", str ) );
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_SayTeam( int client, char* str )
{
	botimport.BotClientCommand( client, va( "say_team %s", str ) );
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_UseItem( int client, char* it )
{
	botimport.BotClientCommand( client, va( "use %s", it ) );
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_DropItem( int client, char* it )
{
	botimport.BotClientCommand( client, va( "drop %s", it ) );
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_UseInv( int client, char* inv )
{
	botimport.BotClientCommand( client, va( "invuse %s", inv ) );
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_DropInv( int client, char* inv )
{
	botimport.BotClientCommand( client, va( "invdrop %s", inv ) );
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Gesture( int client )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_GESTURE;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_Command( int client, char* command )
{
	botimport.BotClientCommand( client, command );
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_SelectWeapon( int client, int weapon )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	bi->weapon = weapon;
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Attack( int client )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_ATTACK;
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Reload( int client )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_RELOAD;
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Talk( int client )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_TALK;
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Use( int client )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_USE;
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Respawn( int client )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_RESPAWN;
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Jump( int client )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	if( bi->actionflags & ACTION_JUMPEDLASTFRAME ) {
		bi->actionflags &= ~ACTION_JUMP;
	} else {
		bi->actionflags |= ACTION_JUMP;
	}
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_DelayedJump( int client )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	if( bi->actionflags & ACTION_JUMPEDLASTFRAME ) {
		bi->actionflags &= ~ACTION_DELAYEDJUMP;
	} else {
		bi->actionflags |= ACTION_DELAYEDJUMP;
	}
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Crouch( int client )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_CROUCH;
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Walk( int client )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_WALK;
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_MoveUp( int client )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVEUP;
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_MoveDown( int client )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVEDOWN;
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_MoveForward( int client )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVEFORWARD;
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_MoveBack( int client )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVEBACK;
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_MoveLeft( int client )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVELEFT;
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_MoveRight( int client )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVERIGHT;
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Move( int client, vec3_t dir, float speed )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	VectorCopy( dir, bi->dir );
	// cap speed
	if( speed > MAX_USERMOVE ) {
		speed = MAX_USERMOVE;
	} else if( speed < -MAX_USERMOVE ) {
		speed = -MAX_USERMOVE;
	}
	bi->speed = speed;
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_View( int client, vec3_t viewangles )
{
	bot_input_t* bi;

	bi = &botinputs[client];

	VectorCopy( viewangles, bi->viewangles );
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_EndRegular( int client, float thinktime )
{
	/*
		bot_input_t *bi;
		int jumped = qfalse;

		bi = &botinputs[client];

		bi->actionflags &= ~ACTION_JUMPEDLASTFRAME;

		bi->thinktime = thinktime;
		botimport.BotInput(client, bi);

		bi->thinktime = 0;
		VectorClear(bi->dir);
		bi->speed = 0;
		jumped = bi->actionflags & ACTION_JUMP;
		bi->actionflags = 0;
		if (jumped) bi->actionflags |= ACTION_JUMPEDLASTFRAME;
	*/
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_GetInput( int client, float thinktime, bot_input_t* input )
{
	bot_input_t* bi;
	//	int jumped = qfalse;

	bi = &botinputs[client];

	//	bi->actionflags &= ~ACTION_JUMPEDLASTFRAME;

	bi->thinktime = thinktime;
	memcpy( input, bi, sizeof( bot_input_t ) );

	/*
	bi->thinktime = 0;
	VectorClear(bi->dir);
	bi->speed = 0;
	jumped = bi->actionflags & ACTION_JUMP;
	bi->actionflags = 0;
	if (jumped) bi->actionflags |= ACTION_JUMPEDLASTFRAME;
	*/
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_ResetInput( int client, bot_input_t* init )
{
	bot_input_t* bi;
	int			 jumped = qfalse;

	bi = &botinputs[client];
	bi->actionflags &= ~ACTION_JUMPEDLASTFRAME;

	bi->thinktime = 0;
	VectorClear( bi->dir );
	bi->speed		= 0;
	jumped			= bi->actionflags & ACTION_JUMP;
	bi->actionflags = 0;
	if( jumped ) {
		bi->actionflags |= ACTION_JUMPEDLASTFRAME;
	}

	if( init ) {
		memcpy( bi, init, sizeof( bot_input_t ) );
	}
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int EA_Setup()
{
	// initialize the bot inputs
	botinputs = ( bot_input_t* )GetClearedHunkMemory( botlibglobals.maxclients * sizeof( bot_input_t ) );
	return BLERR_NOERROR;
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Shutdown()
{
	FreeMemory( botinputs );
	botinputs = NULL;
}
