// snd_main.cpp
//

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "snd_local.h"

#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <vector>
#include <string>
#include <algorithm>

#define AL_ALEXT_PROTOTYPES
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#ifndef ARRAY_LEN
	#define ARRAY_LEN( x ) ( sizeof( x ) / sizeof( ( x )[0] ) )
#endif

#ifndef S_COLOR_RED
	#define S_COLOR_RED ""
#endif

#ifndef S_COLOR_YELLOW
	#define S_COLOR_YELLOW ""
#endif

#ifndef S_COLOR_GREEN
	#define S_COLOR_GREEN ""
#endif

// ------------------------------------------------------------
// Tunables
// ------------------------------------------------------------

#define OAL_MAX_SFX				  4096
#define OAL_MAX_SOURCES			  64
#define OAL_MAX_ENTITIES		  1024
#define OAL_MAX_LOOP_SOUNDS		  256
#define OAL_STREAM_BUFFERS		  8
#define OAL_STREAM_BUFFER_SAMPLES 8192
#define OAL_DEFAULT_GAIN		  1.0f
#define OAL_METERS_PER_UNIT		  0.0254f // optional scale
#define OAL_LOCAL_SOUND_DISTANCE  0.0001f

// ------------------------------------------------------------
// RTCW fallback declarations if not already exposed
// ------------------------------------------------------------

cvar_t*			   s_volume;
cvar_t*			   s_musicvolume;
cvar_t*			   s_doppler;
cvar_t*			   s_mixahead;
cvar_t*			   s_khz;

// New EAX/EFX style controls
cvar_t*			   s_useEAX;
cvar_t*			   s_eaxReverbGain;
cvar_t*			   s_eaxDecayTime;
cvar_t*			   s_eaxRoomRolloff;
cvar_t*			   s_eaxReflectionsGain;
cvar_t*			   s_useBassEnhance;
cvar_t*			   s_bassLowpassGain;
cvar_t*			   s_bassLowpassGainHF;
cvar_t*			   s_pitchJitter2D;
cvar_t*			   s_pitchJitter3D;

// ------------------------------------------------------------
// OpenAL helpers
// ------------------------------------------------------------

static ALCdevice*  g_oalDevice	   = NULL;
static ALCcontext* g_oalContext	   = NULL;
static qboolean	   g_oalInited	   = qfalse;
static qboolean	   g_soundDisabled = qfalse;

// old raw sample mixer timing state
int				   s_soundtime = 0;

static qboolean	   g_oalHasFilters = qfalse;
static ALuint	   g_oalBassFilter = 0;

// old code expects a pointer, not an array symbol
int				   g_s_rawend[MAX_STREAMING_SOUNDS] = { 0 };
int				   s_rawend[MAX_STREAMING_SOUNDS]	= { 0 };

// EFX / EAX reverb support
static qboolean	   g_oalHasEFX		  = qfalse;
static qboolean	   g_oalHasEAXReverb  = qfalse;
static qboolean	   g_oalEffectActive  = qfalse;
static ALuint	   g_oalAuxEffectSlot = 0;
static ALuint	   g_oalEffect		  = 0;
static int		   g_oalNumAuxSends	  = 0;

static void		   OAL_CheckError( const char* where )
{
	ALenum err = alGetError();
	if( err != AL_NO_ERROR ) {
		Com_Printf( S_COLOR_RED "OpenAL error at %s: 0x%x\n", where, err );
	}
}

static float OAL_RandFloat01()
{
	return ( float )rand() / ( float )RAND_MAX;
}

static float OAL_RandomPitchOffset( float amount )
{
	float r;

	if( amount <= 0.0f ) {
		return 1.0f;
	}

	r = ( OAL_RandFloat01() * 2.0f ) - 1.0f; // -1..1
	return 1.0f + ( r * amount );
}

static void OAL_ClearDirectFilter( ALuint source )
{
	if( !g_oalHasFilters || !source ) {
		return;
	}

	alSourcei( source, AL_DIRECT_FILTER, AL_FILTER_NULL );
	OAL_CheckError( "alSourcei clear direct filter" );
}

static void OAL_ApplyBassFilterToSource( ALuint source, qboolean enable )
{
	if( !source ) {
		return;
	}

	if( !g_oalHasFilters || !g_oalBassFilter || !enable || !s_useBassEnhance || !s_useBassEnhance->integer ) {
		OAL_ClearDirectFilter( source );
		return;
	}

	alFilterf( g_oalBassFilter, AL_LOWPASS_GAIN, s_bassLowpassGain ? s_bassLowpassGain->value : 1.0f );
	alFilterf( g_oalBassFilter, AL_LOWPASS_GAINHF, s_bassLowpassGainHF ? s_bassLowpassGainHF->value : 0.45f );
	OAL_CheckError( "alFilterf bass lowpass" );

	alSourcei( source, AL_DIRECT_FILTER, ( ALint )g_oalBassFilter );
	OAL_CheckError( "alSourcei apply direct filter" );
}

static qboolean OAL_IsInitialized()
{
	return ( g_oalInited && g_oalDevice && g_oalContext );
}

static ALuint OAL_GenBufferSafe()
{
	ALuint b = 0;
	alGenBuffers( 1, &b );
	OAL_CheckError( "alGenBuffers" );
	return b;
}

static ALuint OAL_GenSourceSafe()
{
	ALuint s = 0;
	alGenSources( 1, &s );
	OAL_CheckError( "alGenSources" );
	return s;
}

static void OAL_DeleteBufferSafe( ALuint b )
{
	if( b ) {
		alDeleteBuffers( 1, &b );
		OAL_CheckError( "alDeleteBuffers" );
	}
}

static void OAL_DeleteSourceSafe( ALuint s )
{
	if( s ) {
		alDeleteSources( 1, &s );
		OAL_CheckError( "alDeleteSources" );
	}
}

static inline void OAL_CopyVec3( const vec3_t in, vec3_t out )
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

static inline void OAL_GameToAL( const vec3_t in, ALfloat out[3] )
{
	// Q3/RTCW coordinates are already right-handed enough for basic OpenAL use.
	// We keep X/Y/Z as-is here. If front/back sounds flipped, negate one axis.
	out[0] = in[0] * OAL_METERS_PER_UNIT;
	out[1] = in[1] * OAL_METERS_PER_UNIT;
	out[2] = in[2] * OAL_METERS_PER_UNIT;
}

// ------------------------------------------------------------
// EFX / EAX helpers
// ------------------------------------------------------------

static void OAL_DestroyFilters()
{
	if( g_oalBassFilter ) {
		alDeleteFilters( 1, &g_oalBassFilter );
		OAL_CheckError( "alDeleteFilters" );
		g_oalBassFilter = 0;
	}

	g_oalHasFilters = qfalse;
}

static void OAL_InitFilters()
{
	ALenum err;

	g_oalHasFilters = qfalse;
	g_oalBassFilter = 0;

	if( !g_oalHasEFX ) {
		return;
	}

	alGenFilters( 1, &g_oalBassFilter );
	err = alGetError();
	if( err != AL_NO_ERROR || !g_oalBassFilter ) {
		Com_Printf( S_COLOR_YELLOW "OpenAL: failed to create direct filter\n" );
		g_oalBassFilter = 0;
		return;
	}

	alFilteri( g_oalBassFilter, AL_FILTER_TYPE, AL_FILTER_LOWPASS );
	OAL_CheckError( "alFilteri lowpass type" );

	alFilterf( g_oalBassFilter, AL_LOWPASS_GAIN, s_bassLowpassGain ? s_bassLowpassGain->value : 1.0f );
	alFilterf( g_oalBassFilter, AL_LOWPASS_GAINHF, s_bassLowpassGainHF ? s_bassLowpassGainHF->value : 0.45f );
	OAL_CheckError( "alFilterf lowpass init" );

	g_oalHasFilters = qtrue;
	Com_Printf( S_COLOR_GREEN "OpenAL: direct lowpass bass filter enabled\n" );
}

static void OAL_ClearAuxSend( ALuint source )
{
	if( !g_oalHasEFX || !source ) {
		return;
	}

	alSource3i( source, AL_AUXILIARY_SEND_FILTER, 0, 0, AL_FILTER_NULL );
	OAL_CheckError( "alSource3i clear aux" );
}

static void OAL_AttachWorldReverbToSource( ALuint source )
{
	if( !g_oalHasEFX || !g_oalEffectActive || !source ) {
		return;
	}

	if( g_oalNumAuxSends <= 0 ) {
		return;
	}

	alSource3i( source, AL_AUXILIARY_SEND_FILTER, ( ALint )g_oalAuxEffectSlot, 0, AL_FILTER_NULL );
	OAL_CheckError( "alSource3i attach aux" );
}

static void OAL_DestroyEffects()
{
	g_oalEffectActive = qfalse;

	if( g_oalAuxEffectSlot ) {
		alAuxiliaryEffectSloti( g_oalAuxEffectSlot, AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL );
		OAL_CheckError( "alAuxiliaryEffectSloti clear" );
		alDeleteAuxiliaryEffectSlots( 1, &g_oalAuxEffectSlot );
		OAL_CheckError( "alDeleteAuxiliaryEffectSlots" );
		g_oalAuxEffectSlot = 0;
	}

	if( g_oalEffect ) {
		alDeleteEffects( 1, &g_oalEffect );
		OAL_CheckError( "alDeleteEffects" );
		g_oalEffect = 0;
	}
}

static void OAL_ApplyCurrentReverbParameters()
{
	ALint effectType;

	if( !g_oalHasEFX || !g_oalEffect || !g_oalAuxEffectSlot ) {
		return;
	}

	if( !s_useEAX || !s_useEAX->integer ) {
		alAuxiliaryEffectSloti( g_oalAuxEffectSlot, AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL );
		OAL_CheckError( "disable effect slot" );
		g_oalEffectActive = qfalse;
		return;
	}

	effectType = g_oalHasEAXReverb ? AL_EFFECT_EAXREVERB : AL_EFFECT_REVERB;

	alEffecti( g_oalEffect, AL_EFFECT_TYPE, effectType );
	OAL_CheckError( "alEffecti type" );

	if( effectType == AL_EFFECT_EAXREVERB ) {
		alEffectf( g_oalEffect, AL_EAXREVERB_DENSITY, 1.0f );
		alEffectf( g_oalEffect, AL_EAXREVERB_DIFFUSION, 1.0f );
		alEffectf( g_oalEffect, AL_EAXREVERB_GAIN, s_eaxReverbGain ? s_eaxReverbGain->value : 0.32f );
		alEffectf( g_oalEffect, AL_EAXREVERB_GAINHF, 0.89f );
		alEffectf( g_oalEffect, AL_EAXREVERB_DECAY_TIME, s_eaxDecayTime ? s_eaxDecayTime->value : 1.45f );
		alEffectf( g_oalEffect, AL_EAXREVERB_DECAY_HFRATIO, 0.83f );
		alEffectf( g_oalEffect, AL_EAXREVERB_REFLECTIONS_GAIN, s_eaxReflectionsGain ? s_eaxReflectionsGain->value : 0.12f );
		alEffectf( g_oalEffect, AL_EAXREVERB_REFLECTIONS_DELAY, 0.007f );
		alEffectf( g_oalEffect, AL_EAXREVERB_LATE_REVERB_GAIN, 1.26f );
		alEffectf( g_oalEffect, AL_EAXREVERB_LATE_REVERB_DELAY, 0.011f );
		alEffectf( g_oalEffect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, 0.994f );
		alEffectf( g_oalEffect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, s_eaxRoomRolloff ? s_eaxRoomRolloff->value : 0.0f );
		alEffecti( g_oalEffect, AL_EAXREVERB_DECAY_HFLIMIT, AL_TRUE );
		OAL_CheckError( "alEffectf eaxreverb" );
	} else {
		alEffectf( g_oalEffect, AL_REVERB_DENSITY, 1.0f );
		alEffectf( g_oalEffect, AL_REVERB_DIFFUSION, 1.0f );
		alEffectf( g_oalEffect, AL_REVERB_GAIN, s_eaxReverbGain ? s_eaxReverbGain->value : 0.32f );
		alEffectf( g_oalEffect, AL_REVERB_GAINHF, 0.89f );
		alEffectf( g_oalEffect, AL_REVERB_DECAY_TIME, s_eaxDecayTime ? s_eaxDecayTime->value : 1.45f );
		alEffectf( g_oalEffect, AL_REVERB_DECAY_HFRATIO, 0.83f );
		alEffectf( g_oalEffect, AL_REVERB_REFLECTIONS_GAIN, s_eaxReflectionsGain ? s_eaxReflectionsGain->value : 0.12f );
		alEffectf( g_oalEffect, AL_REVERB_REFLECTIONS_DELAY, 0.007f );
		alEffectf( g_oalEffect, AL_REVERB_LATE_REVERB_GAIN, 1.26f );
		alEffectf( g_oalEffect, AL_REVERB_LATE_REVERB_DELAY, 0.011f );
		alEffectf( g_oalEffect, AL_REVERB_AIR_ABSORPTION_GAINHF, 0.994f );
		alEffectf( g_oalEffect, AL_REVERB_ROOM_ROLLOFF_FACTOR, s_eaxRoomRolloff ? s_eaxRoomRolloff->value : 0.0f );
		alEffecti( g_oalEffect, AL_REVERB_DECAY_HFLIMIT, AL_TRUE );
		OAL_CheckError( "alEffectf reverb" );
	}

	alAuxiliaryEffectSloti( g_oalAuxEffectSlot, AL_EFFECTSLOT_EFFECT, g_oalEffect );
	OAL_CheckError( "alAuxiliaryEffectSloti attach" );

	g_oalEffectActive = qtrue;
}

static void OAL_InitEffects()
{
	ALCboolean alcEfxPresent;
	ALenum	   err = AL_NO_ERROR;

	g_oalHasEFX		   = qfalse;
	g_oalHasEAXReverb  = qfalse;
	g_oalEffectActive  = qfalse;
	g_oalAuxEffectSlot = 0;
	g_oalEffect		   = 0;
	g_oalNumAuxSends   = 0;

	if( !g_oalDevice ) {
		return;
	}

	alcEfxPresent = alcIsExtensionPresent( g_oalDevice, "ALC_EXT_EFX" );
	if( !alcEfxPresent ) {
		Com_Printf( S_COLOR_YELLOW "OpenAL: ALC_EXT_EFX not present, no EAX/EFX reverb\n" );
		return;
	}

	g_oalHasEFX = qtrue;

	alGetIntegerv( ALC_MAX_AUXILIARY_SENDS, &g_oalNumAuxSends );
	OAL_CheckError( "alGetIntegerv ALC_MAX_AUXILIARY_SENDS" );

	if( g_oalNumAuxSends <= 0 ) {
		Com_Printf( S_COLOR_YELLOW "OpenAL: EFX present but device reports 0 auxiliary sends\n" );
		g_oalHasEFX = qfalse;
		return;
	}

	g_oalHasEAXReverb = alIsExtensionPresent( "AL_EXT_EAXREVERB" ) ? qtrue : qfalse;

	alGenAuxiliaryEffectSlots( 1, &g_oalAuxEffectSlot );
	err = alGetError();
	if( err != AL_NO_ERROR || !g_oalAuxEffectSlot ) {
		Com_Printf( S_COLOR_YELLOW "OpenAL: failed to create auxiliary effect slot\n" );
		g_oalHasEFX		   = qfalse;
		g_oalAuxEffectSlot = 0;
		return;
	}

	alGenEffects( 1, &g_oalEffect );
	err = alGetError();
	if( err != AL_NO_ERROR || !g_oalEffect ) {
		Com_Printf( S_COLOR_YELLOW "OpenAL: failed to create effect object\n" );
		OAL_DestroyEffects();
		g_oalHasEFX = qfalse;
		return;
	}

	OAL_ApplyCurrentReverbParameters();

	if( g_oalEffectActive ) {
		if( g_oalHasEAXReverb ) {
			Com_Printf( S_COLOR_GREEN "OpenAL: EFX enabled, using EAX reverb path\n" );
		} else {
			Com_Printf( S_COLOR_GREEN "OpenAL: EFX enabled, using standard reverb path\n" );
		}
	} else {
		Com_Printf( S_COLOR_YELLOW "OpenAL: EFX available but effect disabled by cvar\n" );
	}
}

// ------------------------------------------------------------
// WAV loader
// Only supports uncompressed PCM WAV
// ------------------------------------------------------------

static int OAL_ReadLE16( const byte* p )
{
	return ( int )( p[0] | ( p[1] << 8 ) );
}

static int OAL_ReadLE32( const byte* p )
{
	return ( int )( p[0] | ( p[1] << 8 ) | ( p[2] << 16 ) | ( p[3] << 24 ) );
}

static qboolean OAL_LoadWavFromMemory( const byte* data, int dataSize, wavinfo_t* out )
{
	const byte* p;
	const byte* end;
	int			fmtTag	 = 0;
	int			channels = 0;
	int			rate	 = 0;
	int			bits	 = 0;
	const byte* pcmData	 = NULL;
	int			pcmSize	 = 0;

	if( !data || dataSize < 44 || !out ) {
		return qfalse;
	}

	memset( out, 0, sizeof( *out ) );

	p	= data;
	end = data + dataSize;

	if( memcmp( p, "RIFF", 4 ) != 0 || memcmp( p + 8, "WAVE", 4 ) != 0 ) {
		return qfalse;
	}

	p += 12;

	while( p + 8 <= end ) {
		const char* chunkId	  = ( const char* )p;
		int			chunkSize = OAL_ReadLE32( p + 4 );
		p += 8;

		if( p + chunkSize > end ) {
			return qfalse;
		}

		if( memcmp( chunkId, "fmt ", 4 ) == 0 ) {
			if( chunkSize < 16 ) {
				return qfalse;
			}

			fmtTag	 = OAL_ReadLE16( p + 0 );
			channels = OAL_ReadLE16( p + 2 );
			rate	 = OAL_ReadLE32( p + 4 );
			bits	 = OAL_ReadLE16( p + 14 );
		} else if( memcmp( chunkId, "data", 4 ) == 0 ) {
			pcmData = p;
			pcmSize = chunkSize;
		}

		p += chunkSize;
		if( chunkSize & 1 ) {
			p++;
		}
	}

	if( fmtTag != 1 ) {
		Com_Printf( S_COLOR_YELLOW "OpenAL WAV loader: only PCM WAV supported\n" );
		return qfalse;
	}

	if( ( channels != 1 && channels != 2 ) || ( bits != 8 && bits != 16 ) || !pcmData || pcmSize <= 0 ) {
		return qfalse;
	}

	out->rate	  = rate;
	out->width	  = bits / 8;
	out->channels = channels;
	out->samples  = pcmSize / ( out->width * channels );
	out->data	  = ( byte* )Z_Malloc( pcmSize );

	memcpy( out->data, pcmData, pcmSize );
	return qtrue;
}

static void OAL_FreeWav( wavinfo_t* w )
{
	if( !w ) {
		return;
	}

	if( w->data ) {
		Z_Free( w->data );
	}

	memset( w, 0, sizeof( *w ) );
}

static ALenum OAL_GetFormat( int channels, int width )
{
	if( channels == 1 && width == 1 ) {
		return AL_FORMAT_MONO8;
	}
	if( channels == 1 && width == 2 ) {
		return AL_FORMAT_MONO16;
	}
	if( channels == 2 && width == 1 ) {
		return AL_FORMAT_STEREO8;
	}
	if( channels == 2 && width == 2 ) {
		return AL_FORMAT_STEREO16;
	}
	return 0;
}

// ------------------------------------------------------------
// SFX registry
// ------------------------------------------------------------

typedef struct {
	qboolean inUse;
	char	 name[MAX_QPATH];
	ALuint	 buffer;
	int		 rate;
	int		 width;
	int		 channels;
	int		 sizeBytes;
	qboolean placeholder;
} oalSfx_t;

static oalSfx_t	   g_sfx[OAL_MAX_SFX];
static int		   g_numSfx = 1; // 0 reserved as invalid

static sfxHandle_t OAL_FindSfx( const char* sample )
{
	int i;
	for( i = 1; i < g_numSfx; ++i ) {
		if( g_sfx[i].inUse && !Q_stricmp( g_sfx[i].name, sample ) ) {
			return ( sfxHandle_t )i;
		}
	}
	return 0;
}

static sfxHandle_t OAL_AllocSfx( const char* sample )
{
	int idx;
	if( g_numSfx >= OAL_MAX_SFX ) {
		Com_Printf( S_COLOR_RED "OpenAL: OAL_MAX_SFX reached\n" );
		return 0;
	}

	idx = g_numSfx++;
	memset( &g_sfx[idx], 0, sizeof( g_sfx[idx] ) );
	g_sfx[idx].inUse = qtrue;
	Q_strncpyz( g_sfx[idx].name, sample, sizeof( g_sfx[idx].name ) );
	return ( sfxHandle_t )idx;
}

static qboolean OAL_LoadSfxBuffer( const char* sample, ALuint* outBuffer, int* outRate, int* outWidth, int* outChannels, int* outSizeBytes )
{
	void*	  fileData = NULL;
	int		  fileLen;
	wavinfo_t w;
	ALenum	  fmt;
	ALuint	  buf;

	memset( &w, 0, sizeof( w ) );

	fileLen = FS_ReadFile( sample, &fileData );
	if( fileLen <= 0 || !fileData ) {
		fileLen = FS_ReadFile( va( "%s.wav", sample ), &fileData );
		if( fileLen <= 0 || !fileData ) {
			return qfalse;
		}
	}

	if( !OAL_LoadWavFromMemory( ( const byte* )fileData, fileLen, &w ) ) {
		FS_FreeFile( fileData );
		return qfalse;
	}

	fmt = OAL_GetFormat( w.channels, w.width );
	if( !fmt ) {
		OAL_FreeWav( &w );
		FS_FreeFile( fileData );
		return qfalse;
	}

	buf = OAL_GenBufferSafe();
	if( !buf ) {
		OAL_FreeWav( &w );
		FS_FreeFile( fileData );
		return qfalse;
	}

	alBufferData( buf, fmt, w.data, w.samples * w.channels * w.width, w.rate );
	OAL_CheckError( "alBufferData" );

	*outBuffer	  = buf;
	*outRate	  = w.rate;
	*outWidth	  = w.width;
	*outChannels  = w.channels;
	*outSizeBytes = w.samples * w.channels * w.width;

	OAL_FreeWav( &w );
	FS_FreeFile( fileData );
	return qtrue;
}

static sfxHandle_t OAL_RegisterPlaceholder( const char* sample )
{
	sfxHandle_t h = OAL_AllocSfx( sample );
	if( !h ) {
		return 0;
	}

	g_sfx[h].placeholder = qtrue;
	g_sfx[h].buffer		 = 0;
	return h;
}

// ------------------------------------------------------------
// Entity positions
// ------------------------------------------------------------

typedef struct {
	qboolean valid;
	vec3_t	 origin;
} oalEntity_t;

static oalEntity_t g_entities[OAL_MAX_ENTITIES];

// ------------------------------------------------------------
// Runtime source channels
// ------------------------------------------------------------

typedef struct {
	qboolean	inUse;
	qboolean	local;
	qboolean	looping;
	qboolean	isStream;
	int			entnum;
	int			entchannel;
	sfxHandle_t sfx;
	ALuint		source;
	int			startTime;
	int			loopEntityNum;
} oalChannel_t;

static oalChannel_t g_channels[OAL_MAX_SOURCES];

// ------------------------------------------------------------
// Looping sounds
// ------------------------------------------------------------

typedef struct {
	qboolean	active;
	qboolean	realLoop;
	int			entityNum;
	vec3_t		origin;
	vec3_t		velocity;
	int			range;
	sfxHandle_t sfx;
	int			volume;
} oalLoopSound_t;

static oalLoopSound_t g_loopSounds[OAL_MAX_LOOP_SOUNDS];

// ------------------------------------------------------------
// Listener state
// ------------------------------------------------------------

static vec3_t		  g_listenerOrigin;
static vec3_t		  g_listenerAxis[3];
static int			  g_listenerEntNum = -1;

// ------------------------------------------------------------
// Raw streaming
// ------------------------------------------------------------

typedef struct {
	qboolean active;
	ALuint	 source;
	ALuint	 buffers[OAL_STREAM_BUFFERS];
	int		 sampleRate;
	int		 width;
	int		 channels;
	int		 index;
} oalRawStream_t;

// 0 = music/background
// 1..n = extra streaming tracks
static oalRawStream_t g_rawStreams[4];

// ------------------------------------------------------------
// Background tracks / stream sounds
// For now this uses static WAV buffers rather than true decode-streaming.
// Good enough to get RTCW making sound immediately.
// ------------------------------------------------------------

typedef struct {
	qboolean active;
	qboolean looped;
	char	 intro[MAX_QPATH];
	char	 loop[MAX_QPATH];
	ALuint	 source;
	ALuint	 introBuffer;
	ALuint	 loopBuffer;
	qboolean introPlaying;
	float	 gain;
} oalBgTrack_t;

static oalBgTrack_t g_bgTrack;

typedef struct {
	qboolean active;
	int		 entnum;
	int		 channel;
	int		 attenuation;
	char	 intro[MAX_QPATH];
	char	 loop[MAX_QPATH];
	ALuint	 source;
	ALuint	 introBuffer;
	ALuint	 loopBuffer;
	qboolean introPlaying;
} oalEntStream_t;

static oalEntStream_t g_entStreams[4];

// ------------------------------------------------------------
// Utilities
// ------------------------------------------------------------

static int			  OAL_Milliseconds()
{
	return Sys_Milliseconds();
}

static void OAL_StopChannel( oalChannel_t* ch )
{
	if( !ch || !ch->inUse ) {
		return;
	}

	alSourceStop( ch->source );
	alSourcei( ch->source, AL_BUFFER, 0 );
	OAL_ClearAuxSend( ch->source );
	OAL_ClearDirectFilter( ch->source );

	ch->inUse		  = qfalse;
	ch->local		  = qfalse;
	ch->looping		  = qfalse;
	ch->isStream	  = qfalse;
	ch->entnum		  = -1;
	ch->entchannel	  = 0;
	ch->sfx			  = 0;
	ch->startTime	  = 0;
	ch->loopEntityNum = -1;
}

static oalChannel_t* OAL_AllocChannel()
{
	int i;
	int oldestIndex = -1;
	int oldestTime	= 0x7fffffff;

	for( i = 0; i < OAL_MAX_SOURCES; ++i ) {
		ALint state = AL_STOPPED;

		if( !g_channels[i].source ) {
			g_channels[i].source = OAL_GenSourceSafe();
		}

		if( !g_channels[i].source ) {
			continue;
		}

		if( !g_channels[i].inUse ) {
			return &g_channels[i];
		}

		alGetSourcei( g_channels[i].source, AL_SOURCE_STATE, &state );
		if( state == AL_STOPPED ) {
			OAL_StopChannel( &g_channels[i] );
			return &g_channels[i];
		}

		if( g_channels[i].startTime < oldestTime && !g_channels[i].looping ) {
			oldestTime	= g_channels[i].startTime;
			oldestIndex = i;
		}
	}

	if( oldestIndex >= 0 ) {
		OAL_StopChannel( &g_channels[oldestIndex] );
		return &g_channels[oldestIndex];
	}

	return NULL;
}

static void OAL_ConfigureSourceCommon( ALuint source, qboolean local, const vec3_t origin, int entnum )
{
	ALfloat pos[3] = { 0, 0, 0 };
	ALfloat vel[3] = { 0, 0, 0 };

	alSourcef( source, AL_GAIN, s_volume->value * 4 );
	alSourcef( source, AL_PITCH, 1.0f );
	alSourcef( source, AL_REFERENCE_DISTANCE, 120.0f * OAL_METERS_PER_UNIT );
	alSourcef( source, AL_MAX_DISTANCE, 8000.0f * OAL_METERS_PER_UNIT );
	alSourcef( source, AL_ROLLOFF_FACTOR, 0.8f );
	alSourcei( source, AL_LOOPING, AL_FALSE );

	if( local ) {
		pos[0] = 0.0f;
		pos[1] = 0.0f;
		pos[2] = -OAL_LOCAL_SOUND_DISTANCE;

		alSourcei( source, AL_SOURCE_RELATIVE, AL_TRUE );
		alSource3f( source, AL_POSITION, pos[0], pos[1], pos[2] );
		alSource3f( source, AL_VELOCITY, 0.0f, 0.0f, 0.0f );

		OAL_ClearAuxSend( source );
	} else {
		vec3_t tmp;

		if( origin ) {
			OAL_CopyVec3( origin, tmp );
		} else if( entnum >= 0 && entnum < OAL_MAX_ENTITIES && g_entities[entnum].valid ) {
			OAL_CopyVec3( g_entities[entnum].origin, tmp );
		} else {
			VectorClear( tmp );
		}

		OAL_GameToAL( tmp, pos );
		OAL_GameToAL( vel, vel );

		alSourcei( source, AL_SOURCE_RELATIVE, AL_FALSE );
		alSource3f( source, AL_POSITION, pos[0], pos[1], pos[2] );
		alSource3f( source, AL_VELOCITY, vel[0], vel[1], vel[2] );

		OAL_AttachWorldReverbToSource( source );
	}
}

static void OAL_StopEntityChannel( int entnum, int entchannel )
{
	int i;
	for( i = 0; i < OAL_MAX_SOURCES; ++i ) {
		if( g_channels[i].inUse && g_channels[i].entnum == entnum && g_channels[i].entchannel == entchannel ) {
			OAL_StopChannel( &g_channels[i] );
		}
	}
}

// ------------------------------------------------------------
// Background track helpers
// ------------------------------------------------------------

static void OAL_FreeBgTrackBuffers( oalBgTrack_t* bg )
{
	if( !bg ) {
		return;
	}

	if( bg->source ) {
		alSourceStop( bg->source );
		alSourcei( bg->source, AL_BUFFER, 0 );
		OAL_ClearAuxSend( bg->source );
	}

	OAL_DeleteBufferSafe( bg->introBuffer );
	OAL_DeleteBufferSafe( bg->loopBuffer );

	bg->introBuffer = 0;
	bg->loopBuffer	= 0;
}

static qboolean OAL_LoadStaticWavBuffer( const char* name, ALuint* outBuffer )
{
	ALuint b;
	int	   rate, width, channels, sizeBytes;

	if( !name || !name[0] ) {
		*outBuffer = 0;
		return qfalse;
	}

	b = 0;
	if( !OAL_LoadSfxBuffer( name, &b, &rate, &width, &channels, &sizeBytes ) ) {
		return qfalse;
	}

	*outBuffer = b;
	return qtrue;
}

static void OAL_UpdateBackgroundTrack()
{
	ALint state;

	if( !g_bgTrack.active || !g_bgTrack.source ) {
		return;
	}

	alGetSourcei( g_bgTrack.source, AL_SOURCE_STATE, &state );

	if( g_bgTrack.introPlaying ) {
		if( state != AL_PLAYING ) {
			g_bgTrack.introPlaying = qfalse;

			if( g_bgTrack.loopBuffer ) {
				alSourcei( g_bgTrack.source, AL_BUFFER, g_bgTrack.loopBuffer );
				alSourcei( g_bgTrack.source, AL_LOOPING, AL_TRUE );
				alSourcef( g_bgTrack.source, AL_GAIN, ( s_musicvolume ? s_musicvolume->value : 1.0f ) * g_bgTrack.gain );
				alSourcePlay( g_bgTrack.source );
			} else {
				g_bgTrack.active = qfalse;
			}
		}
	}
}

static void OAL_UpdateEntStreams()
{
	int i;

	for( i = 0; i < ( int )ARRAY_LEN( g_entStreams ); ++i ) {
		ALint			state;
		ALfloat			pos[3];

		oalEntStream_t* st = &g_entStreams[i];
		if( !st->active || !st->source ) {
			continue;
		}

		if( st->entnum >= 0 && st->entnum < OAL_MAX_ENTITIES && g_entities[st->entnum].valid ) {
			OAL_GameToAL( g_entities[st->entnum].origin, pos );
			alSourcei( st->source, AL_SOURCE_RELATIVE, AL_FALSE );
			alSource3f( st->source, AL_POSITION, pos[0], pos[1], pos[2] );
			OAL_AttachWorldReverbToSource( st->source );
		}

		alGetSourcei( st->source, AL_SOURCE_STATE, &state );

		if( st->introPlaying ) {
			if( state != AL_PLAYING ) {
				st->introPlaying = qfalse;

				if( st->loopBuffer ) {
					alSourcei( st->source, AL_BUFFER, st->loopBuffer );
					alSourcei( st->source, AL_LOOPING, AL_TRUE );
					OAL_AttachWorldReverbToSource( st->source );
					alSourcePlay( st->source );
				} else {
					st->active = qfalse;
				}
			}
		}
	}
}

// ------------------------------------------------------------
// Raw streaming helpers
// ------------------------------------------------------------

static void OAL_InitRawStream( oalRawStream_t* rs )
{
	if( !rs ) {
		return;
	}

	memset( rs, 0, sizeof( *rs ) );

	rs->source = OAL_GenSourceSafe();
	if( !rs->source ) {
		return;
	}

	alGenBuffers( OAL_STREAM_BUFFERS, rs->buffers );
	OAL_CheckError( "alGenBuffers raw stream" );

	alSourcei( rs->source, AL_SOURCE_RELATIVE, AL_TRUE );
	alSource3f( rs->source, AL_POSITION, 0.0f, 0.0f, -OAL_LOCAL_SOUND_DISTANCE );
	alSourcef( rs->source, AL_GAIN, 1.0f );
	OAL_ClearAuxSend( rs->source );
}

static void OAL_ShutdownRawStream( oalRawStream_t* rs )
{
	if( !rs ) {
		return;
	}

	if( rs->source ) {
		alSourceStop( rs->source );

		{
			ALint queued = 0;
			alGetSourcei( rs->source, AL_BUFFERS_QUEUED, &queued );
			while( queued-- > 0 ) {
				ALuint b;
				alSourceUnqueueBuffers( rs->source, 1, &b );
			}
		}

		if( rs->buffers[0] ) {
			alDeleteBuffers( OAL_STREAM_BUFFERS, rs->buffers );
		}

		OAL_DeleteSourceSafe( rs->source );
	}

	memset( rs, 0, sizeof( *rs ) );
}

static void OAL_ServiceRawStream( oalRawStream_t* rs )
{
	ALint processed = 0;

	if( !rs || !rs->source ) {
		return;
	}

	alGetSourcei( rs->source, AL_BUFFERS_PROCESSED, &processed );
	while( processed-- > 0 ) {
		ALuint b;
		alSourceUnqueueBuffers( rs->source, 1, &b );
	}

	{
		ALint state	 = AL_STOPPED;
		ALint queued = 0;

		alGetSourcei( rs->source, AL_SOURCE_STATE, &state );
		alGetSourcei( rs->source, AL_BUFFERS_QUEUED, &queued );

		if( queued > 0 && state != AL_PLAYING ) {
			alSourcePlay( rs->source );
		}
	}
}

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------

void S_Init()
{
	int			   i;
	const ALCchar* deviceName;

	if( g_oalInited ) {
		Com_Printf( "S_Init: already initialized\n" );
		return;
	}

	s_volume	  = Cvar_Get( "s_volume", "0.8", CVAR_ARCHIVE );
	s_musicvolume = Cvar_Get( "s_musicvolume", "0.25", CVAR_ARCHIVE );
	s_doppler	  = Cvar_Get( "s_doppler", "1", CVAR_ARCHIVE );
	s_khz		  = Cvar_Get( "s_khz", "22", CVAR_ARCHIVE );
	s_mixahead	  = Cvar_Get( "s_mixahead", "0.5", CVAR_ARCHIVE );

	s_useEAX			 = Cvar_Get( "s_useEAX", "1", CVAR_ARCHIVE );
	s_eaxReverbGain		 = Cvar_Get( "s_eaxReverbGain", "0.32", CVAR_ARCHIVE );
	s_eaxDecayTime		 = Cvar_Get( "s_eaxDecayTime", "1.45", CVAR_ARCHIVE );
	s_eaxRoomRolloff	 = Cvar_Get( "s_eaxRoomRolloff", "0.0", CVAR_ARCHIVE );
	s_eaxReflectionsGain = Cvar_Get( "s_eaxReflectionsGain", "0.12", CVAR_ARCHIVE );

	s_useBassEnhance	= Cvar_Get( "s_useBassEnhance", "1", CVAR_ARCHIVE );
	s_bassLowpassGain	= Cvar_Get( "s_bassLowpassGain", "1.0", CVAR_ARCHIVE );
	s_bassLowpassGainHF = Cvar_Get( "s_bassLowpassGainHF", "0.42", CVAR_ARCHIVE );
	s_pitchJitter2D		= Cvar_Get( "s_pitchJitter2D", "0.015", CVAR_ARCHIVE );
	s_pitchJitter3D		= Cvar_Get( "s_pitchJitter3D", "0.02", CVAR_ARCHIVE );

	memset( g_sfx, 0, sizeof( g_sfx ) );
	memset( g_entities, 0, sizeof( g_entities ) );
	memset( g_channels, 0, sizeof( g_channels ) );
	memset( g_loopSounds, 0, sizeof( g_loopSounds ) );
	memset( &g_bgTrack, 0, sizeof( g_bgTrack ) );
	memset( g_entStreams, 0, sizeof( g_entStreams ) );
	memset( g_rawStreams, 0, sizeof( g_rawStreams ) );

	g_oalDevice = alcOpenDevice( NULL );
	if( !g_oalDevice ) {
		Com_Printf( S_COLOR_RED "OpenAL: failed to open device\n" );
		g_soundDisabled = qtrue;
		return;
	}

	g_oalContext = alcCreateContext( g_oalDevice, NULL );
	if( !g_oalContext ) {
		Com_Printf( S_COLOR_RED "OpenAL: failed to create context\n" );
		alcCloseDevice( g_oalDevice );
		g_oalDevice		= NULL;
		g_soundDisabled = qtrue;
		return;
	}

	if( !alcMakeContextCurrent( g_oalContext ) ) {
		Com_Printf( S_COLOR_RED "OpenAL: failed to make context current\n" );
		alcDestroyContext( g_oalContext );
		alcCloseDevice( g_oalDevice );
		g_oalContext	= NULL;
		g_oalDevice		= NULL;
		g_soundDisabled = qtrue;
		return;
	}

	deviceName = alcGetString( g_oalDevice, ALC_DEVICE_SPECIFIER );
	Com_Printf( S_COLOR_GREEN "OpenAL initialized: %s\n", deviceName ? deviceName : "unknown device" );

	alDistanceModel( AL_INVERSE_DISTANCE_CLAMPED );
	alDopplerFactor( s_doppler ? s_doppler->value : 1.0f );

	OAL_InitEffects();
	OAL_InitFilters();

	for( i = 0; i < OAL_MAX_SOURCES; ++i ) {
		memset( &g_channels[i], 0, sizeof( g_channels[i] ) );
		g_channels[i].source		= OAL_GenSourceSafe();
		g_channels[i].entnum		= -1;
		g_channels[i].loopEntityNum = -1;
	}

	for( i = 0; i < ( int )ARRAY_LEN( g_rawStreams ); ++i ) {
		OAL_InitRawStream( &g_rawStreams[i] );
	}

	VectorClear( g_listenerOrigin );
	VectorClear( g_listenerAxis[0] );
	VectorClear( g_listenerAxis[1] );
	VectorClear( g_listenerAxis[2] );
	g_listenerAxis[0][0] = 1.0f;
	g_listenerAxis[1][1] = 1.0f;
	g_listenerAxis[2][2] = 1.0f;

	g_oalInited		= qtrue;
	g_soundDisabled = qfalse;
}

void S_Shutdown()
{
	int i;

	if( !g_oalInited ) {
		return;
	}

	S_StopAllSounds();

	OAL_DestroyFilters();

	OAL_FreeBgTrackBuffers( &g_bgTrack );
	if( g_bgTrack.source ) {
		OAL_DeleteSourceSafe( g_bgTrack.source );
		g_bgTrack.source = 0;
	}

	for( i = 0; i < ( int )ARRAY_LEN( g_entStreams ); ++i ) {
		if( g_entStreams[i].source ) {
			alSourceStop( g_entStreams[i].source );
			alSourcei( g_entStreams[i].source, AL_BUFFER, 0 );
			OAL_ClearAuxSend( g_entStreams[i].source );
			OAL_DeleteBufferSafe( g_entStreams[i].introBuffer );
			OAL_DeleteBufferSafe( g_entStreams[i].loopBuffer );
			OAL_DeleteSourceSafe( g_entStreams[i].source );
		}
		memset( &g_entStreams[i], 0, sizeof( g_entStreams[i] ) );
	}

	for( i = 0; i < ( int )ARRAY_LEN( g_rawStreams ); ++i ) {
		OAL_ShutdownRawStream( &g_rawStreams[i] );
	}

	for( i = 0; i < OAL_MAX_SOURCES; ++i ) {
		if( g_channels[i].source ) {
			OAL_StopChannel( &g_channels[i] );
			OAL_DeleteSourceSafe( g_channels[i].source );
			g_channels[i].source = 0;
		}
	}

	for( i = 1; i < g_numSfx; ++i ) {
		if( g_sfx[i].buffer ) {
			OAL_DeleteBufferSafe( g_sfx[i].buffer );
		}
		memset( &g_sfx[i], 0, sizeof( g_sfx[i] ) );
	}

	g_numSfx = 1;

	OAL_DestroyEffects();

	alcMakeContextCurrent( NULL );

	if( g_oalContext ) {
		alcDestroyContext( g_oalContext );
		g_oalContext = NULL;
	}

	if( g_oalDevice ) {
		alcCloseDevice( g_oalDevice );
		g_oalDevice = NULL;
	}

	g_oalInited = qfalse;
}

void S_UpdateThread()
{
	// If you later move mixing/streaming to its own thread, service it here.
	S_Update();
}

void S_StartSound( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx )
{
	S_StartSoundEx( origin, entnum, entchannel, sfx, 0 );
}

void S_StartSoundEx( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx, int flags )
{
	oalChannel_t* ch;
	oalSfx_t*	  s;
	float		  pitch;

	()flags;

	if( !OAL_IsInitialized() || g_soundDisabled ) {
		return;
	}

	if( sfx <= 0 || sfx >= g_numSfx ) {
		return;
	}

	s = &g_sfx[sfx];
	if( !s->inUse || !s->buffer ) {
		return;
	}

	if( entchannel > 0 ) {
		OAL_StopEntityChannel( entnum, entchannel );
	}

	ch = OAL_AllocChannel();
	if( !ch ) {
		return;
	}

	alSourceStop( ch->source );
	alSourcei( ch->source, AL_BUFFER, s->buffer );
	OAL_ConfigureSourceCommon( ch->source, qfalse, origin, entnum );

	pitch = OAL_RandomPitchOffset( s_pitchJitter3D ? s_pitchJitter3D->value : 0.02f );
	alSourcef( ch->source, AL_PITCH, pitch );

	OAL_ApplyBassFilterToSource( ch->source, qtrue );

	ch->inUse	   = qtrue;
	ch->local	   = qfalse;
	ch->looping	   = qfalse;
	ch->isStream   = qfalse;
	ch->entnum	   = entnum;
	ch->entchannel = entchannel;
	ch->sfx		   = sfx;
	ch->startTime  = OAL_Milliseconds();

	alSourcePlay( ch->source );
}

void S_StartLocalSoundEx( sfxHandle_t sfx, int channelNum, int entnum )
{
	oalChannel_t* ch;
	oalSfx_t*	  s;
	float		  pitch;

	if( !OAL_IsInitialized() || g_soundDisabled ) {
		return;
	}

	if( sfx <= 0 || sfx >= g_numSfx ) {
		return;
	}

	s = &g_sfx[sfx];
	if( !s->buffer ) {
		return;
	}

	if( channelNum > 0 ) {
		OAL_StopEntityChannel( -1, channelNum );
	}

	ch = OAL_AllocChannel();
	if( !ch ) {
		return;
	}

	alSourceStop( ch->source );
	alSourcei( ch->source, AL_BUFFER, s->buffer );
	OAL_ConfigureSourceCommon( ch->source, qtrue, NULL, -1 );

	pitch = OAL_RandomPitchOffset( s_pitchJitter2D ? s_pitchJitter2D->value : 0.015f );
	alSourcef( ch->source, AL_PITCH, pitch );

	OAL_ApplyBassFilterToSource( ch->source, qtrue );

	ch->inUse	   = qtrue;
	ch->local	   = qtrue;
	ch->looping	   = qfalse;
	ch->isStream   = qfalse;
	ch->entnum	   = entnum;
	ch->entchannel = channelNum;
	ch->sfx		   = sfx;
	ch->startTime  = OAL_Milliseconds();

	alSourcePlay( ch->source );
}

void S_StartLocalSound( sfxHandle_t sfx, int channelNum )
{
	S_StartLocalSoundEx( sfx, channelNum, -1 );
}

void S_StartBackgroundTrack( const char* intro, const char* loop, int fadeupTime )
{
	()fadeupTime;

	if( !OAL_IsInitialized() || g_soundDisabled ) {
		return;
	}

	if( loop == NULL || loop[0] == 0 ) {
		loop  = intro;
		intro = NULL;
	}

	S_StopBackgroundTrack();

	memset( &g_bgTrack, 0, sizeof( g_bgTrack ) );
	g_bgTrack.source = OAL_GenSourceSafe();
	if( !g_bgTrack.source ) {
		return;
	}

	if( intro && intro[0] ) {
		Q_strncpyz( g_bgTrack.intro, intro, sizeof( g_bgTrack.intro ) );
		OAL_LoadStaticWavBuffer( intro, &g_bgTrack.introBuffer );
	}

	if( loop && loop[0] ) {
		Q_strncpyz( g_bgTrack.loop, loop, sizeof( g_bgTrack.loop ) );
		OAL_LoadStaticWavBuffer( loop, &g_bgTrack.loopBuffer );
	}

	g_bgTrack.gain	 = 1.0f;
	g_bgTrack.active = qtrue;

	alSourcei( g_bgTrack.source, AL_SOURCE_RELATIVE, AL_TRUE );
	alSource3f( g_bgTrack.source, AL_POSITION, 0.0f, 0.0f, -OAL_LOCAL_SOUND_DISTANCE );
	alSourcef( g_bgTrack.source, AL_GAIN, s_musicvolume ? s_musicvolume->value : 1.0f );
	OAL_ClearAuxSend( g_bgTrack.source );
	OAL_ClearDirectFilter( g_bgTrack.source );

	if( g_bgTrack.introBuffer ) {
		g_bgTrack.introPlaying = qtrue;
		alSourcei( g_bgTrack.source, AL_LOOPING, AL_FALSE );
		alSourcei( g_bgTrack.source, AL_BUFFER, g_bgTrack.introBuffer );
		alSourcePlay( g_bgTrack.source );
	} else if( g_bgTrack.loopBuffer ) {
		g_bgTrack.introPlaying = qfalse;
		alSourcei( g_bgTrack.source, AL_LOOPING, AL_TRUE );
		alSourcei( g_bgTrack.source, AL_BUFFER, g_bgTrack.loopBuffer );
		alSourcePlay( g_bgTrack.source );
	} else {
		g_bgTrack.active = qfalse;
	}
}

void S_StopBackgroundTrack()
{
	if( !OAL_IsInitialized() ) {
		return;
	}

	if( g_bgTrack.source ) {
		alSourceStop( g_bgTrack.source );
		alSourcei( g_bgTrack.source, AL_BUFFER, 0 );
		OAL_ClearAuxSend( g_bgTrack.source );
	}

	OAL_FreeBgTrackBuffers( &g_bgTrack );

	if( g_bgTrack.source ) {
		OAL_DeleteSourceSafe( g_bgTrack.source );
	}

	memset( &g_bgTrack, 0, sizeof( g_bgTrack ) );
}

void S_QueueBackgroundTrack( const char* loop )
{
	if( !OAL_IsInitialized() ) {
		return;
	}

	if( !loop || !loop[0] ) {
		return;
	}

	OAL_DeleteBufferSafe( g_bgTrack.loopBuffer );
	g_bgTrack.loopBuffer = 0;
	Q_strncpyz( g_bgTrack.loop, loop, sizeof( g_bgTrack.loop ) );
	OAL_LoadStaticWavBuffer( loop, &g_bgTrack.loopBuffer );

	if( g_bgTrack.active && !g_bgTrack.introPlaying && g_bgTrack.source && g_bgTrack.loopBuffer ) {
		alSourceStop( g_bgTrack.source );
		alSourcei( g_bgTrack.source, AL_BUFFER, g_bgTrack.loopBuffer );
		alSourcei( g_bgTrack.source, AL_LOOPING, AL_TRUE );
		alSourcePlay( g_bgTrack.source );
	}
}

void S_FadeStreamingSound( float targetvol, int time, int ssNum )
{
	()time;

	if( ssNum < 0 || ssNum >= ( int )ARRAY_LEN( g_entStreams ) ) {
		return;
	}

	if( g_entStreams[ssNum].source ) {
		alSourcef( g_entStreams[ssNum].source, AL_GAIN, targetvol );
	}
}

void S_FadeAllSounds( float targetvol, int time )
{
	int i;
	()time;

	for( i = 0; i < OAL_MAX_SOURCES; ++i ) {
		if( g_channels[i].source ) {
			alSourcef( g_channels[i].source, AL_GAIN, targetvol );
		}
	}

	if( g_bgTrack.source ) {
		alSourcef( g_bgTrack.source, AL_GAIN, targetvol );
	}
}

void S_StartStreamingSound( const char* intro, const char* loop, int entnum, int channel, int attenuation )
{
	int i;

	if( !OAL_IsInitialized() || g_soundDisabled ) {
		return;
	}

	for( i = 0; i < ( int )ARRAY_LEN( g_entStreams ); ++i ) {
		if( !g_entStreams[i].active ) {
			oalEntStream_t* st = &g_entStreams[i];
			memset( st, 0, sizeof( *st ) );

			st->source = OAL_GenSourceSafe();
			if( !st->source ) {
				return;
			}

			st->active		= qtrue;
			st->entnum		= entnum;
			st->channel		= channel;
			st->attenuation = attenuation;

			if( intro && intro[0] ) {
				Q_strncpyz( st->intro, intro, sizeof( st->intro ) );
				OAL_LoadStaticWavBuffer( intro, &st->introBuffer );
			}

			if( loop && loop[0] ) {
				Q_strncpyz( st->loop, loop, sizeof( st->loop ) );
				OAL_LoadStaticWavBuffer( loop, &st->loopBuffer );
			}

			alSourcef( st->source, AL_GAIN, 1.0f );
			alSourcef( st->source, AL_REFERENCE_DISTANCE, 180.0f * OAL_METERS_PER_UNIT );
			alSourcef( st->source, AL_MAX_DISTANCE, 12000.0f * OAL_METERS_PER_UNIT );
			alSourcef( st->source, AL_ROLLOFF_FACTOR, 0.8f );

			if( st->entnum >= 0 ) {
				alSourcef( st->source, AL_PITCH, OAL_RandomPitchOffset( s_pitchJitter3D ? s_pitchJitter3D->value : 0.02f ) );
				OAL_ApplyBassFilterToSource( st->source, qtrue );
			} else {
				alSourcef( st->source, AL_PITCH, OAL_RandomPitchOffset( s_pitchJitter2D ? s_pitchJitter2D->value : 0.015f ) );
				OAL_ApplyBassFilterToSource( st->source, qtrue );
			}

			if( st->entnum >= 0 ) {
				OAL_AttachWorldReverbToSource( st->source );
			} else {
				OAL_ClearAuxSend( st->source );
			}

			if( st->introBuffer ) {
				st->introPlaying = qtrue;
				alSourcei( st->source, AL_LOOPING, AL_FALSE );
				alSourcei( st->source, AL_BUFFER, st->introBuffer );
				alSourcePlay( st->source );
			} else if( st->loopBuffer ) {
				st->introPlaying = qfalse;
				alSourcei( st->source, AL_LOOPING, AL_TRUE );
				alSourcei( st->source, AL_BUFFER, st->loopBuffer );
				alSourcePlay( st->source );
			}

			return;
		}
	}
}

void S_StopStreamingSound( int index )
{
	if( index < 0 || index >= ( int )ARRAY_LEN( g_entStreams ) ) {
		return;
	}

	if( g_entStreams[index].source ) {
		alSourceStop( g_entStreams[index].source );
		alSourcei( g_entStreams[index].source, AL_BUFFER, 0 );
		OAL_ClearAuxSend( g_entStreams[index].source );
		OAL_DeleteBufferSafe( g_entStreams[index].introBuffer );
		OAL_DeleteBufferSafe( g_entStreams[index].loopBuffer );
		OAL_DeleteSourceSafe( g_entStreams[index].source );
	}

	memset( &g_entStreams[index], 0, sizeof( g_entStreams[index] ) );
}

void S_StopEntStreamingSound( int entNum )
{
	int i;

	for( i = 0; i < ( int )ARRAY_LEN( g_entStreams ); ++i ) {
		if( g_entStreams[i].active && g_entStreams[i].entnum == entNum ) {
			S_StopStreamingSound( i );
		}
	}
}

void S_RawSamples( int samples, int rate, int width, int s_channels, const byte* data, float lvol, float rvol, int streamingIndex )
{
	oalRawStream_t* rs;
	ALenum			fmt;
	ALuint			b;
	float			gain;

	if( !OAL_IsInitialized() || g_soundDisabled ) {
		return;
	}

	if( streamingIndex < 0 || streamingIndex >= ( int )ARRAY_LEN( g_rawStreams ) ) {
		return;
	}

	rs = &g_rawStreams[streamingIndex];
	if( !rs->source || !data || samples <= 0 ) {
		return;
	}

	fmt = OAL_GetFormat( s_channels, width );
	if( !fmt ) {
		return;
	}

	OAL_ServiceRawStream( rs );

	b = rs->buffers[rs->index % OAL_STREAM_BUFFERS];
	rs->index++;

	gain = ( lvol + rvol ) * 0.5f;

	alBufferData( b, fmt, data, samples * width * s_channels, rate );
	alSourceQueueBuffers( rs->source, 1, &b );
	alSourcef( rs->source, AL_GAIN, gain );

	rs->active	   = qtrue;
	rs->sampleRate = rate;
	rs->width	   = width;
	rs->channels   = s_channels;

	OAL_ServiceRawStream( rs );
}

void S_StopAllSounds()
{
	int i;

	if( !OAL_IsInitialized() ) {
		return;
	}

	for( i = 0; i < OAL_MAX_SOURCES; ++i ) {
		OAL_StopChannel( &g_channels[i] );
	}

	S_StopBackgroundTrack();

	for( i = 0; i < ( int )ARRAY_LEN( g_entStreams ); ++i ) {
		S_StopStreamingSound( i );
	}

	for( i = 0; i < ( int )ARRAY_LEN( g_rawStreams ); ++i ) {
		if( g_rawStreams[i].source ) {
			alSourceStop( g_rawStreams[i].source );
		}
	}
}

void S_ClearLoopingSounds()
{
	memset( g_loopSounds, 0, sizeof( g_loopSounds ) );
}

void S_ClearSounds( qboolean clearStreaming, qboolean clearMusic )
{
	int i;

	for( i = 0; i < OAL_MAX_SOURCES; ++i ) {
		OAL_StopChannel( &g_channels[i] );
	}

	memset( g_loopSounds, 0, sizeof( g_loopSounds ) );

	if( clearStreaming ) {
		for( i = 0; i < ( int )ARRAY_LEN( g_entStreams ); ++i ) {
			S_StopStreamingSound( i );
		}
	}

	if( clearMusic ) {
		S_StopBackgroundTrack();
	}
}

static void OAL_AddLoopSoundInternal( int entityNum, const vec3_t origin, const vec3_t velocity, int range, sfxHandle_t sfxHandle, int volume, qboolean realLoop )
{
	int i;

	if( sfxHandle <= 0 || sfxHandle >= g_numSfx ) {
		return;
	}

	for( i = 0; i < OAL_MAX_LOOP_SOUNDS; ++i ) {
		if( g_loopSounds[i].active && g_loopSounds[i].entityNum == entityNum ) {
			OAL_CopyVec3( origin, g_loopSounds[i].origin );
			OAL_CopyVec3( velocity, g_loopSounds[i].velocity );
			g_loopSounds[i].range	 = range;
			g_loopSounds[i].sfx		 = sfxHandle;
			g_loopSounds[i].volume	 = volume;
			g_loopSounds[i].realLoop = realLoop;
			return;
		}
	}

	for( i = 0; i < OAL_MAX_LOOP_SOUNDS; ++i ) {
		if( !g_loopSounds[i].active ) {
			g_loopSounds[i].active	  = qtrue;
			g_loopSounds[i].entityNum = entityNum;
			OAL_CopyVec3( origin, g_loopSounds[i].origin );
			OAL_CopyVec3( velocity, g_loopSounds[i].velocity );
			g_loopSounds[i].range	 = range;
			g_loopSounds[i].sfx		 = sfxHandle;
			g_loopSounds[i].volume	 = volume;
			g_loopSounds[i].realLoop = realLoop;
			return;
		}
	}
}

void S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, const int range, sfxHandle_t sfxHandle, int volume )
{
	OAL_AddLoopSoundInternal( entityNum, origin, velocity, range, sfxHandle, volume, qfalse );
}

void S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, const int range, sfxHandle_t sfx )
{
	OAL_AddLoopSoundInternal( entityNum, origin, velocity, range, sfx, 255, qtrue );
}

void S_StopLoopingSound( int entityNum )
{
	int i;

	for( i = 0; i < OAL_MAX_LOOP_SOUNDS; ++i ) {
		if( g_loopSounds[i].active && g_loopSounds[i].entityNum == entityNum ) {
			g_loopSounds[i].active = qfalse;
		}
	}

	for( i = 0; i < OAL_MAX_SOURCES; ++i ) {
		if( g_channels[i].inUse && g_channels[i].looping && g_channels[i].loopEntityNum == entityNum ) {
			OAL_StopChannel( &g_channels[i] );
		}
	}
}

#ifdef DOOMSOUND
void S_ClearSoundBuffer()
{
	int i;

	for( i = 0; i < ( int )ARRAY_LEN( g_rawStreams ); ++i ) {
		if( g_rawStreams[i].source ) {
			alSourceStop( g_rawStreams[i].source );

			{
				ALint queued = 0;
				alGetSourcei( g_rawStreams[i].source, AL_BUFFERS_QUEUED, &queued );
				while( queued-- > 0 ) {
					ALuint b;
					alSourceUnqueueBuffers( g_rawStreams[i].source, 1, &b );
				}
			}
		}
	}
}
#endif

void S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater )
{
	ALfloat pos[3];
	ALfloat ori[6];
	int		i;

	()entityNum;
	()inwater;

	if( !OAL_IsInitialized() || g_soundDisabled ) {
		return;
	}

	OAL_CopyVec3( origin, g_listenerOrigin );
	OAL_CopyVec3( axis[0], g_listenerAxis[0] ); // forward
	OAL_CopyVec3( axis[1], g_listenerAxis[1] ); // right
	OAL_CopyVec3( axis[2], g_listenerAxis[2] ); // up

	OAL_GameToAL( origin, pos );

	ori[0] = axis[0][0];
	ori[1] = axis[0][1];
	ori[2] = axis[0][2];
	ori[3] = axis[2][0];
	ori[4] = axis[2][1];
	ori[5] = axis[2][2];

	alListener3f( AL_POSITION, pos[0], pos[1], pos[2] );
	alListener3f( AL_VELOCITY, 0.0f, 0.0f, 0.0f );
	alListenerfv( AL_ORIENTATION, ori );
	alDopplerFactor( s_doppler ? s_doppler->value : 1.0f );

	for( i = 0; i < OAL_MAX_SOURCES; ++i ) {
		if( g_channels[i].inUse && !g_channels[i].local && !g_channels[i].looping ) {
			if( g_channels[i].entnum >= 0 && g_channels[i].entnum < OAL_MAX_ENTITIES && g_entities[g_channels[i].entnum].valid ) {
				ALfloat srcPos[3];
				OAL_GameToAL( g_entities[g_channels[i].entnum].origin, srcPos );
				alSource3f( g_channels[i].source, AL_POSITION, srcPos[0], srcPos[1], srcPos[2] );
				OAL_AttachWorldReverbToSource( g_channels[i].source );
			}
		}
	}
}

void S_UpdateEntityPosition( int entityNum, const vec3_t origin )
{
	if( entityNum < 0 || entityNum >= OAL_MAX_ENTITIES ) {
		return;
	}

	g_entities[entityNum].valid = qtrue;
	OAL_CopyVec3( origin, g_entities[entityNum].origin );
}

static void OAL_UpdateLoopingSounds()
{
	int i, j;

	for( i = 0; i < OAL_MAX_LOOP_SOUNDS; ++i ) {
		oalLoopSound_t* ls = &g_loopSounds[i];
		if( !ls->active ) {
			continue;
		}

		for( j = 0; j < OAL_MAX_SOURCES; ++j ) {
			if( g_channels[j].inUse && g_channels[j].looping && g_channels[j].loopEntityNum == ls->entityNum ) {
				ALfloat pos[3];
				OAL_GameToAL( ls->origin, pos );
				alSource3f( g_channels[j].source, AL_POSITION, pos[0], pos[1], pos[2] );
				alSourcef( g_channels[j].source, AL_GAIN, ( ls->volume / 255.0f ) * ( s_volume ? s_volume->value : 1.0f ) );
				OAL_AttachWorldReverbToSource( g_channels[j].source );
				goto nextLoopSound;
			}
		}

		{
			oalChannel_t* ch;
			oalSfx_t*	  sfx;

			if( ls->sfx <= 0 || ls->sfx >= g_numSfx ) {
				continue;
			}

			sfx = &g_sfx[ls->sfx];
			if( !sfx->buffer ) {
				continue;
			}

			ch = OAL_AllocChannel();
			if( !ch ) {
				continue;
			}

			alSourceStop( ch->source );
			alSourcei( ch->source, AL_BUFFER, sfx->buffer );
			alSourcei( ch->source, AL_LOOPING, AL_TRUE );
			OAL_ConfigureSourceCommon( ch->source, qfalse, ls->origin, ls->entityNum );
			alSourcef( ch->source, AL_PITCH, OAL_RandomPitchOffset( s_pitchJitter3D ? s_pitchJitter3D->value : 0.02f ) );
			OAL_ApplyBassFilterToSource( ch->source, qtrue );
			alSourcef( ch->source, AL_GAIN, ( ls->volume / 255.0f ) * ( s_volume ? s_volume->value : 1.0f ) );
			alSourcePlay( ch->source );

			ch->inUse		  = qtrue;
			ch->local		  = qfalse;
			ch->looping		  = qtrue;
			ch->isStream	  = qfalse;
			ch->entnum		  = ls->entityNum;
			ch->entchannel	  = 0;
			ch->sfx			  = ls->sfx;
			ch->startTime	  = OAL_Milliseconds();
			ch->loopEntityNum = ls->entityNum;
		}

	nextLoopSound:;
	}

	for( j = 0; j < OAL_MAX_SOURCES; ++j ) {
		if( g_channels[j].inUse && g_channels[j].looping ) {
			qboolean found = qfalse;

			for( i = 0; i < OAL_MAX_LOOP_SOUNDS; ++i ) {
				if( g_loopSounds[i].active && g_loopSounds[i].entityNum == g_channels[j].loopEntityNum ) {
					found = qtrue;
					break;
				}
			}

			if( !found ) {
				OAL_StopChannel( &g_channels[j] );
			}
		}
	}
}

void S_Update()
{
	int i;

	if( !OAL_IsInitialized() || g_soundDisabled ) {
		return;
	}

	// allow runtime tweaking of the reverb cvars
	if( g_oalHasEFX ) {
		OAL_ApplyCurrentReverbParameters();
	}

	for( i = 0; i < OAL_MAX_SOURCES; ++i ) {
		if( g_channels[i].inUse ) {
			ALint state = AL_STOPPED;
			alGetSourcei( g_channels[i].source, AL_SOURCE_STATE, &state );
			if( !g_channels[i].looping && state == AL_STOPPED ) {
				OAL_StopChannel( &g_channels[i] );
			}
		}
	}

	OAL_UpdateLoopingSounds();
	OAL_UpdateBackgroundTrack();
	OAL_UpdateEntStreams();

	for( i = 0; i < ( int )ARRAY_LEN( g_rawStreams ); ++i ) {
		OAL_ServiceRawStream( &g_rawStreams[i] );
	}
}

void S_DisableSounds()
{
	g_soundDisabled = qtrue;
	S_StopAllSounds();
}

void S_BeginRegistration()
{
	g_soundDisabled = qfalse;
}

#ifdef DOOMSOUND
sfxHandle_t S_RegisterSound( const char* sample )
#else
sfxHandle_t S_RegisterSound( const char* sample, qboolean compressed )
#endif
{
	sfxHandle_t h;
	ALuint		buf;
	int			rate, width, channels, sizeBytes;

#ifndef DOOMSOUND
	()compressed;
#endif

	if( !sample || !sample[0] ) {
		return 0;
	}

	h = OAL_FindSfx( sample );
	if( h ) {
		return h;
	}

	h = OAL_AllocSfx( sample );
	if( !h ) {
		return 0;
	}

	buf = 0;
	if( !OAL_LoadSfxBuffer( sample, &buf, &rate, &width, &channels, &sizeBytes ) ) {
		// Com_Printf(S_COLOR_YELLOW "OpenAL: failed to load sound '%s'\n", sample);
		g_sfx[h].placeholder = qtrue;
		g_sfx[h].buffer		 = 0;
		return h;
	}

	g_sfx[h].buffer	   = buf;
	g_sfx[h].rate	   = rate;
	g_sfx[h].width	   = width;
	g_sfx[h].channels  = channels;
	g_sfx[h].sizeBytes = sizeBytes;
	return h;
}

void S_DisplayFreeMemory()
{
	int i;
	int totalBytes = 0;
	int count	   = 0;

	for( i = 1; i < g_numSfx; ++i ) {
		if( g_sfx[i].inUse && g_sfx[i].buffer ) {
			totalBytes += g_sfx[i].sizeBytes;
			count++;
		}
	}

	Com_Printf( "OpenAL: %d sounds loaded, %.2f MB\n", count, totalBytes / ( 1024.0f * 1024.0f ) );
}

int S_GetVoiceAmplitude( int entityNum )
{
	int i;

	for( i = 0; i < OAL_MAX_SOURCES; ++i ) {
		if( g_channels[i].inUse && g_channels[i].entnum == entityNum ) {
			ALfloat gain = 0.0f;
			alGetSourcef( g_channels[i].source, AL_GAIN, &gain );
			gain = gain * 0.3;
			return ( int )( gain * 255.0f );
		}
	}

	return 0;
}

void S_ClearSoundBuffer( int killStreaming )
{
	int i;

	// clear raw stream bookkeeping for legacy code
	s_soundtime = 0;
	for( i = 0; i < 8; ++i ) {
		g_s_rawend[i] = 0;
	}

	if( !OAL_IsInitialized() ) {
		return;
	}

	// stop queued raw/cinematic/voice streams
	for( i = 0; i < ( int )ARRAY_LEN( g_rawStreams ); ++i ) {
		if( !g_rawStreams[i].source ) {
			continue;
		}

		alSourceStop( g_rawStreams[i].source );

		{
			ALint queued = 0;
			alGetSourcei( g_rawStreams[i].source, AL_BUFFERS_QUEUED, &queued );

			while( queued-- > 0 ) {
				ALuint b = 0;
				alSourceUnqueueBuffers( g_rawStreams[i].source, 1, &b );
			}
		}

		g_rawStreams[i].active	   = qfalse;
		g_rawStreams[i].index	   = 0;
		g_rawStreams[i].sampleRate = 0;
		g_rawStreams[i].width	   = 0;
		g_rawStreams[i].channels   = 0;
	}

	// optional: kill entity/music streams too if caller wants a hard flush
	if( killStreaming ) {
		for( i = 0; i < ( int )ARRAY_LEN( g_entStreams ); ++i ) {
			S_StopStreamingSound( i );
		}

		S_StopBackgroundTrack();
	}
}