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

#pragma once

/*
 * name:		bg_public.h
 *
 * desc:		definitions shared by both the server game and client game modules
 *
 */

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame

#define GAME_VERSION			 "baseq3-1"

#define DEFAULT_GRAVITY			 800
#define GIB_HEALTH				 -40
#define ARMOR_PROTECTION		 0.66

#define MAX_ITEMS				 256

#define RANK_TIED_FLAG			 0x4000

#define DEFAULT_SHOTGUN_SPREAD	 700
#define DEFAULT_SHOTGUN_COUNT	 11

// #define	ITEM_RADIUS			15		// item sizes are needed for client side pickup detection
#define ITEM_RADIUS				 10 // Rafael changed the radius so that the items would fit in the 3 new containers

// RF, zombie getup
#define TIMER_RESPAWN			 ( 38 * ( 1000 / 15 ) + 100 )

#define LIGHTNING_RANGE			 600
#define TESLA_RANGE				 800
#define TESLA_SUPERSOLDIER_RANGE 2000
// JPW NERVE -- make flamethrower range variable with single/multiplayer -- actual routine is in bg_pmove.c
float Com_GetFlamethrowerRange();
#define FLAMETHROWER_RANGE		   Com_GetFlamethrowerRange()
// #define	FLAMETHROWER_RANGE	850
//  jpw
#define ZOMBIE_FLAME_SCALE		   0.3
#define ZOMBIE_FLAME_RADIUS		   ( FLAMETHROWER_RANGE * ZOMBIE_FLAME_SCALE )

// RF, AI effects
#define PORTAL_ZOMBIE_SPAWNTIME	   3000
#define PORTAL_FEMZOMBIE_SPAWNTIME 3000

#define SCORE_NOT_PRESENT		   -9999 // for the CS_SCORES[12] when only one player is present

#define VOTE_TIME				   30000 // 30 seconds before vote times out

#define DEAD_SINK_DURATION		   12000
#define DEAD_SINK_DEPTH			   64

// Ridah, disabled these
// #define	MINS_Z				-24
// #define	DEFAULT_VIEWHEIGHT	26
// #define CROUCH_VIEWHEIGHT	12
// done.

// Rafael
// note to self: Corky test
// #define	DEFAULT_VIEWHEIGHT	26
// #define CROUCH_VIEWHEIGHT	12
#define DEFAULT_VIEWHEIGHT		   40
#define CROUCH_VIEWHEIGHT		   16
#define DEAD_VIEWHEIGHT			   -16

// RF, temp only, use infantryss so we can test new anim system
// #define	DEFAULT_MODEL		"american"
#define DEFAULT_MODEL			   "bj2"
#define DEFAULT_HEAD			   "default" // technically the default head skin.  this means "head_default.skin" for the head

// RF, on fire effects
#define FIRE_FLASH_TIME			   2000
#define FIRE_FLASH_FADEIN_TIME	   1000

#define LIGHTNING_FLASH_TIME	   150

// RF, client damage identifiers
typedef enum { CLDMG_SPIRIT, CLDMG_FLAMETHROWER, CLDMG_TESLA, CLDMG_BOSS1LIGHTNING, CLDMG_DEBRIS, CLDMG_MAX } clientDamage_t;

// RF
#define MAX_TAGCONNECTS			 32

// (SA) zoom sway values
#define ZOOM_PITCH_AMPLITUDE	 0.13f
#define ZOOM_PITCH_FREQUENCY	 0.24f
#define ZOOM_PITCH_MIN_AMPLITUDE 0.1f // minimum amount of sway even if completely settled on target

#define ZOOM_YAW_AMPLITUDE		 0.7f
#define ZOOM_YAW_FREQUENCY		 0.12f
#define ZOOM_YAW_MIN_AMPLITUDE	 0.2f

// DHM - Nerve
#define MAX_OBJECTIVES			 6
// dhm

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
#define CS_MUSIC				 2
#define CS_MESSAGE				 3 // from the map worldspawn's message field
#define CS_MOTD					 4 // g_motd string for server message of the day
#define CS_WARMUP				 5 // server time when the match will be restarted
#define CS_SCORES1				 6
#define CS_SCORES2				 7
#define CS_VOTE_TIME			 8
#define CS_VOTE_STRING			 9
#define CS_VOTE_YES				 10
#define CS_VOTE_NO				 11
#define CS_GAME_VERSION			 12
#define CS_LEVEL_START_TIME		 13 // so the timer only shows the current level
#define CS_INTERMISSION			 14 // when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
// DHM - Nerve :: Wolf Multiplayer information
#define CS_MULTI_INFO			 15
#define CS_MULTI_MAPDESC		 16
#define CS_MULTI_OBJECTIVE1		 17
#define CS_MULTI_OBJECTIVE2		 18
#define CS_MULTI_OBJECTIVE3		 19
#define CS_MULTI_OBJECTIVE4		 20
#define CS_MULTI_OBJECTIVE5		 21
#define CS_MULTI_OBJECTIVE6		 22
// dhm

#define CS_MISSIONSTATS			 23 //----(SA)	added

#define CS_SHADERSTATE			 24
#define CS_MUSIC_QUEUE			 25
#define CS_ITEMS				 27 // string of 0's and 1's that tell which items are present

#define CS_SCREENFADE			 28 // Ridah, used to tell clients to fade their screen to black/normal
#define CS_FOGVARS				 29 //----(SA)	(hoping 29-31 are available) used for saving the current state/settings of the fog
#define CS_SKYBOXORG			 30 // this is where we should view the skybox from
#define CS_TARGETEFFECT			 31 //----(SA)
#define CS_MODELS				 32
#define CS_SOUNDS				 ( CS_MODELS + MAX_MODELS )
#define CS_PLAYERS				 ( CS_SOUNDS + MAX_SOUNDS )
#define CS_LOCATIONS			 ( CS_PLAYERS + MAX_CLIENTS )
#define CS_PARTICLES			 ( CS_LOCATIONS + MAX_LOCATIONS )

// JPW NERVE -- for spawnpoint selection
#define CS_MULTI_SPAWNTARGETS	 ( CS_PARTICLES + MAX_PARTICLES_AREAS )
// jpw

//----(SA)
#define CS_DLIGHTS				 ( CS_MULTI_SPAWNTARGETS + MAX_MULTI_SPAWNTARGETS )
#define CS_CLIPBOARDS			 ( CS_DLIGHTS + MAX_DLIGHT_CONFIGSTRINGS )
#define CS_SPLINES				 ( CS_CLIPBOARDS + MAX_CLIPBOARD_CONFIGSTRINGS )
//----(SA)

// RF
#define CS_TAGCONNECTS			 ( CS_SPLINES + MAX_SPLINE_CONFIGSTRINGS )

// #define CS_MAX			(CS_LOCATIONS+MAX_LOCATIONS)
// #define CS_MAX			(CS_PARTICLES+MAX_PARTICLES_AREAS)
// #define CS_MAX				(CS_DLIGHTS+MAX_DLIGHT_CONFIGSTRINGS)			//----(SA)
#define CS_MAX					 ( CS_TAGCONNECTS + MAX_TAGCONNECTS )

#if( CS_MAX ) > MAX_CONFIGSTRINGS
	#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif

typedef enum {
	GT_FFA,			  // free for all
	GT_TOURNAMENT,	  // one on one tournament
	GT_SINGLE_PLAYER, // single player tournament

	//-- team games go after this --

	GT_TEAM, // team deathmatch
	GT_CTF,	 // capture the flag
	GT_WOLF, // DHM - Nerve :: Wolfenstein Multiplayer

	GT_MAX_GAME_TYPE
} gametype_t;

// Rafael gameskill
typedef enum {
	GSKILL_EASY,
	GSKILL_MEDIUM,
	GSKILL_HARD,
	GSKILL_MAX // must always be last
} gameskill_t;

typedef enum { GENDER_MALE, GENDER_FEMALE, GENDER_NEUTER } gender_t;

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum {
	PM_NORMAL,		// can accelerate and turn
	PM_NOCLIP,		// noclip movement
	PM_SPECTATOR,	// still run into walls
	PM_DEAD,		// no acceleration or turning, but free falling
	PM_FREEZE,		// stuck in place with no control
	PM_INTERMISSION // no movement or status bar
} pmtype_t;

typedef enum {
	WEAPON_READY,
	WEAPON_RAISING,
	WEAPON_RAISING_TORELOAD, //----(SA)	added
	WEAPON_DROPPING,
	WEAPON_DROPPING_TORELOAD, //----(SA)	added.  will reload upon completion of weapon switch
	WEAPON_READYING,		  // getting from 'ready' to 'firing'
	WEAPON_RELAXING,		  // weapon is ready, but since not firing, it's on it's way to a "relaxed" stance
	WEAPON_VENOM_REST,
	WEAPON_FIRING,
	WEAPON_FIRINGALT,
	WEAPON_WAITING,	 //----(SA)	added.  player allowed to switch/reload, but not fire
	WEAPON_RELOADING //----(SA)	added
} weaponstate_t;

// pmove->pm_flags	(sent as max 16 bits in msg.c)
#define PMF_DUCKED		   1
#define PMF_JUMP_HELD	   2
#define PMF_LADDER		   4   // player is on a ladder
#define PMF_BACKWARDS_JUMP 8   // go into backwards land
#define PMF_BACKWARDS_RUN  16  // coast down to backwards run
#define PMF_TIME_LAND	   32  // pm_time is time before rejump
#define PMF_TIME_KNOCKBACK 64  // pm_time is an air-accelerate only time
#define PMF_TIME_WATERJUMP 256 // pm_time is waterjump
#define PMF_RESPAWNED	   512 // clear after attack and jump buttons come up
#define PMF_USE_ITEM_HELD  1024
// RF, removed since it's not used
// #define PMF_GRAPPLE_PULL	2048	// pull towards grapple location
#define PMF_IGNORE_INPUT   2048	 // no movement/firing commands allowed
#define PMF_FOLLOW		   4096	 // spectate following another player
#define PMF_SCOREBOARD	   8192	 // spectate as a scoreboard
#define PMF_LIMBO		   16384 // JPW NERVE limbo state, pm_time is time until reinforce
#define PMF_TIME_LOAD	   32768 // hold for this time after a load game, and prevent large thinks

#define PMF_ALL_TIMES	   ( PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_KNOCKBACK | PMF_TIME_LOAD )

#define MAXTOUCH		   32
typedef struct {
	// state (in / out)
	playerState_t* ps;

	// command (in)
	usercmd_t	   cmd, oldcmd;
	int			   tracemask;	// collide against these types of surfaces
	int			   debugLevel;	// if set, diagnostic output will be printed
	qboolean	   noFootsteps; // if the game is setup for no footsteps by the server
	qboolean	   noWeapClips; // if the game is setup for no weapon clips by the server
	qboolean	   gauntletHit; // true if a gauntlet attack would actually hit something

	// results (out)
	int			   numtouch;
	int			   touchents[MAXTOUCH];

	vec3_t		   mins, maxs; // bounding box size

	int			   watertype;
	int			   waterlevel;

	float		   xyspeed;

	// for fixed msec Pmove
	int			   pmove_fixed;
	int			   pmove_msec;

	// callbacks to test the world
	// these will be different functions during game and cgame
	void ( *trace )( trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );
	int ( *pointcontents )( const vec3_t point, int passEntityNum );
} pmove_t;

/*!
	\brief Updates the player's view angles based on user command input and handles angle clamping to prevent excessive pitch rotation.

	This function modifies the player state's view angles by incorporating the user command's angle changes and delta angles. It ensures that the pitch angle does not exceed 90 degrees in either
   direction by clamping the values. The function also validates that the player state is not in a frozen, intermission, or dead condition before updating the angles. A trace function pointer is set
   up for potential use in the update process, although it's not directly invoked within this function. The function is designed to work in conjunction with the pmove system and is typically called
   during client-side simulation updates.

	\param ps Pointer to the player state structure containing view angles and delta angles
	\param cmd Pointer to the user command structure containing the new angle commands
	\param trace Function pointer to a trace function used for collision detection
*/
void PM_UpdateViewAngles(
	playerState_t* ps, usercmd_t* cmd, void( trace )( trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask ) );

/*!
	\brief Performs player movement processing for a single frame with input command handling and physics simulation.

	This function processes player movement by handling various game states and input conditions. It first checks for special flags like EF_DUMMY_PMOVE which bypasses normal processing, and
   PMF_IGNORE_INPUT which clears input values. The function manages command timing and ensures proper frame rate independent movement by subdividing long time intervals into smaller chunks. It also
   handles special cases like load game states and sprint speed adjustments. The function returns a surface flag value if the player is on a monster slick surface and the player is dead or has no
   health.

	\param pmove pointer to the player movement state structure containing player state and command input
	\return integer value indicating surface flags if player is on monster slick surface and dead, or zero otherwise
*/
int Pmove( pmove_t* pmove );

//===================================================================================

// JPW NERVE
#define PC_SOLDIER			0	  //	shoot stuff
#define PC_MEDIC			1	  //	heal stuff
#define PC_ENGINEER			2	  //	build stuff
#define PC_LT				3	  //	bomb stuff
#define PC_MEDIC_CHARGETIME 30000 // FIXME just for testing, this will change to server cvars for each class
// jpw

// player_state->stats[] indexes
typedef enum {
	STAT_HEALTH,
	STAT_HOLDABLE_ITEM,
	//	STAT_WEAPONS,					// 16 bit fields
	STAT_ARMOR,
	//----(SA) Keys for Wolf
	STAT_KEYS, // 16 bit fields
	//----(SA) end
	STAT_DEAD_YAW,		// look this direction when dead (FIXME: get rid of?)
	STAT_CLIENTS_READY, // bit mask of clients wishing to exit the intermission (FIXME: configstring?)
	STAT_MAX_HEALTH,	// health / armor limit, changable by handicap
	STAT_PLAYER_CLASS	// DHM - Nerve :: player class in multiplayer
} statIndex_t;

// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
typedef enum {
	PERS_SCORE, // !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_HITS,	// total points damage inflicted so damage beeps can sound on change
	PERS_RANK,
	PERS_TEAM,
	PERS_SPAWN_COUNT,  // incremented every respawn
	PERS_REWARD_COUNT, // incremented for each reward sound
	PERS_REWARD,	   // a reward_t
	PERS_ATTACKER,	   // clientnum of last damage inflicter
	PERS_KILLED,	   // count of the number of times you died
	// these were added for single player awards tracking
	PERS_IMPRESSIVE_COUNT,
	PERS_EXCELLENT_COUNT,
	PERS_GAUNTLET_FRAG_COUNT,
	PERS_ACCURACY_SHOTS,
	PERS_ACCURACY_HITS,

	// Rafael - mg42		// (SA) I don't understand these here.  can someone explain?
	PERS_HWEAPON_USE,
	// Rafael wolfkick
	PERS_WOLFKICK

} persEnum_t;

// entityState_t->eFlags
#define EF_DEAD			   0x00000001		  // don't draw a foe marker over players with EF_DEAD
#define EF_NONSOLID_BMODEL 0x00000002		  // bmodel is visible, but not solid
#define EF_FORCE_END_FRAME EF_NONSOLID_BMODEL // force client to end of current animation (after loading a savegame)
#define EF_TELEPORT_BIT	   0x00000004		  // toggled every time the origin abruptly changes
#define EF_MONSTER_EFFECT  0x00000008		  // draw an aiChar dependant effect for this character
#define EF_CAPSULE		   0x00000010		  // use capsule for collisions
#define EF_CROUCHING	   0x00000020		  // player is crouching
#define EF_MG42_ACTIVE	   0x00000040		  // currently using an MG42
#define EF_NODRAW		   0x00000080		  // may have an event, but no model (unspawned items)
#define EF_FIRING		   0x00000100		  // for lightning gun
#define EF_INHERITSHADER   EF_FIRING		  // some ents will never use EF_FIRING, hijack it for "USESHADER"
#define EF_BOUNCE_HEAVY	   0x00000200		  // more realistic bounce.  not as rubbery as above (currently for c4)
#define EF_SPINNING		   0x00000400		  // (SA) added for level editor control of spinning pickup items
#define EF_BREATH		   EF_SPINNING		  // Characters will not have EF_SPINNING set, hijack for drawing character breath

#define EF_MELEE_ACTIVE	   0x00000800	  // (SA) added for client knowledge of melee items held (chair/etc.)
#define EF_TALK			   0x00001000	  // draw a talk balloon
#define EF_CONNECTION	   0x00002000	  // draw a connection trouble sprite
#define EF_MONSTER_EFFECT2 0x00004000	  // show the secondary special effect for this character
#define EF_HEADSHOT		   0x00008000	  // last hit to player was head shot
#define EF_MONSTER_EFFECT3 0x00010000	  // show the third special effect for this character
#define EF_HEADLOOK		   0x00020000	  // make the head look around
#define EF_STAND_IDLE2	   0x00040000	  // when standing, play idle2 instead of the default
#define EF_VIEWING_CAMERA  EF_STAND_IDLE2 // NOTE: REMOVE STAND_IDLE2 !!
#define EF_TAGCONNECT	   0x00080000	  // connected to another entity via tag
// RF, disabled, not used anymore
// #define EF_MOVER_BLOCKED	0x00100000		// mover was blocked dont lerp on the client
#define EF_NO_TURN_ANIM	   0x00100000 // dont play turning anims in the cgame
#define EF_FORCED_ANGLES   0x00200000 // enforce all body parts to use these angles

#define EF_ZOOMING		   0x00400000 // client is zooming
#define EF_NOSWINGANGLES   0x00800000 // try and keep all parts facing same direction

#define EF_DUMMY_PMOVE	   0x01000000
#define EF_VOTED		   0x02000000		// already cast a vote
#define EF_BOUNCE		   0x04000000		// for missiles
#define EF_CIG			   EF_BOUNCE		// players should never use bounce, hijack for cigarette
#define EF_BOUNCE_HALF	   0x08000000		// for missiles
#define EF_MOVER_STOP	   0x10000000		// will push otherwise	// (SA) moved down to make space for one more client flag
#define EF_MOVER_ANIMATE   0x20000000		// interpolate animation
#define EF_DEATH_FRAME	   EF_MOVER_ANIMATE // hijack to stick death at last frame after vid_restart
#define EF_RECENTLY_FIRING 0x40000000		// fired recently, lock torso angles, etc

typedef enum {
	PW_NONE,

	PW_QUAD,
	PW_BATTLESUIT,
	PW_HASTE,
	PW_INVIS,
	PW_REGEN,
	PW_FLIGHT,

	// (SA) for Wolf
	PW_INVULNERABLE,
	//	PW_FIRE,			//----(SA)
	//	PW_ELECTRIC,		//----(SA)
	//	PW_BREATHER,		//----(SA)
	PW_NOFATIGUE, //----(SA)

	PW_REDFLAG,
	PW_BLUEFLAG,
	PW_BALL,

	PW_NUM_POWERUPS
} powerup_t;

typedef enum {
	//----(SA)	These will probably all change to INV_n to get the word 'key' out of the game.
	//			id and DM don't want references to 'keys' in the game.
	//			I'll change to 'INV' as the item becomes 'permanent' and not a test item.
	KEY_NONE,
	//	KEY_1,		// skull
	//	KEY_2,		// chalice
	//	KEY_3,		// eye
	//	KEY_4,		// field radio
	//	KEY_5,		// satchel charge
	INV_BINOCS, // binoculars
	//	KEY_7,
	//	KEY_8,
	//	KEY_9,
	//	KEY_10,
	//	KEY_11,
	//	KEY_12,
	//	KEY_13,
	//	KEY_14,
	//	KEY_15,
	//	KEY_16,
	KEY_NUM_KEYS,

	KEY_UNLOCKED_ENT,	 // ent is unlocked (will be replaced by KEY_NONE after checks for all manners of being locked)
	KEY_LOCKED_TARGET,	 // ent is locked by virtue of being the target of another ent
	KEY_LOCKED_ENT,		 // ent has key set to -1 in entity
	KEY_LOCKED_TRIGGERED // locked by a target_lock
} wkey_t;				 // key_t conflicted with <types.h>

typedef enum {
	HI_NONE,

	//	HI_TELEPORTER,
	//	HI_MEDKIT,

	// new for Wolf
	HI_WINE,
	//	HI_SKULL,
	//	HI_WATER,
	//	HI_ELECTRIC,
	//	HI_FIRE,
	HI_STAMINA,
	HI_BOOK1, //----(SA)	added
	HI_BOOK2, //----(SA)	added
	HI_BOOK3, //----(SA)	added
	HI_11,
	HI_12,
	HI_13,
	HI_14,
	//	HI_15,	// ?

	HI_NUM_HOLDABLE
} holdable_t;

// Ridah
//
// character presets
typedef enum {
	AICHAR_NONE,

	AICHAR_SOLDIER,
	AICHAR_AMERICAN,
	AICHAR_ZOMBIE,
	AICHAR_WARZOMBIE,
	AICHAR_VENOM,
	AICHAR_LOPER,
	AICHAR_ELITEGUARD,
	AICHAR_STIMSOLDIER1, // dual machineguns
	AICHAR_STIMSOLDIER2, // rocket in left hand
	AICHAR_STIMSOLDIER3, // tesla in left hand
	AICHAR_SUPERSOLDIER,
	AICHAR_BLACKGUARD,
	AICHAR_PROTOSOLDIER,

	AICHAR_FROGMAN,
	AICHAR_HELGA,
	AICHAR_HEINRICH, //----(SA)	added

	AICHAR_PARTISAN,
	AICHAR_CIVILIAN,

	NUM_CHARACTERS
} AICharacters_t;
// done.

// NOTE: we can only use up to 15 in the client-server stream
// SA NOTE: should be 31 now (I added 1 bit in msg.c)
typedef enum {
	WP_NONE, // 0

	WP_KNIFE, // 1
	// German weapons
	WP_LUGER,			 // 2
	WP_MP40,			 // 3
	WP_MAUSER,			 // 4
	WP_FG42,			 // 5
	WP_GRENADE_LAUNCHER, // 6
	WP_PANZERFAUST,		 // 7
	WP_VENOM,			 // 8
	WP_FLAMETHROWER,	 // 9
	WP_TESLA,			 // 10
	//	WP_SPEARGUN,			// 11

	// weapon keys only go 1-0, so put the alternates above that (since selection will be a double click on the german weapon key)

	// American equivalents
	//	WP_KNIFE2,				// 12
	WP_COLT,	 // 11	equivalent american weapon to german luger
	WP_THOMPSON, // 12	equivalent american weapon to german mp40
	WP_GARAND,	 // 13	equivalent american weapon to german mauser
	//	WP_BAR,					// 16	equivalent american weapon to german fg42
	WP_GRENADE_PINEAPPLE, // 14
	//	WP_ROCKET_LAUNCHER,		// 18	equivalent american weapon to german panzerfaust

	// secondary fire weapons
	WP_SNIPERRIFLE,	 // 15
	WP_SNOOPERSCOPE, // 16
	//	WP_VENOM_FULL,			// 21
	//	WP_SPEARGUN_CO2,		// 22
	WP_FG42SCOPE, // 17	fg42 alt fire
	//	WP_BAR2,				// 24

	// more weapons
	WP_STEN,	 // 18	silenced sten sub-machinegun
	WP_SILENCER, // 19	// used to be sp5
	WP_AKIMBO,	 // 20	//----(SA)	added

	// specialized/one-time weapons
	// JPW NERVE -- swapped mortar & antitank (unused?) and added class_special
	WP_CLASS_SPECIAL, // 21	// class-specific multiplayer weapon (airstrike, engineer, or medpack)
	// (SA) go ahead and take the 'freezeray' spot.  it ain't happenin
	//      (I checked for instances of WP_CLASS_SPECIAL and I don't think this'll cause you a problem.  however, if it does, move it where you need to. ) (SA)
	// jpw
	//	WP_CROSS,				// 29
	WP_DYNAMITE, // 22
	//	WP_DYNAMITE2,			// 31
	//	WP_PROX,				// 32

	WP_MONSTER_ATTACK1, // 23	// generic monster attack, slot 1
	WP_MONSTER_ATTACK2, // 24	// generic monster attack, slot 2
	WP_MONSTER_ATTACK3, // 25	// generic monster attack, slot 2

	WP_GAUNTLET, // 26

	WP_SNIPER,		  // 27
	WP_GRENADE_SMOKE, // 28	// smoke grenade for LT multiplayer
	WP_MEDIC_HEAL,	  // 29	// DHM - Nerve :: Medic special weapon
	WP_MORTAR,		  // 30

	VERYBIGEXPLOSION, // 31	// explosion effect for airplanes

	WP_NUM_WEAPONS // 32   NOTE: this cannot be larger than 64 for AI/player weapons!

} weapon_t;

typedef struct ammotable_s {
	int maxammo;	   //
	int uses;		   //
	int maxclip;	   //
	int reloadTime;	   //
	int fireDelayTime; //
	int nextShotTime;  //
	//----(SA)	added
	int maxHeat;  // max active firing time before weapon 'overheats' (at which point the weapon will fail)
	int coolRate; // how fast the weapon cools down. (per second)
	//----(SA)	end
	int mod; // means of death
} ammotable_t;

extern ammotable_t ammoTable[]; // defined in bg_misc.c
extern int		   weapAlts[];	// defined in bg_misc.c

//----(SA)
// for routines that need to check if a WP_ is </=/> a given set of weapons
#define WP_FIRST		  WP_KNIFE
#define WP_BEGINGERMAN	  WP_KNIFE
#define WP_LASTGERMAN	  WP_TESLA
#define WP_BEGINAMERICAN  WP_COLT
#define WP_LASTAMERICAN	  WP_GRENADE_PINEAPPLE
#define WP_BEGINSECONDARY WP_SNIPERRIFLE
#define WP_LASTSECONDARY  WP_FG42SCOPE

#define WEAPS_ONE_HANDED  ( ( 1 << WP_KNIFE ) | ( 1 << WP_LUGER ) | ( 1 << WP_COLT ) | ( 1 << WP_SILENCER ) | ( 1 << WP_GRENADE_LAUNCHER ) | ( 1 << WP_GRENADE_PINEAPPLE ) )
//----(SA)	end

typedef enum { WPOS_HIGH, WPOS_LOW, WPOS_KNIFE, WPOS_PISTOL, WPOS_SHOULDER, WPOS_THROW, WPOS_NUM_POSITIONS } pose_t;

/*
// Original Q3A weaps/order
typedef enum {
	WP_NONE,				// 0
	WP_GAUNTLET,			// 1
	WP_MACHINEGUN = 20,		// 2
	WP_SHOTGUN,				// 3
	WP_GRENADE_LAUNCHER,	// 4
	WP_ROCKET_LAUNCHER,		// 5
	WP_LIGHTNING,			// 6
	WP_RAILGUN,				// 7
	WP_PLASMAGUN,			// 8
	WP_BFG,					// 9
	WP_GRAPPLING_HOOK		// 10
	WP_NUM_WEAPONS			// 11
} weapon_t;

*/
// reward sounds
typedef enum {
	REWARD_BAD,

	REWARD_IMPRESSIVE,
	REWARD_EXCELLENT,
	REWARD_DENIED,
	REWARD_GAUNTLET
} reward_t;

// entityState_t->event values
// entity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define EV_EVENT_BIT1 0x00000100
#define EV_EVENT_BIT2 0x00000200
#define EV_EVENT_BITS ( EV_EVENT_BIT1 | EV_EVENT_BIT2 )

typedef enum {
	EV_NONE,
	EV_FOOTSTEP,
	EV_FOOTSTEP_METAL,
	EV_FOOTSTEP_WOOD,
	EV_FOOTSTEP_GRASS,
	EV_FOOTSTEP_GRAVEL,
	EV_FOOTSTEP_ROOF,
	EV_FOOTSTEP_SNOW,
	EV_FOOTSTEP_CARPET,
	EV_FOOTSPLASH,
	EV_FOOTWADE,
	EV_SWIM,
	EV_STEP_4,
	EV_STEP_8,
	EV_STEP_12,
	EV_STEP_16,
	EV_FALL_SHORT,
	EV_FALL_MEDIUM,
	EV_FALL_FAR,
	EV_FALL_NDIE,
	EV_FALL_DMG_10,
	EV_FALL_DMG_15,
	EV_FALL_DMG_25,
	EV_FALL_DMG_50,
	EV_JUMP_PAD, // boing sound at origin, jump sound on player
	EV_JUMP,
	EV_WATER_TOUCH,		   // foot touches
	EV_WATER_LEAVE,		   // foot leaves
	EV_WATER_UNDER,		   // head touches
	EV_WATER_CLEAR,		   // head leaves
	EV_ITEM_PICKUP,		   // normal item pickups are predictable
	EV_ITEM_PICKUP_QUIET,  // (SA) same, but don't play the default pickup sound as it was specified in the ent
	EV_GLOBAL_ITEM_PICKUP, // powerup / team sounds are broadcast to everyone
	EV_NOITEM,
	EV_NOAMMO,
	EV_EMPTYCLIP,
	EV_FILL_CLIP,
	EV_WEAP_OVERHEAT,
	EV_CHANGE_WEAPON,
	EV_FIRE_WEAPON,
	EV_FIRE_WEAPONB,
	EV_FIRE_WEAPON_LASTSHOT,
	EV_FIRE_QUICKGREN, // "Quickgrenade"
	EV_NOFIRE_UNDERWATER,
	EV_FIRE_WEAPON_MG42,
	EV_SUGGESTWEAP,		//----(SA)	added
	EV_GRENADE_SUICIDE, //----(SA)	added
	EV_USE_ITEM0,
	EV_USE_ITEM1,
	EV_USE_ITEM2,
	EV_USE_ITEM3,
	EV_USE_ITEM4,
	EV_USE_ITEM5,
	EV_USE_ITEM6,
	EV_USE_ITEM7,
	EV_USE_ITEM8,
	EV_USE_ITEM9,
	EV_USE_ITEM10,
	EV_USE_ITEM11,
	EV_USE_ITEM12,
	EV_USE_ITEM13,
	EV_USE_ITEM14,
	EV_USE_ITEM15,
	EV_ITEM_RESPAWN,
	EV_ITEM_POP,
	EV_PLAYER_TELEPORT_IN,
	EV_PLAYER_TELEPORT_OUT,
	EV_GRENADE_BOUNCE, // eventParm will be the soundindex
	EV_GENERAL_SOUND,
	EV_GLOBAL_SOUND, // no attenuation
	EV_BULLET_HIT_FLESH,
	EV_BULLET_HIT_WALL,
	EV_MISSILE_HIT,
	EV_MISSILE_MISS,
	EV_RAILTRAIL,
	EV_VENOM,
	EV_VENOMFULL,
	EV_BULLET,	 // otherEntity is the shooter
	EV_LOSE_HAT, //----(SA)
	EV_GIB_HEAD, // only blow off the head
	EV_PAIN,
	EV_CROUCH_PAIN,
	EV_DEATH1,
	EV_DEATH2,
	EV_DEATH3,
	EV_ENTDEATH, //----(SA)	added
	EV_OBITUARY,
	EV_POWERUP_QUAD,
	EV_POWERUP_BATTLESUIT,
	EV_POWERUP_REGEN,
	EV_GIB_PLAYER, // gib a previously living player
	EV_DEBUG_LINE,
	EV_STOPLOOPINGSOUND,
	EV_STOPSTREAMINGSOUND,
	EV_TAUNT,
	EV_SMOKE,
	EV_SPARKS,
	EV_SPARKS_ELECTRIC,
	EV_BATS,
	EV_BATS_UPDATEPOSITION,
	EV_BATS_DEATH,
	EV_EXPLODE,	  // func_explosive
	EV_EFFECT,	  // target_effect
	EV_MORTAREFX, // mortar firing
	EV_SPINUP,	  // JPW NERVE panzerfaust preamble
	EV_SNOW_ON,
	EV_SNOW_OFF,
	EV_MISSILE_MISS_SMALL,
	EV_MISSILE_MISS_LARGE,
	EV_WOLFKICK_HIT_FLESH,
	EV_WOLFKICK_HIT_WALL,
	EV_WOLFKICK_MISS,
	EV_SPIT_HIT,
	EV_SPIT_MISS,
	EV_SHARD,
	EV_JUNK,
	EV_EMITTER, //----(SA)	added // generic particle emitter that uses client-side particle scripts
	EV_OILPARTICLES,
	EV_OILSLICK,
	EV_OILSLICKREMOVE,
	EV_MG42EFX,
	EV_FLAMEBARREL_BOUNCE,
	EV_FLAKGUN1,
	EV_FLAKGUN2,
	EV_FLAKGUN3,
	EV_FLAKGUN4,
	EV_EXERT1,
	EV_EXERT2,
	EV_EXERT3,
	EV_SNOWFLURRY,
	EV_CONCUSSIVE,
	EV_DUST,
	EV_RUMBLE_EFX,
	EV_GUNSPARKS,
	EV_FLAMETHROWER_EFFECT,
	EV_SNIPER_SOUND,
	EV_POPUP,
	EV_POPUPBOOK,
	EV_GIVEPAGE,  //----(SA)	added
	EV_CLOSEMENU, //----(SA)	added
	EV_SPAWN_SPIRIT,

	EV_MAX_EVENTS // just added as an 'endcap'

} entity_event_t;

// animations
/*	// straight Q3A for reference (SA)
typedef enum {
	BOTH_DEATH1,
	BOTH_DEAD1,
	BOTH_DEATH2,
	BOTH_DEAD2,
	BOTH_DEATH3,
	BOTH_DEAD3,

	TORSO_GESTURE,

	TORSO_ATTACK,
	TORSO_ATTACK2,

	TORSO_DROP,
	TORSO_RAISE,

	TORSO_STAND,
	TORSO_STAND2,

	LEGS_WALKCR,
	LEGS_WALK,
	LEGS_RUN,
	LEGS_BACK,
	LEGS_SWIM,

	LEGS_JUMP,
	LEGS_LAND,

	LEGS_JUMPB,
	LEGS_LANDB,

	LEGS_IDLE,
	LEGS_IDLECR,

	LEGS_TURN,

	MAX_ANIMATIONS
} animNumber_t;
*/

// NOTE: this must be synched with the text list below

// new (10/18/00)
typedef enum {
	BOTH_DEATH1,
	BOTH_DEAD1,
	BOTH_DEAD1_WATER,
	BOTH_DEATH2,
	BOTH_DEAD2,
	BOTH_DEAD2_WATER,
	BOTH_DEATH3,
	BOTH_DEAD3,
	BOTH_DEAD3_WATER,

	BOTH_CLIMB,
	/*10*/ BOTH_CLIMB_DOWN,
	BOTH_CLIMB_DISMOUNT,

	BOTH_SALUTE,

	BOTH_PAIN1,		   // head
	BOTH_PAIN2,		   // chest
	BOTH_PAIN3,		   // groin
	BOTH_PAIN4,		   // right shoulder
	BOTH_PAIN5,		   // left shoulder
	BOTH_PAIN6,		   // right knee
	BOTH_PAIN7,		   // left knee
	/*20*/ BOTH_PAIN8, // dazed

	BOTH_GRAB_GRENADE,

	BOTH_ATTACK1,
	BOTH_ATTACK2,
	BOTH_ATTACK3,
	BOTH_ATTACK4,
	BOTH_ATTACK5,

	BOTH_EXTRA1,
	BOTH_EXTRA2,
	BOTH_EXTRA3,
	/*30*/ BOTH_EXTRA4,
	BOTH_EXTRA5,
	BOTH_EXTRA6,
	BOTH_EXTRA7,
	BOTH_EXTRA8,
	BOTH_EXTRA9,
	BOTH_EXTRA10,
	BOTH_EXTRA11,
	BOTH_EXTRA12,
	BOTH_EXTRA13,
	/*40*/ BOTH_EXTRA14,
	BOTH_EXTRA15,
	BOTH_EXTRA16,
	BOTH_EXTRA17,
	BOTH_EXTRA18,
	BOTH_EXTRA19,
	BOTH_EXTRA20,

	TORSO_GESTURE,
	TORSO_GESTURE2,
	TORSO_GESTURE3,
	/*50*/ TORSO_GESTURE4,

	TORSO_DROP,

	TORSO_RAISE, // (low)
	TORSO_ATTACK,
	TORSO_STAND,
	TORSO_STAND_ALT1,
	TORSO_STAND_ALT2,
	TORSO_READY,
	TORSO_RELAX,

	TORSO_RAISE2, // (high)
	/*60*/ TORSO_ATTACK2,
	TORSO_STAND2,
	TORSO_STAND2_ALT1,
	TORSO_STAND2_ALT2,
	TORSO_READY2,
	TORSO_RELAX2,

	TORSO_RAISE3, // (pistol)
	TORSO_ATTACK3,
	TORSO_STAND3,
	TORSO_STAND3_ALT1,
	/*70*/ TORSO_STAND3_ALT2,
	TORSO_READY3,
	TORSO_RELAX3,

	TORSO_RAISE4, // (shoulder)
	TORSO_ATTACK4,
	TORSO_STAND4,
	TORSO_STAND4_ALT1,
	TORSO_STAND4_ALT2,
	TORSO_READY4,
	TORSO_RELAX4,

	/*80*/ TORSO_RAISE5, // (throw)
	TORSO_ATTACK5,
	TORSO_ATTACK5B,
	TORSO_STAND5,
	TORSO_STAND5_ALT1,
	TORSO_STAND5_ALT2,
	TORSO_READY5,
	TORSO_RELAX5,

	TORSO_RELOAD1,		  // (low)
	TORSO_RELOAD2,		  // (high)
	/*90*/ TORSO_RELOAD3, // (pistol)
	TORSO_RELOAD4,		  // (shoulder)

	TORSO_MG42, // firing tripod mounted weapon animation

	TORSO_MOVE, // torso anim to play while moving and not firing (swinging arms type thing)
	TORSO_MOVE_ALT,

	TORSO_EXTRA,
	TORSO_EXTRA2,
	TORSO_EXTRA3,
	TORSO_EXTRA4,
	TORSO_EXTRA5,
	/*100*/ TORSO_EXTRA6,
	TORSO_EXTRA7,
	TORSO_EXTRA8,
	TORSO_EXTRA9,
	TORSO_EXTRA10,

	LEGS_WALKCR,
	LEGS_WALKCR_BACK,
	LEGS_WALK,
	LEGS_RUN,
	LEGS_BACK,
	/*110*/ LEGS_SWIM,
	LEGS_SWIM_IDLE,

	LEGS_JUMP,
	LEGS_JUMPB,
	LEGS_LAND,

	LEGS_IDLE,
	LEGS_IDLE_ALT, // LEGS_IDLE2
	LEGS_IDLECR,

	LEGS_TURN,

	LEGS_BOOT, // kicking animation

	/*120*/ LEGS_EXTRA1,
	LEGS_EXTRA2,
	LEGS_EXTRA3,
	LEGS_EXTRA4,
	LEGS_EXTRA5,
	LEGS_EXTRA6,
	LEGS_EXTRA7,
	LEGS_EXTRA8,
	LEGS_EXTRA9,
	LEGS_EXTRA10,

	/*130*/ MAX_ANIMATIONS
} animNumber_t;

// text represenation for scripting
extern char* animStrings[];	   // defined in bg_misc.c
extern char* animStringsOld[]; // defined in bg_misc.c

typedef enum {
	WEAP_IDLE1,
	WEAP_IDLE2,
	WEAP_ATTACK1,
	WEAP_ATTACK2,
	WEAP_ATTACK_LASTSHOT, // used when firing the last round before having an empty clip.
	WEAP_DROP,
	WEAP_RAISE,
	WEAP_RELOAD1,
	WEAP_RELOAD2,
	WEAP_RELOAD3,
	WEAP_ALTSWITCHFROM, // switch from alt fire mode weap (scoped/silencer/etc)
	WEAP_ALTSWITCHTO,	// switch to alt fire mode weap
	MAX_WP_ANIMATIONS
} weapAnimNumber_t;

#define ANIMFL_LADDERANIM 0x1
#define ANIMFL_FIRINGANIM 0x2

typedef struct animation_s {
	char  name[MAX_QPATH];
	int	  firstFrame;
	int	  numFrames;
	int	  loopFrames;  // 0 to numFrames
	int	  frameLerp;   // msec between frames
	int	  initialLerp; // msec to get to first frame
	int	  moveSpeed;
	int	  animBlend; // take this long to blend to next anim
	int	  priority;
	//
	// derived
	//
	int	  duration;
	int	  nameHash;
	int	  flags;
	int	  movetype;
	float stepGap;
} animation_t;

// Ridah, head animations
typedef enum {
	HEAD_NEUTRAL_CLOSED,
	HEAD_NEUTRAL_A,
	HEAD_NEUTRAL_O,
	HEAD_NEUTRAL_I,
	HEAD_NEUTRAL_E,
	HEAD_HAPPY_CLOSED,
	HEAD_HAPPY_O,
	HEAD_HAPPY_I,
	HEAD_HAPPY_E,
	HEAD_HAPPY_A,
	HEAD_ANGRY_CLOSED,
	HEAD_ANGRY_O,
	HEAD_ANGRY_I,
	HEAD_ANGRY_E,
	HEAD_ANGRY_A,

	MAX_HEAD_ANIMS
} animHeadNumber_t;

typedef struct headAnimation_s {
	int firstFrame;
	int numFrames;
} headAnimation_t;
// done.

// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define ANIM_TOGGLEBIT ( 1 << ( ANIM_BITS - 1 ) )

typedef enum {
	TEAM_FREE,
	TEAM_RED,
	TEAM_BLUE,
	TEAM_SPECTATOR,

	TEAM_NUM_TEAMS
} team_t;

// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME 1000

// How many players on the overlay
#define TEAM_MAXOVERLAY			  8

// means of death
typedef enum {
	MOD_UNKNOWN,
	MOD_SHOTGUN,
	MOD_GAUNTLET,
	MOD_MACHINEGUN,
	MOD_GRENADE,
	MOD_GRENADE_SPLASH,
	MOD_ROCKET,
	MOD_ROCKET_SPLASH,
	MOD_RAILGUN,
	MOD_LIGHTNING,
	MOD_BFG,
	MOD_BFG_SPLASH,
	MOD_KNIFE,
	MOD_KNIFE2,
	MOD_KNIFE_STEALTH,
	MOD_LUGER,
	MOD_COLT,
	MOD_MP40,
	MOD_THOMPSON,
	MOD_STEN,
	MOD_MAUSER,
	MOD_SNIPERRIFLE,
	MOD_GARAND,
	MOD_SNOOPERSCOPE,
	MOD_SILENCER, //----(SA)
	MOD_AKIMBO,	  //----(SA)
	MOD_BAR,	  //----(SA)
	MOD_FG42,
	MOD_FG42SCOPE,
	MOD_PANZERFAUST,
	MOD_ROCKET_LAUNCHER,
	MOD_GRENADE_LAUNCHER,
	MOD_VENOM,
	MOD_VENOM_FULL,
	MOD_FLAMETHROWER,
	MOD_TESLA,
	MOD_SPEARGUN,
	MOD_SPEARGUN_CO2,
	MOD_GRENADE_PINEAPPLE,
	MOD_CROSS,
	MOD_MORTAR,
	MOD_MORTAR_SPLASH,
	MOD_KICKED,
	MOD_GRABBER,
	MOD_DYNAMITE,
	MOD_DYNAMITE_SPLASH,
	MOD_AIRSTRIKE, // JPW NERVE
	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH,
	MOD_TELEFRAG,
	MOD_FALLING,
	MOD_SUICIDE,
	MOD_TARGET_LASER,
	MOD_TRIGGER_HURT,
	MOD_GRAPPLE,
	MOD_EXPLOSIVE,
	MOD_POISONGAS,
	MOD_ZOMBIESPIT,
	MOD_ZOMBIESPIT_SPLASH,
	MOD_ZOMBIESPIRIT,
	MOD_ZOMBIESPIRIT_SPLASH,
	MOD_LOPER_LEAP,
	MOD_LOPER_GROUND,
	MOD_LOPER_HIT,

	// JPW NERVE multiplayer class-specific MODs
	MOD_LT_ARTILLERY,
	MOD_LT_AIRSTRIKE,
	MOD_ENGINEER, // not sure if we'll use
	MOD_MEDIC,	  // these like this or not
	//
	MOD_BAT

} meansOfDeath_t;

//---------------------------------------------------------

// gitem_t->type
typedef enum {
	IT_BAD,
	IT_WEAPON, // EFX: rotate + upscale + minlight

	IT_AMMO,	// EFX: rotate
	IT_ARMOR,	// EFX: rotate + minlight
	IT_HEALTH,	// EFX: static external sphere + rotating internal
	IT_POWERUP, // instant on, timer based
	// EFX: rotate + external ring that rotates
	IT_HOLDABLE, // single use, holdable item
	// EFX: rotate + bob
	IT_KEY,
	IT_TREASURE,  // gold bars, etc.  things that can be picked up and counted for a tally at end-level
	IT_CLIPBOARD, // 'clipboard' used as a general term for 'popup' items where you pick up the item and it pauses and opens a menu
	IT_TEAM
} itemType_t;

#define MAX_ITEM_MODELS 5
#define MAX_ITEM_ICONS	4

// JOSEPH 4-17-00
typedef struct gitem_s {
	char*	   classname; // spawning name
	char*	   pickup_sound;
	char*	   world_model[MAX_ITEM_MODELS];

	char*	   icon;
	char*	   ammoicon;
	char*	   pickup_name; // for printing on pickup

	int		   quantity; // for ammo how much, or duration of powerup (value not necessary for ammo/health.  that value set in gameskillnumber[] below)
	itemType_t giType;	 // IT_* flags

	int		   giTag;

	int		   giAmmoIndex; // type of weapon ammo this uses.  (ex. WP_MP40 and WP_LUGER share 9mm ammo, so they both have WP_LUGER for giAmmoIndex)
	int		   giClipIndex; // which clip this weapon uses.  this allows the sniper rifle to use the same clip as the garand, etc.

	char*	   precaches; // string of all models and images this item will use
	char*	   sounds;	  // string of all sounds this item will use

	int		   gameskillnumber[4];
} gitem_t;
// END JOSEPH

// included in both the game dll and the client
extern gitem_t bg_itemlist[];
extern int	   bg_numItems;

/*!
	\brief Finds and returns a game item by its pickup name from the global item list

	This function searches through the global item list starting from the second element to find an item that matches the provided pickup name. The comparison is case-insensitive. If no matching item
   is found, the function returns NULL. This function is commonly used to retrieve item definitions for weapon and powerup registration

	\param pickupName The name of the item to search for
	\return A pointer to the matching gitem_t structure if found, or NULL if no item with the specified pickup name exists
*/
gitem_t*	   BG_FindItem( const char* pickupName );

/*!
	\brief Finds and returns a pointer to a gitem_t structure that matches the given item name, searching by pickup name or classname.

	This function iterates through the global item list to locate an item that matches the specified name either by its pickup_name or classname. It performs case-insensitive comparisons using
   Q_stricmp and Q_strcasecmp. If no matching item is found, an error message is printed to the console and NULL is returned.

	\param name The name of the item to search for, which can match either the pickup_name or classname of items in the global list
	\return A pointer to the gitem_t structure that matches the specified name, or NULL if no match is found
*/
gitem_t*	   BG_FindItem2( const char* name );

/*!
	\brief Returns the item definition corresponding to a specified weapon type.

	This function uses a lookup table to efficiently find the gitem_t structure that corresponds to a given weapon_t value. The lookup table is initialized once on the first call to this function. The
   function performs bounds checking on the weapon parameter and will generate an error if the weapon is out of range or if no item is found for the specified weapon. The function is designed to work
   with the game's item and weapon systems to facilitate weapon-based item spawning and ammunition management.

	\param weapon The weapon type to look up
	\return A pointer to the gitem_t structure that represents the item corresponding to the specified weapon type
	\throws ERR_DROP if the weapon parameter is out of range or if no item is found for the specified weapon
*/
gitem_t*	   BG_FindItemForWeapon( weapon_t weapon );

/*!
	\brief Finds the item definition corresponding to a given powerup type

	This function searches through the global item list to locate an item that matches the specified powerup type. It checks both powerup items and team items, returning the first match found. The
   function is used to retrieve item information for displaying powerup icons and other UI elements.

	\param pw The powerup type to search for
	\return A pointer to the gitem_t structure matching the powerup type, or NULL if no match is found
*/
gitem_t*	   BG_FindItemForPowerup( powerup_t pw );

/*!
	\brief Returns a pointer to the game item structure corresponding to a given holdable item type

	This function searches through the global item list to find an item that matches the specified holdable type. It iterates through all registered items and checks if the item type is IT_HOLDABLE
   and if its tag matches the provided holdable type. The function returns a pointer to the matching item structure or NULL if no match is found. The returned pointer is a direct reference to an item
   in the global item list.

	\param pw The holdable item type to search for
	\return Pointer to the gitem_t structure for the matching holdable item, or NULL if no match is found
*/
gitem_t*	   BG_FindItemForHoldable( holdable_t pw );

/*!
	\brief Finds and returns a pointer to the item in the item list that corresponds to the specified ammo type

	This function searches through the global item list for an item that is of type IT_AMMO and has a giAmmoIndex matching the provided ammo parameter. If such an item is found, a pointer to it is
   returned. If no matching item is found, the function triggers a fatal error using Com_Error with an ERR_DROP code

	\param ammo The ammo type index to search for in the item list
	\return A pointer to the gitem_t structure representing the item that matches the specified ammo type
	\throws ERR_DROP error when no item matching the specified ammo type is found in the item list
*/
gitem_t*	   BG_FindItemForAmmo( int ammo );

/*!
	\brief Finds a key item in the item list that matches the specified key type and returns a pointer to it

	This function searches through the global item list to find an item that is a key (IT_KEY) and matches the specified key type. If found, it updates the provided index pointer with the item's
   position in the list and returns a pointer to the item. If no matching key is found, it triggers a fatal error.

	\param k The key type to search for
	\param index Pointer to an integer that will be set to the index of the found item
	\return A pointer to the gitem_t structure representing the key item that matches the specified key type
	\throws ERR_DROP error when the specified key type is not found in the item list
*/
gitem_t*	   BG_FindItemForKey( wkey_t k, int* index );

/*!
	\brief Returns the ammo index associated with the specified weapon

	This function looks up the ammo index for a given weapon type using a precomputed lookup table. The table is initialized once on first call and then reused for subsequent calls. The function
   performs bounds checking on the weapon parameter and will generate an error if the weapon is out of range. The ammo index returned can be used to access ammo quantities in player state arrays.

	\param weapon The weapon type to look up the ammo index for
	\return The ammo index associated with the specified weapon type
	\throws ERR_DROP error when weapon parameter is out of range
*/
weapon_t	   BG_FindAmmoForWeapon( weapon_t weapon );

/*!
	\brief Returns the clip index for a given weapon by looking up a precomputed table

	This function uses a static lookup table to quickly determine the clip index associated with a specific weapon type. The table is initialized once on the first call to the function, and subsequent
   calls retrieve the clip index directly from the table. The function performs bounds checking to ensure the weapon parameter is within valid range, and will trigger a fatal error if an invalid
   weapon is specified.

	\param weapon The weapon type to look up the clip index for
	\return The clip index associated with the specified weapon type
	\throws ERR_DROP if the weapon parameter is out of range
*/
weapon_t	   BG_FindClipForWeapon( weapon_t weapon );

/*!
	\brief Determines whether the left or right hand should fire in an akimbo weapon sequence based on clip ammo counts

	This function implements the logic for alternating fire between two weapons in an akimbo setup. It evaluates the ammo counts in both the akimbo weapon and the colt weapon to determine which weapon
   should fire next. The function returns true when the left hand (akimbo weapon) should fire, and false when the right hand (colt weapon) should fire. The function is specifically designed for the
   WP_AKIMBO weapon type and will return false for any other weapon. When clips are disabled via dmflags 64, the function behavior may be affected as noted in the implementation. The alternating
   pattern is determined by the sum of both clip counts being odd or even.

	\param weapon The weapon type identifier
	\param akimboClip Ammo count in the akimbo weapon clip
	\param coltClip Ammo count in the colt weapon clip
	\return True if the left hand (akimbo weapon) should fire, false if the right hand (colt weapon) should fire
*/
qboolean	   BG_AkimboFireSequence( int weapon, int akimboClip, int coltClip );
// qboolean BG_AkimboFireSequence	( playerState_t *ps );	//----(SA)	added

#define ITEM_INDEX( x ) ( ( x ) - bg_itemlist )

/*!
	\brief Determines whether a player can pick up a specific item based on their current state and the item type.

	This function evaluates if a player can grab an item from the game world by checking various conditions based on item type. For weapons, it checks if the player has the weapon or if they have
   enough ammunition to pick up more. For ammo, it checks if the player has enough space in their ammo inventory. For armor, it checks if the player's armor value is already at maximum. For health
   items, it checks if the player's health is already at maximum. For powerups, it checks if the player already has full fatigue resistance. For team items like flags, it verifies that the player is
   on the correct team and can legally pick up the flag. For holdable items, clipboards, keys, treasure, and other special items, it allows pickup in all cases. The function also supports multiplayer
   logic where certain classes can only pick up specific weapons.

	\param ent The entity state of the item being picked up
	\param ps The player state of the player attempting to pick up the item
	\return True if the player can pick up the item, false otherwise
*/
qboolean BG_CanItemBeGrabbed( const entityState_t* ent, const playerState_t* ps );

// g_dmflags->integer flags
#define DF_NO_FALLING	 8
#define DF_FIXED_FOV	 16
#define DF_NO_FOOTSTEPS	 32
#define DF_NO_WEAPRELOAD 64

// content masks
#define MASK_ALL		 ( -1 )
#define MASK_SOLID		 ( CONTENTS_SOLID )
#define MASK_PLAYERSOLID ( CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_BODY )
#define MASK_DEADSOLID	 ( CONTENTS_SOLID | CONTENTS_PLAYERCLIP )
#define MASK_WATER		 ( CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_SLIME )
// #define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define MASK_OPAQUE		 ( CONTENTS_SOLID | CONTENTS_LAVA ) //----(SA)	modified since slime is no longer deadly
#define MASK_SHOT		 ( CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_CLIPSHOT )
#define MASK_MISSILESHOT ( MASK_SHOT | CONTENTS_MISSILECLIP )
#define MASK_AISIGHT	 ( CONTENTS_SOLID | CONTENTS_AI_NOSIGHT )

//
// entityState_t->eType
//
typedef enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_MOVER,
	ET_BEAM,
	ET_PORTAL,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_INVISIBLE,
	ET_GRAPPLE, // grapple hooked on wall

	//---- (SA) Wolf
	ET_EXPLOSIVE, // brush that will break into smaller bits when damaged
	ET_TESLA_EF,
	ET_SPOTLIGHT_EF,
	ET_EFFECT3,
	ET_ALARMBOX,
	ET_CORONA,
	ET_TRAP,

	ET_GAMEMODEL,  // misc_gamemodel.  similar to misc_model, but it's a dynamic model so we have LOD
	ET_FOOTLOCKER, //----(SA)	added
	ET_LEAKY,	   //----(SA)	added
	ET_MG42,	   //----(SA)	why didn't we do /this/ earlier...
	//---- end

	ET_ZOMBIESPIT,
	ET_FLAMEBARREL,
	ET_ZOMBIESPIRIT,

	ET_FP_PARTS,

	// FIRE PROPS
	ET_FIRE_COLUMN,
	ET_FIRE_COLUMN_SMOKE,
	ET_RAMJET,

	ET_EXPLO_PART,

	ET_CROWBAR,

	ET_PROP,
	ET_BAT,

	ET_AI_EFFECT,

	ET_CAMERA,
	ET_MOVERSCALED,

	ET_RUMBLE,

	ET_SPIRIT_SPAWNER,

	ET_FLAMETHROWER_PROP,

	ET_LIGHT,
	ET_LIGHTJUNIOR,

	ET_EVENTS // any of the EV_* events can be added freestanding
			  // by setting eType to ET_EVENTS + eventNum
			  // this avoids having to set eFlags and eventNum
} entityType_t;

// cursorhints (stored in ent->s.dmgFlags since that's only used for players at the moment)
typedef enum {
	HINT_NONE,		// reserved
	HINT_FORCENONE, // reserved
	HINT_PLAYER,
	HINT_ACTIVATE,
	HINT_NOACTIVATE,
	HINT_DOOR,
	HINT_DOOR_ROTATING,
	HINT_DOOR_LOCKED,
	HINT_DOOR_ROTATING_LOCKED,
	HINT_MG42,
	HINT_BREAKABLE, // 10
	HINT_BREAKABLE_DYNAMITE,
	HINT_CHAIR,
	HINT_ALARM,
	HINT_HEALTH,
	HINT_TREASURE,
	HINT_KNIFE,
	HINT_LADDER,
	HINT_BUTTON,
	HINT_WATER,
	HINT_CAUTION, // 20
	HINT_DANGER,
	HINT_SECRET,
	HINT_QUESTION,
	HINT_EXCLAMATION,
	HINT_CLIPBOARD,
	HINT_WEAPON,
	HINT_AMMO,
	HINT_ARMOR,
	HINT_POWERUP,
	HINT_HOLDABLE, // 30
	HINT_INVENTORY,
	HINT_SCENARIC,
	HINT_EXIT,
	HINT_NOEXIT,
	HINT_EXIT_FAR,
	HINT_NOEXIT_FAR,
	HINT_PLYR_FRIEND,
	HINT_PLYR_NEUTRAL,
	HINT_PLYR_ENEMY,
	HINT_PLYR_UNKNOWN, // 40
	HINT_BUILD,

	HINT_BAD_USER, // invisible user with no target

	HINT_NUM_HINTS
} hintType_t;

/*!
	\brief Evaluates the position of a trajectory at a specific time.

	This function computes the position of a trajectory at the given time by evaluating the trajectory type and applying the appropriate mathematical calculations. It supports various trajectory types
   including linear, sine, gravity-based, and acceleration/deceleration patterns. The result is stored in the provided vector.

	\param tr Pointer to the trajectory structure containing the trajectory data
	\param atTime The time at which to evaluate the trajectory, in milliseconds
	\param result Output vector that will contain the computed position
	\throws Com_Error if an invalid trajectory type is encountered
*/
void	 BG_EvaluateTrajectory( const trajectory_t* tr, int atTime, vec3_t result );

/*!
	\brief Evaluates the trajectory delta for a given time and trajectory type, storing the result in a 3D vector

	This function calculates the velocity or movement delta for a trajectory at a specific time based on the trajectory type. It handles various trajectory types including stationary, linear, sine,
   gravity-based movements, and acceleration/deceleration. The result is stored in the provided 3D vector. For some trajectory types, the function may modify the result in-place or clear it entirely
   depending on the specific logic implemented for each case.

	\param tr Pointer to the trajectory structure containing trajectory data
	\param atTime The time at which to evaluate the trajectory
	\param result Output 3D vector to store the calculated trajectory delta
	\throws ERROR: This function calls Com_Error with ERR_DROP when an unknown trajectory type is encountered
*/
void	 BG_EvaluateTrajectoryDelta( const trajectory_t* tr, int atTime, vec3_t result );

/*!
	\brief Computes a modified direction vector for marking surfaces based on input direction and normal.

	This function calculates an output direction vector that ensures a mark is properly aligned with the impact surface. It takes into account the input direction and surface normal to determine an
   appropriate marking direction. The function adjusts the direction to maintain a minimum dot product with the normal, ensuring the mark appears on the intended surface. Special handling is included
   for normals with a z-component greater than 0.8, where the minimum dot product is increased to 0.7.

	\param dir Input direction vector used for calculating the mark direction
	\param normal Surface normal vector at the impact point
	\param out Output vector containing the calculated mark direction
*/
void	 BG_GetMarkDir( const vec3_t dir, const vec3_t normal, vec3_t out );

/*!
	\brief Adds a predictable event to the player state for prediction purposes.

	This function stores a new event and its parameter in the player state's event array. The event is added at the position indicated by the event sequence counter, which is then incremented. If the
   debug console variable "showevents" is enabled, the function will print information about the added event to the console. The function uses a circular buffer approach for storing events by masking
   the event sequence with (MAX_EVENTS - 1).

	\param newEvent The event identifier to be added
	\param eventParm The parameter associated with the event
	\param ps Pointer to the player state structure where the event will be stored
*/
void	 BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t* ps );

/*!
	\brief Converts a player state structure to an entity state structure for network synchronization and rendering.

	This function maps the player state data to the corresponding entity state fields, handling various game states such as intermission, spectator mode, and limbo. It sets up the entity type based on
   health and game state, copies origin and view angles, and handles event processing and power-up states. The function also manages the interpolation of movement and animation data, ensuring proper
   client-side rendering and prediction.

	\param ps Pointer to the player state structure containing the player's current state
	\param s Pointer to the entity state structure to be filled with player data
	\param snap Flag indicating whether to snap coordinates to integer values for network transmission
*/
void	 BG_PlayerStateToEntityState( playerState_t* ps, entityState_t* s, qboolean snap );

/*!
	\brief Converts a player state to an entity state with extrapolation for network updates

	This function transforms a player state structure into an entity state structure, applying extrapolation for network interpolation. It handles different player states such as intermission,
   spectator, and limbo modes by setting appropriate entity types. The function also processes player movement data, animations, events, and power-up states. It supports both snapshot and non-snapshot
   modes for coordinate precision, and manages event sequencing for client-server synchronization.

	\param ps Pointer to the player state structure to convert
	\param s Pointer to the destination entity state structure
	\param time Time value for linear prediction
	\param snap Flag indicating whether to snap coordinates to integer values
*/
void	 BG_PlayerStateToEntityStateExtraPolate( playerState_t* ps, entityState_t* s, int time, qboolean snap );

/*!
	\brief Checks whether a player touches a given item at a specific time

	This function evaluates if a player's position intersects with an item's position at a specified time. It uses the item's trajectory to determine the item's location and compares it with the
   player's origin. The comparison considers a bounding box around the item with specific dimensions to determine if the player is close enough to touch it. The function ignores differences in ducked
   states when making the comparison.

	\param ps Pointer to the player state structure containing the player's origin
	\param item Pointer to the item's state structure containing the item's trajectory
	\param atTime The time at which to evaluate the item's position
	\return True if the player is within the bounding box of the item, false otherwise
*/
qboolean BG_PlayerTouchesItem( playerState_t* ps, entityState_t* item, int atTime );
qboolean BG_PlayerSeesItem( playerState_t* ps, entityState_t* item, int atTime );

/*!
	\brief Clips a velocity vector against a normal vector with optional overbounce scaling.

	This function modifies a velocity vector by reflecting it off a surface defined by a normal vector. The reflection is adjusted based on the overbounce factor, which determines how much the
   velocity should be scaled during the reflection. When the dot product of the input velocity and the normal is negative, the overbounce factor is applied to the negative dot product, otherwise it's
   applied to the positive dot product. The result is stored in the output vector.

	\param in The input velocity vector to be clipped
	\param normal The normal vector of the surface to clip against
	\param out The output velocity vector after clipping
	\param overbounce The overbounce scaling factor applied during clipping
*/
void	 PM_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float overbounce );

#define ARENAS_PER_TIER 4
#define MAX_ARENAS		64
#define MAX_ARENAS_TEXT 8192

#define MAX_BOTS		64
#define MAX_BOTS_TEXT	8192

typedef enum {
	FOOTSTEP_NORMAL,
	FOOTSTEP_BOOT,
	FOOTSTEP_FLESH,
	FOOTSTEP_MECH,
	FOOTSTEP_ENERGY,
	FOOTSTEP_METAL,
	FOOTSTEP_WOOD,
	FOOTSTEP_GRASS,
	FOOTSTEP_GRAVEL,
	// END JOSEPH
	FOOTSTEP_SPLASH,

	FOOTSTEP_ROOF,
	FOOTSTEP_SNOW,
	FOOTSTEP_CARPET, //----(SA)	added

	FOOTSTEP_ELITE_STEP,
	FOOTSTEP_ELITE_METAL,
	FOOTSTEP_ELITE_ROOF,
	FOOTSTEP_ELITE_WOOD,
	FOOTSTEP_ELITE_GRAVEL,

	FOOTSTEP_SUPERSOLDIER_METAL,
	FOOTSTEP_SUPERSOLDIER_GRASS,
	FOOTSTEP_SUPERSOLDIER_GRAVEL,
	FOOTSTEP_SUPERSOLDIER_STEP,
	FOOTSTEP_SUPERSOLDIER_WOOD,

	FOOTSTEP_PROTOSOLDIER_METAL,
	FOOTSTEP_PROTOSOLDIER_GRASS,
	FOOTSTEP_PROTOSOLDIER_GRAVEL,
	FOOTSTEP_PROTOSOLDIER_STEP,
	FOOTSTEP_PROTOSOLDIER_WOOD,

	FOOTSTEP_LOPER_METAL,
	FOOTSTEP_LOPER_STEP,
	FOOTSTEP_LOPER_WOOD,

	FOOTSTEP_ZOMBIE_GRAVEL,
	FOOTSTEP_ZOMBIE_STEP,
	FOOTSTEP_ZOMBIE_WOOD,

	FOOTSTEP_BEAST,

	FOOTSTEP_HEINRICH,

	FOOTSTEP_TOTAL
} footstep_t;

//----(SA)	added
typedef enum { GRENBOUNCE_DIRT, GRENBOUNCE_WOOD, GRENBOUNCE_METAL, GRENBOUNCE_DEFAULT, GRENBOUNCE_TOTAL } grenbounde_t;
//----(SA)	added

//==================================================================
// New Animation Scripting Defines

#define MAX_ANIMSCRIPT_MODELS		   32 // allocated dynamically, so limit is scalable
#define MAX_ANIMSCRIPT_ITEMS_PER_MODEL 256
#define MAX_MODEL_ANIMATIONS		   256 // animations per model
#define MAX_ANIMSCRIPT_ANIMCOMMANDS	   8
#define MAX_ANIMSCRIPT_ITEMS		   32

// NOTE: these must all be in sync with string tables in bg_animation.c

typedef enum {
	ANIM_MT_UNUSED,
	ANIM_MT_IDLE,
	ANIM_MT_IDLECR,
	ANIM_MT_WALK,
	ANIM_MT_WALKBK,
	ANIM_MT_WALKCR,
	ANIM_MT_WALKCRBK,
	ANIM_MT_RUN,
	ANIM_MT_RUNBK,
	ANIM_MT_SWIM,
	ANIM_MT_SWIMBK,
	ANIM_MT_STRAFERIGHT,
	ANIM_MT_STRAFELEFT,
	ANIM_MT_TURNRIGHT,
	ANIM_MT_TURNLEFT,
	ANIM_MT_CLIMBUP,
	ANIM_MT_CLIMBDOWN,

	NUM_ANIM_MOVETYPES
} scriptAnimMoveTypes_t;

typedef enum {
	ANIM_ET_PAIN,
	ANIM_ET_DEATH,
	ANIM_ET_FIREWEAPON,
	ANIM_ET_JUMP,
	ANIM_ET_JUMPBK,
	ANIM_ET_LAND,
	ANIM_ET_DROPWEAPON,
	ANIM_ET_RAISEWEAPON,
	ANIM_ET_CLIMB_MOUNT,
	ANIM_ET_CLIMB_DISMOUNT,
	ANIM_ET_RELOAD,
	ANIM_ET_PICKUPGRENADE,
	ANIM_ET_KICKGRENADE,
	ANIM_ET_QUERY,
	ANIM_ET_INFORM_FRIENDLY_OF_ENEMY,
	ANIM_ET_KICK,
	ANIM_ET_REVIVE,
	ANIM_ET_FIRSTSIGHT,
	ANIM_ET_ROLL,
	ANIM_ET_FLIP,
	ANIM_ET_DIVE,
	ANIM_ET_PRONE_TO_CROUCH,
	ANIM_ET_BULLETIMPACT,
	ANIM_ET_INSPECTSOUND,
	ANIM_ET_SECONDLIFE,

	NUM_ANIM_EVENTTYPES
} scriptAnimEventTypes_t;

typedef enum {
	ANIM_BP_UNUSED,
	ANIM_BP_LEGS,
	ANIM_BP_TORSO,
	ANIM_BP_BOTH,

	NUM_ANIM_BODYPARTS
} animBodyPart_t;

typedef enum {
	ANIM_COND_WEAPON,
	ANIM_COND_ENEMY_POSITION,
	ANIM_COND_ENEMY_WEAPON,
	ANIM_COND_UNDERWATER,
	ANIM_COND_MOUNTED,
	ANIM_COND_MOVETYPE,
	ANIM_COND_UNDERHAND,
	ANIM_COND_LEANING,
	ANIM_COND_IMPACT_POINT,
	ANIM_COND_CROUCHING,
	ANIM_COND_STUNNED,
	ANIM_COND_FIRING,
	ANIM_COND_SHORT_REACTION,
	ANIM_COND_ENEMY_TEAM,
	ANIM_COND_PARACHUTE,
	ANIM_COND_CHARGING,
	ANIM_COND_SECONDLIFE,
	ANIM_COND_HEALTH_LEVEL,
	ANIM_COND_DEFENSE,
	ANIM_COND_SPECIAL_CONDITION,

	NUM_ANIM_CONDITIONS
} scriptAnimConditions_t;

//-------------------------------------------------------------------

typedef struct {
	char* string;
	int	  hash;
} animStringItem_t;

typedef struct {
	int index;	  // reference into the table of possible conditionals
	int value[2]; // can store anything from weapon bits, to position enums, etc
} animScriptCondition_t;

typedef struct {
	short int bodyPart[2];	// play this animation on legs/torso/both
	short int animIndex[2]; // animation index in our list of animations
	short int animDuration[2];
	short int soundIndex;
	short int accShowBits; //----(SA)	added
	short int accHideBits; //----(SA)	added
} animScriptCommand_t;

typedef struct {
	int					  numConditions;
	animScriptCondition_t conditions[NUM_ANIM_CONDITIONS];
	int					  numCommands;
	animScriptCommand_t	  commands[MAX_ANIMSCRIPT_ANIMCOMMANDS];
} animScriptItem_t;

typedef struct {
	int				  numItems;
	animScriptItem_t* items[MAX_ANIMSCRIPT_ITEMS]; // pointers into a global list of items
} animScript_t;

typedef struct {
	char			 modelname[MAX_QPATH]; // name of the model

	// parsed from the start of the cfg file
	gender_t		 gender;
	footstep_t		 footsteps;
	vec3_t			 headOffset;
	int				 version;
	qboolean		 isSkeletal;

	// parsed from cfg file
	animation_t		 animations[MAX_MODEL_ANIMATIONS]; // anim names, frame ranges, etc
	headAnimation_t	 headAnims[MAX_HEAD_ANIMS];
	int				 numAnimations, numHeadAnims;

	// parsed from script file
	animScript_t	 scriptAnims[MAX_AISTATES][NUM_ANIM_MOVETYPES];		  // locomotive anims, etc
	animScript_t	 scriptCannedAnims[MAX_AISTATES][NUM_ANIM_MOVETYPES]; // played randomly
	animScript_t	 scriptStateChange[MAX_AISTATES][MAX_AISTATES];		  // state change events
	animScript_t	 scriptEvents[NUM_ANIM_EVENTTYPES];					  // events that trigger special anims

	// global list of script items for this model
	animScriptItem_t scriptItems[MAX_ANIMSCRIPT_ITEMS_PER_MODEL];
	int				 numScriptItems;

} animModelInfo_t;

// this is the main structure that is duplicated on the client and server
typedef struct {
	int				 clientModels[MAX_CLIENTS]; // so we know which model each client is using
	animModelInfo_t* modelInfo[MAX_ANIMSCRIPT_MODELS];
	int				 clientConditions[MAX_CLIENTS][NUM_ANIM_CONDITIONS][2];
	//
	// pointers to functions from the owning module
	//
	int ( *soundIndex )( const char* name );
	void ( *playSound )( int soundIndex, vec3_t org, int clientNum );
} animScriptData_t;

//------------------------------------------------------------------
// Conditional Constants

typedef enum {
	POSITION_UNUSED,
	POSITION_BEHIND,
	POSITION_INFRONT,
	POSITION_RIGHT,
	POSITION_LEFT,

	NUM_ANIM_COND_POSITIONS
} animScriptPosition_t;

typedef enum {
	MOUNTED_UNUSED,
	MOUNTED_MG42,

	NUM_ANIM_COND_MOUNTED
} animScriptMounted_t;

typedef enum {
	LEANING_UNUSED,
	LEANING_RIGHT,
	LEANING_LEFT,

	NUM_ANIM_COND_LEANING
} animScriptLeaning_t;

typedef enum {
	IMPACTPOINT_UNUSED,
	IMPACTPOINT_HEAD,
	IMPACTPOINT_CHEST,
	IMPACTPOINT_GUT,
	IMPACTPOINT_GROIN,
	IMPACTPOINT_SHOULDER_RIGHT,
	IMPACTPOINT_SHOULDER_LEFT,
	IMPACTPOINT_KNEE_RIGHT,
	IMPACTPOINT_KNEE_LEFT,

	NUM_ANIM_COND_IMPACTPOINT
} animScriptImpactPoint_t;

/*!
	\brief Retrieves animation model information for a given model name.

	This function searches through the global animation script data to find and return the animation model information that corresponds to the specified model name. It iterates through all available
   model info entries and performs a case-insensitive string comparison to match the provided model name. If no matching entry is found, it returns NULL.

	\param modelname The name of the model for which to retrieve animation information
	\return A pointer to the animation model information structure if a match is found, otherwise NULL
*/
animModelInfo_t*		BG_ModelInfoForModelname( char* modelname );

/*!
	\brief Parses animation configuration data from a given input string and populates the animation model information structure.

	This function processes a configuration file or input string to extract animation parameters and store them in the provided animation model information structure. It handles parsing of various
   animation-related settings such as footstep types, head offset, gender, version, and skeletal model flags. The function also reads animation data including frame counts, loop frames, frame lerp
   values, move speeds, blending, and priority settings. It supports both legacy and newer animation configuration formats based on the version number.

	\param animModelInfo Pointer to the animation model information structure to be populated
	\param filename Name of the configuration file being parsed
	\param input Input string containing the animation configuration data
	\return qboolean indicating whether the parsing was successful
*/
qboolean				BG_AnimParseAnimConfig( animModelInfo_t* animModelInfo, const char* filename, const char* input );

/*!
	\brief Parses an animation script file and populates the animation model and script data structures for a given client.

	This function processes an animation script input to initialize animation data structures for a specific client. It handles parsing of defines, animation states, movement types, and animation
   items. The function manages indentation levels to structure the parsing of nested animation data and supports both regular and canned animations. It initializes global parsing state variables and
   processes tokens to build the internal animation script structure used by the game engine.

	\param modelInfo Pointer to the animation model information structure to be populated
	\param scriptData Pointer to the animation script data structure containing the script information
	\param client Index of the client whose animation data is being parsed
	\param filename Name of the animation script file being parsed
	\param input Pointer to the character buffer containing the animation script data to parse
*/
void					BG_AnimParseAnimScript( animModelInfo_t* modelInfo, animScriptData_t* scriptData, int client, char* filename, char* input );

/*!
	\brief Executes an animation script command based on the player state, movement type, and animation conditions.

	This function determines the appropriate animation script to play for a player based on their current state, movement type, and animation conditions. It searches through available animations in
   decreasing order of priority until a valid animation is found. The function handles different animation types such as idle, swimming, climbing, and walking. It also includes debug output when the
   DBGANIMS flag is enabled. The function returns -1 if no valid animation is found or an error occurs, otherwise it returns the result of executing the animation command.

	\param ps pointer to the player state structure
	\param state the AI state of the player
	\param movetype the movement type that determines which animation script to use
	\param isContinue flag indicating whether to continue or restart the animation
	\return -1 if no valid animation script is found or an error occurs, otherwise the result of executing the animation command
*/
int						BG_AnimScriptAnimation( playerState_t* ps, aistateEnum_t state, scriptAnimMoveTypes_t movetype, qboolean isContinue );

/*!
	\brief Executes a random canned animation script for the specified player state and AI state.

	This function retrieves and executes a randomized canned animation based on the player's state and movement type. It first checks if the player is dead and returns -1 if so. Then it determines the
   movement type for the client and fetches the appropriate animation script. If no valid script items exist, it returns -1. Otherwise, it selects a valid animation item and executes a random command
   from that item's command list.

	\param ps Pointer to the player state structure containing client information
	\param state The AI state enum indicating the current state of the player
	\return The result of executing the animation command, or -1 if no valid animation can be found or executed
*/
int						BG_AnimScriptCannedAnimation( playerState_t* ps, aistateEnum_t state );

/*!
	\brief Changes the animation script state for a player based on the transition from an old state to a new state.

	This function handles the transition between different AI states for a player by executing an appropriate animation script. It first checks if the player is dead, and if so, returns -1. Otherwise,
   it retrieves the model information and script associated with the state change, validates the script existence, and finds the first valid script item that matches the current conditions. If a valid
   item is found, it selects a random command from the item's commands and executes it. The function returns the duration of the animation in milliseconds.

	\param ps Pointer to the player state structure
	\param newState The new AI state to transition to
	\param oldState The previous AI state to transition from
	\return The return value represents the duration of the animation in milliseconds, or -1 if the transition is invalid or no script is found.
*/
int						BG_AnimScriptStateChange( playerState_t* ps, aistateEnum_t newState, aistateEnum_t oldState );

/*!
	\brief Processes an animation script event for a player state and returns the duration of the animation.

	This function handles animation script events by looking up the appropriate animation script for the given event type, finding a valid script item based on current conditions, and executing a
   random command from the script item. It returns the duration of the animation that was executed, or -1 if no valid animation was found. The function also handles special cases like death events and
   dead player states.

	\param ps Pointer to the player state structure
	\param event Type of animation event to process
	\param isContinue Flag indicating if the animation should continue from its previous state
	\param force Flag indicating if the animation should be forced to play regardless of conditions
	\return Returns the duration of the executed animation, or -1 if no valid animation was found
*/
int						BG_AnimScriptEvent( playerState_t* ps, scriptAnimEventTypes_t event, qboolean isContinue, qboolean force );

/*!
	\brief Finds the index of a token string in an array of animStringItem_t structures, with optional failure handling

	This function searches through an array of animation string items to find a matching token. It uses a hash-based lookup for efficiency, computing the hash value of the input token and comparing it
   with precomputed hashes in the string array. If a match is found, it returns the index of the matching string item. If no match is found and allowFail is false, it triggers an animation parsing
   error. The function is commonly used to map string identifiers to numerical indices for animation states and parts.

	\param token The string token to search for in the array
	\param strings Array of animStringItem_t structures containing string names and their precomputed hash values
	\param allowFail Flag indicating whether to allow failure (return -1) or trigger an error when token is not found
	\return The index of the matching string item in the array, or -1 if no match is found and allowFail is true
	\throws BG_AnimParseError is called when no match is found and allowFail is false
*/
int						BG_IndexForString( char* token, animStringItem_t* strings, qboolean allowFail );

/*!
	\brief Plays an animation by name on the specified player state for the given body part with optional timer and continuation settings

	This function looks up an animation by its name and plays it on the specified player state. It uses the animation index derived from the name to invoke the actual animation playing function. The
   function supports setting timers, continuing animations, and forcing playback. It is typically used in game AI scripting to control character animations. The return value represents the duration of
   the played animation.

	\param ps pointer to the player state structure that will control the animation
	\param animName name of the animation to play
	\param bodyPart which body part the animation should affect
	\param setTimer whether to set a timer for the animation
	\param isContinue whether this is a continuation of a previous animation
	\param force whether to force the animation to play even if already playing
	\return duration of the animation that was played
*/
int						BG_PlayAnimName( playerState_t* ps, char* animName, animBodyPart_t bodyPart, qboolean setTimer, qboolean isContinue, qboolean force );

/*!
	\brief Checks if a client has a valid animation script associated with their model.

	This function verifies that the specified client has a valid animation script by ensuring their model is registered and has script items defined. It performs two validation checks: first, that the
   client model exists in the global script data, and second, that the model has script items defined. If either check fails, the function returns false, indicating no valid animation script is
   available for the client.

	\param clientNum The index of the client to validate
	\return true if the client has a valid animation script, false otherwise
*/
qboolean				BG_ValidAnimScript( int clientNum );

/*!
	\brief Returns the animation string for a specified client and animation index.

	This function retrieves the animation string associated with a given client and animation index from the model information. It first obtains the model information for the client and then checks if
   the animation index is within the valid range. If the index is out of range, it triggers an animation parse error. Otherwise, it returns the name of the animation at the specified index.

	\param client Index of the client whose model information is to be used
	\param anim Index of the animation to retrieve the string for
	\return Pointer to the animation string for the specified client and animation index
	\throws Throws an animation parse error if the animation index is out of range
*/
char*					BG_GetAnimString( int client, int anim );

/*!
	\brief Updates a condition value for a client, with optional bitflag conversion support

	This function updates a specific condition value for a given client in the global script data. When the checkConversion flag is set to true, it checks if the condition type requires bitflag
   conversion and handles that accordingly. For bitflag conditions, it clears the existing bitflags and sets the new value using a bitset operation. For non-bitflag conditions, it directly assigns the
   value to the client condition array.

	\param client The client identifier
	\param condition The condition identifier to update
	\param value The value to set for the condition
	\param checkConversion Flag indicating whether to check for bitflag conversion
*/
void					BG_UpdateConditionValue( int client, int condition, int value, qboolean checkConversion );

/*!
	\brief Retrieves the value of a specified condition for a given client, with optional bitflag conversion support

	This function fetches the value of a condition for a specific client from the global script data. If the checkConversion flag is set to true and the condition type is bitflags, the function
   performs a conversion to return the appropriate index value. The conversion searches through bitflag values to find the first set bit and returns its position. If no bit is set, it returns 0. This
   function is used to determine character states and behaviors in the animation system, particularly in relation to movement types and firing states.

	\param client The client number identifying the character for which to retrieve the condition value
	\param condition The condition identifier to retrieve the value for
	\param checkConversion Flag indicating whether to perform bitflag conversion when the condition type is ANIM_CONDTYPE_BITFLAGS
	\return The condition value for the specified client and condition, or the converted bitflag index if conversion is performed and the condition is of type ANIM_CONDTYPE_BITFLAGS
*/
int						BG_GetConditionValue( int client, int condition, qboolean checkConversion );

/*!
	\brief Retrieves the animation index for a specific client, AI state, and movement type from the animation script.

	This function looks up the appropriate animation script item based on the client's model information, AI state, and movement type. It searches through different states starting from the given
   state down to the lowest state until a valid animation is found. If no suitable animation is found, it returns -1. The function ensures that each character has a consistent animation by selecting
   the appropriate animation index based on the client number.

	\param client The client number identifying the character for which to retrieve the animation
	\param state The AI state that determines the animation context
	\param movetype The movement type that specifies the kind of motion for the animation
	\return The animation index if a valid animation is found, otherwise -1
*/
int						BG_GetAnimScriptAnimation( int client, aistateEnum_t state, scriptAnimMoveTypes_t movetype );

/*!
	\brief Updates player state conditions for animation system based on current player state

	This function synchronizes various player state variables with the animation system by updating condition values. It handles weapon state, mounted status (specifically MG42), underhand view angle,
   leaning direction, crouching state, and firing status. The function examines the player state structure to determine current conditions and updates the corresponding animation conditions
   accordingly.

	\param pmove Pointer to the player movement structure containing player state and command data
*/
void					BG_AnimUpdatePlayerStateConditions( pmove_t* pmove );

/*!
	\brief Returns the index of an animation given its string name and client identifier.

	This function searches through the animations of a specified client model to find the index of an animation that matches the provided string name. It uses a hash table lookup for efficiency and
   performs a string comparison to ensure an exact match. If no matching animation is found, it triggers an animation parsing error and returns -1.

	\param string The name of the animation to search for
	\param client Identifier for the client whose model animations are being searched
	\return The index of the matching animation, or -1 if no match is found
	\throws BG_AnimParseError is thrown when an unknown animation index is requested
*/
int						BG_AnimationIndexForString( char* string, int client );

/*!
	\brief Returns a pointer to an animation structure that matches the given string name and model information

	This function searches through the animations of a given model information structure to find an animation that matches the provided string name. It uses a hash-based lookup for performance and
   performs a string comparison to ensure an exact match. If no matching animation is found, the function will trigger a fatal error using Com_Error

	\param string The name of the animation to search for
	\param modelInfo Pointer to the model information structure containing the animations
	\return Pointer to the animation structure that matches the provided string name, or NULL if no match is found (though a fatal error will be triggered before returning NULL)
	\throws ERR_DROP error when no matching animation is found for the given model
*/
animation_t*			BG_AnimationForString( char* string, animModelInfo_t* modelInfo );

/*!
	\brief Retrieves a specific animation structure from a client's model information based on the provided index

	This function fetches an animation structure from the model information associated with a given client. It performs bounds checking to ensure the index is within the valid range of animations for
   that client. If the index is out of bounds, the function will trigger a fatal error. The function is primarily used to access animation data for character movements and actions within the game.

	\param client The client identifier for which to retrieve the animation
	\param index The index of the animation within the client's animation array
	\return A pointer to the animation_t structure corresponding to the specified client and index
	\throws ERR_DROP error when the index is out of bounds for the client's animation array
*/
animation_t*			BG_GetAnimationForIndex( int client, int index );

/*!
	\brief Retrieves the index of a random animation script command for a specified event from the client's model info

	This function looks up animation script events for a player state and returns the index of a randomly selected animation command that matches the given event type. It first checks if the player is
   dead and the event is not a death event, returning -1 in that case. It then retrieves the model info for the client, accesses the script events for the specified event type, and finds the first
   valid script item. If a valid item is found, it selects a random command from the available commands and returns the animation index of that command. If no valid script item is found, it returns
   -1.

	\param ps Pointer to the player state structure containing client information
	\param event Type of animation script event to look up
	\return Index of a randomly selected animation command, or -1 if no valid animation is found
*/
int						BG_GetAnimScriptEvent( playerState_t* ps, scriptAnimEventTypes_t event );

/*!
	\brief Updates the condition value for a client based on the provided condition and value strings.

	This function maps the given condition and value strings to their respective indices and stores the value index in the global script data for the specified client. It uses the animation conditions
   table to find the appropriate indices for the condition and value strings.

	\param client The client index for which the condition value is being updated
	\param conditionStr The string representation of the condition to update
	\param valueStr The string representation of the value to set for the condition
*/
void					BG_UpdateConditionValueStrings( int client, char* conditionStr, char* valueStr );

/*!
	\brief Calculates the gap between footstep sounds based on the player's animation and movement speed

	This function determines the appropriate interval between footstep sounds by analyzing the player's current leg animation and movement speed. It uses the animation's step gap and adjusts it based
   on whether the player is moving faster than the animation's designated move speed. If the animation doesn't have a move speed defined, it returns -1 to indicate the old method should be used
   instead.

	\param ps Pointer to the player state structure containing animation information
	\param xyspeed Current horizontal speed of the player
	\return The calculated gap between footstep sounds, or -1 if the animation doesn't define a move speed
	\throws ERR_DROP if the animation index is out of bounds
*/
float					BG_AnimGetFootstepGap( playerState_t* ps, float xyspeed );

extern animStringItem_t animStateStr[];
extern animStringItem_t animBodyPartsStr[];
