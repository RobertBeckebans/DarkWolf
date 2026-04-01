/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code ("RTCW SP Source Code").

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

// win_input.cpp -- win32 mouse and XInput controller code

#include "../client/client.h"
#include "win_local.h"

#include <mmsystem.h>
#include <xinput.h>

#pragma comment( lib, "winmm.lib" )

typedef struct {
	int		 oldButtonState;

	qboolean mouseActive;
	qboolean mouseInitialized;
} WinMouseVars_t;

static WinMouseVars_t s_wmv;

static int			  window_center_x, window_center_y;

//
// MIDI definitions
//
static void			  IN_StartupMIDI();
static void			  IN_ShutdownMIDI();

#define MAX_MIDIIN_DEVICES 8

typedef struct {
	int		   numDevices;
	MIDIINCAPS caps[MAX_MIDIIN_DEVICES];

	HMIDIIN	   hMidiIn;
} MidiInfo_t;

static MidiInfo_t s_midiInfo;

//
// XInput definitions
//
typedef DWORD( WINAPI* PFN_XInputGetState )( DWORD dwUserIndex, XINPUT_STATE* pState );
typedef DWORD( WINAPI* PFN_XInputSetState )( DWORD dwUserIndex, XINPUT_VIBRATION* pVibration );

static PFN_XInputGetState pXInputGetState = NULL;
static PFN_XInputSetState pXInputSetState = NULL;
static HMODULE			  s_xinputDll	  = NULL;

typedef struct {
	qboolean	 avail;
	int			 id; // 0..3

	DWORD		 oldPacketNumber;
	WORD		 oldButtons;
	BYTE		 oldLeftTriggerPressed;
	BYTE		 oldRightTriggerPressed;
	int			 oldpovstate;

	XINPUT_STATE state;
} joystickInfo_t;

static joystickInfo_t joy;

cvar_t*				  k_language;

cvar_t*				  in_midi;
cvar_t*				  in_midiport;
cvar_t*				  in_midichannel;
cvar_t*				  in_mididevice;

cvar_t*				  in_mouse;
cvar_t*				  in_joystick;
cvar_t*				  in_joyBallScale;
cvar_t*				  in_debugJoystick;
cvar_t*				  joy_threshold;

qboolean			  in_appactive;

// forward-referenced functions
void				  IN_StartupJoystick();
void				  IN_JoyMove();

static void			  MidiInfo_f();

static qboolean		  IN_InitXInput();
static void			  IN_ShutdownXInput();

/*
============================================================

WIN32 MOUSE CONTROL

============================================================
*/

/*
================
IN_InitWin32Mouse
================
*/
void				  IN_InitWin32Mouse()
{
}

/*
================
IN_ShutdownWin32Mouse
================
*/
void IN_ShutdownWin32Mouse()
{
}

/*
================
IN_ActivateWin32Mouse
================
*/
void IN_ActivateWin32Mouse()
{
	int	 width, height;
	RECT window_rect;

	width  = GetSystemMetrics( SM_CXSCREEN );
	height = GetSystemMetrics( SM_CYSCREEN );

	GetWindowRect( g_wv.hWnd, &window_rect );
	if( window_rect.left < 0 ) {
		window_rect.left = 0;
	}
	if( window_rect.top < 0 ) {
		window_rect.top = 0;
	}
	if( window_rect.right >= width ) {
		window_rect.right = width - 1;
	}
	if( window_rect.bottom >= height - 1 ) {
		window_rect.bottom = height - 1;
	}
	window_center_x = ( window_rect.right + window_rect.left ) / 2;
	window_center_y = ( window_rect.bottom + window_rect.top ) / 2;

	SetCursorPos( window_center_x, window_center_y );

	SetCapture( g_wv.hWnd );
	ClipCursor( &window_rect );
	while( ShowCursor( FALSE ) >= 0 )
		;
}

/*
================
IN_DeactivateWin32Mouse
================
*/
void IN_DeactivateWin32Mouse()
{
	ClipCursor( NULL );
	ReleaseCapture();
	while( ShowCursor( TRUE ) < 0 )
		;
}

/*
================
IN_Win32Mouse
================
*/
void IN_Win32Mouse( int* mx, int* my )
{
	POINT current_pos;

	GetCursorPos( &current_pos );

	SetCursorPos( window_center_x, window_center_y );

	*mx = current_pos.x - window_center_x;
	*my = current_pos.y - window_center_y;
}

/*
============================================================

  MOUSE CONTROL

============================================================
*/

/*
===========
IN_ActivateMouse
===========
*/
void IN_ActivateMouse()
{
	if( !s_wmv.mouseInitialized ) {
		return;
	}
	if( !in_mouse->integer ) {
		s_wmv.mouseActive = qfalse;
		return;
	}
	if( s_wmv.mouseActive ) {
		return;
	}

	s_wmv.mouseActive = qtrue;
	IN_ActivateWin32Mouse();
}

/*
===========
IN_DeactivateMouse
===========
*/
void IN_DeactivateMouse()
{
	if( !s_wmv.mouseInitialized ) {
		return;
	}
	if( !s_wmv.mouseActive ) {
		return;
	}

	s_wmv.mouseActive = qfalse;
	IN_DeactivateWin32Mouse();
}

/*
===========
IN_StartupMouse
===========
*/
void IN_StartupMouse()
{
	s_wmv.mouseInitialized = qfalse;

	if( in_mouse->integer == 0 ) {
		Com_Printf( "Mouse control not active.\n" );
		return;
	}

	s_wmv.mouseInitialized = qtrue;
	IN_InitWin32Mouse();
}

/*
===========
IN_MouseEvent
===========
*/
void IN_MouseEvent( int mstate )
{
	int i;

	if( !s_wmv.mouseInitialized ) {
		return;
	}

	for( i = 0; i < 3; i++ ) {
		if( ( mstate & ( 1 << i ) ) && !( s_wmv.oldButtonState & ( 1 << i ) ) ) {
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MOUSE1 + i, qtrue, 0, NULL );
		}

		if( !( mstate & ( 1 << i ) ) && ( s_wmv.oldButtonState & ( 1 << i ) ) ) {
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MOUSE1 + i, qfalse, 0, NULL );
		}
	}

	s_wmv.oldButtonState = mstate;
}

/*
===========
IN_MouseMove
===========
*/
void IN_MouseMove()
{
	int mx, my;

	IN_Win32Mouse( &mx, &my );

	if( !mx && !my ) {
		return;
	}

	Sys_QueEvent( 0, SE_MOUSE, mx, my, 0, NULL );
}

/*
=========================================================================

=========================================================================
*/

/*
===========
IN_Startup
===========
*/
void IN_Startup()
{
	IN_StartupMouse();
	IN_StartupJoystick();
	IN_StartupMIDI();

	in_mouse->modified	  = qfalse;
	in_joystick->modified = qfalse;
}

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown()
{
	IN_DeactivateMouse();
	IN_ShutdownMIDI();
	IN_ShutdownXInput();
	Cmd_RemoveCommand( "midiinfo" );
}

/*
===========
IN_Init
===========
*/
void IN_Init()
{
	in_midi		   = Cvar_Get( "in_midi", "0", CVAR_ARCHIVE );
	in_midiport	   = Cvar_Get( "in_midiport", "1", CVAR_ARCHIVE );
	in_midichannel = Cvar_Get( "in_midichannel", "1", CVAR_ARCHIVE );
	in_mididevice  = Cvar_Get( "in_mididevice", "0", CVAR_ARCHIVE );

	Cmd_AddCommand( "midiinfo", MidiInfo_f );

	in_mouse = Cvar_Get( "in_mouse", "1", CVAR_ARCHIVE | CVAR_LATCH );

	in_joystick		 = Cvar_Get( "in_joystick", "0", CVAR_ARCHIVE | CVAR_LATCH );
	in_joyBallScale	 = Cvar_Get( "in_joyBallScale", "0.12", CVAR_ARCHIVE );
	in_debugJoystick = Cvar_Get( "in_debugjoystick", "0", CVAR_TEMP );
	joy_threshold	 = Cvar_Get( "joy_threshold", "0.15", CVAR_ARCHIVE );

	k_language = Cvar_Get( "k_language", "american", CVAR_ARCHIVE | CVAR_NORESTART );

	IN_Startup();
}

/*
===========
IN_Activate
===========
*/
void IN_Activate( qboolean active )
{
	in_appactive = active;

	if( !active ) {
		IN_DeactivateMouse();
	}
}

/*
==================
IN_Frame
==================
*/
void IN_Frame()
{
	IN_JoyMove();

	if( !s_wmv.mouseInitialized ) {
		return;
	}

	if( cls.keyCatchers & KEYCATCH_CONSOLE ) {
		if( Cvar_VariableValue( "r_fullscreen" ) == 0 && strcmp( Cvar_VariableString( "r_glDriver" ), _3DFX_DRIVER_NAME ) ) {
			IN_DeactivateMouse();
			return;
		}
	}

	if( !in_appactive ) {
		IN_DeactivateMouse();
		return;
	}

	IN_ActivateMouse();

	// post events to the system queue
	IN_MouseMove();
}

/*
===================
IN_ClearStates
===================
*/
void IN_ClearStates()
{
	s_wmv.oldButtonState = 0;

	joy.oldButtons			   = 0;
	joy.oldLeftTriggerPressed  = 0;
	joy.oldRightTriggerPressed = 0;
	joy.oldpovstate			   = 0;
}

/*
=========================================================================

XINPUT CONTROLLER

=========================================================================
*/

static qboolean IN_InitXInput()
{
	if( s_xinputDll ) {
		return qtrue;
	}

	s_xinputDll = LoadLibraryA( "xinput1_4.dll" );
	if( !s_xinputDll ) {
		s_xinputDll = LoadLibraryA( "xinput9_1_0.dll" );
	}
	if( !s_xinputDll ) {
		s_xinputDll = LoadLibraryA( "xinput1_3.dll" );
	}
	if( !s_xinputDll ) {
		return qfalse;
	}

	pXInputGetState = ( PFN_XInputGetState )GetProcAddress( s_xinputDll, "XInputGetState" );
	pXInputSetState = ( PFN_XInputSetState )GetProcAddress( s_xinputDll, "XInputSetState" );

	if( !pXInputGetState ) {
		FreeLibrary( s_xinputDll );
		s_xinputDll		= NULL;
		pXInputGetState = NULL;
		pXInputSetState = NULL;
		return qfalse;
	}

	return qtrue;
}

static void IN_ShutdownXInput()
{
	pXInputGetState = NULL;
	pXInputSetState = NULL;

	if( s_xinputDll ) {
		FreeLibrary( s_xinputDll );
		s_xinputDll = NULL;
	}
}

/*
===============
IN_StartupJoystick
===============
*/
void IN_StartupJoystick()
{
	DWORD result;
	int	  i;

	joy.avail				   = qfalse;
	joy.id					   = -1;
	joy.oldPacketNumber		   = 0;
	joy.oldButtons			   = 0;
	joy.oldLeftTriggerPressed  = 0;
	joy.oldRightTriggerPressed = 0;
	joy.oldpovstate			   = 0;
	memset( &joy.state, 0, sizeof( joy.state ) );

	if( !IN_InitXInput() ) {
		Com_Printf( "XInput not available\n" );
		return;
	}

	for( i = 0; i < XUSER_MAX_COUNT; i++ ) {
		XINPUT_STATE state;
		memset( &state, 0, sizeof( state ) );

		result = pXInputGetState( i, &state );
		if( result == ERROR_SUCCESS ) {
			joy.avail				   = qtrue;
			joy.id					   = i;
			joy.state				   = state;
			joy.oldPacketNumber		   = state.dwPacketNumber;
			joy.oldButtons			   = state.Gamepad.wButtons;
			joy.oldLeftTriggerPressed  = ( state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) ? 1 : 0;
			joy.oldRightTriggerPressed = ( state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) ? 1 : 0;

			Com_Printf( "XInput controller found on port %d\n", i );
			return;
		}
	}

	Com_Printf( "No XInput controller found\n" );
}

/*
===========
JoyToF
===========
*/
static float JoyToF( SHORT value )
{
	float fValue;

	if( value >= 0 ) {
		fValue = ( float )value / 32767.0f;
	} else {
		fValue = ( float )value / 32768.0f;
	}

	if( fValue < -1.0f ) {
		fValue = -1.0f;
	}
	if( fValue > 1.0f ) {
		fValue = 1.0f;
	}
	return fValue;
}

static float JoyToFDeadZone( SHORT value, SHORT deadZone )
{
	float fValue;

	if( value > deadZone ) {
		fValue = ( float )( value - deadZone ) / ( float )( 32767 - deadZone );
	} else if( value < -deadZone ) {
		fValue = ( float )( value + deadZone ) / ( float )( 32768 - deadZone );
	} else {
		fValue = 0.0f;
	}

	if( fValue < -1.0f ) {
		fValue = -1.0f;
	}
	if( fValue > 1.0f ) {
		fValue = 1.0f;
	}
	return fValue;
}

static void IN_EmitKeyTransition( int key, qboolean down )
{
	Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, key, down, 0, NULL );
}

typedef struct {
	WORD mask;
	int	 key;
} xinputButtonMap_t;

static const xinputButtonMap_t xinputButtonMap[] = { { XINPUT_GAMEPAD_A, K_JOY1 },
	{ XINPUT_GAMEPAD_B, K_JOY2 },
	{ XINPUT_GAMEPAD_X, K_JOY3 },
	{ XINPUT_GAMEPAD_Y, K_JOY4 },
	{ XINPUT_GAMEPAD_LEFT_SHOULDER, K_JOY5 },
	{ XINPUT_GAMEPAD_RIGHT_SHOULDER, K_JOY6 },
	{ XINPUT_GAMEPAD_BACK, K_JOY7 },
	{ XINPUT_GAMEPAD_START, K_JOY8 },
	{ XINPUT_GAMEPAD_LEFT_THUMB, K_JOY9 },
	{ XINPUT_GAMEPAD_RIGHT_THUMB, K_JOY10 } };

// 0..3   = left stick digital directions
// 12..15 = dpad digital directions
int							   joyDirectionKeys[16] = { K_JOY13,
							   K_JOY14,
							   K_JOY15,
							   K_JOY16,

							   K_JOY17,
							   K_JOY18,
							   K_JOY19,
							   K_JOY20,
							   K_JOY21,
							   K_JOY22,
							   K_JOY23,
							   K_JOY24,

							   K_JOY24,
							   K_JOY25,
							   K_JOY26,
							   K_JOY27 };

/*
===========
IN_JoyMove
===========
*/
void						   IN_JoyMove()
{
	DWORD		 result;
	XINPUT_STATE state;
	WORD		 buttons;
	int			 i;
	int			 povstate;
	float		 lx, ly, rx, ry;
	int			 mouseX, mouseY;
	qboolean	 newDown, oldDown;
	qboolean	 ltPressed, rtPressed;

	if( !joy.avail ) {
		return;
	}

	if( !pXInputGetState ) {
		return;
	}

	memset( &state, 0, sizeof( state ) );
	result = pXInputGetState( joy.id, &state );
	if( result != ERROR_SUCCESS ) {
		joy.avail = qfalse;
		Com_Printf( "XInput controller disconnected\n" );
		return;
	}

	buttons = state.Gamepad.wButtons;

	if( in_debugJoystick->integer ) {
		Com_Printf( "XInput %d: buttons=%04x LT=%3u RT=%3u LX=%6d LY=%6d RX=%6d RY=%6d\n",
			joy.id,
			( int )buttons,
			( int )state.Gamepad.bLeftTrigger,
			( int )state.Gamepad.bRightTrigger,
			( int )state.Gamepad.sThumbLX,
			( int )state.Gamepad.sThumbLY,
			( int )state.Gamepad.sThumbRX,
			( int )state.Gamepad.sThumbRY );
	}

	// Normal digital buttons
	for( i = 0; i < ( int )( sizeof( xinputButtonMap ) / sizeof( xinputButtonMap[0] ) ); i++ ) {
		newDown = ( buttons & xinputButtonMap[i].mask ) ? qtrue : qfalse;
		oldDown = ( joy.oldButtons & xinputButtonMap[i].mask ) ? qtrue : qfalse;

		if( newDown != oldDown ) {
			IN_EmitKeyTransition( xinputButtonMap[i].key, newDown );
		}
	}

	// Triggers as digital buttons
	ltPressed = ( state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) ? qtrue : qfalse;
	rtPressed = ( state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) ? qtrue : qfalse;

	if( ltPressed != ( joy.oldLeftTriggerPressed ? qtrue : qfalse ) ) {
		IN_EmitKeyTransition( K_JOY11, ltPressed );
	}
	if( rtPressed != ( joy.oldRightTriggerPressed ? qtrue : qfalse ) ) {
		IN_EmitKeyTransition( K_JOY12, rtPressed );
	}

	joy.oldLeftTriggerPressed  = ltPressed ? 1 : 0;
	joy.oldRightTriggerPressed = rtPressed ? 1 : 0;
	joy.oldButtons			   = buttons;

	povstate = 0;

	//
	// LEFT STICK = movement
	//   X = strafe left/right
	//   Y = forward/back
	//
	lx = JoyToFDeadZone( state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE );
	ly = JoyToFDeadZone( state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE );

	if( lx < -joy_threshold->value ) {
		povstate |= ( 1 << 0 ); // JOY13
	} else if( lx > joy_threshold->value ) {
		povstate |= ( 1 << 1 ); // JOY14
	}

	if( ly > joy_threshold->value ) {
		povstate |= ( 1 << 2 ); // JOY15
	} else if( ly < -joy_threshold->value ) {
		povstate |= ( 1 << 3 ); // JOY16
	}

	//
	// DPAD = separate digital buttons
	//
	if( buttons & XINPUT_GAMEPAD_DPAD_LEFT ) {
		povstate |= ( 1 << 12 ); // JOY24
	}
	if( buttons & XINPUT_GAMEPAD_DPAD_RIGHT ) {
		povstate |= ( 1 << 13 ); // JOY25
	}
	if( buttons & XINPUT_GAMEPAD_DPAD_UP ) {
		povstate |= ( 1 << 14 ); // JOY26
	}
	if( buttons & XINPUT_GAMEPAD_DPAD_DOWN ) {
		povstate |= ( 1 << 15 ); // JOY27
	}

	for( i = 0; i < 16; i++ ) {
		if( ( povstate & ( 1 << i ) ) && !( joy.oldpovstate & ( 1 << i ) ) ) {
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, joyDirectionKeys[i], qtrue, 0, NULL );
		}

		if( !( povstate & ( 1 << i ) ) && ( joy.oldpovstate & ( 1 << i ) ) ) {
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, joyDirectionKeys[i], qfalse, 0, NULL );
		}
	}
	joy.oldpovstate = povstate;

	//
	// RIGHT STICK = look
	//   X = yaw
	//   Y = pitch
	//
	rx = JoyToFDeadZone( state.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE );
	ry = JoyToFDeadZone( state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE );

	// faster than before
	mouseX = ( int )( rx * 900.0f * in_joyBallScale->value * 0.5f );
	mouseY = ( int )( -ry * 700.0f * in_joyBallScale->value * 0.5f );

	if( mouseX || mouseY ) {
		Sys_QueEvent( g_wv.sysMsgTime, SE_MOUSE, mouseX, mouseY, 0, NULL );
	}

	joy.state			= state;
	joy.oldPacketNumber = state.dwPacketNumber;
}

/*
=========================================================================

MIDI

=========================================================================
*/

static void MIDI_NoteOff( int note )
{
	int qkey;

	qkey = note - 60 + K_AUX1;

	if( qkey > 255 || qkey < K_AUX1 ) {
		return;
	}

	Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, qkey, qfalse, 0, NULL );
}

static void MIDI_NoteOn( int note, int velocity )
{
	int qkey;

	if( velocity == 0 ) {
		MIDI_NoteOff( note );
		return;
	}

	qkey = note - 60 + K_AUX1;

	if( qkey > 255 || qkey < K_AUX1 ) {
		return;
	}

	Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, qkey, qtrue, 0, NULL );
}

static void CALLBACK MidiInProc( HMIDIIN hMidiIn, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2 )
{
	int message;

	switch( uMsg ) {
		case MIM_OPEN:
			break;
		case MIM_CLOSE:
			break;
		case MIM_DATA:
			message = dwParam1 & 0xff;

			if( ( message & 0xf0 ) == 0x90 ) {
				if( ( ( message & 0x0f ) + 1 ) == in_midichannel->integer ) {
					MIDI_NoteOn( ( dwParam1 & 0xff00 ) >> 8, ( dwParam1 & 0xff0000 ) >> 16 );
				}
			} else if( ( message & 0xf0 ) == 0x80 ) {
				if( ( ( message & 0x0f ) + 1 ) == in_midichannel->integer ) {
					MIDI_NoteOff( ( dwParam1 & 0xff00 ) >> 8 );
				}
			}
			break;
		case MIM_LONGDATA:
			break;
		case MIM_ERROR:
			break;
		case MIM_LONGERROR:
			break;
	}
}

static void MidiInfo_f()
{
	int			i;
	const char* enableStrings[] = { "disabled", "enabled" };

	Com_Printf( "\nMIDI control:       %s\n", enableStrings[in_midi->integer != 0] );
	Com_Printf( "port:               %d\n", in_midiport->integer );
	Com_Printf( "channel:            %d\n", in_midichannel->integer );
	Com_Printf( "current device:     %d\n", in_mididevice->integer );
	Com_Printf( "number of devices:  %d\n", s_midiInfo.numDevices );

	for( i = 0; i < s_midiInfo.numDevices; i++ ) {
		if( i == Cvar_VariableValue( "in_mididevice" ) ) {
			Com_Printf( "***" );
		} else {
			Com_Printf( "..." );
		}
		Com_Printf( "device %2d:       %s\n", i, s_midiInfo.caps[i].szPname );
		Com_Printf( "...manufacturer ID: 0x%hx\n", s_midiInfo.caps[i].wMid );
		Com_Printf( "...product ID:      0x%hx\n", s_midiInfo.caps[i].wPid );
		Com_Printf( "\n" );
	}
}

static void IN_StartupMIDI()
{
	int i;

	if( !Cvar_VariableValue( "in_midi" ) ) {
		return;
	}

	s_midiInfo.numDevices = midiInGetNumDevs();
	if( s_midiInfo.numDevices > MAX_MIDIIN_DEVICES ) {
		s_midiInfo.numDevices = MAX_MIDIIN_DEVICES;
	}

	for( i = 0; i < s_midiInfo.numDevices; i++ ) {
		midiInGetDevCaps( i, &s_midiInfo.caps[i], sizeof( s_midiInfo.caps[i] ) );
	}

	if( in_mididevice->integer < 0 || in_mididevice->integer >= s_midiInfo.numDevices ) {
		Com_Printf( "WARNING: invalid MIDI device index %d\n", in_mididevice->integer );
		return;
	}

	if( midiInOpen( &s_midiInfo.hMidiIn, in_mididevice->integer, ( DWORD_PTR )MidiInProc, ( DWORD_PTR )NULL, CALLBACK_FUNCTION ) != MMSYSERR_NOERROR ) {
		Com_Printf( "WARNING: could not open MIDI device %d\n", in_mididevice->integer );
		return;
	}

	midiInStart( s_midiInfo.hMidiIn );
}

static void IN_ShutdownMIDI()
{
	if( s_midiInfo.hMidiIn ) {
		midiInStop( s_midiInfo.hMidiIn );
		midiInClose( s_midiInfo.hMidiIn );
		s_midiInfo.hMidiIn = NULL;
	}
	memset( &s_midiInfo, 0, sizeof( s_midiInfo ) );
}