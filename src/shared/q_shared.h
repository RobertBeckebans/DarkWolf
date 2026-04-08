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

#ifndef __Q_SHARED_H
#define __Q_SHARED_H

// q_shared.h -- included first by ALL program modules.
// A user mod should never modify this file

#define Q3_VERSION "Wolf 1.41"
// ver 1.0.0	- release
// ver 1.0.1	- post-release work
// ver 1.1.0	- patch 1 (12/12/01)
// ver 1.1b		- TTimo SP linux release (+ MP update)
// ver 1.2.b5	- Mac code merge in
// ver 1.3		- patch 2 (02/13/02)

#define NEW_ANIMS
#define MAX_TEAMNAME 32

#ifdef _WIN32

	#pragma warning( disable : 4018 ) // signed/unsigned mismatch
	#pragma warning( disable : 4032 )
	#pragma warning( disable : 4051 )
	#pragma warning( disable : 4057 ) // slightly different base types
	#pragma warning( disable : 4100 ) // unreferenced formal parameter
	#pragma warning( disable : 4115 )
	#pragma warning( disable : 4125 ) // decimal digit terminates octal escape sequence
	#pragma warning( disable : 4127 ) // conditional expression is constant
	#pragma warning( disable : 4136 )
	#pragma warning( disable : 4152 ) // nonstandard extension, function/data pointer conversion in expression
	#pragma warning( disable : 4201 )
	#pragma warning( disable : 4214 )
	#pragma warning( disable : 4244 )
	// #pragma warning(disable	: 4142)		// benign redefinition
	#pragma warning( disable : 4305 ) // truncation from const double to float
	// #pragma warning(disable : 4310)		// cast truncates constant value
	// #pragma warning(disable :	4505)		// unreferenced local function has been removed
	#pragma warning( disable : 4514 )
	#pragma warning( disable : 4702 ) // unreachable code
	#pragma warning( disable : 4711 ) // selected for automatic inline expansion
	#pragma warning( disable : 4220 ) // varargs matches remaining parameters
#endif

#if defined( ppc ) || defined( __ppc ) || defined( __ppc__ ) || defined( __POWERPC__ )
	#define idppc 1
#endif

/**********************************************************************
  VM Considerations

  The VM can not use the standard system headers because we aren't really
  using the compiler they were meant for.  We use bg_lib.h which contains
  prototypes for the functions we define for our own use in bg_lib.c.

  When writing mods, please add needed headers HERE, do not start including
  stuff like <stdio.h> in the various .c files that make up each of the VMs
  since you will be including system headers files can will have issues.

  Remember, if you use a C library function that is not defined in bg_lib.c,
  you will have to add your own version for support in the VM.

 **********************************************************************/

#ifdef Q3_VM

	#include "bg_lib.h"

#else

	#include <assert.h>
	#include <math.h>
	#include <stdio.h>
	#include <stdarg.h>
	#include <string.h>
	#include <stdlib.h>
	#include <time.h>
	#include <ctype.h>
	#include <limits.h>
	#include <stdint.h>

#endif

#ifdef _WIN32

	// #pragma intrinsic( memset, memcpy )

#endif

// this is the define for determining if we have an asm version of a C function
#if( defined _M_IX86 || defined __i386__ ) && !defined __sun__ && !defined __LCC__
	#define id386 1
#else
	#define id386 0
#endif

// for windows fastcall option

#define QDECL

//======================= WIN32 DEFINES =================================

#ifdef WIN32

	#define MAC_STATIC

	#undef QDECL
	#define QDECL __cdecl

	// buildstring will be incorporated into the version string
	#ifdef NDEBUG
		#define CPUSTRING "win-x64"
	#else
		#define CPUSTRING "win-x64-debug"
	#endif

	#define PATH_SEP '\\'

#endif

//======================= MAC OS X SERVER DEFINES =====================

#if defined( __MACH__ ) && defined( __APPLE__ )

	#define MAC_STATIC

	#ifdef __ppc__
		#define CPUSTRING "MacOSXS-ppc"
	#elif defined __i386__
		#define CPUSTRING "MacOSXS-i386"
	#else
		#define CPUSTRING "MacOSXS-other"
	#endif

	#define PATH_SEP '/'

	#define GAME_HARD_LINKED
	#define CGAME_HARD_LINKED
	#define UI_HARD_LINKED
	#define BOTLIB_HARD_LINKED

#endif

//======================= MAC DEFINES =================================

#ifdef __MACOS__

	#include <MacTypes.h>
	// DAJ #define	MAC_STATIC	static
	#define MAC_STATIC

	#define CPUSTRING "MacOS-PPC"

	#define PATH_SEP  ':'

	#define GAME_HARD_LINKED
	#define CGAME_HARD_LINKED
	#define UI_HARD_LINKED
	#define BOTLIB_HARD_LINKED

void Sys_PumpEvents();

#endif

//======================= LINUX DEFINES =================================

// the mac compiler can't handle >32k of locals, so we
// just waste space and make big arrays static...
#ifdef __linux__

	#define MAC_STATIC

	#ifdef __i386__
		#define CPUSTRING "linux-i386"
	#elif defined __axp__
		#define CPUSTRING "linux-alpha"
	#else
		#define CPUSTRING "linux-other"
	#endif

	#define PATH_SEP '/'

#endif

//=============================================================

typedef unsigned char byte;

typedef int			  qboolean;
#define qfalse 0
#define qtrue  1

typedef int		 qhandle_t;
typedef int		 sfxHandle_t;
typedef int		 fileHandle_t;
typedef int		 clipHandle_t;

typedef intptr_t dllhandle_t;

#ifndef ID_INLINE
	#ifdef _WIN32
		#define ID_INLINE __inline
	#else
		#define ID_INLINE inline
	#endif
#endif

// #define	SND_NORMAL			0x000	// (default) Allow sound to be cut off only by the same sound on this channel
#define SND_OKTOCUT	   0x001 // Allow sound to be cut off by any following sounds on this channel
#define SND_REQUESTCUT 0x002 // Allow sound to be cut off by following sounds on this channel only for sounds who request cutoff
#define SND_CUTOFF	   0x004 // Cut off sounds on this channel that are marked 'SND_REQUESTCUT'
#define SND_CUTOFF_ALL 0x008 // Cut off all sounds on this channel
#define SND_NOCUT	   0x010 // Don't cut off.  Always let finish (overridden by SND_CUTOFF_ALL)

#ifndef NULL
	#define NULL ( ( void* )0 )
#endif

#define MAX_QINT 0x7fffffff
#define MIN_QINT ( -MAX_QINT - 1 )

#ifndef max
	#define max( x, y ) ( ( ( x ) > ( y ) ) ? ( x ) : ( y ) )
	#define min( x, y ) ( ( ( x ) < ( y ) ) ? ( x ) : ( y ) )
#endif

// angle indexes
#define PITCH			  0 // up / down
#define YAW				  1 // left / right
#define ROLL			  2 // fall over

// RF, this is just here so different elements of the engine can be aware of this setting as it changes
#define MAX_SP_CLIENTS	  64 // increasing this will increase memory usage significantly

// the game guarantees that no string from the network will ever
// exceed MAX_STRING_CHARS
#define MAX_STRING_CHARS  1024 // max length of a string passed to Cmd_TokenizeString
#define MAX_STRING_TOKENS 256  // max tokens resulting from Cmd_TokenizeString
#define MAX_TOKEN_CHARS	  1024 // max length of an individual token

#define MAX_INFO_STRING	  1024
#define MAX_INFO_KEY	  1024
#define MAX_INFO_VALUE	  1024

#define BIG_INFO_STRING	  8192 // used for system info key only
#define BIG_INFO_KEY	  8192
#define BIG_INFO_VALUE	  8192

#define MAX_QPATH		  64  // max length of a quake game pathname
#define MAX_OSPATH		  256 // max length of a filesystem pathname

#define MAX_NAME_LENGTH	  32 // max length of a client name

#define MAX_SAY_TEXT	  150

// paramters for command buffer stuffing
typedef enum {
	EXEC_NOW, // don't return until completed, a VM should NEVER use this,
	// because some commands might cause the VM to be unloaded...
	EXEC_INSERT, // insert at current position, but don't run yet
	EXEC_APPEND	 // add to end of the command buffer (normal case)
} cbufExec_t;

//
// these aren't needed by any of the VMs.  put in another header?
//
#define MAX_MAP_AREA_BYTES 32 // bit vector of area visibility

// print levels from renderer (FIXME: set up for game / cgame?)
typedef enum {
	PRINT_ALL,
	PRINT_DEVELOPER, // only print when "developer 1"
	PRINT_WARNING,
	PRINT_ERROR
} printParm_t;

#ifdef ERR_FATAL
	#undef ERR_FATAL // this is be defined in malloc.h
#endif

// parameters to the main Error routine
typedef enum {
	ERR_FATAL,			  // exit the entire game with a popup window
	ERR_DROP,			  // print to console and disconnect from game
	ERR_SERVERDISCONNECT, // don't kill server
	ERR_DISCONNECT,		  // client disconnected from the server
	ERR_NEED_CD,		  // pop up the need-cd dialog
	ERR_ENDGAME			  // not an error.  just clean up properly, exit to the menu, and start up the "endgame" menu  //----(SA)	added
} errorParm_t;

// font rendering values used by ui and cgame

#define PROP_GAP_WIDTH		  3
#define PROP_SPACE_WIDTH	  8
#define PROP_HEIGHT			  27
#define PROP_SMALL_SIZE_SCALE 0.75

#define BLINK_DIVISOR		  200
#define PULSE_DIVISOR		  75

#define UI_LEFT				  0x00000000 // default
#define UI_CENTER			  0x00000001
#define UI_RIGHT			  0x00000002
#define UI_FORMATMASK		  0x00000007
#define UI_SMALLFONT		  0x00000010
#define UI_BIGFONT			  0x00000020 // default
#define UI_GIANTFONT		  0x00000040
#define UI_DROPSHADOW		  0x00000800
#define UI_BLINK			  0x00001000
#define UI_INVERSE			  0x00002000
#define UI_PULSE			  0x00004000
// JOSEPH 10-24-99
#define UI_MENULEFT			  0x00008000
#define UI_MENURIGHT		  0x00010000
#define UI_EXSMALLFONT		  0x00020000
#define UI_MENUFULL			  0x00080000
// END JOSEPH

#define UI_SMALLFONT75		  0x00100000

#if defined( _DEBUG ) && !defined( BSPC )
	#define HUNK_DEBUG
#endif

typedef enum { h_high, h_low, h_dontcare } ha_pref;

#ifdef HUNK_DEBUG
	#define Hunk_Alloc( size, preference ) Hunk_AllocDebug( size, preference, #size, __FILE__, __LINE__ )
void* Hunk_AllocDebug( int size, ha_pref preference, char* label, char* file, int line );
#else

/*!
	\brief Allocates a block of memory from the hunk memory pool with the specified size and allocation preference.

	This function allocates memory from a pre-allocated hunk memory pool, which is organized into low and high segments. The allocation preference determines whether the memory is allocated in the low
   or high segment of the hunk. If the requested memory cannot fit within the available hunk space, an error is generated. The allocated memory is initialized to zero.

	\param size The size in bytes of the memory block to allocate
	\param preference Allocation preference indicating whether to use the low or high segment of the hunk
	\return A pointer to the allocated memory block, or NULL if allocation fails
	\throws Com_Error is called with ERR_FATAL if the hunk memory system is not initialized, or with ERR_DROP if the allocation fails due to insufficient memory
*/
void* Hunk_Alloc( int size, ha_pref preference );
#endif

/*!
	\brief Sets a block of memory to a specific value.

	This function initializes a block of memory by setting each byte in the specified memory region to the given value. It is a wrapper around the standard C library memset function. The memory block
   is identified by the destination pointer, the value to set, and the number of bytes to initialize.

	\param dest pointer to the memory block to be initialized
	\param val the value to set each byte to
	\param count the number of bytes to initialize
*/
void Com_Memset( void* dest, const int val, const size_t count );

/*!
	\brief Copies a specified number of bytes from the source memory block to the destination memory block.

	This function provides a wrapper around the standard C library memcpy function. It copies count bytes from the source memory location to the destination memory location. The function does not
   perform any bounds checking, so it is the responsibility of the caller to ensure that the destination buffer is large enough to hold the copied data. The source and destination memory blocks must
   not overlap, otherwise the behavior is undefined.

	\param dest Pointer to the destination memory block where data will be copied to
	\param src Pointer to the source memory block from which data will be copied
	\param count Number of bytes to copy from source to destination
*/
void Com_Memcpy( void* dest, const void* src, const size_t count );

#define CIN_system	  0x01
#define CIN_loop	  0x02
#define CIN_hold	  0x04
#define CIN_silent	  0x08
#define CIN_shader	  0x10
#define CIN_letterBox 0x20

/*
==============================================================

MATHLIB

==============================================================
*/

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];

typedef int	  fixed4_t;
typedef int	  fixed8_t;
typedef int	  fixed16_t;

#ifndef M_PI
	#define M_PI 3.14159265358979323846f // matches value in gcc v2 math.h
#endif

#define NUMVERTEXNORMALS 162
extern vec3_t bytedirs[NUMVERTEXNORMALS];

// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define SCREEN_WIDTH	 640
#define SCREEN_HEIGHT	 480

#define TINYCHAR_WIDTH	 ( SMALLCHAR_WIDTH )
#define TINYCHAR_HEIGHT	 ( SMALLCHAR_HEIGHT / 2 )

#define SMALLCHAR_WIDTH	 8
#define SMALLCHAR_HEIGHT 16

#define BIGCHAR_WIDTH	 16
#define BIGCHAR_HEIGHT	 16

#define GIANTCHAR_WIDTH	 32
#define GIANTCHAR_HEIGHT 48

extern vec4_t colorBlack;
extern vec4_t colorRed;
extern vec4_t colorGreen;
extern vec4_t colorBlue;
extern vec4_t colorYellow;
extern vec4_t colorMagenta;
extern vec4_t colorCyan;
extern vec4_t colorWhite;
extern vec4_t colorLtGrey;
extern vec4_t colorMdGrey;
extern vec4_t colorDkGrey;

#define Q_COLOR_ESCAPE		 '^'
#define Q_IsColorString( p ) ( p && *( p ) == Q_COLOR_ESCAPE && *( ( p ) + 1 ) && *( ( p ) + 1 ) != Q_COLOR_ESCAPE )

#define COLOR_BLACK			 '0'
#define COLOR_RED			 '1'
#define COLOR_GREEN			 '2'
#define COLOR_YELLOW		 '3'
#define COLOR_BLUE			 '4'
#define COLOR_CYAN			 '5'
#define COLOR_MAGENTA		 '6'
#define COLOR_WHITE			 '7'
#define ColorIndex( c )		 ( ( ( c ) - '0' ) & 7 )

#define S_COLOR_BLACK		 "^0"
#define S_COLOR_RED			 "^1"
#define S_COLOR_GREEN		 "^2"
#define S_COLOR_YELLOW		 "^3"
#define S_COLOR_BLUE		 "^4"
#define S_COLOR_CYAN		 "^5"
#define S_COLOR_MAGENTA		 "^6"
#define S_COLOR_WHITE		 "^7"

extern vec4_t g_color_table[8];

#define MAKERGB( v, r, g, b ) \
	v[0] = r;                 \
	v[1] = g;                 \
	v[2] = b
#define MAKERGBA( v, r, g, b, a ) \
	v[0] = r;                     \
	v[1] = g;                     \
	v[2] = b;                     \
	v[3] = a

#define DEG2RAD( a ) ( ( ( a ) * M_PI ) / 180.0F )
#define RAD2DEG( a ) ( ( ( a ) * 180.0f ) / M_PI )

struct cplane_s;

extern vec3_t vec3_origin;
extern vec3_t axisDefault[3];

#define nanmask		( 255 << 23 )
#define IS_NAN( x ) ( ( ( *( int* )&x ) & nanmask ) == nanmask )

// TTimo
// handy stuff when tracking isnan problems
#ifndef NDEBUG
	#define CHECK_NAN( x )	   assert( !IS_NAN( x ) )
	#define CHECK_NAN_VEC( v ) assert( !IS_NAN( v[0] ) && !IS_NAN( v[1] ) && !IS_NAN( v[2] ) )
#else
	#define CHECK_NAN
	#define CHECK_NAN_VEC
#endif

float Q_fabs( float f );

/*!
	\brief Computes an approximation of the reciprocal square root of a floating-point number.

	This function implements the famous fast inverse square root algorithm, originally used in the Quake III Arena game engine. It provides a highly optimized approximation of 1/sqrt(f) using bit
   manipulation and Newton-Raphson iteration. The algorithm uses a magic constant 0x5f3759df combined with bit-level hacking to produce an initial guess, followed by one iteration of Newton-Raphson
   refinement for improved accuracy. This technique was widely adopted in graphics programming for performance-critical operations involving vector normalizations and lighting calculations.

	\param f The input floating-point number to compute the reciprocal square root for
	\return Approximation of 1/sqrt(f) as a floating-point value
*/
float Q_rsqrt( float f );

#define SQRTFAST( x ) ( 1.0f / Q_rsqrt( x ) )

/*!
	\brief Clamps an integer value to the range of a signed char.

	This function takes an integer input and ensures it falls within the valid range for a signed char, which is from -128 to 127. If the input integer is less than -128, the function returns -128. If
   the input integer is greater than 127, the function returns 127. Otherwise, it returns the input integer cast to a signed char. This function is commonly used to safely convert floating-point
   values or other integer types to the range expected by the game's input system.

	\param i The integer value to be clamped to the signed char range
	\return A signed char value that is clamped to the range [-128, 127]
*/
signed char	 ClampChar( int i );

/*!
	\brief Clamps an integer value to the range of a signed short.

	This function takes an integer input and ensures it fits within the valid range for a signed short int, which is from -32768 to 32767. If the input is less than -32768, it returns -32768. If the
   input is greater than 32767, it returns 32767. Otherwise, it returns the input value unchanged.

	\param i The integer value to be clamped to signed short range
	\return The input integer clamped to the range of a signed short integer
*/
signed short ClampShort( int i );

/*!
	\brief Converts a 3D direction vector into a byte representation by finding the closest matching normal vector

	This function takes a 3D direction vector and maps it to one of the predefined byte directions by computing the dot product with each of the NUMVERTEXNORMALS predefined direction vectors. It
   returns the index of the direction vector that has the highest dot product with the input direction, effectively finding the closest matching normal. The function handles null input by returning 0.

	\param dir The 3D direction vector to be converted to a byte representation
	\return The index of the closest matching normal vector from the predefined set of direction vectors
*/
int			 DirToByte( vec3_t dir );

/*!
	\brief Converts a byte value to a direction vector using a lookup table.

	This function maps a byte value to a corresponding direction vector by indexing into a precomputed lookup table called bytedirs. If the input byte is out of valid range, it sets the output
   direction vector to the origin.

	\param b The byte value to convert, expected to be between 0 and NUMVERTEXNORMALS-1
	\param dir Output parameter that will contain the resulting direction vector
*/
void		 ByteToDir( int b, vec3_t dir );

#if 1

	#define DotProduct( x, y )		  ( ( x )[0] * ( y )[0] + ( x )[1] * ( y )[1] + ( x )[2] * ( y )[2] )
	#define VectorSubtract( a, b, c ) ( ( c )[0] = ( a )[0] - ( b )[0], ( c )[1] = ( a )[1] - ( b )[1], ( c )[2] = ( a )[2] - ( b )[2] )
	#define VectorAdd( a, b, c )	  ( ( c )[0] = ( a )[0] + ( b )[0], ( c )[1] = ( a )[1] + ( b )[1], ( c )[2] = ( a )[2] + ( b )[2] )
	#define VectorCopy( a, b )		  ( ( b )[0] = ( a )[0], ( b )[1] = ( a )[1], ( b )[2] = ( a )[2] )
	#define VectorCopy4( a, b )		  ( ( b )[0] = ( a )[0], ( b )[1] = ( a )[1], ( b )[2] = ( a )[2], ( b )[3] = ( a )[3] )
	#define VectorScale( v, s, o )	  ( ( o )[0] = ( v )[0] * ( s ), ( o )[1] = ( v )[1] * ( s ), ( o )[2] = ( v )[2] * ( s ) )
	#define VectorMA( v, s, b, o )	  ( ( o )[0] = ( v )[0] + ( b )[0] * ( s ), ( o )[1] = ( v )[1] + ( b )[1] * ( s ), ( o )[2] = ( v )[2] + ( b )[2] * ( s ) )

#else

	#define DotProduct( x, y )		  _DotProduct( x, y )
	#define VectorSubtract( a, b, c ) _VectorSubtract( a, b, c )
	#define VectorAdd( a, b, c )	  _VectorAdd( a, b, c )
	#define VectorCopy( a, b )		  _VectorCopy( a, b )
	#define VectorScale( v, s, o )	  _VectorScale( v, s, o )
	#define VectorMA( v, s, b, o )	  _VectorMA( v, s, b, o )

#endif

#ifdef __LCC__
	#ifdef VectorCopy
		#undef VectorCopy
// this is a little hack to get more efficient copies in our interpreter
typedef struct {
	float v[3];
} vec3struct_t;
		#define VectorCopy( a, b ) *( vec3struct_t* )b = *( vec3struct_t* )a;
	#endif
#endif

#define VectorClear( a )			( ( a )[0] = ( a )[1] = ( a )[2] = 0 )
#define VectorNegate( a, b )		( ( b )[0] = -( a )[0], ( b )[1] = -( a )[1], ( b )[2] = -( a )[2] )
#define VectorSet( v, x, y, z )		( ( v )[0] = ( x ), ( v )[1] = ( y ), ( v )[2] = ( z ) )

#define Vector4Set( v, x, y, z, n ) ( ( v )[0] = ( x ), ( v )[1] = ( y ), ( v )[2] = ( z ), ( v )[3] = ( n ) )
#define Vector4Copy( a, b )			( ( b )[0] = ( a )[0], ( b )[1] = ( a )[1], ( b )[2] = ( a )[2], ( b )[3] = ( a )[3] )
#define Vector4MA( v, s, b, o )		( ( o )[0] = ( v )[0] + ( b )[0] * ( s ), ( o )[1] = ( v )[1] + ( b )[1] * ( s ), ( o )[2] = ( v )[2] + ( b )[2] * ( s ), ( o )[3] = ( v )[3] + ( b )[3] * ( s ) )
#define Vector4Average( v, b, s, o )                                    \
	( ( o )[0]	 = ( ( v )[0] * ( 1 - ( s ) ) ) + ( ( b )[0] * ( s ) ), \
		( o )[1] = ( ( v )[1] * ( 1 - ( s ) ) ) + ( ( b )[1] * ( s ) ), \
		( o )[2] = ( ( v )[2] * ( 1 - ( s ) ) ) + ( ( b )[2] * ( s ) ), \
		( o )[3] = ( ( v )[3] * ( 1 - ( s ) ) ) + ( ( b )[3] * ( s ) ) )

#define SnapVector( v )             \
	{                               \
		v[0] = ( ( int )( v[0] ) ); \
		v[1] = ( ( int )( v[1] ) ); \
		v[2] = ( ( int )( v[2] ) ); \
	}

/*!
	\brief Computes the dot product of two 3D vectors

	This function calculates the dot product of two 3D vectors by multiplying corresponding components and summing the results. The dot product is a scalar value that represents the product of the
   magnitudes of the two vectors and the cosine of the angle between them. This implementation provides a straightforward computation without any additional optimizations or checks.

	\param v1 First 3D vector represented as an array of three components
	\param v2 Second 3D vector represented as an array of three components
	\return The scalar dot product of the two input vectors
*/
vec_t	 _DotProduct( const vec3_t v1, const vec3_t v2 );
void	 _VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out );

/*!
	\brief Performs vector addition by adding corresponding components of two input vectors and storing the result in an output vector.

	This function takes two input vectors, veca and vecb, and computes their sum component-wise. Each component of the resulting vector is calculated by adding the corresponding components of the
   input vectors. The result is stored in the output vector out. The function assumes that all input and output vectors are valid and properly allocated. This is a basic vector arithmetic operation
   commonly used in 3D graphics and game development for combining vector quantities such as positions, velocities, or forces.

	\param veca First input vector containing x, y, z components
	\param vecb Second input vector containing x, y, z components
	\param out Output vector that will contain the sum of veca and vecb
*/
void	 _VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out );

/*!
	\brief Copies the components of a 3D input vector to an output vector.

	This function performs a component-wise copy of a 3D vector. It takes an input vector and assigns each of its three components (x, y, z) to the corresponding positions in the output vector. The
   input vector remains unchanged during this operation.

	\param in The input 3D vector whose components are to be copied
	\param out The output 3D vector that receives the copied components
*/
void	 _VectorCopy( const vec3_t in, vec3_t out );

/*!
	\brief Scales each component of the input vector by the provided scale factor and stores the result in the output vector.

	This function performs component-wise multiplication of a 3D vector by a scalar value. It takes an input vector, multiplies each of its three components (x, y, z) by the scale factor, and stores
   the resulting vector in the output parameter. The input vector is not modified during this operation.

	\param in The input 3D vector to be scaled
	\param scale The scalar factor by which each component of the input vector is multiplied
	\param out The output 3D vector that receives the scaled result
*/
void	 _VectorScale( const vec3_t in, float scale, vec3_t out );

/*!
	\brief Performs vector addition scaled by a factor and stores the result in a third vector.

	This function computes the mathematical operation veca + scale * vecb and stores the result in vecc. It is commonly used in 3D graphics and game engine calculations for vector arithmetic. The
   function modifies the destination vector vecc in place by adding the scaled components of vecb to the corresponding components of veca.

	\param veca The first vector to be added to
	\param scale The scalar value to scale vecb by
	\param vecb The second vector to be scaled and added
	\param vecc The destination vector to store the result
*/
void	 _VectorMA( const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc );

/*!
	\brief Converts RGB color values to a packed unsigned integer representation.

	This function takes three floating-point values representing red, green, and blue color components, each in the range [0.0, 1.0], and converts them to a packed unsigned integer where each color
   component is stored as a byte. The red component is stored in the least significant byte, followed by green and then blue.

	\param r The red color component as a float in the range [0.0, 1.0]
	\param g The green color component as a float in the range [0.0, 1.0]
	\param b The blue color component as a float in the range [0.0, 1.0]
	\return A packed unsigned integer containing the RGB color values, with each component stored as a byte in the order RGBA
*/
unsigned ColorBytes3( float r, float g, float b );

/*!
	\brief Combines four floating-point color components into a single unsigned integer value.

	This function takes four color components represented as floating-point values between 0 and 1 and packs them into a single unsigned integer. Each component is scaled by 255 to convert from the
   normalized range to the standard 0-255 byte range. The components are stored in the resulting integer in the order: red, green, blue, and alpha. This is commonly used for color representation in
   graphics programming where a single 32-bit integer can store all four color channels.

	\param r red color component as a floating-point value between 0 and 1
	\param g green color component as a floating-point value between 0 and 1
	\param b blue color component as a floating-point value between 0 and 1
	\param a alpha color component as a floating-point value between 0 and 1
	\return A single unsigned integer value containing the packed color information in RGBA format.
*/
unsigned ColorBytes4( float r, float g, float b, float a );

float	 NormalizeColor( const vec3_t in, vec3_t out );

/*!
	\brief Computes the radius of the bounding sphere from the minimum and maximum bounds of an object.

	This function calculates the radius of the bounding sphere that encloses a given axis-aligned bounding box defined by its minimum and maximum coordinates. It does so by finding the maximum
   absolute coordinate value along each axis and then computing the Euclidean length of the resulting vector.

	\param mins The minimum coordinates of the bounding box
	\param maxs The maximum coordinates of the bounding box
	\return The radius of the bounding sphere that encloses the given bounds
*/
float	 RadiusFromBounds( const vec3_t mins, const vec3_t maxs );
void	 ClearBounds( vec3_t mins, vec3_t maxs );
void	 AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs );

/*!
	\brief Compares two 3D vectors and returns true if they are equal

	This function performs an element-wise comparison of two 3D vectors represented as arrays of three floating-point values. It checks if all corresponding components of the vectors are equal. The
   function is commonly used in the game engine to determine if vector values have changed, particularly when updating entity states in the AAS (Area Awareness System) system.

	\param v1 First 3D vector to compare
	\param v2 Second 3D vector to compare
	\return Integer value of 1 if the vectors are equal, or 0 if any component differs
*/
int		 VectorCompare( const vec3_t v1, const vec3_t v2 );

/*!
	\brief Computes the Euclidean length of a 3D vector.

	This function calculates the magnitude of a 3D vector using the standard Euclidean distance formula. It takes the square root of the sum of the squares of the vector's components. The input vector
   is expected to be a constant reference to avoid modification and ensure that the function operates on a copy of the input data.

	\param v A constant reference to a 3D vector represented as an array of three components.
	\return The Euclidean length of the input vector as a vec_t type.
*/
vec_t	 VectorLength( const vec3_t v );

/*!
	\brief Computes the squared length of a 3D vector

	This function calculates the sum of squares of the vector's components without computing the square root. It is useful for performance-critical operations where the actual length is not needed,
   such as comparisons or distance calculations. The squared length is mathematically equivalent to the dot product of the vector with itself.

	\param v The input 3D vector represented as an array of three components
	\return The squared length of the input vector
*/
vec_t	 VectorLengthSquared( const vec3_t v );

/*!
	\brief Calculates the Euclidean distance between two 3D points.

	This function computes the distance between two points in three-dimensional space. It first calculates the vector difference between the two points and then returns the length of that vector. The
   implementation uses VectorSubtract to compute the difference and VectorLength to calculate the magnitude of the resulting vector.

	\param p1 The first 3D point coordinates
	\param p2 The second 3D point coordinates
	\return The Euclidean distance between the two input points
*/
vec_t	 Distance( const vec3_t p1, const vec3_t p2 );

/*!
	\brief Computes the squared distance between two 3D points.

	This function calculates the squared Euclidean distance between two points in 3D space. It is useful for performance-critical applications where the actual distance square root is not needed, as
   it avoids the computationally expensive sqrt operation. The calculation is performed by subtracting the coordinates of the first point from the second point, then summing the squares of the
   resulting vector components.

	\param p1 The first 3D point represented as a vector of three components.
	\param p2 The second 3D point represented as a vector of three components.
	\return The squared distance between the two input points.
*/
vec_t	 DistanceSquared( const vec3_t p1, const vec3_t p2 );
void	 CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross );

/*!
	\brief Normalizes a 3D vector and returns its length

	This function takes a 3D vector as input, calculates its length, and then normalizes the vector by dividing each component by the length. If the length is zero, the vector remains unchanged. The
   function returns the original length of the vector before normalization.

	\param v The 3D vector to be normalized, modified in place
	\return The length of the input vector before normalization
*/
vec_t	 VectorNormalize( vec3_t v );
void	 VectorNormalizeFast( vec3_t v ); // does NOT return vector length, uses rsqrt approximation
vec_t	 VectorNormalize2( const vec3_t v, vec3_t out );
void	 VectorInverse( vec3_t v );

/*!
	\brief Scales each component of a 4D vector by the given factor.

	This function takes an input 4D vector and multiplies each of its components by the specified scale factor, storing the result in an output vector. It performs component-wise multiplication, which
   is commonly used in graphics and mathematical computations involving vectors.

	\param in The input 4D vector to be scaled
	\param scale The factor by which each component of the input vector will be multiplied
	\param out The output vector that will contain the scaled components
*/
void	 Vector4Scale( const vec4_t in, vec_t scale, vec4_t out );

/*!
	\brief Performs vector rotation using a 3x3 rotation matrix

	This function applies a linear transformation to the input vector by multiplying it with a 3x3 rotation matrix. The transformation is computed as a series of dot products between the input vector
   and the rows of the rotation matrix. The result is stored in the output vector. This is commonly used in 3D graphics and game engine applications for rotating vectors according to a given
   transformation matrix.

	\param in The input vector to be rotated
	\param matrix A 3x3 rotation matrix represented as an array of three 3D vectors
	\param out The output vector containing the rotated result
*/
void	 VectorRotate( vec3_t in, vec3_t matrix[3], vec3_t out );

/*!
	\brief Computes the base-2 logarithm of the given integer value.

	This function calculates the base-2 logarithm of the input integer by repeatedly right-shifting the value until it becomes zero. Each shift operation represents a power of 2, and the count of
   shifts gives the base-2 logarithm. The function effectively determines the position of the most significant bit in the binary representation of the input value.

	\param val The input integer value for which to compute the base-2 logarithm
	\return The base-2 logarithm of the input value, which is the position of the most significant bit
*/
int		 Q_log2( int val );

/*!
	\brief Computes the arc cosine of the input value with clamping to the range [0, M_PI]

	This function calculates the arc cosine of the input parameter c using the standard math library function acos. The result is then clamped to the range [0, M_PI] to ensure valid output values.
   This is necessary because the acos function can return values outside this range in certain edge cases, and the clamping ensures consistent behavior for all inputs.

	\param c The input value for which to compute the arc cosine
	\return The arc cosine of c clamped to the range [0, M_PI]
*/
float	 Q_acos( float c );

int		 Q_rand( int* seed );

/*!
	\brief Generates a pseudo-random floating-point number between 0 and 1 using a linear congruential generator.

	This function implements a simple linear congruential random number generator to produce a uniformly distributed floating-point value in the range [0, 1). It uses the provided seed to maintain
   state between calls, making it suitable for generating reproducible sequences of random numbers. The function internally calls Q_rand to obtain a 32-bit integer, masks it to 16 bits, and scales it
   to the desired floating-point range. This approach is commonly used in games and simulations where a simple, fast, and deterministic random number generator is needed.

	\param seed Pointer to an integer seed value that gets updated with each call to produce the next random number in the sequence
	\return A floating-point number in the range [0, 1) representing the next pseudo-random value in the sequence
*/
float	 Q_random( int* seed );

/*!
	\brief Generates a random floating-point value in the range [-1.0, 1.0] using a given seed for pseudo-random number generation.

	This function takes an integer pointer as a seed and uses it to generate a pseudo-random float value. It relies on another function Q_random which is expected to return a value in the range
   [0.0, 1.0]. The returned value will be scaled and shifted to the range [-1.0, 1.0] to provide a centered distribution suitable for generating random offsets or spread patterns in game calculations.

	\param seed Pointer to an integer seed used for pseudo-random number generation
	\return A floating-point value in the range [-1.0, 1.0] generated from the provided seed
*/
float	 Q_crandom( int* seed );

#define random()  ( ( rand() & 0x7fff ) / ( ( float )0x7fff ) )
#define crandom() ( 2.0 * ( random() - 0.5 ) )

/*!
	\brief Converts a 3D vector into yaw and pitch angles.

	This function takes a 3D vector and computes the corresponding yaw and pitch angles. The yaw represents the horizontal rotation, while the pitch represents the vertical rotation. The roll is
   always set to zero. Special cases are handled when the vector lies along the Z-axis to prevent division by zero.

	\param value1 The input 3D vector for which angles are to be calculated
	\param angles The output array where the calculated yaw, pitch, and roll angles will be stored
*/
void  vectoangles( const vec3_t value1, vec3_t angles );

/*!
	\brief Computes the yaw angle from a 3D vector

	This function calculates the yaw angle in degrees from a given 3D vector. The yaw represents the horizontal rotation around the Y-axis. The function handles special cases where the vector
   components are zero or when the pitch is non-zero. It ensures the returned angle is within the range of 0 to 360 degrees.

	\param vec The input 3D vector from which to calculate the yaw angle
	\return The yaw angle in degrees calculated from the input vector
*/
float vectoyaw( const vec3_t vec );

/*!
	\brief Converts Euler angles into a rotation matrix represented by three axis vectors

	This function takes a set of Euler angles and computes the corresponding rotation matrix. The matrix is stored as three axis vectors in the axis array. The first axis represents the forward
   direction, the second axis represents the right direction (with inverted sign), and the third axis represents the up direction. The implementation uses the AngleVectors function to compute the
   forward and right vectors, then computes the up vector directly from the angles.

	\param angles The Euler angles in degrees for x (pitch), y (yaw), and z (roll) rotations
	\param axis Output array of three vectors representing the rotation matrix axes
*/
void  AnglesToAxis( const vec3_t angles, vec3_t axis[3] );

/*!
	\brief Converts a 3x3 rotation matrix represented by axis vectors into Euler angles.

	This function takes a 3x3 rotation matrix stored as an array of three 3D vectors representing the forward, right, and up axes, and converts it into a set of Euler angles (pitch, yaw, roll). The
   computation first extracts the yaw and pitch from the forward vector using vectoangles, then computes the roll angle from the right vector by performing rotations and angle calculations. Special
   handling is included to adjust the pitch when the yaw difference exceeds 90 degrees to maintain consistent angle representation.

	\param axis Array of three 3D vectors representing the forward, right, and up axes of a rotation matrix
	\param angles Output array to store the resulting Euler angles (pitch, yaw, roll)
*/
void	 AxisToAngles(

	/*!
		\brief Calculates the Euclidean distance between two 3D vectors.

		This function computes the distance between two points in 3D space represented as vec3_t structures. It first calculates the direction vector between the two points by subtracting the first vector from the second, then returns the length of this direction vector. The implementation leverages the existing VectorSubtract and VectorLength helper functions to perform the calculation.

		\param v1 First 3D vector point
		\param v2 Second 3D vector point
		\return The Euclidean distance between the two input vectors as a floating-point value
	*/
float	 VectorDistance( vec3_t v1, vec3_t v2 );

	/*!
		\brief Initializes a 3x3 axis matrix to the identity matrix.

		This function sets the provided 3x3 axis matrix to the identity matrix configuration. Each row of the matrix is initialized such that the diagonal elements are set to 1 and all off-diagonal elements are set to 0. This operation is commonly used to reset or initialize rotation matrices in 3D graphics and game development.

		\param axis A pointer to a 3x3 matrix represented as an array of three 3D vectors.
	*/
void	 AxisClear( vec3_t axis[3] );

	/*!
		\brief Copies three 3D vectors from the input array to the output array.

		This function performs element-wise copying of three 3D vectors from the input array to the output array. Each vector in the input array is copied to the corresponding position in the output array using the VectorCopy function.

		\param in Source array of three 3D vectors
		\param out Destination array of three 3D vectors
	*/
void	 AxisCopy( vec3_t in[3], vec3_t out[3] );

/*!
	\brief Sets the sign bits for a plane normal vector in the given cplane_s structure.

	The function calculates the sign bits for each component of the plane normal vector and stores the result in the signbits field of the cplane_s structure. Each bit in the signbits field
   corresponds to a component of the normal vector, with bit j set if the jth component of the normal vector is negative. This is used for fast box on planeside tests.

	\param out pointer to the cplane_s structure whose signbits field will be set
*/
void	 SetPlaneSignbits( struct cplane_s* out );

/*!
	\brief Determines which side of a plane a bounding box resides on.

	This function tests whether a bounding box defined by its minimum and maximum coordinates is on one side of a plane or intersects it. It returns a bitmask indicating the relation: 1 for the front
   side, 2 for the back side, and 3 for intersection. For axial planes (aligned with X, Y, or Z axes), it uses a fast path. For general planes, it computes distances using the plane's normal and
   signbits to determine the corner points that are closest and farthest from the plane.

	\param emins The minimum coordinates of the bounding box
	\param emaxs The maximum coordinates of the bounding box
	\param plane Pointer to the plane structure containing the normal, distance, and signbits
	\return An integer bitmask indicating the side of the plane the box is on: 1 for front, 2 for back, 3 for both sides
*/
int		 BoxOnPlaneSide( vec3_t emins, vec3_t emaxs, struct cplane_s* plane );

	/*!
		\brief Normalizes an angle to the range [0, 360) degrees

		This function takes an angle in degrees and normalizes it to the range [0, 360) degrees. It uses a bit-wise AND operation with 65535 to perform the normalization, which is a common technique for handling angle wrapping in fixed-point arithmetic. The function is frequently used in game code for managing angular movement and interpolation, particularly when dealing with swing calculations and angle clamping as shown in the call examples.

		\param a input angle in degrees to be normalized
		\return the normalized angle in the range [0, 360) degrees
	*/
float	 AngleMod( float a );

	/*!
		\brief Interpolates between two angles, handling angle wrapping across the 180-degree boundary.

		This function performs linear interpolation between two angles, taking into account the circular nature of angles. It adjusts the target angle to ensure the shortest path is taken when interpolating, avoiding unnecessary large angle jumps. This is particularly useful for interpolating angular data such as player orientations or entity rotations in game animations.

		\param from The starting angle in degrees
		\param to The target angle in degrees
		\param frac The interpolation factor between 0 and 1
		\return The interpolated angle in degrees, normalized to the range that maintains the shortest angular path between from and to.
	*/
float	 LerpAngle( float from, float to, float frac );

	/*!
		\brief Computes the difference between two angles, normalized to the range [-180, 180).

		This function calculates the angular difference between two angles a1 and a2. It ensures the result is normalized to the range [-180, 180) by adjusting for angle wraparound. This is useful in game development for calculating angular differences without ambiguity caused by angle periodicity.

		\param a1 The first angle in degrees
		\param a2 The second angle in degrees
		\return The normalized angular difference between a1 and a2, in the range [-180, 180)
	*/
float	 AngleSubtract( float a1, float a2 );

	/*!
		\brief Subtracts corresponding angle components of two 3D vectors and stores the result in a third vector.

		This function performs element-wise subtraction of angle components between two 3D vectors v1 and v2, storing the resulting angles in v3. Each component of the result vector is computed using the AngleSubtract function, which properly handles angle wrapping to maintain valid angle ranges. The function operates on three-dimensional vectors representing Euler angles, commonly used in 3D graphics and game development for orientation representation.

		\param v1 First 3D vector containing angle components to be subtracted from
		\param v2 Second 3D vector containing angle components to subtract
		\param v3 Output 3D vector to store the result of the component-wise subtraction
	*/
void	 AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 );

	/*!
		\brief Normalizes an angle to the range [0, 360) degrees.

		This function takes an angle in degrees and normalizes it to the range [0, 360). It uses a bit manipulation technique to wrap the angle value, ensuring that it falls within the expected range. The function is commonly used in game development for managing angular rotations and avoiding angle overflow issues.

		\param angle The input angle in degrees to be normalized.
		\return The normalized angle in the range [0, 360) degrees.
	*/
float	 AngleNormalize360( float angle );

	/*!
		\brief Normalizes an angle to the range [-180, 180).

		This function takes an angle in degrees and normalizes it to the range [-180, 180). It first normalizes the angle to the range [0, 360) using AngleNormalize360, then adjusts it to the [-180, 180) range. This is commonly used in animation blending and rotation calculations where angles need to be kept within a standard range to ensure correct interpolation and avoid gimbal lock issues.

		\param angle The input angle in degrees to be normalized
		\return The normalized angle in the range [-180, 180)
	*/
float	 AngleNormalize180( float angle );

	/*!
		\brief Calculates the normalized angular difference between two angles.

		This function computes the difference between two angles and normalizes the result to the range [-180, 180). This is useful for determining the smallest angular distance between two orientations, which is commonly needed in game AI for turning calculations and angle comparisons.

		\param angle1 The first angle in degrees
		\param angle2 The second angle in degrees
		\return The normalized angular difference between angle1 and angle2, constrained to the range [-180, 180).
	*/
float	 AngleDelta( float angle1, float angle2 );

qboolean PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c );

	/*!
		\brief Projects a point onto a plane defined by a normal vector

		This function calculates the orthogonal projection of a given point onto a plane. The plane is defined by its normal vector. The calculation involves computing the distance from the point to the plane along the normal direction, then subtracting the appropriate multiple of the normal vector from the original point to obtain the projected point. The function handles the normalization of the normal vector internally to ensure accurate projection.

		\param dst output parameter that will contain the projected point on the plane
		\param p the input point to be projected
		\param normal the normal vector of the plane onto which the point is projected
	*/
void	 ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal );
void	 RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees );

	/*!
		\brief Rotates a direction vector around an axis to compute orthogonal vectors

		This function takes a forward vector and computes two additional orthogonal vectors to form a right-handed coordinate system. It first creates a perpendicular vector to the input forward direction, then rotates this vector around the forward axis by the specified yaw angle. Finally, it computes the third orthogonal vector using the cross product of the forward and rotated vectors.

		\param axis Array of three 3D vectors representing the forward, right, and up directions
		\param yaw The angle in degrees to rotate the right vector around the forward axis
	*/
void	 RotateAroundDirection( vec3_t axis[3], float yaw );

	/*!
		\brief Computes a set of orthogonal vectors (right, up) from a forward vector.

		This function takes a forward vector and generates two additional vectors that are orthogonal to each other and to the forward vector. It first creates a right vector that is not colinear with the forward vector, then normalizes it and uses it to compute an up vector through a cross product with the forward vector. The resulting vectors form a right-handed coordinate system.

		\param forward A 3D vector representing the forward direction
		\param right Output vector representing the right direction, computed orthogonally to forward
		\param up Output vector representing the up direction, computed orthogonally to forward and right
	*/
void	 MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up );

/*!
	\brief Determines the plane type based on the orientation of a normal vector.

	This function analyzes the components of a 3D normal vector to classify the plane it represents. It returns specific plane type constants based on whether the vector is aligned with the primary
   axes or falls into a general category. The function is used in collision detection and rendering systems to optimize plane-related calculations.

	\param normal A 3D vector representing the normal to a plane
	\return An integer representing the plane type, which can be PLANE_X, PLANE_Y, PLANE_Z, PLANE_ANYX, PLANE_ANYY, or PLANE_ANYZ depending on the orientation of the input normal vector.
*/
int		 PlaneTypeForNormal( vec3_t normal );

void	 MatrixMultiply( float in1[3][3], float in2[3][3], float out[3][3] );
void	 AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up );

	/*!
		\brief Computes a vector perpendicular to the input vector by projecting an axially aligned vector onto the plane defined by the input vector and normalizing the result.

		The function first identifies the component of the input vector with the smallest magnitude. It then creates a unit vector along that axis and projects it onto the plane defined by the input vector. The resulting vector is normalized to produce a unit vector perpendicular to the input.

		\param dst Output vector that will contain the perpendicular vector
		\param src Input vector to compute the perpendicular for
	*/
void	 PerpendicularVector( vec3_t dst, const vec3_t src );

	/*!
		\brief Computes a normalized vector perpendicular to the plane defined by three points.

		The function calculates two vectors from the given point to the other two points, normalizes them, and then computes their cross product to obtain a perpendicular vector. The resulting vector is normalized and returned in the up parameter.

		\param point A point used to calculate vectors to p1 and p2
		\param p1 First point defining the plane
		\param p2 Second point defining the plane
		\param up Output parameter for the resulting perpendicular vector
	*/
void	 GetPerpendicularViewVector( const vec3_t point, const vec3_t p1, const vec3_t p2, vec3_t up );

	/*!
		\brief Projects a point onto a vector defined by two endpoints

		This function calculates the orthogonal projection of a given point onto a vector segment defined by two endpoints. It first computes the vectors from the start point to the target point and from the start point to the end point. The direction vector is normalized, then the dot product of the vector from start to target point and the normalized direction vector is computed. This scalar projection is used to find the projected point along the vector segment by adding the scaled direction vector to the start point.

		\param point The point to be projected
		\param vStart The starting point of the vector segment
		\param vEnd The ending point of the vector segment
		\param vProj The resulting projected point
	*/
void	 ProjectPointOntoVector( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj );
// done.

//=============================================

float	 Com_Clamp( float min, float max, float value );

	/*!
		\brief Returns a pointer to the last part of a path, after the final slash.

		The function traverses the input path string and updates a pointer to point to the last segment of the path, which starts after the final forward slash character. If there is no forward slash in the path, the pointer will remain at the beginning of the string. This is commonly used to extract the filename component from a full path.

		\param pathname Input string representing a file path.
		\return Pointer to the start of the last component of the path, which is the filename or the last directory name.
	*/
char*	 COM_SkipPath( char* pathname );

	/*!
		\brief Strips the file extension from a given path string.

		This function copies characters from the input string to the output string until it encounters a period character, which indicates the start of a file extension. The period and any following characters are not copied. The output string is null-terminated. This is commonly used to extract the base name of a file without its extension.

		\param in Input string containing the path or filename with extension
		\param out Output buffer to store the filename without extension
	*/
void	 COM_StripExtension( const char* in, char* out );

	/*!
		\brief Removes the filename portion from a file path string.

		This function takes an input file path and copies it to an output buffer, then strips the filename portion from the path by null-terminating the string at the position where the filename starts. It utilizes COM_SkipPath to locate the start of the filename within the path.

		\param in Input string containing the full file path
		\param out Output buffer where the stripped path will be stored
	*/
void	 COM_StripFilename( char* in, char* out );

	/*!
		\brief Appends a default file extension to a path if it does not already have one.

		This function checks if the provided path already contains a file extension by looking for a period in the path. If no extension is found, it appends the specified extension to the path. The function ensures that the resulting path does not exceed the maximum allowed size. The extension parameter should include the leading period, such as ".txt" or ".map".

		\param path The file path to which the extension may be appended
		\param maxSize The maximum size of the path buffer
		\param extension The default extension to append, including the leading period
	*/
void	 COM_DefaultExtension( char* path, int maxSize, const char* extension );

	/*!
		\brief Initializes a new parsing session with the specified file name.

		This function sets up the parsing environment by resetting the line counter and storing the provided file name for use during parsing operations. It prepares the global parsing state for processing a new source file.

		\param name The name of the file to be parsed
	*/
void	 COM_BeginParseSession( const char* name );

	/*!
		\brief Restores a parsing session by resetting the line counter and updating the data pointer to the backup text.

		This function is used to restore a previously saved parsing state. It resets the global com_lines variable to the backup_lines value and updates the data pointer provided by the data_p parameter to point to the backup_text. This is typically used when parsing needs to be rolled back to a previous state, such as when encountering an unexpected token or reaching the end of a parsing section.

		\param data_p Pointer to a character pointer that will be updated to point to the backup text
	*/
void	 COM_RestoreParseSession( char** data_p );

	/*!
		\brief Sets the current parsing line number for the COM library.

		This function updates the internal line counter used by the COM library for parsing operations. It is typically used to track the progress of file parsing or to set a specific line number for error reporting purposes.

		\param line The line number to set as the current parsing line
	*/
void	 COM_SetCurrentParseLine( int line );

	//! Returns the current line number of the parser.
int		 COM_GetCurrentParseLine();

	/*!
		\brief Parses the next token from a string, handling comments and quoted strings.

		This function processes an input string to extract the next token, advancing the data pointer to point to the next unparsed character. It handles both quoted and unquoted tokens, skipping over comments and managing line breaks. The parsed token is stored in a global com_token buffer and returned. The function supports parsing of quoted strings, single-line and multi-line comments, and can optionally allow line breaks within tokens.

		\param data_p Pointer to a pointer to the input string to parse. Updated to point to the next unparsed character after the token.
		\return Pointer to the parsed token stored in the global com_token buffer.
	*/
char*	 COM_Parse( char** data_p );

/*!
	\brief Parses the next token from a string, handling comments and quoted strings.

/*!
	\brief Parses the next token from a string, handling quoted strings and comments, with optional line break support.

	This function processes an input string to extract the next token, advancing the data pointer to point to the next unparsed character. It handles both quoted and unquoted tokens, skipping over
comments and managing line breaks based on the allowLineBreaks flag. The parsed token is stored in a global com_token buffer and returned. The function supports parsing of quoted strings, single-line
and multi-line comments, and can optionally allow line breaks within tokens.

	\param data_p Pointer to a pointer to the input string to parse. Updated to point to the next unparsed character after the token.
	\param allowLineBreak Flag indicating whether line breaks should be allowed within tokens.
	\return Pointer to the parsed token stored in the global com_token buffer.
*/
char*	 COM_ParseExt( char** data_p, qboolean allowLineBreak );

	/*!
		\brief Compresses the given data string by removing comments and excess whitespace.

		This function processes a null-terminated string and removes single-line comments (starting with //), multi-line comments (enclosed in /* */), and compresses whitespace sequences into single spaces. It modifies the input string in-place and returns the new length of the compressed string. The function handles carriage return and line feed characters by preserving them but removing extra spaces around them.

		\param data_p Pointer to the null-terminated input string to be compressed
		\return The length of the compressed string after processing
	*/
int		 COM_Compress( char* data_p );

/*!
	\brief Handles parse errors by formatting an error message with file name, line number, and error details

	This function is used to report parse errors during file processing. It takes a format string and variable arguments to construct an error message, then outputs the message including the current
   parsing file name and line number. The function uses variadic arguments to allow flexible error message formatting and incorporates a static buffer for message construction.

	\param format Format string for the error message
	\param  Variable arguments for the format string
*/
void	 COM_ParseError( char* format, ... );

/*!
	\brief Reports a warning message during parsing with the file name and line number.

	This function handles parsing warnings by accepting a format string and variable arguments, formatting them into a warning message, and then outputting the message along with the current parsing
   file name and line number. It uses a static buffer to hold the formatted string and leverages the Com_Printf function for output.

	\param format format string specifying the warning message format
*/
void	 COM_ParseWarning( char* format, ... );

/*!
	\brief Checks if a specific bit is set in an array of integers.

	This function determines whether a particular bit is set within a bitfield represented as an array of integers. The bitfield is processed in chunks of 32 bits, with each integer in the array
   representing one such chunk. The function handles bit numbers that exceed the range of a single integer by iterating through the array elements.

	\param array An array of integers representing a bitfield
	\param bitNum The bit index to check (0-based)
	\return qboolean indicating whether the specified bit is set
*/
qboolean COM_BitCheck( const int array[], int bitNum );

/*!
	\brief Sets a specific bit at the given bit number within the provided array of integers.

	This function is designed to set a bit at a specified position within an array of integers, where each integer can hold 32 bits. The function calculates which integer in the array contains the
   specified bit by dividing the bit number by 32. It then sets the appropriate bit using a bitwise OR operation with a shifted power of two.

	\param array Pointer to the array of integers where the bit will be set
	\param bitNum The bit number to be set within the array
*/
void	 COM_BitSet( int array[], int bitNum );

/*!
	\brief Clears a specific bit in an array of integers.

	This function clears a bit at a specified position within an array of integers. The bit position is determined by bitNum, which can be larger than 32, allowing for bits to be cleared across
   multiple integers in the array. The function calculates which integer in the array contains the bit and then clears the specific bit using a bitwise AND operation with the complement of the bit
   mask.

	\param array Pointer to the array of integers where the bit will be cleared
	\param bitNum The position of the bit to clear, can be any non-negative integer
*/
void	 COM_BitClear( int array[], int bitNum );

#define MAX_TOKENLENGTH 1024

#ifndef TT_STRING
	// token types
	#define TT_STRING	   1 // string
	#define TT_LITERAL	   2 // literal
	#define TT_NUMBER	   3 // number
	#define TT_NAME		   4 // name
	#define TT_PUNCTUATION 5 // punctuation
#endif

typedef struct pc_token_s {
	int	  type;
	int	  subtype;
	int	  intvalue;
	float floatvalue;
	char  string[MAX_TOKENLENGTH];
} pc_token_t;

// data is an in/out parm, returns a parsed out token

void	   COM_MatchToken( char** buf_p, char* match );

/*!
	\brief Skips over a braced section in a tokenized program, handling nested braces by tracking depth.

	This function processes a tokenized program string and advances through it until all nested braces are properly matched. It uses a depth counter to track opening and closing braces, ensuring that
   nested structures are correctly handled. The function continues parsing until the depth returns to zero or the program pointer becomes null.

	\param program Pointer to a pointer to the current position in the program token stream
*/
void	   SkipBracedSection( char** program );

/*!
	\brief Skips the rest of the current line in the input data, updating the data pointer to point after the newline character.

	This function processes a character pointer pointing to a string buffer and advances it until it encounters a newline character or reaches the end of the string. It also increments a global line
   counter when a newline is found. The function modifies the input pointer to point to the position after the newline character, allowing subsequent parsing operations to continue from that point.

	\param data Pointer to a character pointer that points to the current position in the input buffer
*/
void	   SkipRestOfLine( char** data );

/*!
	\brief Parses a 1D matrix of specified size from a tokenized buffer

	This function reads a sequence of floating-point values from a tokenized input buffer and stores them in a float array. It expects the input to be formatted as a parenthesized list of numbers. The
   function advances the buffer pointer as it parses tokens and validates that the format matches the expected structure.

	\param buf_p Pointer to a pointer to the current position in the input buffer
	\param x Number of elements to parse and store in the matrix
	\param m Pointer to the destination array where parsed values will be stored
*/
void	   Parse1DMatrix( char** buf_p, int x, float* m );

/*!
	\brief Parses a 2D matrix from a token stream into a flat float array

	This function reads a 2D matrix from a token stream represented by the buffer pointer. It expects the matrix to be enclosed in parentheses and parses it row by row. The matrix is stored in
   row-major order in the provided float array. The function advances the buffer pointer as it parses the tokens.

	\param buf_p Pointer to the buffer pointer containing the token stream
	\param y Number of rows in the matrix
	\param x Number of columns in the matrix
	\param m Pointer to the destination float array where the matrix will be stored
*/
void	   Parse2DMatrix( char** buf_p, int y, int x, float* m );

/*!
	\brief Parses a 3D matrix from a tokenized buffer into a flat float array.

	This function reads a 3D matrix from a tokenized input buffer and stores it in a flattened float array. It expects the input to start with an opening parenthesis, followed by z number of 2D
   matrices of dimensions y by x, and ends with a closing parenthesis. Each 2D matrix is parsed using the Parse2DMatrix function. The parsed values are stored in the float array m, with each 2D matrix
   stored contiguously in memory.

	\param buf_p Pointer to a pointer to the current position in the tokenized input buffer
	\param z Number of 2D matrices to parse
	\param y Number of rows in each 2D matrix
	\param x Number of columns in each 2D matrix
	\param m Pointer to the destination float array where the 3D matrix will be stored
*/
void	   Parse3DMatrix( char** buf_p, int z, int y, int x, float* m );

/*!
	\brief Formats a string using a printf-style format and arguments into a destination buffer with overflow checking.

	This function performs formatted string output similar to printf, but with additional safety checks. It takes a format string and a variable number of arguments, then writes the formatted result
   into the provided destination buffer. The function ensures that the output does not exceed the specified buffer size, and will issue warnings or errors if overflow occurs. The implementation uses a
   large temporary buffer to handle the formatting, then copies the result to the destination with proper size limiting.

	\param dest Destination buffer to store the formatted string
	\param size Size of the destination buffer in bytes
	\param fmt Printf-style format string describing how to format the arguments
	\throws Com_Error if the formatted string exceeds the internal bigbuffer size, or Com_Printf if the formatted string exceeds the destination buffer size
*/
void QDECL Com_sprintf( char* dest, int size, const char* fmt, ... );

// mode parm for FS_FOpenFile
typedef enum { FS_READ, FS_WRITE, FS_APPEND, FS_APPEND_SYNC } fsMode_t;

typedef enum { FS_SEEK_CUR, FS_SEEK_END, FS_SEEK_SET } fsOrigin_t;

//=============================================

int	  Q_isprint( int c );
int	  Q_islower( int c );
int	  Q_isupper( int c );
int	  Q_isalpha( int c );

/*!
	\brief Checks if a character is a numeric digit.

	This function determines whether the provided character is a numeric digit between '0' and '9'. It returns a non-zero value if the character is numeric, and zero otherwise. The function is
   commonly used to validate input or parse numeric strings.

	\param c The character to check for numeric validity
	\return Non-zero value if the character is a numeric digit, zero otherwise
*/
int	  Q_isnumeric( int c );

/*!
	\brief Checks if a character is alphanumeric, returning 1 if true and 0 if false.

	This function determines whether the provided character is either alphabetic or numeric. It utilizes the existing Q_isalpha and Q_isnumeric helper functions to perform the check. The function
   returns 1 if the character satisfies either condition, and 0 otherwise. This is commonly used in text processing and validation routines to ensure that a character is a valid part of an identifier
   or word.

	\param c The character to be checked for alphanumeric property
	\return 1 if the character is alphabetic or numeric, 0 otherwise
*/
int	  Q_isalphanumeric( int c );

/*!
	\brief Checks if a character is valid for use in a filename.

	This function determines whether a given character is acceptable for use in a filename. It allows alphanumeric characters and underscores, but excludes spaces and other special characters. The
   function is used to validate user input when saving game files or other operations requiring valid filenames. It returns 1 if the character is valid for a filename, and 0 otherwise.

	\param c The character to check for filename validity
	\return 1 if the character is valid for use in a filename, 0 otherwise
*/
int	  Q_isforfilename( int c );

// portable case insensitive compare
int	  Q_stricmp( const char* s1, const char* s2 );
int	  Q_strncmp( const char* s1, const char* s2, int n );
int	  Q_stricmpn( const char* s1, const char* s2, int n );
char* Q_strlwr( char* s1 );
char* Q_strupr( char* s1 );
char* Q_strrchr( const char* string, int c );

#ifdef _WIN32
	#define Q_putenv _putenv
#else
	#define Q_putenv putenv
#endif

// buffer size safe library replacements
void  Q_strncpyz( char* dest, const char* src, int destsize );
void  Q_strcat( char* dest, int size, const char* src );

// strlen that discounts Quake color sequences
int	  Q_PrintStrlen( const char* string );
// removes color sequences from string
char* Q_CleanStr( char* string );
// Ridah
int	  Q_strncasecmp( char* s1, char* s2, int n );
int	  Q_strcasecmp( char* s1, char* s2 );
// done.
//=============================================

// 64-bit integers for global rankings interface
// implemented as a struct for qvm compatibility
typedef struct {
	byte b0;
	byte b1;
	byte b2;
	byte b3;
	byte b4;
	byte b5;
	byte b6;
	byte b7;
} qint64;

//=============================================

short		BigShort( short l );
short		LittleShort( short l );
int			BigLong( int l );
int			LittleLong( int l );

/*!
	\brief Converts a 64-bit integer from little-endian to big-endian byte order.

	This function takes a 64-bit integer value and reverses its byte order, converting it from little-endian representation to big-endian representation. It is typically used when data needs to be
   serialized or transmitted across systems with different endianness. The implementation delegates to an internal helper function _BigLong64 to perform the actual byte swapping operation.

	\param l The 64-bit integer value to convert from little-endian to big-endian byte order
	\return The 64-bit integer with its byte order reversed from little-endian to big-endian
*/
qint64		BigLong64( qint64 l );

/*!
	\brief Returns the byte-swapped value of a 64-bit integer.

	This function performs byte-order conversion on a 64-bit integer to convert it from big-endian to little-endian format or vice versa. It is typically used when data needs to be serialized or
   deserialized for cross-platform compatibility.

	\param l The 64-bit integer value to be byte-swapped
	\return The byte-swapped 64-bit integer value
*/
qint64		LittleLong64( qint64 l );
float		BigFloat( float l );
float		LittleFloat( float l );

void		Swap_Init();
char* QDECL va( char* format, ... );
float*		tv( float x, float y, float z );

//=============================================

//
// key / value info strings
//
char*		Info_ValueForKey( const char* s, const char* key );
void		Info_RemoveKey( char* s, const char* key );
void		Info_RemoveKey_big( char* s, const char* key );
void		Info_SetValueForKey( char* s, const char* key, const char* value );
void		Info_SetValueForKey_Big( char* s, const char* key, const char* value );
qboolean	Info_Validate( const char* s );

/*!
	\brief Parses the next key-value pair from a string and advances the string pointer.

	This function extracts a key-value pair from a string buffer that is formatted as a series of \key\value\key\value\... sequences. It reads the key first, then the value, and updates the head
   pointer to point to the next location in the string after the current pair. Both key and value buffers must be pre-allocated and large enough to hold the parsed data. The function handles the case
   where either key or value might be empty, and returns early if the end of the string is reached.

	\param s Pointer to the current position in the string buffer to parse
	\param key Buffer to store the parsed key string
	\param value Buffer to store the parsed value string
*/
void		Info_NextPair( const char** s, char* key, char* value );

/*!
	\brief Handles system errors by logging the error message, performing cleanup operations, and terminating the program.

	This function is responsible for managing error conditions within the system. It accepts an error level and a formatted message, then processes the error according to its type. For different error
   codes, it performs specific actions such as disconnecting the client, shutting down the server, or displaying error messages. The function also tracks repeated errors to prevent infinite loops and
   ensures proper cleanup before termination. It uses a variable argument list to handle formatted error messages and supports various error types including fatal errors, server disconnections, and
   missing CD requirements.

	\param level The severity level of the error, determining how it's handled
	\param error A format string specifying the error message with optional arguments
	\return This function does not return as it terminates the program after handling the error
	\throws This function may throw an error and terminate the program
*/
void QDECL	Com_Error( int level, const char* error, ... );

/*!
	\brief Prints a formatted message to the console and log file.

	This function handles formatted output to the console, log file, and dedicated server console. It supports variable arguments and can route output to multiple destinations including the console, a
   buffered rcon output, and a log file. The function manages the opening and writing to the rtcwconsole.log file when needed, and ensures proper flushing of buffered data. It handles both regular and
   dedicated server console output, and writes to a log file when enabled.

	\param msg Format string for the message to be printed
*/
void QDECL	Com_Printf( const char* msg, ... );

/*
==============================================================

SAVE

	12 -
	13 - (SA) added 'episode' tracking to savegame
	14 - RF added 'skill'
	15 - (SA) moved time info above the main game reading
	16 - (SA) added fog
	17 - (SA) rats, changed fog.
  18 - TTimo targetdeath fix
	   show_bug.cgi?id=434

==============================================================
*/

#define SAVE_VERSION		   18
#define SAVE_INFOSTRING_LENGTH 256

/*
==========================================================

  RELOAD STATES

==========================================================
*/

#define RELOAD_SAVEGAME		   0x01
#define RELOAD_NEXTMAP		   0x02
#define RELOAD_NEXTMAP_WAITING 0x04
#define RELOAD_FAILED		   0x08
#define RELOAD_ENDGAME		   0x10

/*
==========================================================

CVARS (console variables)

Many variables can be used for cheating purposes, so when
cheats is zero, force all unspecified variables to their
default values.
==========================================================
*/

#define CVAR_ARCHIVE		   1 // set to cause it to be saved to vars.rc
// used for system variables, not for player
// specific configurations
#define CVAR_USERINFO		   2  // sent to server on connect or change
#define CVAR_SERVERINFO		   4  // sent in response to front end requests
#define CVAR_SYSTEMINFO		   8  // these cvars will be duplicated on all clients
#define CVAR_INIT			   16 // don't allow change from console at all,
// but can be set from the command line
#define CVAR_LATCH			   32 // will only change when C code next does
// a Cvar_Get(), so it can't be changed
// without proper initialization.  modified
// will be set, even though the value hasn't
// changed yet
#define CVAR_ROM			   64	// display only, cannot be set by user at all
#define CVAR_USER_CREATED	   128	// created by a set command
#define CVAR_TEMP			   256	// can be set even when cheats are disabled, but is not archived
#define CVAR_CHEAT			   512	// can not be changed if cheats are disabled
#define CVAR_NORESTART		   1024 // do not clear when a cvar_restart is issued

// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s {
	char*		   name;
	char*		   string;
	char*		   resetString;	  // cvar_restart will reset to this value
	char*		   latchedString; // for CVAR_LATCH vars
	int			   flags;
	qboolean	   modified;		  // set each time the cvar is changed
	int			   modificationCount; // incremented each time the cvar is changed
	float		   value;			  // atof( string )
	int			   integer;			  // atoi( string )
	struct cvar_s* next;
	struct cvar_s* hashNext;
} cvar_t;

#define MAX_CVAR_VALUE_STRING 256

typedef int cvarHandle_t;

// the modules that run in the virtual machine can't access the cvar_t directly,
// so they must ask for structured updates
typedef struct {
	cvarHandle_t handle;
	int			 modificationCount;
	float		 value;
	int			 integer;
	char		 string[MAX_CVAR_VALUE_STRING];
} vmCvar_t;

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

#include "surfaceflags.h" // shared with the q3map utility

// plane types are used to speed some tests
// 0-2 are axial planes
#define PLANE_X					0
#define PLANE_Y					1
#define PLANE_Z					2
#define PLANE_NON_AXIAL			3

/*
=================
PlaneTypeForNormal
=================
*/

#define PlaneTypeForNormal( x ) ( x[0] == 1.0 ? PLANE_X : ( x[1] == 1.0 ? PLANE_Y : ( x[2] == 1.0 ? PLANE_Z : PLANE_NON_AXIAL ) ) )

// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
typedef struct cplane_s {
	vec3_t normal;
	float  dist;
	byte   type;	 // for fast side tests: 0,1,2 = axial, 3 = nonaxial
	byte   signbits; // signx + (signy<<1) + (signz<<2), used as lookup during collision
	byte   pad[2];
} cplane_t;

// a trace is returned when a box is swept through the world
typedef struct {
	qboolean allsolid;	   // if true, plane is not valid
	qboolean startsolid;   // if true, the initial point was in a solid area
	float	 fraction;	   // time completed, 1.0 = didn't hit anything
	vec3_t	 endpos;	   // final position
	cplane_t plane;		   // surface normal at impact, transformed to world space
	int		 surfaceFlags; // surface hit
	int		 contents;	   // contents on other side of surface hit
	int		 entityNum;	   // entity the contacted sirface is a part of
} trace_t;

// trace->entityNum can also be 0 to (MAX_GENTITIES-1)
// or ENTITYNUM_NONE, ENTITYNUM_WORLD

// markfragments are returned by CM_MarkFragments()
typedef struct {
	int firstPoint;
	int numPoints;
} markFragment_t;

typedef struct {
	vec3_t origin;
	vec3_t axis[3];
} orientation_t;

//=====================================================================

// in order from highest priority to lowest
// if none of the catchers are active, bound key strings will be executed
#define KEYCATCH_CONSOLE 0x0001
#define KEYCATCH_UI		 0x0002
#define KEYCATCH_MESSAGE 0x0004
#define KEYCATCH_CGAME	 0x0008

// sound channels
// channel 0 never willingly overrides
// other channels will allways override a playing sound on that channel
typedef enum {
	CHAN_AUTO,
	CHAN_LOCAL, // menu sounds, etc
	CHAN_WEAPON,
	CHAN_VOICE,
	CHAN_ITEM,
	CHAN_BODY,
	CHAN_LOCAL_SOUND, // chat messages, etc
	CHAN_ANNOUNCER	  // announcer voices, etc
} soundChannel_t;

/*
========================================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

========================================================================
*/
#define ANIM_BITS					10

#define ANGLE2SHORT( x )			( ( int )( ( x ) * 65536 / 360 ) & 65535 )
#define SHORT2ANGLE( x )			( ( x ) * ( 360.0 / 65536 ) )

#define SNAPFLAG_RATE_DELAYED		1
#define SNAPFLAG_NOT_ACTIVE			2 // snapshot used during connection and for zombies
#define SNAPFLAG_SERVERCOUNT		4 // toggled every map_restart so transitions can be detected

//
// per-level limits
//
#define MAX_CLIENTS					128 // absolute limit
#define MAX_LOCATIONS				64

#define GENTITYNUM_BITS				11 // don't need to send any more		(SA) upped 4/21/2001 adjusted: tr_local.h (802-822), tr_main.c (1501), sv_snapshot (206)
#define MAX_GENTITIES				( 1 << GENTITYNUM_BITS )

// entitynums are communicated with GENTITY_BITS, so any reserved
// values thatare going to be communcated over the net need to
// also be in this range
#define ENTITYNUM_NONE				( MAX_GENTITIES - 1 )
#define ENTITYNUM_WORLD				( MAX_GENTITIES - 2 )
#define ENTITYNUM_MAX_NORMAL		( MAX_GENTITIES - 2 )

#define MAX_MODELS					256 // these are sent over the net as 8 bits
#define MAX_SOUNDS					256 // so they cannot be blindly increased

#define MAX_PARTICLES_AREAS			128

#define MAX_MULTI_SPAWNTARGETS		16 // JPW NERVE

// #define	MAX_CONFIGSTRINGS	1024
#define MAX_CONFIGSTRINGS			2048

#define MAX_DLIGHT_CONFIGSTRINGS	128
#define MAX_CLIPBOARD_CONFIGSTRINGS 64
#define MAX_SPLINE_CONFIGSTRINGS	64

#define PARTICLE_SNOW128			1
#define PARTICLE_SNOW64				2
#define PARTICLE_SNOW32				3
#define PARTICLE_SNOW256			0

#define PARTICLE_BUBBLE8			4
#define PARTICLE_BUBBLE16			5
#define PARTICLE_BUBBLE32			6
#define PARTICLE_BUBBLE64			7

// these are the only configstrings that the system reserves, all the
// other ones are strictly for servergame to clientgame communication
#define CS_SERVERINFO				0 // an info string with all the serverinfo cvars
#define CS_SYSTEMINFO				1 // an info string for server system to client system configuration (timescale, etc)

#define RESERVED_CONFIGSTRINGS		2 // game can't modify below this, only the system can

#define MAX_GAMESTATE_CHARS			16000
typedef struct {
	int	 stringOffsets[MAX_CONFIGSTRINGS];
	char stringData[MAX_GAMESTATE_CHARS];
	int	 dataCount;
} gameState_t;

#define REF_FORCE_DLIGHT  ( 1 << 31 ) // RF, passed in through overdraw parameter, force this dlight under all conditions
#define REF_JUNIOR_DLIGHT ( 1 << 30 ) // (SA) this dlight does not light surfaces.  it only affects dynamic light grid

//=========================================================
// shared by AI and animation scripting
//
typedef enum {
	AISTATE_RELAXED,
	AISTATE_QUERY,
	AISTATE_ALERT,
	AISTATE_COMBAT,

	MAX_AISTATES
} aistateEnum_t;

//=========================================================

// weapon grouping
#define MAX_WEAP_BANKS		   12
#define MAX_WEAPS_IN_BANK	   3
// JPW NERVE
#define MAX_WEAPS_IN_BANK_MP   8
#define MAX_WEAP_BANKS_MP	   7
// jpw
#define MAX_WEAP_ALTS		   WP_DYNAMITE

// bit field limits
#define MAX_STATS			   16
#define MAX_PERSISTANT		   16
#define MAX_POWERUPS		   16
#define MAX_WEAPONS			   64 // (SA) and yet more!
#define MAX_HOLDABLE		   16

// Ridah, increased this
// #define	MAX_PS_EVENTS			2
// ACK: I'd really like to make this 4, but that seems to cause network problems
#define MAX_EVENTS			   4 // max events per frame before we drop events
// #define	MAX_EVENTS				2	// max events per frame before we drop events

#define PS_PMOVEFRAMECOUNTBITS 6

// playerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c

// playerState_t is a full superset of entityState_t as it is used by players,
// so if a playerState_t is transmitted, the entityState_t can be fully derived
// from it.
//
// NOTE: all fields in here must be 32 bits (or those within sub-structures)
typedef struct playerState_s {
	int			  commandTime; // cmd->serverTime of last executed command
	int			  pm_type;
	int			  bobCycle; // for view bobbing and footstep generation
	int			  pm_flags; // ducked, jump_held, etc
	int			  pm_time;

	vec3_t		  origin;
	vec3_t		  velocity;
	int			  weaponTime;
	int			  weaponDelay;	   // for weapons that don't fire immediately when 'fire' is hit (grenades, venom, ...)
	int			  grenadeTimeLeft; // for delayed grenade throwing.  this is set to a #define for grenade
	// lifetime when the attack button goes down, then when attack is released
	// this is the amount of time left before the grenade goes off (or if it
	// gets to 0 while in players hand, it explodes)

	int			  gravity;
	float		  leanf; // amount of 'lean' when player is looking around corner //----(SA)	added

	int			  speed;
	int			  delta_angles[3]; // add to command angles to get view direction
	// changed by spawns, rotating objects, and teleporters

	int			  groundEntityNum; // ENTITYNUM_NONE = in air

	int			  legsTimer; // don't change low priority animations until this runs out
	int			  legsAnim;	 // mask off ANIM_TOGGLEBIT

	int			  torsoTimer; // don't change low priority animations until this runs out
	int			  torsoAnim;  // mask off ANIM_TOGGLEBIT

	int			  movementDir; // a number 0 to 7 that represents the reletive angle
	// of movement to the view angle (axial and diagonals)
	// when at rest, the value will remain unchanged
	// used to twist the legs during strafing

	int			  eFlags; // copied to entityState_t->eFlags

	int			  eventSequence; // pmove generated events
	int			  events[MAX_EVENTS];
	int			  eventParms[MAX_EVENTS];
	int			  oldEventSequence; // so we can see which events have been added since we last converted to entityState_t

	int			  externalEvent; // events set on player from another source
	int			  externalEventParm;
	int			  externalEventTime;

	int			  clientNum; // ranges from 0 to MAX_CLIENTS-1

	// weapon info
	int			  weapon; // copied to entityState_t->weapon
	int			  weaponstate;

	// item info
	int			  item;

	vec3_t		  viewangles; // for fixed views
	int			  viewheight;

	// damage feedback
	int			  damageEvent; // when it changes, latch the other parms
	int			  damageYaw;
	int			  damagePitch;
	int			  damageCount;

	int			  stats[MAX_STATS];
	int			  persistant[MAX_PERSISTANT]; // stats that aren't cleared on death
	int			  powerups[MAX_POWERUPS];	  // level.time that the powerup runs out
	int			  ammo[MAX_WEAPONS];		  // total amount of ammo
	int			  ammoclip[MAX_WEAPONS];	  // ammo in clip
	int			  holdable[MAX_HOLDABLE];
	int			  holding;										// the current item in holdable[] that is selected (held)
	int			  weapons[MAX_WEAPONS / ( sizeof( int ) * 8 )]; // 64 bits for weapons held

	// Ridah, allow for individual bounding boxes
	vec3_t		  mins, maxs;
	float		  crouchMaxZ;
	float		  crouchViewHeight, standViewHeight, deadViewHeight;
	// variable movement speed
	float		  runSpeedScale, sprintSpeedScale, crouchSpeedScale;
	// done.

	// Ridah, view locking for mg42
	int			  viewlocked;
	int			  viewlocked_entNum;

	// Ridah, need this to fix friction problems with slow zombie's whereby
	// the friction prevents them from accelerating to their full potential
	float		  friction;

	// Ridah, AI character id is used for weapon association
	int			  aiChar;
	int			  teamNum;

	// Rafael
	int			  gunfx;

	// RF, burning effect is required for view blending effect
	int			  onFireStart;

	int			  serverCursorHint;	   // what type of cursor hint the server is dictating
	int			  serverCursorHintVal; // a value (0-255) associated with the above

	trace_t		  serverCursorHintTrace; // not communicated over net, but used to store the current server-side cursorhint trace

	// ----------------------------------------------------------------------
	// not communicated over the net at all
	// FIXME: this doesn't get saved between predicted frames on the clients-side (cg.predictedPlayerState)
	// So to use persistent variables here, which don't need to come from the server,
	// we could use a marker variable, and use that to store everything after it
	// before we read in the new values for the predictedPlayerState, then restore them
	// after copying the structure recieved from the server.

	// (SA) yeah.  this is causing me a little bit of trouble too.  can we go ahead with the above suggestion or find an alternative?

	int			  ping;				// server to game info for scoreboard
	int			  pmove_framecount; // FIXME: don't transmit over the network
	int			  entityEventSequence;

	int			  sprintTime;
	int			  sprintExertTime;

	// JPW NERVE -- value for all multiplayer classes with regenerating "class weapons" -- ie LT artillery, medic medpack, engineer build points, etc
	int			  classWeaponTime;
	int			  jumpTime; // used in SP/MP to prevent jump accel
	// jpw

	int			  weapAnimTimer; // don't change low priority animations until this runs out
	int			  weapAnim;		 // mask off ANIM_TOGGLEBIT

	qboolean	  releasedFire;

	float		  aimSpreadScaleFloat; // (SA) the server-side aimspreadscale that lets it track finer changes but still only
	// transmit the 8bit int to the client
	int			  aimSpreadScale; // 0 - 255 increases with angular movement
	int			  lastFireTime;	  // used by server to hold last firing frame briefly when randomly releasing trigger (AI)

	int			  quickGrenTime;

	int			  leanStopDebounceTime;

	int			  weapHeat[MAX_WEAPONS]; // some weapons can overheat.  this tracks (server-side) how hot each weapon currently is.
	int			  curWeapHeat;			 // value for the currently selected weapon (for transmission to client)

	int			  venomTime;

	//----(SA)	added
	int			  accShowBits; // RF (changed from short), these should all be 32 bit
	int			  accHideBits;
	//----(SA)	end

	aistateEnum_t aiState;

	float		  footstepCount;

} playerState_t;

//====================================================================

//
// usercmd_t->button bits, many of which are generated by the client system,
// so they aren't game/cgame only definitions
//
#define BUTTON_ATTACK		1
#define BUTTON_TALK			2 // displays talk balloon and disables actions
#define BUTTON_USE_HOLDABLE 4
#define BUTTON_GESTURE		8
#define BUTTON_WALKING		16 // walking can't just be infered from MOVE_RUN
// because a key pressed late in the frame will
// only generate a small move value for that frame
// walking will use different animations and
// won't generate footsteps
//----(SA)	added
#define BUTTON_SPRINT		32
#define BUTTON_ACTIVATE		64
//----(SA)	end

#define BUTTON_ANY			128 // any key whatsoever

//----(SA) wolf buttons
#define WBUTTON_ATTACK2		1
#define WBUTTON_ZOOM		2
#define WBUTTON_QUICKGREN	4
#define WBUTTON_RELOAD		8
#define WBUTTON_LEANLEFT	16
#define WBUTTON_LEANRIGHT	32

// unused
#define WBUTTON_EXTRA6		64
#define WBUTTON_EXTRA7		128
//----(SA) end

#define MOVE_RUN			120 // if forwardmove or rightmove are >= MOVE_RUN,
// then BUTTON_WALKING should be set

// usercmd_t is sent to the server each client frame
typedef struct usercmd_s {
	int			   serverTime;
	byte		   buttons;
	byte		   wbuttons;
	byte		   weapon;
	byte		   holdable; //----(SA)	added
	int			   angles[3];

	signed char	   forwardmove, rightmove, upmove;
	signed char	   wolfkick; // RF, we should move this over to a wbutton, this is a huge waste of bandwidth

	unsigned short cld; // NERVE - SMF - send client damage in usercmd instead of as a server command
} usercmd_t;

//===================================================================

// if entityState->solid == SOLID_BMODEL, modelindex is an inline model number
#define SOLID_BMODEL 0xffffff

typedef enum {
	TR_STATIONARY,
	TR_INTERPOLATE, // non-parametric, but interpolate between snapshots
	TR_LINEAR,
	TR_LINEAR_STOP,
	TR_LINEAR_STOP_BACK, //----(SA)	added.  so reverse movement can be different than forward
	TR_SINE,			 // value = base + sin( time / duration ) * delta
	TR_GRAVITY,
	// Ridah
	TR_GRAVITY_LOW,
	TR_GRAVITY_FLOAT,  // super low grav with no gravity acceleration (floating feathers/fabric/leaves/...)
	TR_GRAVITY_PAUSED, //----(SA)	has stopped, but will still do a short trace to see if it should be switched back to TR_GRAVITY
	TR_ACCELERATE,
	TR_DECCELERATE
} trType_t;

typedef struct {
	trType_t trType;
	int		 trTime;
	int		 trDuration; // if non 0, trTime + trDuration = stop time
	//----(SA)	removed
	vec3_t	 trBase;
	vec3_t	 trDelta; // velocity, etc
					  //----(SA)	removed
} trajectory_t;

// RF, put this here so we have a central means of defining a Zombie (kind of a hack, but this is to minimize bandwidth usage)
#define SET_FLAMING_ZOMBIE( x, y ) ( x.frame = y )
#define IS_FLAMING_ZOMBIE( x )	   ( x.frame == 1 )

// entityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large
//
// NOTE: all fields in here must be 32 bits (or those within sub-structures)

typedef struct entityLighting_s {
	int	   lightRadius;
	vec3_t lightColor;
} entityLighting_t;

typedef struct entityState_s {
	int				 number; // entity index
	int				 eType;	 // entityType_t
	int				 eFlags;

	trajectory_t	 pos;  // for calculating position
	trajectory_t	 apos; // for calculating angles

	entityLighting_t light;

	int				 time;
	int				 time2;

	vec3_t			 origin;
	vec3_t			 origin2;

	vec3_t			 angles;
	vec3_t			 angles2;

	int				 otherEntityNum; // shotgun sources, etc
	int				 otherEntityNum2;

	int				 groundEntityNum; // -1 = in air

	int				 constantLight; // r + (g<<8) + (b<<16) + (intensity<<24)
	int				 dl_intensity;	// used for coronas
	int				 loopSound;		// constantly loop this sound

	int				 modelindex;
	int				 modelindex2;
	int				 clientNum; // 0 to (MAX_CLIENTS - 1), for players and corpses
	int				 frame;

	int				 solid; // for client side prediction, sys->linkentity sets this properly

	// old style events, in for compatibility only
	int				 event;
	int				 eventParm;

	int				 eventSequence; // pmove generated events
	int				 events[MAX_EVENTS];
	int				 eventParms[MAX_EVENTS];

	// for players
	int				 powerups;	// bit flags
	int				 weapon;	// determines weapon and flash model, etc
	int				 legsAnim;	// mask off ANIM_TOGGLEBIT
	int				 torsoAnim; // mask off ANIM_TOGGLEBIT
	//	int		weapAnim;		// mask off ANIM_TOGGLEBIT	//----(SA)	removed (weap anims will be client-side only)

	int				 density; // for particle effects

	int				 dmgFlags; // to pass along additional information for damage effects for players/ Also used for cursorhints for non-player entities

	// Ridah
	int				 onFireStart, onFireEnd;

	int				 aiChar, teamNum;

	int				 effect1Time, effect2Time, effect3Time;

	aistateEnum_t	 aiState;

	int				 animMovetype; // clients can't derive movetype of other clients for anim scripting system

} entityState_t;

typedef enum {
	CA_UNINITIALIZED,
	CA_DISCONNECTED, // not talking to a server
	CA_AUTHORIZING,	 // not used any more, was checking cd key
	CA_CONNECTING,	 // sending request packets to the server
	CA_CHALLENGING,	 // sending challenge packets to the server
	CA_CONNECTED,	 // netchan_t established, getting gamestate
	CA_LOADING,		 // only during cgame initialization, never during main loop
	CA_PRIMED,		 // got gamestate, waiting for first frame
	CA_ACTIVE,		 // game views should be displayed
	CA_CINEMATIC	 // playing a cinematic or a static pic, not connected to a server
} connstate_t;

// font support

#define GLYPH_START		0
#define GLYPH_END		255
#define GLYPH_CHARSTART 32
#define GLYPH_CHAREND	127
#define GLYPHS_PER_FONT GLYPH_END - GLYPH_START + 1
typedef struct {
	int		  height;	   // number of scan lines
	int		  top;		   // top of glyph in buffer
	int		  bottom;	   // bottom of glyph in buffer
	int		  pitch;	   // width for copying
	int		  xSkip;	   // x adjustment
	int		  imageWidth;  // width of actual image
	int		  imageHeight; // height of actual image
	float	  s;		   // x offset in image where glyph starts
	float	  t;		   // y offset in image where glyph starts
	float	  s2;
	float	  t2;
	qhandle_t glyph; // handle to the shader with the glyph
	char	  shaderName[32];
} glyphInfo_t;

typedef struct {
	glyphInfo_t glyphs[GLYPHS_PER_FONT];
	float		glyphScale;
	char		name[MAX_QPATH];
} fontInfo_t;

#define Square( x ) ( ( x ) * ( x ) )

// real time
//=============================================

typedef struct qtime_s {
	int tm_sec;	  /* seconds after the minute - [0,59] */
	int tm_min;	  /* minutes after the hour - [0,59] */
	int tm_hour;  /* hours since midnight - [0,23] */
	int tm_mday;  /* day of the month - [1,31] */
	int tm_mon;	  /* months since January - [0,11] */
	int tm_year;  /* years since 1900 */
	int tm_wday;  /* days since Sunday - [0,6] */
	int tm_yday;  /* days since January 1 - [0,365] */
	int tm_isdst; /* daylight savings time flag */
} qtime_t;

// server browser sources
#define AS_LOCAL	 0
#define AS_MPLAYER	 1
#define AS_GLOBAL	 2
#define AS_FAVORITES 3

// cinematic states
typedef enum {
	FMV_IDLE,
	FMV_PLAY, // play
	FMV_EOF,  // all other conditions, i.e. stop/EOF/abort
	FMV_ID_BLT,
	FMV_ID_IDLE,
	FMV_LOOPED,
	FMV_ID_WAIT
} e_status;

typedef enum _flag_status {
	FLAG_ATBASE = 0,
	FLAG_TAKEN,		 // CTF
	FLAG_TAKEN_RED,	 // One Flag CTF
	FLAG_TAKEN_BLUE, // One Flag CTF
	FLAG_DROPPED
} flagStatus_t;

#define MAX_GLOBAL_SERVERS		 2048
#define MAX_OTHER_SERVERS		 128
#define MAX_PINGREQUESTS		 16
#define MAX_SERVERSTATUSREQUESTS 16

#define SAY_ALL					 0
#define SAY_TEAM				 1
#define SAY_TELL				 2

#define CDKEY_LEN				 16
#define CDCHKSUM_LEN			 2

// NERVE - SMF - localization
typedef enum { LANGUAGE_FRENCH = 0, LANGUAGE_GERMAN, LANGUAGE_ITALIAN, LANGUAGE_SPANISH, MAX_LANGUAGES } languages_t;

typedef union {
	float		 f;
	int			 i;
	unsigned int ui;
} floatint_t;

/*!
	\brief Converts a float value to its integer representation by bit casting.

	This function performs a bit-level conversion of a float value to an integer representation. It uses a union-like structure to reinterpret the bits of the float as an integer without any
   arithmetic conversion. This technique is often used when low-level bit manipulation is required, such as when implementing certain mathematical operations or when interfacing with hardware that
   expects specific bit patterns.

	\param f The float value to convert to integer representation
	\return The integer representation of the bits that constitute the input float value
*/
static int FloatAsInt( float f )
{
	floatint_t fi;
	fi.f = f;
	return fi.i;
}

#endif // __Q_SHARED_H
