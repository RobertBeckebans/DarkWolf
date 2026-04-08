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

// Copyright (C) 1999-2000 Id Software, Inc.
//
// q_math.c -- stateless support routines that are included in each code module
#include "q_shared.h"

vec3_t vec3_origin	  = { 0, 0, 0 };
vec3_t axisDefault[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

vec4_t colorBlack	= { 0, 0, 0, 1 };
vec4_t colorRed		= { 1, 0, 0, 1 };
vec4_t colorGreen	= { 0, 1, 0, 1 };
vec4_t colorBlue	= { 0, 0, 1, 1 };
vec4_t colorYellow	= { 1, 1, 0, 1 };
vec4_t colorMagenta = { 1, 0, 1, 1 };
vec4_t colorCyan	= { 0, 1, 1, 1 };
vec4_t colorWhite	= { 1, 1, 1, 1 };
vec4_t colorLtGrey	= { 0.75, 0.75, 0.75, 1 };
vec4_t colorMdGrey	= { 0.5, 0.5, 0.5, 1 };
vec4_t colorDkGrey	= { 0.25, 0.25, 0.25, 1 };

vec4_t g_color_table[8] = {
	{ 0.0, 0.0, 0.0, 1.0 },
	{ 1.0, 0.0, 0.0, 1.0 },
	{ 0.0, 1.0, 0.0, 1.0 },
	{ 1.0, 1.0, 0.0, 1.0 },
	{ 0.0, 0.0, 1.0, 1.0 },
	{ 0.0, 1.0, 1.0, 1.0 },
	{ 1.0, 0.0, 1.0, 1.0 },
	{ 1.0, 1.0, 1.0, 1.0 },
};

vec3_t bytedirs[NUMVERTEXNORMALS] = { { -0.525731, 0.000000, 0.850651 },
	{ -0.442863, 0.238856, 0.864188 },
	{ -0.295242, 0.000000, 0.955423 },
	{ -0.309017, 0.500000, 0.809017 },
	{ -0.162460, 0.262866, 0.951056 },
	{ 0.000000, 0.000000, 1.000000 },
	{ 0.000000, 0.850651, 0.525731 },
	{ -0.147621, 0.716567, 0.681718 },
	{ 0.147621, 0.716567, 0.681718 },
	{ 0.000000, 0.525731, 0.850651 },
	{ 0.309017, 0.500000, 0.809017 },
	{ 0.525731, 0.000000, 0.850651 },
	{ 0.295242, 0.000000, 0.955423 },
	{ 0.442863, 0.238856, 0.864188 },
	{ 0.162460, 0.262866, 0.951056 },
	{ -0.681718, 0.147621, 0.716567 },
	{ -0.809017, 0.309017, 0.500000 },
	{ -0.587785, 0.425325, 0.688191 },
	{ -0.850651, 0.525731, 0.000000 },
	{ -0.864188, 0.442863, 0.238856 },
	{ -0.716567, 0.681718, 0.147621 },
	{ -0.688191, 0.587785, 0.425325 },
	{ -0.500000, 0.809017, 0.309017 },
	{ -0.238856, 0.864188, 0.442863 },
	{ -0.425325, 0.688191, 0.587785 },
	{ -0.716567, 0.681718, -0.147621 },
	{ -0.500000, 0.809017, -0.309017 },
	{ -0.525731, 0.850651, 0.000000 },
	{ 0.000000, 0.850651, -0.525731 },
	{ -0.238856, 0.864188, -0.442863 },
	{ 0.000000, 0.955423, -0.295242 },
	{ -0.262866, 0.951056, -0.162460 },
	{ 0.000000, 1.000000, 0.000000 },
	{ 0.000000, 0.955423, 0.295242 },
	{ -0.262866, 0.951056, 0.162460 },
	{ 0.238856, 0.864188, 0.442863 },
	{ 0.262866, 0.951056, 0.162460 },
	{ 0.500000, 0.809017, 0.309017 },
	{ 0.238856, 0.864188, -0.442863 },
	{ 0.262866, 0.951056, -0.162460 },
	{ 0.500000, 0.809017, -0.309017 },
	{ 0.850651, 0.525731, 0.000000 },
	{ 0.716567, 0.681718, 0.147621 },
	{ 0.716567, 0.681718, -0.147621 },
	{ 0.525731, 0.850651, 0.000000 },
	{ 0.425325, 0.688191, 0.587785 },
	{ 0.864188, 0.442863, 0.238856 },
	{ 0.688191, 0.587785, 0.425325 },
	{ 0.809017, 0.309017, 0.500000 },
	{ 0.681718, 0.147621, 0.716567 },
	{ 0.587785, 0.425325, 0.688191 },
	{ 0.955423, 0.295242, 0.000000 },
	{ 1.000000, 0.000000, 0.000000 },
	{ 0.951056, 0.162460, 0.262866 },
	{ 0.850651, -0.525731, 0.000000 },
	{ 0.955423, -0.295242, 0.000000 },
	{ 0.864188, -0.442863, 0.238856 },
	{ 0.951056, -0.162460, 0.262866 },
	{ 0.809017, -0.309017, 0.500000 },
	{ 0.681718, -0.147621, 0.716567 },
	{ 0.850651, 0.000000, 0.525731 },
	{ 0.864188, 0.442863, -0.238856 },
	{ 0.809017, 0.309017, -0.500000 },
	{ 0.951056, 0.162460, -0.262866 },
	{ 0.525731, 0.000000, -0.850651 },
	{ 0.681718, 0.147621, -0.716567 },
	{ 0.681718, -0.147621, -0.716567 },
	{ 0.850651, 0.000000, -0.525731 },
	{ 0.809017, -0.309017, -0.500000 },
	{ 0.864188, -0.442863, -0.238856 },
	{ 0.951056, -0.162460, -0.262866 },
	{ 0.147621, 0.716567, -0.681718 },
	{ 0.309017, 0.500000, -0.809017 },
	{ 0.425325, 0.688191, -0.587785 },
	{ 0.442863, 0.238856, -0.864188 },
	{ 0.587785, 0.425325, -0.688191 },
	{ 0.688191, 0.587785, -0.425325 },
	{ -0.147621, 0.716567, -0.681718 },
	{ -0.309017, 0.500000, -0.809017 },
	{ 0.000000, 0.525731, -0.850651 },
	{ -0.525731, 0.000000, -0.850651 },
	{ -0.442863, 0.238856, -0.864188 },
	{ -0.295242, 0.000000, -0.955423 },
	{ -0.162460, 0.262866, -0.951056 },
	{ 0.000000, 0.000000, -1.000000 },
	{ 0.295242, 0.000000, -0.955423 },
	{ 0.162460, 0.262866, -0.951056 },
	{ -0.442863, -0.238856, -0.864188 },
	{ -0.309017, -0.500000, -0.809017 },
	{ -0.162460, -0.262866, -0.951056 },
	{ 0.000000, -0.850651, -0.525731 },
	{ -0.147621, -0.716567, -0.681718 },
	{ 0.147621, -0.716567, -0.681718 },
	{ 0.000000, -0.525731, -0.850651 },
	{ 0.309017, -0.500000, -0.809017 },
	{ 0.442863, -0.238856, -0.864188 },
	{ 0.162460, -0.262866, -0.951056 },
	{ 0.238856, -0.864188, -0.442863 },
	{ 0.500000, -0.809017, -0.309017 },
	{ 0.425325, -0.688191, -0.587785 },
	{ 0.716567, -0.681718, -0.147621 },
	{ 0.688191, -0.587785, -0.425325 },
	{ 0.587785, -0.425325, -0.688191 },
	{ 0.000000, -0.955423, -0.295242 },
	{ 0.000000, -1.000000, 0.000000 },
	{ 0.262866, -0.951056, -0.162460 },
	{ 0.000000, -0.850651, 0.525731 },
	{ 0.000000, -0.955423, 0.295242 },
	{ 0.238856, -0.864188, 0.442863 },
	{ 0.262866, -0.951056, 0.162460 },
	{ 0.500000, -0.809017, 0.309017 },
	{ 0.716567, -0.681718, 0.147621 },
	{ 0.525731, -0.850651, 0.000000 },
	{ -0.238856, -0.864188, -0.442863 },
	{ -0.500000, -0.809017, -0.309017 },
	{ -0.262866, -0.951056, -0.162460 },
	{ -0.850651, -0.525731, 0.000000 },
	{ -0.716567, -0.681718, -0.147621 },
	{ -0.716567, -0.681718, 0.147621 },
	{ -0.525731, -0.850651, 0.000000 },
	{ -0.500000, -0.809017, 0.309017 },
	{ -0.238856, -0.864188, 0.442863 },
	{ -0.262866, -0.951056, 0.162460 },
	{ -0.864188, -0.442863, 0.238856 },
	{ -0.809017, -0.309017, 0.500000 },
	{ -0.688191, -0.587785, 0.425325 },
	{ -0.681718, -0.147621, 0.716567 },
	{ -0.442863, -0.238856, 0.864188 },
	{ -0.587785, -0.425325, 0.688191 },
	{ -0.309017, -0.500000, 0.809017 },
	{ -0.147621, -0.716567, 0.681718 },
	{ -0.425325, -0.688191, 0.587785 },
	{ -0.162460, -0.262866, 0.951056 },
	{ 0.442863, -0.238856, 0.864188 },
	{ 0.162460, -0.262866, 0.951056 },
	{ 0.309017, -0.500000, 0.809017 },
	{ 0.147621, -0.716567, 0.681718 },
	{ 0.000000, -0.525731, 0.850651 },
	{ 0.425325, -0.688191, 0.587785 },
	{ 0.587785, -0.425325, 0.688191 },
	{ 0.688191, -0.587785, 0.425325 },
	{ -0.955423, 0.295242, 0.000000 },
	{ -0.951056, 0.162460, 0.262866 },
	{ -1.000000, 0.000000, 0.000000 },
	{ -0.850651, 0.000000, 0.525731 },
	{ -0.955423, -0.295242, 0.000000 },
	{ -0.951056, -0.162460, 0.262866 },
	{ -0.864188, 0.442863, -0.238856 },
	{ -0.951056, 0.162460, -0.262866 },
	{ -0.809017, 0.309017, -0.500000 },
	{ -0.864188, -0.442863, -0.238856 },
	{ -0.951056, -0.162460, -0.262866 },
	{ -0.809017, -0.309017, -0.500000 },
	{ -0.681718, 0.147621, -0.716567 },
	{ -0.681718, -0.147621, -0.716567 },
	{ -0.850651, 0.000000, -0.525731 },
	{ -0.688191, 0.587785, -0.425325 },
	{ -0.587785, 0.425325, -0.688191 },
	{ -0.425325, 0.688191, -0.587785 },
	{ -0.425325, -0.688191, -0.587785 },
	{ -0.587785, -0.425325, -0.688191 },
	{ -0.688191, -0.587785, -0.425325 } };

//==============================================================

int	   Q_rand( int* seed )
{
	*seed = ( 69069 * *seed + 1 );
	return *seed;
}

float Q_random( int* seed )
{
	return ( Q_rand( seed ) & 0xffff ) / ( float )0x10000;
}

float Q_crandom( int* seed )
{
	return 2.0 * ( Q_random( seed ) - 0.5 );
}

signed char ClampChar( int i )
{
	if( i < -128 ) {
		return -128;
	}

	if( i > 127 ) {
		return 127;
	}

	return i;
}

signed short ClampShort( int i )
{
	if( i < -32768 ) {
		return -32768;
	}

	if( i > 0x7fff ) {
		return 0x7fff;
	}

	return i;
}

int DirToByte( vec3_t dir )
{
	int	  i, best;
	float d, bestd;

	if( !dir ) {
		return 0;
	}

	bestd = 0;
	best  = 0;

	for( i = 0; i < NUMVERTEXNORMALS; i++ ) {
		d = DotProduct( dir, bytedirs[i] );

		if( d > bestd ) {
			bestd = d;
			best  = i;
		}
	}

	return best;
}

void ByteToDir( int b, vec3_t dir )
{
	if( b < 0 || b >= NUMVERTEXNORMALS ) {
		VectorCopy( vec3_origin, dir );
		return;
	}

	VectorCopy( bytedirs[b], dir );
}

unsigned ColorBytes3( float r, float g, float b )
{
	unsigned i;

	( ( byte* )&i )[0] = r * 255;
	( ( byte* )&i )[1] = g * 255;
	( ( byte* )&i )[2] = b * 255;

	return i;
}

unsigned ColorBytes4( float r, float g, float b, float a )
{
	unsigned i;

	( ( byte* )&i )[0] = r * 255;
	( ( byte* )&i )[1] = g * 255;
	( ( byte* )&i )[2] = b * 255;
	( ( byte* )&i )[3] = a * 255;

	return i;
}

float NormalizeColor( const vec3_t in, vec3_t out )
{
	float max;

	max = in[0];

	if( in[1] > max ) {
		max = in[1];
	}

	if( in[2] > max ) {
		max = in[2];
	}

	if( !max ) {
		VectorClear( out );

	} else {
		out[0] = in[0] / max;
		out[1] = in[1] / max;
		out[2] = in[2] / max;
	}

	return max;
}

/*
=====================
PlaneFromPoints

Returns false if the triangle is degenrate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
qboolean PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c )
{
	vec3_t d1, d2;

	VectorSubtract( b, a, d1 );
	VectorSubtract( c, a, d2 );
	CrossProduct( d2, d1, plane );

	if( VectorNormalize( plane ) == 0 ) {
		return qfalse;
	}

	plane[3] = DotProduct( a, plane );
	return qtrue;
}

/*
===============
RotatePointAroundVector

This is not implemented very well...
===============
*/
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees )
{
	float  m[3][3];
	float  im[3][3];
	float  zrot[3][3];
	float  tmpmat[3][3];
	float  rot[3][3];
	int	   i;
	vec3_t vr, vup, vf;
	float  rad;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector( vr, dir );
	CrossProduct( vr, vf, vup );

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy( im, m, sizeof( im ) );

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset( zrot, 0, sizeof( zrot ) );
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	rad		   = DEG2RAD( degrees );
	zrot[0][0] = cos( rad );
	zrot[0][1] = sin( rad );
	zrot[1][0] = -sin( rad );
	zrot[1][1] = cos( rad );

	MatrixMultiply( m, zrot, tmpmat );
	MatrixMultiply( tmpmat, im, rot );

	for( i = 0; i < 3; i++ ) {
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}

void RotateAroundDirection( vec3_t axis[3], float yaw )
{
	// create an arbitrary axis[1]
	PerpendicularVector( axis[1], axis[0] );

	// rotate it around axis[0] by yaw
	if( yaw ) {
		vec3_t temp;

		VectorCopy( axis[1], temp );
		RotatePointAroundVector( axis[1], axis[0], temp, yaw );
	}

	// cross to get axis[2]
	CrossProduct( axis[0], axis[1], axis[2] );
}

void vectoangles( const vec3_t value1, vec3_t angles )
{
	float forward;
	float yaw, pitch;

	if( value1[1] == 0 && value1[0] == 0 ) {
		yaw = 0;

		if( value1[2] > 0 ) {
			pitch = 90;

		} else {
			pitch = 270;
		}

	} else {
		if( value1[0] ) {
			yaw = ( atan2( value1[1], value1[0] ) * 180 / M_PI );

		} else if( value1[1] > 0 ) {
			yaw = 90;

		} else {
			yaw = 270;
		}

		if( yaw < 0 ) {
			yaw += 360;
		}

		forward = sqrt( value1[0] * value1[0] + value1[1] * value1[1] );
		pitch	= ( atan2( value1[2], forward ) * 180 / M_PI );

		if( pitch < 0 ) {
			pitch += 360;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW]	  = yaw;
	angles[ROLL]  = 0;
}

void AnglesToAxis( const vec3_t angles, vec3_t axis[3] )
{
	vec3_t right;

	// angle vectors returns "right" instead of "y axis"
	AngleVectors( angles, axis[0], right, axis[2] );
	VectorSubtract( vec3_origin, right, axis[1] );
}

void AxisClear( vec3_t axis[3] )
{
	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
}

void AxisCopy( vec3_t in[3], vec3_t out[3] )
{
	VectorCopy( in[0], out[0] );
	VectorCopy( in[1], out[1] );
	VectorCopy( in[2], out[2] );
}

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
	float  d;
	vec3_t n;
	float  inv_denom;

	inv_denom = 1.0F / DotProduct( normal, normal );

	d = DotProduct( normal, p ) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up )
{
	float d;

	// this rotate and negate guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct( right, forward );
	VectorMA( right, -d, forward, right );
	VectorNormalize( right );
	CrossProduct( right, forward, up );
}

void VectorRotate( vec3_t in, vec3_t matrix[3], vec3_t out )
{
	out[0] = DotProduct( in, matrix[0] );
	out[1] = DotProduct( in, matrix[1] );
	out[2] = DotProduct( in, matrix[2] );
}

/*!
	\brief Computes an approximation of the reciprocal square root of a floating-point number using bit-level manipulation and Newton-Raphson iteration.

	This function implements the famous 'fast inverse square root' algorithm, originally used in the Quake III Arena engine. It uses a clever bit-level hack to compute an initial approximation,
   followed by one iteration of Newton-Raphson method for improved accuracy. The function is commonly used in graphics programming for normalizing vectors quickly.

	\param f The input floating-point value to compute the reciprocal square root of
	\return Approximation of 1/sqrt(number) as a floating-point value
*/
float Q_rsqrt( float number )
{
	long		i;
	float		x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y  = number;
	i  = *( long* )&y;			  // evil floating point bit level hacking
	i  = 0x5f3759df - ( i >> 1 ); // what the fuck?
	y  = *( float* )&i;
	y  = y * ( threehalfs - ( x2 * y * y ) ); // 1st iteration
	//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}

/*!
	\brief Computes the absolute value of a floating-point number without using the standard library abs function.

	This function calculates the absolute value of a floating-point number by directly manipulating the binary representation of the float. It works by masking out the sign bit of the IEEE 754
   floating-point representation, effectively setting it to zero. This approach avoids potential issues with standard library implementations and can be more efficient in certain contexts. The
   function is commonly used in graphics and game engine code where performance and deterministic behavior are important.

	\param f The floating-point number to compute the absolute value of
	\return The absolute value of the input floating-point number
*/
float Q_fabs( float f )
{
	int tmp = *( int* )&f;
	tmp &= 0x7FFFFFFF;
	return *( float* )&tmp;
}

float LerpAngle( float from, float to, float frac )
{
	float a;

	if( to - from > 180 ) {
		to -= 360;
	}

	if( to - from < -180 ) {
		to += 360;
	}

	a = from + frac * ( to - from );

	return a;
}

float AngleSubtract( float a1, float a2 )
{
	float a;

	a = a1 - a2;

	while( a > 180 ) {
		a -= 360;
	}

	while( a < -180 ) {
		a += 360;
	}

	return a;
}

void AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 )
{
	v3[0] = AngleSubtract( v1[0], v2[0] );
	v3[1] = AngleSubtract( v1[1], v2[1] );
	v3[2] = AngleSubtract( v1[2], v2[2] );
}

float AngleMod( float a )
{
	a = ( 360.0 / 65536 ) * ( ( int )( a * ( 65536 / 360.0 ) ) & 65535 );
	return a;
}

float AngleNormalize360( float angle )
{
	return ( 360.0 / 65536 ) * ( ( int )( angle * ( 65536 / 360.0 ) ) & 65535 );
}

float AngleNormalize180( float angle )
{
	angle = AngleNormalize360( angle );

	if( angle > 180.0 ) {
		angle -= 360.0;
	}

	return angle;
}

float AngleDelta( float angle1, float angle2 )
{
	return AngleNormalize180( angle1 - angle2 );
}

/*!
	\brief Sets the sign bits for a plane based on the signs of its normal vector components.

	This function computes the sign bits for a plane by examining the signs of the three components of the plane's normal vector. Each bit in the resulting signbits field corresponds to one component
   of the normal vector, where a bit is set to 1 if the corresponding normal component is negative, and 0 otherwise. This is used for fast box on planeside testing.

	\param out Pointer to the plane structure whose signbits field will be set
*/
void SetPlaneSignbits( cplane_t* out )
{
	int bits, j;

	// for fast box on planeside test
	bits = 0;

	for( j = 0; j < 3; j++ ) {
		if( out->normal[j] < 0 ) {
			bits |= 1 << j;
		}
	}

	out->signbits = bits;
}

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2

int BoxOnPlaneSide2 (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	int		i;
	float	dist1, dist2;
	int		sides;
	vec3_t	corners[2];

	for (i=0 ; i<3 ; i++)
	{
		if (p->normal[i] < 0)
		{
			corners[0][i] = emins[i];
			corners[1][i] = emaxs[i];
		}
		else
		{
			corners[1][i] = emins[i];
			corners[0][i] = emaxs[i];
		}
	}
	dist1 = DotProduct (p->normal, corners[0]) - p->dist;
	dist2 = DotProduct (p->normal, corners[1]) - p->dist;
	sides = 0;
	if (dist1 >= 0)
		sides = 1;
	if (dist2 < 0)
		sides |= 2;

	return sides;
}

==================
*/
#if !( defined __linux__ && defined __i386__ && !defined C_ONLY )
	#if defined __LCC__ || defined C_ONLY || !id386

/*!
	\brief Determines which side of a plane a box is on, returning a bitmask indicating the result.

	This function checks whether a box defined by its minimum and maximum coordinates is on one side, both sides, or straddling a given plane. For axial planes (those aligned with the x, y, or z
   axis), it performs a fast test by comparing the plane's distance to the box coordinates. For non-axial planes, it computes the distances from the box to the plane using the box's extreme
   coordinates based on the plane's signbits to determine which corners to use. The return value is a bitmask where 1 indicates the box is on the positive side of the plane, and 2 indicates the box is
   on the negative side.

	\param emins The minimum coordinates of the box
	\param emaxs The maximum coordinates of the box
	\param plane Pointer to the plane to test against
	\return An integer bitmask where 1 indicates the box is on the positive side of the plane, 2 indicates the box is on the negative side, and 3 indicates the box straddles the plane
*/
int BoxOnPlaneSide( vec3_t emins, vec3_t emaxs, struct cplane_s* p )
{
	float dist1, dist2;
	int	  sides;

	// fast axial cases
	if( p->type < 3 ) {
		if( p->dist <= emins[p->type] ) {
			return 1;
		}

		if( p->dist >= emaxs[p->type] ) {
			return 2;
		}

		return 3;
	}

	// general case
	switch( p->signbits ) {
		case 0:
			dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
			dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
			break;

		case 1:
			dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
			dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
			break;

		case 2:
			dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
			dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
			break;

		case 3:
			dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
			dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
			break;

		case 4:
			dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
			dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
			break;

		case 5:
			dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
			dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
			break;

		case 6:
			dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
			dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
			break;

		case 7:
			dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
			dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
			break;

		default:
			dist1 = dist2 = 0; // shut up compiler
			break;
	}

	sides = 0;

	if( dist1 >= p->dist ) {
		sides = 1;
	}

	if( dist2 < p->dist ) {
		sides |= 2;
	}

	return sides;
}

	#else
		#pragma warning( disable : 4035 )

__declspec( naked ) int BoxOnPlaneSide( vec3_t emins, vec3_t emaxs, struct cplane_s* p )
{
	static int bops_initialized;
	static int Ljmptab[8];

	__asm {

		push ebx

		cmp bops_initialized, 1
		je initialized
		mov bops_initialized, 1

		mov Ljmptab[0 * 4], offset Lcase0
		mov Ljmptab[1 * 4], offset Lcase1
		mov Ljmptab[2 * 4], offset Lcase2
		mov Ljmptab[3 * 4], offset Lcase3
		mov Ljmptab[4 * 4], offset Lcase4
		mov Ljmptab[5 * 4], offset Lcase5
		mov Ljmptab[6 * 4], offset Lcase6
		mov Ljmptab[7 * 4], offset Lcase7

		initialized:

		mov edx, dword ptr[4 + 12 + esp]
		mov ecx, dword ptr[4 + 4 + esp]
		xor eax, eax
		mov ebx, dword ptr[4 + 8 + esp]
		mov al, byte ptr[17 + edx]
		cmp al, 8
		jge Lerror
		fld dword ptr[0 + edx]
		fld st( 0 )
		jmp dword ptr[Ljmptab + eax * 4]
		Lcase0 :
		fmul dword ptr[ebx]
		fld dword ptr[0 + 4 + edx]
		fxch st( 2 )
		fmul dword ptr[ecx]
		fxch st( 2 )
		fld st( 0 )
		fmul dword ptr[4 + ebx]
		fld dword ptr[0 + 8 + edx]
		fxch st( 2 )
		fmul dword ptr[4 + ecx]
		fxch st( 2 )
		fld st( 0 )
		fmul dword ptr[8 + ebx]
		fxch st( 5 )
		faddp st( 3 ), st( 0 )
		fmul dword ptr[8 + ecx]
		fxch st( 1 )
		faddp st( 3 ), st( 0 )
		fxch st( 3 )
		faddp st( 2 ), st( 0 )
		jmp LSetSides
		Lcase1 :
		fmul dword ptr[ecx]
		fld dword ptr[0 + 4 + edx]
		fxch st( 2 )
		fmul dword ptr[ebx]
		fxch st( 2 )
		fld st( 0 )
		fmul dword ptr[4 + ebx]
		fld dword ptr[0 + 8 + edx]
		fxch st( 2 )
		fmul dword ptr[4 + ecx]
		fxch st( 2 )
		fld st( 0 )
		fmul dword ptr[8 + ebx]
		fxch st( 5 )
		faddp st( 3 ), st( 0 )
		fmul dword ptr[8 + ecx]
		fxch st( 1 )
		faddp st( 3 ), st( 0 )
		fxch st( 3 )
		faddp st( 2 ), st( 0 )
		jmp LSetSides
		Lcase2 :
		fmul dword ptr[ebx]
		fld dword ptr[0 + 4 + edx]
		fxch st( 2 )
		fmul dword ptr[ecx]
		fxch st( 2 )
		fld st( 0 )
		fmul dword ptr[4 + ecx]
		fld dword ptr[0 + 8 + edx]
		fxch st( 2 )
		fmul dword ptr[4 + ebx]
		fxch st( 2 )
		fld st( 0 )
		fmul dword ptr[8 + ebx]
		fxch st( 5 )
		faddp st( 3 ), st( 0 )
		fmul dword ptr[8 + ecx]
		fxch st( 1 )
		faddp st( 3 ), st( 0 )
		fxch st( 3 )
		faddp st( 2 ), st( 0 )
		jmp LSetSides
		Lcase3 :
		fmul dword ptr[ecx]
		fld dword ptr[0 + 4 + edx]
		fxch st( 2 )
		fmul dword ptr[ebx]
		fxch st( 2 )
		fld st( 0 )
		fmul dword ptr[4 + ecx]
		fld dword ptr[0 + 8 + edx]
		fxch st( 2 )
		fmul dword ptr[4 + ebx]
		fxch st( 2 )
		fld st( 0 )
		fmul dword ptr[8 + ebx]
		fxch st( 5 )
		faddp st( 3 ), st( 0 )
		fmul dword ptr[8 + ecx]
		fxch st( 1 )
		faddp st( 3 ), st( 0 )
		fxch st( 3 )
		faddp st( 2 ), st( 0 )
		jmp LSetSides
		Lcase4 :
		fmul dword ptr[ebx]
		fld dword ptr[0 + 4 + edx]
		fxch st( 2 )
		fmul dword ptr[ecx]
		fxch st( 2 )
		fld st( 0 )
		fmul dword ptr[4 + ebx]
		fld dword ptr[0 + 8 + edx]
		fxch st( 2 )
		fmul dword ptr[4 + ecx]
		fxch st( 2 )
		fld st( 0 )
		fmul dword ptr[8 + ecx]
		fxch st( 5 )
		faddp st( 3 ), st( 0 )
		fmul dword ptr[8 + ebx]
		fxch st( 1 )
		faddp st( 3 ), st( 0 )
		fxch st( 3 )
		faddp st( 2 ), st( 0 )
		jmp LSetSides
		Lcase5 :
		fmul dword ptr[ecx]
		fld dword ptr[0 + 4 + edx]
		fxch st( 2 )
		fmul dword ptr[ebx]
		fxch st( 2 )
		fld st( 0 )
		fmul dword ptr[4 + ebx]
		fld dword ptr[0 + 8 + edx]
		fxch st( 2 )
		fmul dword ptr[4 + ecx]
		fxch st( 2 )
		fld st( 0 )
		fmul dword ptr[8 + ecx]
		fxch st( 5 )
		faddp st( 3 ), st( 0 )
		fmul dword ptr[8 + ebx]
		fxch st( 1 )
		faddp st( 3 ), st( 0 )
		fxch st( 3 )
		faddp st( 2 ), st( 0 )
		jmp LSetSides
		Lcase6 :
		fmul dword ptr[ebx]
		fld dword ptr[0 + 4 + edx]
		fxch st( 2 )
		fmul dword ptr[ecx]
		fxch st( 2 )
		fld st( 0 )
		fmul dword ptr[4 + ecx]
		fld dword ptr[0 + 8 + edx]
		fxch st( 2 )
		fmul dword ptr[4 + ebx]
		fxch st( 2 )
		fld st( 0 )
		fmul dword ptr[8 + ecx]
		fxch st( 5 )
		faddp st( 3 ), st( 0 )
		fmul dword ptr[8 + ebx]
		fxch st( 1 )
		faddp st( 3 ), st( 0 )
		fxch st( 3 )
		faddp st( 2 ), st( 0 )
		jmp LSetSides
		Lcase7 :
		fmul dword ptr[ecx]
		fld dword ptr[0 + 4 + edx]
		fxch st( 2 )
		fmul dword ptr[ebx]
		fxch st( 2 )
		fld st( 0 )
		fmul dword ptr[4 + ecx]
		fld dword ptr[0 + 8 + edx]
		fxch st( 2 )
		fmul dword ptr[4 + ebx]
		fxch st( 2 )
		fld st( 0 )
		fmul dword ptr[8 + ecx]
		fxch st( 5 )
		faddp st( 3 ), st( 0 )
		fmul dword ptr[8 + ebx]
		fxch st( 1 )
		faddp st( 3 ), st( 0 )
		fxch st( 3 )
		faddp st( 2 ), st( 0 )
		LSetSides :
		faddp st( 2 ), st( 0 )
		fcomp dword ptr[12 + edx]
		xor ecx, ecx
		fnstsw ax
		fcomp dword ptr[12 + edx]
		and ah, 1
		xor ah, 1
		add cl, ah
		fnstsw ax
		and ah, 1
		add ah, ah
		add cl, ah
		pop ebx
		mov eax, ecx
		ret
		Lerror :
		int 3
	}
}

		#pragma warning( default : 4035 )

	#endif
#endif

float RadiusFromBounds( const vec3_t mins, const vec3_t maxs )
{
	int	   i;
	vec3_t corner;
	float  a, b;

	for( i = 0; i < 3; i++ ) {
		a		  = fabs( mins[i] );
		b		  = fabs( maxs[i] );
		corner[i] = a > b ? a : b;
	}

	return VectorLength( corner );
}

/*!
	\brief Initializes the minimum and maximum bounds vectors to extreme values.

	This function sets all components of the minimum bounds vector to 99999 and all components of the maximum bounds vector to -99999. This initialization is typically used to prepare bounding boxes
   for subsequent calculations where the actual bounds will be determined by iterating through a set of points or objects.

	\param mins The minimum bounds vector to be initialized
	\param maxs The maximum bounds vector to be initialized
*/
void ClearBounds( vec3_t mins, vec3_t maxs )
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

/*!
	\brief Updates the bounding box defined by mins and maxs to include the point v.

	This function takes a point v and expands the bounding box defined by mins and maxs to ensure that the point is within or on the boundary of the box. For each coordinate (x, y, z), if the point's
   coordinate is smaller than the corresponding minimum value, it updates the minimum. Similarly, if the point's coordinate is larger than the corresponding maximum value, it updates the maximum. This
   operation is commonly used in spatial algorithms and collision detection to maintain bounding volumes.

	\param v The point to be included in the bounding box
	\param mins The minimum coordinates of the bounding box, updated to include v
	\param maxs The maximum coordinates of the bounding box, updated to include v
*/
void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs )
{
	if( v[0] < mins[0] ) {
		mins[0] = v[0];
	}

	if( v[0] > maxs[0] ) {
		maxs[0] = v[0];
	}

	if( v[1] < mins[1] ) {
		mins[1] = v[1];
	}

	if( v[1] > maxs[1] ) {
		maxs[1] = v[1];
	}

	if( v[2] < mins[2] ) {
		mins[2] = v[2];
	}

	if( v[2] > maxs[2] ) {
		maxs[2] = v[2];
	}
}

int VectorCompare( const vec3_t v1, const vec3_t v2 )
{
	if( v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2] ) {
		return 0;
	}

	return 1;
}

vec_t VectorNormalize( vec3_t v )
{
	float length, ilength;

	length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = sqrt( length );

	if( length ) {
		ilength = 1 / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return length;
}

//
// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length
//
void VectorNormalizeFast( vec3_t v )
{
	float ilength;

	ilength = Q_rsqrt( DotProduct( v, v ) );

	v[0] *= ilength;
	v[1] *= ilength;
	v[2] *= ilength;
}

/*!
	\brief Normalizes a 3D vector and returns its original length.

	This function takes a 3D vector, normalizes it to unit length, and stores the result in the output vector. It also returns the original length of the input vector. If the input vector is zero
   length, the output vector is set to zero and the function returns zero. The function is commonly used in graphics and physics calculations where normalized vectors are required.

	\param v The input 3D vector to be normalized
	\param out The output 3D vector that will contain the normalized result
	\return The original length of the input vector before normalization
*/
vec_t VectorNormalize2( const vec3_t v, vec3_t out )
{
	float length, ilength;

	length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = sqrt( length );

	if( length ) {
		ilength = 1 / length;
		out[0]	= v[0] * ilength;
		out[1]	= v[1] * ilength;
		out[2]	= v[2] * ilength;

	} else {
		VectorClear( out );
	}

	return length;
}

void _VectorMA( const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc )
{
	vecc[0] = veca[0] + scale * vecb[0];
	vecc[1] = veca[1] + scale * vecb[1];
	vecc[2] = veca[2] + scale * vecb[2];
}

vec_t _DotProduct( const vec3_t v1, const vec3_t v2 )
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

void _VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out )
{
	out[0] = veca[0] - vecb[0];
	out[1] = veca[1] - vecb[1];
	out[2] = veca[2] - vecb[2];
}

void _VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out )
{
	out[0] = veca[0] + vecb[0];
	out[1] = veca[1] + vecb[1];
	out[2] = veca[2] + vecb[2];
}

void _VectorCopy( const vec3_t in, vec3_t out )
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

/*!
	\brief Scales each component of the input vector by the given scale factor and stores the result in the output vector.

	This function performs component-wise multiplication of a 3D vector by a scalar value. It takes an input vector, multiplies each of its three components (x, y, z) by the provided scale factor, and
   stores the resulting scaled vector in the output vector. The operation is performed in-place on the output vector without modifying the input vector.

	\param in The input 3D vector to be scaled
	\param scale The scalar value by which to scale each component of the input vector
	\param out The output 3D vector that will contain the scaled result
*/
void _VectorScale( const vec3_t in, vec_t scale, vec3_t out )
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}

/*!
	\brief Computes the cross product of two 3D vectors and stores the result in a third vector.

	This function calculates the cross product of two 3D vectors v1 and v2, storing the resulting vector in the cross parameter. The cross product is calculated using the standard formula where each
   component of the result vector is computed as the determinant of a 2x2 matrix formed by the other two components of the input vectors.

	\param v1 The first 3D vector input
	\param v2 The second 3D vector input
	\param cross The output 3D vector that will contain the cross product of v1 and v2
*/
void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross )
{
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

vec_t VectorLength( const vec3_t v )
{
	return sqrt( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] );
}

vec_t VectorLengthSquared( const vec3_t v )
{
	return ( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] );
}

vec_t Distance( const vec3_t p1, const vec3_t p2 )
{
	vec3_t v;

	VectorSubtract( p2, p1, v );
	return VectorLength( v );
}

vec_t DistanceSquared( const vec3_t p1, const vec3_t p2 )
{
	vec3_t v;

	VectorSubtract( p2, p1, v );
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

/*!
	\brief Negates each component of the given 3D vector.

	This function takes a 3D vector and negates each of its three components, effectively reflecting the vector through the origin. The operation is performed in-place on the input vector.

	\param v The 3D vector whose components are to be negated
*/
void VectorInverse( vec3_t v )
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void Vector4Scale( const vec4_t in, vec_t scale, vec4_t out )
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
	out[3] = in[3] * scale;
}

int Q_log2( int val )
{
	int answer;

	answer = 0;

	while( ( val >>= 1 ) != 0 ) {
		answer++;
	}

	return answer;
}

/*
=================
PlaneTypeForNormal
=================
*/
/*
int	PlaneTypeForNormal (vec3_t normal) {
	if ( normal[0] == 1.0 )
		return PLANE_X;
	if ( normal[1] == 1.0 )
		return PLANE_Y;
	if ( normal[2] == 1.0 )
		return PLANE_Z;

	return PLANE_NON_AXIAL;
}
*/

/*
================
MatrixMultiply
================
*/
void MatrixMultiply( float in1[3][3], float in2[3][3], float out[3][3] )
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
}

/*!
	\brief Converts angular coordinates into three orthogonal unit vectors representing forward, right, and up directions.

	This function takes a set of Euler angles (pitch, yaw, roll) and computes the corresponding forward, right, and up vectors. The angles are expected to be in degrees and are converted internally to
   radians. Each of the output vectors is normalized and mutually perpendicular. If any of the output vectors is null, that component will not be computed to avoid unnecessary calculations.

	\param angles An array of three angular values representing pitch, yaw, and roll in degrees
	\param forward Output vector representing the forward direction based on the given angles
	\param right Output vector representing the right direction based on the given angles
	\param up Output vector representing the up direction based on the given angles
*/
void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up )
{
	float		 angle;
	static float sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

	angle = angles[YAW] * ( M_PI * 2 / 360 );
	sy	  = sin( angle );
	cy	  = cos( angle );
	angle = angles[PITCH] * ( M_PI * 2 / 360 );
	sp	  = sin( angle );
	cp	  = cos( angle );
	angle = angles[ROLL] * ( M_PI * 2 / 360 );
	sr	  = sin( angle );
	cr	  = cos( angle );

	if( forward ) {
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}

	if( right ) {
		right[0] = ( -1 * sr * sp * cy + -1 * cr * -sy );
		right[1] = ( -1 * sr * sp * sy + -1 * cr * cy );
		right[2] = -1 * sr * cp;
	}

	if( up ) {
		up[0] = ( cr * sp * cy + -sr * -sy );
		up[1] = ( cr * sp * sy + -sr * cy );
		up[2] = cr * cp;
	}
}

void PerpendicularVector( vec3_t dst, const vec3_t src )
{
	int	   pos;
	int	   i;
	float  minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for( pos = 0, i = 0; i < 3; i++ ) {
		if( fabs( src[i] ) < minelem ) {
			pos		= i;
			minelem = fabs( src[i] );
		}
	}

	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos]						 = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane( dst, tempvec, src );

	/*
	** normalize the result
	*/
	VectorNormalize( dst );
}

void GetPerpendicularViewVector( const vec3_t point, const vec3_t p1, const vec3_t p2, vec3_t up )
{
	vec3_t v1, v2;

	VectorSubtract( point, p1, v1 );
	VectorNormalize( v1 );

	VectorSubtract( point, p2, v2 );
	VectorNormalize( v2 );

	CrossProduct( v1, v2, up );
	VectorNormalize( up );
}

void ProjectPointOntoVector( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj )
{
	vec3_t pVec, vec;

	VectorSubtract( point, vStart, pVec );
	VectorSubtract( vEnd, vStart, vec );
	VectorNormalize( vec );
	// project onto the directional vector for this segment
	VectorMA( vStart, DotProduct( pVec, vec ), vec, vProj );
}

float vectoyaw( const vec3_t vec )
{
	float yaw;

	if( vec[YAW] == 0 && vec[PITCH] == 0 ) {
		yaw = 0;

	} else {
		if( vec[PITCH] ) {
			yaw = ( atan2( vec[YAW], vec[PITCH] ) * 180 / M_PI );

		} else if( vec[YAW] > 0 ) {
			yaw = 90;

		} else {
			yaw = 270;
		}

		if( yaw < 0 ) {
			yaw += 360;
		}
	}

	return yaw;
}

void AxisToAngles( vec3_t axis[3], vec3_t angles )
{
	vec3_t right, roll_angles, tvec;

	// first get the pitch and yaw from the forward vector
	vectoangles( axis[0], angles );

	// now get the roll from the right vector
	VectorCopy( axis[1], right );
	// get the angle difference between the tmpAxis[2] and axis[2] after they have been reverse-rotated
	RotatePointAroundVector( tvec, axisDefault[2], right, -angles[YAW] );
	RotatePointAroundVector( right, axisDefault[1], tvec, -angles[PITCH] );
	// now find the angles, the PITCH is effectively our ROLL
	vectoangles( right, roll_angles );
	roll_angles[PITCH] = AngleNormalize180( roll_angles[PITCH] );

	// if the yaw is more than 90 degrees difference, we should adjust the pitch
	if( DotProduct( right, axisDefault[1] ) < 0 ) {
		if( roll_angles[PITCH] < 0 ) {
			roll_angles[PITCH] = -90 + ( -90 - roll_angles[PITCH] );

		} else {
			roll_angles[PITCH] = 90 + ( 90 - roll_angles[PITCH] );
		}
	}

	angles[ROLL] = -roll_angles[PITCH];
}

float VectorDistance( vec3_t v1, vec3_t v2 )
{
	vec3_t dir;

	VectorSubtract( v2, v1, dir );
	return VectorLength( dir );
}

// done.
