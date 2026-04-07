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
 * name:		be_ai_move.c
 *
 * desc:		bot movement AI
 *
 *
 *****************************************************************************/

#include "q_shared.h"
#include "l_memory.h"
#include "l_libvar.h"
#include "l_utils.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "botshared/botlib.h"
#include "botshared/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"

#include "botshared/be_ea.h"
#include "botshared/be_ai_goal.h"
#include "botshared/be_ai_move.h"

// #define DEBUG_AI_MOVE
// #define DEBUG_ELEVATOR
// #define DEBUG_GRAPPLE
// movement state

// NOTE: the moveflags MFL_ONGROUND, MFL_TELEPORTED and MFL_WATERJUMP must be set outside the movement code
typedef struct bot_movestate_s {
	// input vars (all set outside the movement code)
	vec3_t origin;		 // origin of the bot
	vec3_t velocity;	 // velocity of the bot
	vec3_t viewoffset;	 // view offset
	int	   entitynum;	 // entity number of the bot
	int	   client;		 // client number of the bot
	float  thinktime;	 // time the bot thinks
	int	   presencetype; // presencetype of the bot
	vec3_t viewangles;	 // view angles of the bot
	// state vars
	int	   areanum;			// area the bot is in
	int	   lastareanum;		// last area the bot was in
	int	   lastgoalareanum; // last goal area number
	int	   lastreachnum;	// last reachability number
	vec3_t lastorigin;		// origin previous cycle
	float  lasttime;
	int	   reachareanum;					// area number of the reachabilty
	int	   moveflags;						// movement flags
	int	   jumpreach;						// set when jumped
	float  grapplevisible_time;				// last time the grapple was visible
	float  lastgrappledist;					// last distance to the grapple end
	float  reachability_time;				// time to use current reachability
	int	   avoidreach[MAX_AVOIDREACH];		// reachabilities to avoid
	float  avoidreachtimes[MAX_AVOIDREACH]; // times to avoid the reachabilities
	int	   avoidreachtries[MAX_AVOIDREACH]; // number of tries before avoiding
} bot_movestate_t;

// used to avoid reachability links for some time after being used
//  Ridah, disabled this to prevent wierd navigational behaviour (mostly by Zombie, since it's so slow)
// #define AVOIDREACH
#define AVOIDREACH_TIME				6 // avoid links for 6 seconds after use
#define AVOIDREACH_TRIES			4
// prediction times
#define PREDICTIONTIME_JUMP			3 // in seconds
#define PREDICTIONTIME_MOVE			2 // in seconds
// hook commands
#define CMD_HOOKOFF					"hookoff"
#define CMD_HOOKON					"hookon"
// weapon indexes for weapon jumping
#define WEAPONINDEX_ROCKET_LAUNCHER 5
#define WEAPONINDEX_BFG				9

#define MODELTYPE_FUNC_PLAT			1
#define MODELTYPE_FUNC_BOB			2

float			 sv_maxstep;
float			 sv_maxbarrier;
float			 sv_gravity;
// type of model, func_plat or func_bobbing
int				 modeltypes[MAX_MODELS];

bot_movestate_t* botmovestates[MAX_CLIENTS + 1];

//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
int				 BotAllocMoveState()
{
	int i;

	for( i = 1; i <= MAX_CLIENTS; i++ ) {
		if( !botmovestates[i] ) {
			botmovestates[i] = ( bot_movestate_t* )GetClearedMemory( sizeof( bot_movestate_t ) );
			return i;
		}
	}

	return 0;
}

void BotFreeMoveState( int handle )
{
	if( handle <= 0 || handle > MAX_CLIENTS ) {
		botimport.Print( PRT_FATAL, "move state handle %d out of range\n", handle );
		return;
	}

	if( !botmovestates[handle] ) {
		botimport.Print( PRT_FATAL, "invalid move state %d\n", handle );
		return;
	}

	FreeMemory( botmovestates[handle] );
	botmovestates[handle] = NULL;
}

/*!
	\brief Looks up a bot move state by its handle after validating the handle.

	The function expects the handle to be an integer index into the global botmovestates array. It checks that the handle is positive and does not exceed MAX_CLIENTS. If the handle is out of bounds or
   the corresponding entry in the array is null, it logs a fatal message and returns null. Otherwise, it returns the pointer to the move state.

	\param handle An integer identifying the bot's move state.
	\return Pointer to the bot's move state, or null if the handle is invalid.
*/
bot_movestate_t* BotMoveStateFromHandle( int handle )
{
	if( handle <= 0 || handle > MAX_CLIENTS ) {
		botimport.Print( PRT_FATAL, "move state handle %d out of range\n", handle );
		return NULL;
	}

	if( !botmovestates[handle] ) {
		botimport.Print( PRT_FATAL, "invalid move state %d\n", handle );
		return NULL;
	}

	return botmovestates[handle];
}

// Ridah, provide a means of resetting the avoidreach, so if a bot stops moving, they don't avoid the area they were heading for
//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
void BotInitAvoidReach( int handle )
{
	bot_movestate_t* ms;

	ms = BotMoveStateFromHandle( handle );

	if( !ms ) {
		return;
	}

	memset( ms->avoidreach, 0, sizeof( ms->avoidreach ) );
	memset( ms->avoidreachtries, 0, sizeof( ms->avoidreachtries ) );
	memset( ms->avoidreachtimes, 0, sizeof( ms->avoidreachtimes ) );
}

// done.

//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
void BotInitMoveState( int handle, bot_initmove_t* initmove )
{
	bot_movestate_t* ms;

	ms = BotMoveStateFromHandle( handle );

	if( !ms ) {
		return;
	}

	VectorCopy( initmove->origin, ms->origin );
	VectorCopy( initmove->velocity, ms->velocity );
	VectorCopy( initmove->viewoffset, ms->viewoffset );
	ms->entitynum	 = initmove->entitynum;
	ms->client		 = initmove->client;
	ms->thinktime	 = initmove->thinktime;
	ms->presencetype = initmove->presencetype;
	VectorCopy( initmove->viewangles, ms->viewangles );
	//
	ms->moveflags &= ~MFL_ONGROUND;

	if( initmove->or_moveflags & MFL_ONGROUND ) {
		ms->moveflags |= MFL_ONGROUND;
	}

	ms->moveflags &= ~MFL_TELEPORTED;

	if( initmove->or_moveflags & MFL_TELEPORTED ) {
		ms->moveflags |= MFL_TELEPORTED;
	}

	ms->moveflags &= ~MFL_WATERJUMP;

	if( initmove->or_moveflags & MFL_WATERJUMP ) {
		ms->moveflags |= MFL_WATERJUMP;
	}

	ms->moveflags &= ~MFL_WALK;

	if( initmove->or_moveflags & MFL_WALK ) {
		ms->moveflags |= MFL_WALK;
	}
}

/*!
	\brief Computes the minimal signed angle difference between two angles given in degrees.

	The function subtracts the second angle from the first and then adjusts the result so it lies within the range of -180 to 180 degrees. It corrects for cases where the raw difference exceeds that
   range by adding or subtracting 360 degrees.

	\param ang1 First angle in degrees.
	\param ang2 Second angle in degrees.
	\return The signed difference in degrees, in the range -180 to 180.
*/
float AngleDiff( float ang1, float ang2 )
{
	float diff;

	diff = ang1 - ang2;

	if( ang1 > ang2 ) {
		if( diff > 180.0 ) {
			diff -= 360.0;
		}

	} else {
		if( diff < -180.0 ) {
			diff += 360.0;
		}
	}

	return diff;
}

int BotFuzzyPointReachabilityArea( vec3_t origin )
{
	int	   firstareanum, j, x, y, z;
	int	   areas[10], numareas, areanum, bestareanum;
	float  dist, bestdist;
	vec3_t points[10], v, end;

	firstareanum = 0;
	areanum		 = AAS_PointAreaNum( origin );

	if( areanum ) {
		firstareanum = areanum;

		if( AAS_AreaReachability( areanum ) ) {
			return areanum;
		}
	}

	VectorCopy( origin, end );
	end[2] += 4;
	numareas = AAS_TraceAreas( origin, end, areas, points, 10 );

	for( j = 0; j < numareas; j++ ) {
		if( AAS_AreaReachability( areas[j] ) ) {
			return areas[j];
		}
	}

	bestdist	= 999999;
	bestareanum = 0;

	// z = 0;
	for( z = 1; z >= -1; z -= 1 ) {
		for( x = 1; x >= -1; x -= 2 ) {
			for( y = 1; y >= -1; y -= 2 ) {
				VectorCopy( origin, end );
				// Ridah, increased this for Wolf larger bounding boxes
				end[0] += x * 256; // 8;
				end[1] += y * 256; // 8;
				end[2] += z * 200; // 12;
				numareas = AAS_TraceAreas( origin, end, areas, points, 10 );

				for( j = 0; j < numareas; j++ ) {
					if( AAS_AreaReachability( areas[j] ) ) {
						VectorSubtract( points[j], origin, v );
						dist = VectorLength( v );

						if( dist < bestdist ) {
							bestareanum = areas[j];
							bestdist	= dist;
						}
					}

					if( !firstareanum ) {
						firstareanum = areas[j];
					}
				}
			}
		}

		if( bestareanum ) {
			return bestareanum;
		}
	}

	return firstareanum;
}

int BotReachabilityArea( vec3_t origin, int client )
{
	int				   modelnum, modeltype, reachnum, areanum;
	aas_reachability_t reach;
	vec3_t			   org, end, mins, maxs, up = { 0, 0, 1 };
	bsp_trace_t		   bsptrace;
	aas_trace_t		   trace;

	// check if the bot is standing on something
	AAS_PresenceTypeBoundingBox( PRESENCE_CROUCH, mins, maxs );
	VectorMA( origin, -3, up, end );
	bsptrace = AAS_Trace( origin, mins, maxs, end, client, CONTENTS_SOLID | CONTENTS_PLAYERCLIP );

	if( !bsptrace.startsolid && bsptrace.fraction < 1 && bsptrace.ent != ENTITYNUM_NONE ) {
		// if standing on the world the bot should be in a valid area
		if( bsptrace.ent == ENTITYNUM_WORLD ) {
			return BotFuzzyPointReachabilityArea( origin );
		}

		modelnum  = AAS_EntityModelindex( bsptrace.ent );
		modeltype = modeltypes[modelnum];

		// if standing on a func_plat or func_bobbing then the bot is assumed to be
		// in the area the reachability points to
		if( modeltype == MODELTYPE_FUNC_PLAT || modeltype == MODELTYPE_FUNC_BOB ) {
			reachnum = AAS_NextModelReachability( 0, modelnum );

			if( reachnum ) {
				AAS_ReachabilityFromNum( reachnum, &reach );
				return reach.areanum;
			}
		}

		// if the bot is swimming the bot should be in a valid area
		if( AAS_Swimming( origin ) ) {
			return BotFuzzyPointReachabilityArea( origin );
		}

		//
		areanum = BotFuzzyPointReachabilityArea( origin );

		// if the bot is in an area with reachabilities
		if( areanum && AAS_AreaReachability( areanum ) ) {
			return areanum;
		}

		// trace down till the ground is hit because the bot is standing on some other entity
		VectorCopy( origin, org );
		VectorCopy( org, end );
		end[2] -= 800;
		trace = AAS_TraceClientBBox( org, end, PRESENCE_CROUCH, -1 );

		if( !trace.startsolid ) {
			VectorCopy( trace.endpos, org );
		}

		//
		return BotFuzzyPointReachabilityArea( org );
	}

	//
	return BotFuzzyPointReachabilityArea( origin );
}

/*!
	\brief Checks whether a point is on a moving platform identified by a reachability structure.

	The function retrieves the model number from the reachability face number and fetches the bounding box of that model. It then verifies that the entity’s origin lies within a 32x32x16 box slightly
   offset from the model’s origin; if not, it immediately returns false. Otherwise, it performs a trace with a narrow vertical box from 24 units above the origin to 48 units below it, looking for a
   solid or playerclip volume. If the trace ends on the same entity that owns the model, the function returns true, indicating the point is on an active mover.

	\param origin The 3‑D location to test for placement on a moving platform.
	\param entnum The entity number of the moving platform candidate, typically the bot’s own entity.
	\param reach A reachability structure containing the face number (which encodes the model number) and end point information for the platform.
	\return Non‑zero if the origin is on the mover platform, zero otherwise (qtrue/qfalse).
*/
int BotOnMover( vec3_t origin, int entnum, aas_reachability_t* reach )
{
	int			i, modelnum;
	vec3_t		mins, maxs, modelorigin, org, end;
	vec3_t		angles	= { 0, 0, 0 };
	vec3_t		boxmins = { -16, -16, -8 }, boxmaxs = { 16, 16, 8 };
	bsp_trace_t trace;

	modelnum = reach->facenum & 0x0000FFFF;
	// get some bsp model info
	AAS_BSPModelMinsMaxsOrigin( modelnum, angles, mins, maxs, NULL );

	//
	if( !AAS_OriginOfEntityWithModelNum( modelnum, modelorigin ) ) {
		botimport.Print( PRT_MESSAGE, "no entity with model %d\n", modelnum );
		return qfalse;
	}

	//
	for( i = 0; i < 2; i++ ) {
		if( origin[i] > modelorigin[i] + maxs[i] + 16 ) {
			return qfalse;
		}

		if( origin[i] < modelorigin[i] + mins[i] - 16 ) {
			return qfalse;
		}
	}

	//
	VectorCopy( origin, org );
	org[2] += 24;
	VectorCopy( origin, end );
	end[2] -= 48;
	//
	trace = AAS_Trace( org, boxmins, boxmaxs, end, entnum, CONTENTS_SOLID | CONTENTS_PLAYERCLIP );

	if( !trace.startsolid && !trace.allsolid ) {
		// NOTE: the reachability face number is the model number of the elevator
		if( trace.ent != ENTITYNUM_NONE && AAS_EntityModelNum( trace.ent ) == modelnum ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*!
	\brief Determines whether the moving platform associated with the given reachability is currently down.

	The function extracts the model number from the lower 16 bits of reach->facenum and retrieves the model’s bounds and origin. It verifies that an entity with that model number exists, and if the
   top surface of the platform lies below the start point of the reachability, it reports that the platform is down. Otherwise it reports that the platform is not down.

	\param reach Pointer to a reachability structure used to identify the moving platform.
	\return true (nonzero) if the platform is down, false (zero) otherwise.
*/
int MoverDown( aas_reachability_t* reach )
{
	int	   modelnum;
	vec3_t mins, maxs, origin;
	vec3_t angles = { 0, 0, 0 };

	modelnum = reach->facenum & 0x0000FFFF;
	// get some bsp model info
	AAS_BSPModelMinsMaxsOrigin( modelnum, angles, mins, maxs, origin );

	//
	if( !AAS_OriginOfEntityWithModelNum( modelnum, origin ) ) {
		botimport.Print( PRT_MESSAGE, "no entity with model %d\n", modelnum );
		return qfalse;
	}

	// if the top of the plat is below the reachability start point
	if( origin[2] + maxs[2] < reach->start[2] ) {
		return qtrue;
	}

	return qfalse;
}

void BotSetBrushModelTypes()
{
	int	 ent, modelnum;
	char classname[MAX_EPAIRKEY], model[MAX_EPAIRKEY];

	memset( modeltypes, 0, MAX_MODELS * sizeof( int ) );

	//
	for( ent = AAS_NextBSPEntity( 0 ); ent; ent = AAS_NextBSPEntity( ent ) ) {
		if( !AAS_ValueForBSPEpairKey( ent, "classname", classname, MAX_EPAIRKEY ) ) {
			continue;
		}

		if( !AAS_ValueForBSPEpairKey( ent, "model", model, MAX_EPAIRKEY ) ) {
			continue;
		}

		if( model[0] ) {
			modelnum = atoi( model + 1 );

		} else {
			modelnum = 0;
		}

		if( modelnum < 0 || modelnum > MAX_MODELS ) {
			botimport.Print( PRT_MESSAGE, "entity %s model number out of range\n", classname );
			continue;
		}

		if( !strcmp( classname, "func_bobbing" ) ) {
			modeltypes[modelnum] = MODELTYPE_FUNC_BOB;

		} else if( !strcmp( classname, "func_plat" ) ) {
			modeltypes[modelnum] = MODELTYPE_FUNC_PLAT;
		}
	}
}

/*!
	\brief Detects whether an entity sits directly below the bot and returns its entity number.

	The function builds a bounding box that matches the bot’s current presence type, then casts a short trace straight downward from the bot’s origin by three units. If the trace doesn’t start in
   solid space and intersects an entity other than the world or a special NONE marker, the entity’s number is returned. Otherwise, the function returns –1 to indicate no valid entity was found below
   the bot.

	\param ms Pointer to the bot’s movement state, which contains its current origin, presence type and entity number.
	\return The entity number beneath the bot or –1 if there is none.
*/
int BotOnTopOfEntity( bot_movestate_t* ms )
{
	vec3_t		mins, maxs, end, up = { 0, 0, 1 };
	bsp_trace_t trace;

	AAS_PresenceTypeBoundingBox( ms->presencetype, mins, maxs );
	VectorMA( ms->origin, -3, up, end );
	trace = AAS_Trace( ms->origin, mins, maxs, end, ms->entitynum, CONTENTS_SOLID | CONTENTS_PLAYERCLIP );

	if( !trace.startsolid && ( trace.ent != ENTITYNUM_WORLD && trace.ent != ENTITYNUM_NONE ) ) {
		return trace.ent;
	}

	return -1;
}

/*!
	\brief Determines if a reachability is suitable for traveling based on specified travel flags.

	The routine verifies that the reachability’s travel type matches one of the permitted flags supplied in travelflags. It then checks that the area the reachability leads to contains travel flags
   consistent with the same mask. If either test fails the function returns qfalse; otherwise it returns qtrue. The origin parameter represents the bot’s current position but is not used in the logic
   of this function.

	\param origin The bot's current position in world coordinates (unused in this routine).
	\param reach Pointer to the reachability structure that is being evaluated for validity.
	\param travelflags Bitmask specifying which travel types are allowed for the destination.
	\return qtrue (1) if the reachability is valid for the given flags, otherwise qfalse (0).
*/
int BotValidTravel( vec3_t origin, aas_reachability_t* reach, int travelflags )
{
	// if the reachability uses an unwanted travel type
	if( AAS_TravelFlagForType( reach->traveltype ) & ~travelflags ) {
		return qfalse;
	}

	// don't go into areas with bad travel types
	if( AAS_AreaContentsTravelFlag( reach->areanum ) & ~travelflags ) {
		return qfalse;
	}

	return qtrue;
}

/*!
	\brief Adds or refreshes a reachability that the bot should temporarily avoid during movement.

	The function scans the bot's avoid list for an existing entry matching the given reachability number. If found and the stored avoidance time has not expired, it increments the retry counter;
   otherwise it resets the counter to one. In either case it updates the expire time to the current time plus the specified avoidance duration. If no matching entry exists, the function looks for the
   first entry whose expire time is already in the past, replaces it with the new reachability number, resets the counter, and sets a new expire time. The function leaves the state unchanged if all
   list slots are currently active and none match the number.

	\param ms pointer to the bot's movement state containing the avoid list arrays
	\param number the identifier of the reachability to add or refresh
	\param avoidtime time in seconds for which the reachability should be avoided
*/
void BotAddToAvoidReach( bot_movestate_t* ms, int number, float avoidtime )
{
	int i;

	for( i = 0; i < MAX_AVOIDREACH; i++ ) {
		if( ms->avoidreach[i] == number ) {
			if( ms->avoidreachtimes[i] > AAS_Time() ) {
				ms->avoidreachtries[i]++;

			} else {
				ms->avoidreachtries[i] = 1;
			}

			ms->avoidreachtimes[i] = AAS_Time() + avoidtime;
			return;
		}
	}

	// add the reachability to the reachabilities to avoid for a while
	for( i = 0; i < MAX_AVOIDREACH; i++ ) {
		if( ms->avoidreachtimes[i] < AAS_Time() ) {
			ms->avoidreach[i]	   = number;
			ms->avoidreachtimes[i] = AAS_Time() + avoidtime;
			ms->avoidreachtries[i] = 1;
			return;
		}
	}
}

int BotGetReachabilityToGoal( vec3_t origin,
	int								 areanum,
	int								 entnum,
	int								 lastgoalareanum,
	int								 lastareanum,
	int*							 avoidreach,
	float*							 avoidreachtimes,
	int*							 avoidreachtries,
	bot_goal_t*						 goal,
	int								 travelflags,
	int								 movetravelflags )
{
	int				   t, besttime, bestreachnum, reachnum;
	aas_reachability_t reach;

	// if not in a valid area
	if( !areanum ) {
		return 0;
	}

	//
	if( AAS_AreaDoNotEnter( areanum ) || AAS_AreaDoNotEnter( goal->areanum ) ) {
		travelflags |= TFL_DONOTENTER;
		movetravelflags |= TFL_DONOTENTER;
	}

	if( AAS_AreaDoNotEnterLarge( areanum ) || AAS_AreaDoNotEnterLarge( goal->areanum ) ) {
		travelflags |= TFL_DONOTENTER_LARGE;
		movetravelflags |= TFL_DONOTENTER_LARGE;
	}

	// use the routing to find the next area to go to
	besttime	 = 0;
	bestreachnum = 0;

	//
	for( reachnum = AAS_NextAreaReachability( areanum, 0 ); reachnum; reachnum = AAS_NextAreaReachability( areanum, reachnum ) ) {
#ifdef AVOIDREACH
		int i;

		// check if it isn't an reachability to avoid
		for( i = 0; i < MAX_AVOIDREACH; i++ ) {
			if( avoidreach[i] == reachnum && avoidreachtimes[i] >= AAS_Time() ) {
				break;
			}
		}

		if( i != MAX_AVOIDREACH && avoidreachtries[i] > AVOIDREACH_TRIES ) {
	#ifdef DEBUG

			if( bot_developer ) {
				botimport.Print( PRT_MESSAGE, "avoiding reachability %d\n", avoidreach[i] );
			}

	#endif // DEBUG
			continue;
		}

#endif // AVOIDREACH
	   // get the reachability from the number
		AAS_ReachabilityFromNum( reachnum, &reach );

		// NOTE: do not go back to the previous area if the goal didn't change
		// NOTE: is this actually avoidance of local routing minima between two areas???
		if( lastgoalareanum == goal->areanum && reach.areanum == lastareanum ) {
			continue;
		}

		// if (AAS_AreaContentsTravelFlag(reach.areanum) & ~travelflags) continue;
		// if the travel isn't valid
		if( !BotValidTravel( origin, &reach, movetravelflags ) ) {
			continue;
		}

		// RF, ignore disabled areas
		if( !AAS_AreaReachability( reach.areanum ) ) {
			continue;
		}

		// get the travel time (ignore routes that leads us back our current area)
		t = AAS_AreaTravelTimeToGoalAreaCheckLoop( reach.areanum, reach.end, goal->areanum, travelflags, areanum );

		// if the goal area isn't reachable from the reachable area
		if( !t ) {
			continue;
		}

		// Ridah, if this sends us to a looped route, ignore it
		// if (AAS_AreaTravelTimeToGoalArea(areanum, reach.start, goal->areanum, travelflags) + reach.traveltime < t)
		//	continue;

		// add the travel time towards the area
		//  Ridah, not sure why this was disabled, but it causes looped links in the route-cache
		//  RF, update.. seems to work better like this....
		t += reach.traveltime; // + AAS_AreaTravelTime(areanum, origin, reach.start);
		// t += reach.traveltime + AAS_AreaTravelTime(areanum, origin, reach.start);

		// Ridah, if there exists other entities in this area, avoid it
		//		if (reach.areanum != goal->areanum && AAS_IsEntityInArea( entnum, goal->entitynum, reach.areanum )) {
		//			t += 50;
		//		}

		// if the travel time is better than the ones already found
		if( !besttime || t < besttime ) {
			besttime	 = t;
			bestreachnum = reachnum;
		}
	}

	//
	return bestreachnum;
}

/*!
	\brief Adds a waypoint into a target path while respecting a maximum cumulative distance, returning whether the limit was reached.

	The function computes the direction from ``start`` to ``end`` and normalizes it. It then updates the running distance ``*dist`` by the segment length. If the updated distance stays below
   ``maxdist`` the entire segment is added: the target position is set to ``end`` and the function returns 0 (false). When the segment would exceed the limit, only the portion up to ``maxdist`` is
   added by stepping from ``start`` along the direction vector. In this case the running distance is capped at ``maxdist``, the target is set to the partial position, and the function returns 1
   (true). This allows callers to know when the look‑ahead distance has been exhausted.

	The function is used by the bot movement logic to progressively build a view target while traversing reachability links, ensuring that the total traversal length does not exceed the specified
   lookahead.


	\param start Start point of the segment, used to compute direction.
	\param end End point of the segment; its position is copied if the segment fits within the remaining distance.
	\param maxdist Maximum allowed cumulative distance from the start of the overall path.
	\param dist Pointer to the current cumulative distance; it is increased by the segment length or capped at ``maxdist``.
	\param target Output vector set to the final target position, either the full end point or the partial point when the limit is reached.
	\return An integer used as a boolean: 0 (false) if the segment was fully added and the limit was not reached; 1 (true) if the cumulative distance had to be truncated and the limit was reached.
*/
int BotAddToTarget( vec3_t start, vec3_t end, float maxdist, float* dist, vec3_t target )
{
	vec3_t dir;
	float  curdist;

	VectorSubtract( end, start, dir );
	curdist = VectorNormalize( dir );

	if( *dist + curdist < maxdist ) {
		VectorCopy( end, target );
		*dist += curdist;
		return qfalse;

	} else {
		VectorMA( start, maxdist - *dist, dir, target );
		*dist = maxdist;
		return qtrue;
	}
}

int BotMovementViewTarget( int movestate, bot_goal_t* goal, int travelflags, float lookahead, vec3_t target )
{
	aas_reachability_t reach;
	int				   reachnum, lastareanum;
	bot_movestate_t*   ms;
	vec3_t			   end;
	float			   dist;

	ms = BotMoveStateFromHandle( movestate );

	if( !ms ) {
		return qfalse;
	}

	reachnum = 0;

	// if the bot has no goal or no last reachability
	if( !ms->lastreachnum || !goal ) {
		return qfalse;
	}

	reachnum = ms->lastreachnum;
	VectorCopy( ms->origin, end );
	lastareanum = ms->lastareanum;
	dist		= 0;

	while( reachnum && dist < lookahead ) {
		AAS_ReachabilityFromNum( reachnum, &reach );

		if( BotAddToTarget( end, reach.start, lookahead, &dist, target ) ) {
			return qtrue;
		}

		// never look beyond teleporters
		if( reach.traveltype == TRAVEL_TELEPORT ) {
			return qtrue;
		}

		// don't add jump pad distances
		if( reach.traveltype != TRAVEL_JUMPPAD && reach.traveltype != TRAVEL_ELEVATOR && reach.traveltype != TRAVEL_FUNCBOB ) {
			if( BotAddToTarget( reach.start, reach.end, lookahead, &dist, target ) ) {
				return qtrue;
			}
		}

		reachnum = BotGetReachabilityToGoal( reach.end, reach.areanum, -1, ms->lastgoalareanum, lastareanum, ms->avoidreach, ms->avoidreachtimes, ms->avoidreachtries, goal, travelflags, travelflags );
		VectorCopy( reach.end, end );
		lastareanum = reach.areanum;

		if( lastareanum == goal->areanum ) {
			BotAddToTarget( reach.end, goal->origin, lookahead, &dist, target );
			return qtrue;
		}
	}

	//
	return qfalse;
}

/*!
	\brief Determines whether a target point is visible from an eye position, optionally ignoring a specific entity.

	The function performs a trace from the eye vector to the target vector using AAS_Trace, treating the supplied entity as a non‑solid reference. If the trace does not hit any solid or player clip
   objects, the target is considered visible and qtrue (1) is returned; otherwise qfalse (0) is returned.

	\param ent The entity number that should be ignored or treated specially during the trace.
	\param eye The starting position for the visibility check.
	\param target The destination point to test visibility against.
	\return qtrue if the target is visible from the eye position, otherwise qfalse.
*/
int BotVisible( int ent, vec3_t eye, vec3_t target )
{
	bsp_trace_t trace;

	trace = AAS_Trace( eye, NULL, NULL, target, ent, CONTENTS_SOLID | CONTENTS_PLAYERCLIP );

	if( trace.fraction >= 1 ) {
		return qtrue;
	}

	return qfalse;
}

int BotPredictVisiblePosition( vec3_t origin, int areanum, bot_goal_t* goal, int travelflags, vec3_t target )
{
	aas_reachability_t reach;
	int				   reachnum, lastgoalareanum, lastareanum, i;
	int				   avoidreach[MAX_AVOIDREACH];
	float			   avoidreachtimes[MAX_AVOIDREACH];
	int				   avoidreachtries[MAX_AVOIDREACH];
	vec3_t			   end;

	// if the bot has no goal or no last reachability
	if( !goal ) {
		return qfalse;
	}

	// if the areanum is not valid
	if( !areanum ) {
		return qfalse;
	}

	// if the goal areanum is not valid
	if( !goal->areanum ) {
		return qfalse;
	}

	memset( avoidreach, 0, MAX_AVOIDREACH * sizeof( int ) );
	lastgoalareanum = goal->areanum;
	lastareanum		= areanum;
	VectorCopy( origin, end );

	// only do 20 hops
	for( i = 0; i < 20 && ( areanum != goal->areanum ); i++ ) {
		//
		reachnum = BotGetReachabilityToGoal( end, areanum, -1, lastgoalareanum, lastareanum, avoidreach, avoidreachtimes, avoidreachtries, goal, travelflags, travelflags );

		if( !reachnum ) {
			return qfalse;
		}

		AAS_ReachabilityFromNum( reachnum, &reach );

		//
		if( BotVisible( goal->entitynum, goal->origin, reach.start ) ) {
			VectorCopy( reach.start, target );
			return qtrue;
		}

		//
		if( BotVisible( goal->entitynum, goal->origin, reach.end ) ) {
			VectorCopy( reach.end, target );
			return qtrue;
		}

		//
		if( reach.areanum == goal->areanum ) {
			VectorCopy( reach.end, target );
			return qtrue;
		}

		//
		lastareanum = areanum;
		areanum		= reach.areanum;
		VectorCopy( reach.end, end );
		//
	}

	//
	return qfalse;
}

/*!
	\brief Computes the world‑space position of the bottom‑center point for a mover reachability and stores it in the provided array.

	The function extracts the model number from the reachability's face number and retrieves the model's bounding box and origin using zero angles. It then obtains the entity origin corresponding to
   that model number. By averaging the model's minimum and maximum extents, it calculates a local midpoint. This midpoint is added to the entity origin to get a global center point which is then
   copied into the bottomcenter array. The vertical component of the point is finally set to the reachability's starting height, ensuring the result lies on the platform's bottom at the reachability's
   y coordinate.

	\param reach pointer to the reachability struct used to compute the bottom‑center point
	\param bottomcenter array to receive the resulting world‑space coordinates
*/
void MoverBottomCenter( aas_reachability_t* reach, vec3_t bottomcenter )
{
	int	   modelnum;
	vec3_t mins, maxs, origin, mids;
	vec3_t angles = { 0, 0, 0 };

	modelnum = reach->facenum & 0x0000FFFF;
	// get some bsp model info
	AAS_BSPModelMinsMaxsOrigin( modelnum, angles, mins, maxs, origin );

	//
	if( !AAS_OriginOfEntityWithModelNum( modelnum, origin ) ) {
		botimport.Print( PRT_MESSAGE, "no entity with model %d\n", modelnum );
	}

	// get a point just above the plat in the bottom position
	VectorAdd( mins, maxs, mids );
	VectorMA( origin, 0.5, mids, bottomcenter );
	bottomcenter[2] = reach->start[2];
}

/*!
	\brief Determines the horizontal distance to the next gap the bot could fall into from a given position.

	The function first performs a downward trace to determine if the bot can step down; if no surface is hit, it reports a value of 1 indicating a clear drop. It then steps forward in increments of
   eight units, performing a vertical trace for each position to detect obstacles and potential drops. If a gap is found (the trace ends below the expected step height and the point is not water or
   slime), the distance to that gap is returned. If no such gap is detected within 100 units, it returns 0.

	\param origin current position of the bot
	\param hordir normalized horizontal direction vector for movement
	\param entnum entity number used for trace queries
	\return If a gap is detected ahead, the horizontal distance to the gap in units; returns 1 if the area below the bot is free of solid, otherwise returns 0.
*/
float BotGapDistance( vec3_t origin, vec3_t hordir, int entnum )
{
	float		dist, startz;
	vec3_t		start, end;
	aas_trace_t trace;

	// do gap checking
	startz = origin[2];
	// this enables walking down stairs more fluidly
	{
		VectorCopy( origin, start );
		VectorCopy( origin, end );
		end[2] -= 60;
		trace = AAS_TraceClientBBox( start, end, PRESENCE_CROUCH, entnum );

		if( trace.fraction >= 1 ) {
			return 1;
		}

		startz = trace.endpos[2] + 1;
	}

	//
	for( dist = 8; dist <= 100; dist += 8 ) {
		VectorMA( origin, dist, hordir, start );
		start[2] = startz + 24;
		VectorCopy( start, end );
		end[2] -= 48 + sv_maxbarrier;
		trace = AAS_TraceClientBBox( start, end, PRESENCE_CROUCH, entnum );

		// if solid is found the bot can't walk any further and fall into a gap
		if( !trace.startsolid ) {
			// if it is a gap
			if( trace.endpos[2] < startz - sv_maxstep - 8 ) {
				VectorCopy( trace.endpos, end );
				end[2] -= 20;

				if( AAS_PointContents( end ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) {
					break; //----(SA)	modified since slime is no longer deadly
				}

				//				if (AAS_PointContents(end) & CONTENTS_WATER) break;
				// if a gap is found slow down
				// botimport.Print(PRT_MESSAGE, "gap at %f\n", dist);
				return dist;
			}

			startz = trace.endpos[2];
		}
	}

	return 0;
}

/*!
	\brief Determines whether a bot can execute a barrier jump and commands the jump and movement if possible.

	The function first traces upward from the bot’s current position to confirm that a barrier of sufficient height exists. If the vertical clearance is adequate, it traces horizontally in the desired
   direction to locate the barrier’s top surface. A downward trace then verifies that the target surface is high enough to stand on. When all traces succeed, the bot performs a jump and moves forward
   with the requested speed, and the BARRIERJUMP flag is set. The routine returns a true value if the jump was performed; otherwise it returns false. All traces are performed with the client’s
   bounding box and respect solid geometry and maximum step/barrier limits.

	\param ms The bot’s movement state structure.
	\param dir Desired horizontal direction vector for the potential jump.
	\param speed Speed to use for forward motion associated with the jump.
	\return Non‑zero if the barrier jump was executed; zero otherwise.
*/
int BotCheckBarrierJump( bot_movestate_t* ms, vec3_t dir, float speed )
{
	vec3_t		start, hordir, end;
	aas_trace_t trace;

	VectorCopy( ms->origin, end );
	end[2] += sv_maxbarrier;
	// trace right up
	trace = AAS_TraceClientBBox( ms->origin, end, PRESENCE_NORMAL, ms->entitynum );

	// this shouldn't happen... but we check anyway
	if( trace.startsolid ) {
		return qfalse;
	}

	// if very low ceiling it isn't possible to jump up to a barrier
	if( trace.endpos[2] - ms->origin[2] < sv_maxstep ) {
		return qfalse;
	}

	//
	hordir[0] = dir[0];
	hordir[1] = dir[1];
	hordir[2] = 0;
	VectorNormalize( hordir );
	VectorMA( ms->origin, ms->thinktime * speed * 0.5, hordir, end );
	VectorCopy( trace.endpos, start );
	end[2] = trace.endpos[2];
	// trace from previous trace end pos horizontally in the move direction
	trace = AAS_TraceClientBBox( start, end, PRESENCE_NORMAL, ms->entitynum );

	// again this shouldn't happen
	if( trace.startsolid ) {
		return qfalse;
	}

	//
	VectorCopy( trace.endpos, start );
	VectorCopy( trace.endpos, end );
	end[2] = ms->origin[2];
	// trace down from the previous trace end pos
	trace = AAS_TraceClientBBox( start, end, PRESENCE_NORMAL, ms->entitynum );

	// if solid
	if( trace.startsolid ) {
		return qfalse;
	}

	// if no obstacle at all
	if( trace.fraction >= 1.0 ) {
		return qfalse;
	}

	// if less than the maximum step height
	if( trace.endpos[2] - ms->origin[2] < sv_maxstep ) {
		return qfalse;
	}

	//
	EA_Jump( ms->client );
	EA_Move( ms->client, hordir, speed );
	ms->moveflags |= MFL_BARRIERJUMP;
	// there is a barrier
	return qtrue;
}

/*!
	\brief Moves a bot through water along a specified direction at a given speed.

	The function normalizes the provided direction vector and uses the entity animation system to apply movement to the bot's client. It returns a non‑zero value to indicate the motion was
   successfully applied. The supplied movement type parameter is currently unused.

	\param ms Pointer to the bot's movement state structure
	\param dir Desired movement direction as a vector
	\param speed Target speed in units per second
	\param type Indicator of movement type, ignored in this implementation
	\return Non‑zero on success (qtrue).
*/
int BotSwimInDirection( bot_movestate_t* ms, vec3_t dir, float speed, int type )
{
	vec3_t normdir;

	VectorCopy( dir, normdir );
	VectorNormalize( normdir );
	EA_Move( ms->client, normdir, speed );
	return qtrue;
}

/*!
	\brief Attempts to move the bot in a specified direction, handling jumping and crouching when required, and returns whether the movement was successful.

	The function first checks if the bot is on the ground. If so, it evaluates potential barrier jumps and sets a presence type based on crouch or normal movement. It calculates a horizontal direction
   vector and may decide to jump if a gap is detected. A command movement vector is constructed, and if a jump is needed the vertical component is set accordingly. Prediction parameters such as
   maximum frames and stop events are configured. The function then calls the AAS movement prediction routine. If the prediction exceeds the allowed frames, if a hazardous event would occur, or if a
   ground hit would leave the bot near a gap, the function signals failure. When movement is clear, it issues the appropriate entity‐action commands for jump, crouch, and moving. If the bot is in the
   air, a simpler air‐control path may be taken. The function returns a non‑zero result on successful motion and zero on failure.

	\param ms Pointer to the bot's current movement state
	\param dir Desired horizontal direction vector
	\param speed Desired movement speed
	\param type Bitwise flags indicating movement type, such as jump or crouch
	\return Non‑zero if the movement succeeded, zero otherwise
*/
int BotWalkInDirection( bot_movestate_t* ms, vec3_t dir, float speed, int type )
{
	vec3_t			 hordir, cmdmove, velocity, tmpdir, origin;
	int				 presencetype, maxframes, cmdframes, stopevent;
	aas_clientmove_t move;
	float			 dist;

	// if the bot is on the ground
	if( ms->moveflags & MFL_ONGROUND ) {
		// if there is a barrier the bot can jump on
		if( BotCheckBarrierJump( ms, dir, speed ) ) {
			return qtrue;
		}

		// remove barrier jump flag
		ms->moveflags &= ~MFL_BARRIERJUMP;

		// get the presence type for the movement
		if( ( type & MOVE_CROUCH ) && !( type & MOVE_JUMP ) ) {
			presencetype = PRESENCE_CROUCH;

		} else {
			presencetype = PRESENCE_NORMAL;
		}

		// horizontal direction
		hordir[0] = dir[0];
		hordir[1] = dir[1];
		hordir[2] = 0;
		VectorNormalize( hordir );

		// if the bot is not supposed to jump
		if( !( type & MOVE_JUMP ) ) {
			// if there is a gap, try to jump over it
			if( BotGapDistance( ms->origin, hordir, ms->entitynum ) > 0 ) {
				type |= MOVE_JUMP;
			}
		}

		// get command movement
		VectorScale( hordir, speed, cmdmove );
		VectorCopy( ms->velocity, velocity );

		//
		if( type & MOVE_JUMP ) {
			// botimport.Print(PRT_MESSAGE, "trying jump\n");
			cmdmove[2] = 400;
			maxframes  = PREDICTIONTIME_JUMP / 0.1;
			cmdframes  = 1;
			stopevent  = SE_HITGROUND | SE_HITGROUNDDAMAGE | SE_ENTERWATER | SE_ENTERSLIME | SE_ENTERLAVA;

		} else {
			maxframes = 2;
			cmdframes = 2;
			stopevent = SE_HITGROUNDDAMAGE | SE_ENTERWATER | SE_ENTERSLIME | SE_ENTERLAVA;
		}

		// AAS_ClearShownDebugLines();
		//
		VectorCopy( ms->origin, origin );
		origin[2] += 0.5;
		AAS_PredictClientMovement( &move, ms->entitynum, origin, presencetype, qtrue, velocity, cmdmove, cmdframes, maxframes, 0.1, stopevent, 0, qfalse ); // qtrue);

		// if prediction time wasn't enough to fully predict the movement
		if( move.frames >= maxframes && ( type & MOVE_JUMP ) ) {
			// botimport.Print(PRT_MESSAGE, "client %d: max prediction frames\n", ms->client);
			return qfalse;
		}

		// don't enter slime or lava and don't fall from too high
		if( move.stopevent & ( SE_ENTERLAVA | SE_HITGROUNDDAMAGE ) ) { //----(SA)	modified since slime is no longer deadly
			//		if (move.stopevent & (SE_ENTERSLIME|SE_ENTERLAVA|SE_HITGROUNDDAMAGE))
			// botimport.Print(PRT_MESSAGE, "client %d: would be hurt ", ms->client);
			// if (move.stopevent & SE_ENTERSLIME) botimport.Print(PRT_MESSAGE, "slime\n");
			// if (move.stopevent & SE_ENTERLAVA) botimport.Print(PRT_MESSAGE, "lava\n");
			// if (move.stopevent & SE_HITGROUNDDAMAGE) botimport.Print(PRT_MESSAGE, "hitground\n");
			return qfalse;
		}

		// if ground was hit
		if( move.stopevent & SE_HITGROUND ) {
			// check for nearby gap
			VectorNormalize2( move.velocity, tmpdir );
			dist = BotGapDistance( move.endpos, tmpdir, ms->entitynum );

			if( dist > 0 ) {
				return qfalse;
			}

			//
			dist = BotGapDistance( move.endpos, hordir, ms->entitynum );

			if( dist > 0 ) {
				return qfalse;
			}
		}

		// get horizontal movement
		tmpdir[0] = move.endpos[0] - ms->origin[0];
		tmpdir[1] = move.endpos[1] - ms->origin[1];
		tmpdir[2] = 0;

		//
		// AAS_DrawCross(move.endpos, 4, LINECOLOR_BLUE);
		// the bot is blocked by something
		if( VectorLength( tmpdir ) < speed * ms->thinktime * 0.5 ) {
			return qfalse;
		}

		// perform the movement
		if( type & MOVE_JUMP ) {
			EA_Jump( ms->client );
		}

		if( type & MOVE_CROUCH ) {
			EA_Crouch( ms->client );
		}

		EA_Move( ms->client, hordir, speed );
		// movement was succesfull
		return qtrue;

	} else {
		if( ms->moveflags & MFL_BARRIERJUMP ) {
			// if near the top or going down
			if( ms->velocity[2] < 50 ) {
				EA_Move( ms->client, dir, speed );
			}
		}

		// FIXME: do air control to avoid hazards
		return qtrue;
	}
}

int BotMoveInDirection( int movestate, vec3_t dir, float speed, int type )
{
	bot_movestate_t* ms;

	ms = BotMoveStateFromHandle( movestate );

	if( !ms ) {
		return qfalse;
	}

	// if swimming
	if( AAS_Swimming( ms->origin ) ) {
		return BotSwimInDirection( ms, dir, speed, type );

	} else {
		return BotWalkInDirection( ms, dir, speed, type );
	}
}

/*!
	\brief Calculates the intersection point of two lines defined by point pairs and reports whether the lines intersect

	The function treats (p1,p2) and (p3,p4) as two line segments but solves for the intersection of the infinite lines passing through each pair.
	It computes the determinant d = dy1 * dx2 - dx1 * dy2; if d is non‑zero the lines are not parallel and the intersection coordinates are found using Cramer's rule.
	The resulting coordinates are cast to int before being stored in out, so truncation occurs.
	If d is zero the lines are parallel or coincident and no intersection is reported.

	\param p1 starting point of the first line
	\param p2 ending point of the first line
	\param p3 starting point of the second line
	\param p4 ending point of the second line
	\param out output vector to receive the intersection point
	\return qtrue if the two lines intersect (d != 0), qfalse otherwise; the intersection coordinates are written into out
*/
int Intersection( vec2_t p1, vec2_t p2, vec2_t p3, vec2_t p4, vec2_t out )
{
	float x1, dx1, dy1, x2, dx2, dy2, d;

	dx1 = p2[0] - p1[0];
	dy1 = p2[1] - p1[1];
	dx2 = p4[0] - p3[0];
	dy2 = p4[1] - p3[1];

	d = dy1 * dx2 - dx1 * dy2;

	if( d != 0 ) {
		x1	   = p1[1] * dx1 - p1[0] * dy1;
		x2	   = p3[1] * dx2 - p3[0] * dy2;
		out[0] = ( int )( ( dx1 * x2 - dx2 * x1 ) / d );
		out[1] = ( int )( ( dy1 * x2 - dy2 * x1 ) / d );
		return qtrue;

	} else {
		return qfalse;
	}
}

/*!
	\brief Determines whether a bot’s planned movement direction is blocked and stores any blocking entity information.

	This routine is intended to test the bot’s path for obstacles. It would normally compute a bounding box for the bot’s presence type, adjust it for step height, and trace a line in the desired
   direction for collisions with solid or player‑clip objects. If an entity is found, the result structure is marked blocked and the entity number recorded. An additional check is performed when the
   bot may be standing on an obstacle, setting a flag for top‑of‑obstacle blockage. In the current Wolf AI build, the body is replaced by an early return, so no checking occurs.

	The function operates on global collision data via the AAS_Trace and AAS_AreaReachability helpers and writes results into the provided bot_moveresult_t structure. It does not return a value but
   may modify the result structure.

	\param checkbottom Flag indicating whether to perform an additional check for bottom‑side obstacles when the bot isn’t reachable.
	\param dir Unit direction vector along which the bot intends to move.
	\param ms Pointer to the bot’s movement state, providing its current origin, entity number, and presence type.
	\param result Pointer to a structure that receives information about any blocking entity and flags.
	\param result->blocked Boolean set by this function to indicate a blockage was detected.
	\param result->blockentity Entity number that is blocking the bot, if any.
	\param result->flags Bitfield to be updated with flags such as MOVERESULT_ONTOPOFOBSTACLE when a top obstacle is detected.
*/
void BotCheckBlocked( bot_movestate_t* ms, vec3_t dir, int checkbottom, bot_moveresult_t* result )
{
	vec3_t		mins, maxs, end, up = { 0, 0, 1 };
	bsp_trace_t trace;

	// RF, not required for Wolf AI
	return;

	// test for entities obstructing the bot's path
	AAS_PresenceTypeBoundingBox( ms->presencetype, mins, maxs );

	//
	if( fabs( DotProduct( dir, up ) ) < 0.7 ) {
		mins[2] += sv_maxstep; // if the bot can step on
		maxs[2] -= 10;		   // a little lower to avoid low ceiling
	}

	VectorMA( ms->origin, 3, dir, end );
	trace = AAS_Trace( ms->origin, mins, maxs, end, ms->entitynum, CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_BODY );

	// if not started in solid and not hitting the world entity
	if( !trace.startsolid && ( trace.ent != ENTITYNUM_WORLD && trace.ent != ENTITYNUM_NONE ) ) {
		result->blocked		= qtrue;
		result->blockentity = trace.ent;
#ifdef DEBUG
		// botimport.Print(PRT_MESSAGE, "%d: BotCheckBlocked: I'm blocked\n", ms->client);
#endif // DEBUG
	}

	// if not in an area with reachability
	else if( checkbottom && !AAS_AreaReachability( ms->areanum ) ) {
		// check if the bot is standing on something
		AAS_PresenceTypeBoundingBox( ms->presencetype, mins, maxs );
		VectorMA( ms->origin, -3, up, end );
		trace = AAS_Trace( ms->origin, mins, maxs, end, ms->entitynum, CONTENTS_SOLID | CONTENTS_PLAYERCLIP );

		if( !trace.startsolid && ( trace.ent != ENTITYNUM_WORLD && trace.ent != ENTITYNUM_NONE ) ) {
			result->blocked		= qtrue;
			result->blockentity = trace.ent;
			result->flags |= MOVERESULT_ONTOPOFOBSTACLE;
#ifdef DEBUG
			// botimport.Print(PRT_MESSAGE, "%d: BotCheckBlocked: I'm blocked\n", ms->client);
#endif // DEBUG
		}
	}
}

/*!
	\brief Reset all fields of a bot move result to the default state.

	This function clears the status information associated with a bot's attempted movement. It sets the failure flag, blocked flag, travel type, and various internal flags to their zeroed or false
   values and clears the block entity reference.

	After calling this routine the resulting bot_moveresult_t instance is ready to be populated with new movement data.

	\param moveresult Pointer to the moveresult structure to be reset.
*/
void BotClearMoveResult( bot_moveresult_t* moveresult )
{
	moveresult->failure		= qfalse;
	moveresult->type		= 0;
	moveresult->blocked		= qfalse;
	moveresult->blockentity = 0;
	moveresult->traveltype	= 0;
	moveresult->flags		= 0;
}

/*!
	\brief returns a bot_moveresult_t that represents walking movement toward a reachability node

	The function first calculates a horizontal direction vector from the bot’s current position to the start of the specified reachability node and normalizes it, producing a distance value. It then
   checks whether the path is blocked; if it is blocked, the move result is updated accordingly and the function returns. If the bot is close enough to the start (less than 32 units), the target point
   is changed to the reachability’s end instead. When the destination area requires crouching and the bot is within 20 units, a crouch action is issued. The gap distance to the next obstacle is then
   computed to adjust the target speed: on a pure walk move the speed calculation is based on a base of 200 units (with a tweak that subtracts 180 minus the distance), whereas a faster move uses a
   base of 400 units and subtracts 360 minus twice the distance. Finally, a move command is issued in the calculated direction and speed, the finished direction is stored in the result structure, and
   the structure is returned.

	\param ms pointer to the bot’s current movement state
	\param reach pointer to the reachability node the bot should travel toward
	\return a bot_moveresult_t containing the resulting move direction and any state updates
*/
bot_moveresult_t BotTravel_Walk( bot_movestate_t* ms, aas_reachability_t* reach )
{
	float			 dist, speed;
	vec3_t			 hordir;
	bot_moveresult_t result;

	BotClearMoveResult( &result );
	// first walk straight to the reachability start
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	dist	  = VectorNormalize( hordir );
	//
	BotCheckBlocked( ms, hordir, qtrue, &result );

	//
	// Ridah, tweaked this
	//	if (dist < 10)
	if( dist < 32 ) {
		// walk straight to the reachability end
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		dist	  = VectorNormalize( hordir );
	}

	// if going towards a crouch area

	// Ridah, some areas have a 0 presence (!?!)
	//	if (!(AAS_AreaPresenceType(reach->areanum) & PRESENCE_NORMAL))
	if( ( AAS_AreaPresenceType( reach->areanum ) & PRESENCE_CROUCH ) && !( AAS_AreaPresenceType( reach->areanum ) & PRESENCE_NORMAL ) ) {
		// if pretty close to the reachable area
		if( dist < 20 ) {
			EA_Crouch( ms->client );
		}
	}

	//
	dist = BotGapDistance( ms->origin, hordir, ms->entitynum );

	//
	if( ms->moveflags & MFL_WALK ) {
		if( dist > 0 ) {
			speed = 200 - ( 180 - 1 * dist );

		} else {
			speed = 200;
		}

		EA_Walk( ms->client );

	} else {
		if( dist > 0 ) {
			speed = 400 - ( 360 - 2 * dist );

		} else {
			speed = 400;
		}
	}

	// elemantary action move in direction
	EA_Move( ms->client, hordir, speed );
	VectorCopy( hordir, result.movedir );
	//
	return result;
}

/*!
	\brief Directs the bot toward the end of a given walkability reachability and returns the resulting movement direction.

	The function clears any prior move result and calculates a horizontal direction vector from the bot’s current origin to the end point of the supplied reachability. The direction is normalised and
   the distance is capped at 100 units. The movement speed is set to three times the capped distance, yielding a maximum of 300. An EA_Move command is issued with this direction and speed, and the
   movedir is copied into the result structure which is then returned.

	\param ms Pointer to the bot's movement state, containing its current position and client information.
	\param reach Pointer to the reachability representing the area the bot is finishing walking to.
	\return A bot_moveresult_t structure holding the movement direction used in the EA_Move call.
*/
bot_moveresult_t BotFinishTravel_Walk( bot_movestate_t* ms, aas_reachability_t* reach )
{
	vec3_t			 hordir;
	float			 dist, speed;
	bot_moveresult_t result;

	BotClearMoveResult( &result );
	// if not on the ground and changed areas... don't walk back!!
	//(doesn't seem to help)
	/*
	ms->areanum = BotFuzzyPointReachabilityArea(ms->origin);
	if (ms->areanum == reach->areanum)
	{
	#ifdef DEBUG
		botimport.Print(PRT_MESSAGE, "BotFinishTravel_Walk: already in reach area\n");
	#endif //DEBUG
		return result;
	} //end if*/
	// go straight to the reachability end
	hordir[0] = reach->end[0] - ms->origin[0];
	hordir[1] = reach->end[1] - ms->origin[1];
	hordir[2] = 0;
	dist	  = VectorNormalize( hordir );

	//
	if( dist > 100 ) {
		dist = 100;
	}

	speed = 400 - ( 400 - 3 * dist );
	//
	EA_Move( ms->client, hordir, speed );
	VectorCopy( hordir, result.movedir );
	//
	return result;
}

/*!
	\brief Moves a bot while crouched toward the end of a reachability segment.

	The routine first clears any previous move result data. It then sets a fixed walking speed of 400 and computes the horizontal direction vector from the bot's current origin to the reachability's
   end point, ignoring height differences. After normalising the vector, it checks whether that direction is blocked for the bot. It issues commands to make the bot crouch and to move in the computed
   direction at the chosen speed. Finally, it copies the direction vector into the returned result structure, which the caller can use to understand the direction and status of the movement action.

	\param ms pointer to the current bot movement state
	\param reach pointer to the reachability data that describes the desired travel segment
	\return a bot_moveresult_t structure that contains information about the performed move, including the movement direction that was set.
*/
bot_moveresult_t BotTravel_Crouch( bot_movestate_t* ms, aas_reachability_t* reach )
{
	float			 speed;
	vec3_t			 hordir;
	bot_moveresult_t result;

	BotClearMoveResult( &result );
	//
	speed = 400;
	// walk straight to reachability end
	hordir[0] = reach->end[0] - ms->origin[0];
	hordir[1] = reach->end[1] - ms->origin[1];
	hordir[2] = 0;
	VectorNormalize( hordir );
	//
	BotCheckBlocked( ms, hordir, qtrue, &result );
	// elemantary actions
	EA_Crouch( ms->client );
	EA_Move( ms->client, hordir, speed );
	//
	VectorCopy( hordir, result.movedir );
	//
	return result;
}

/*!
	\brief Travels toward a barrier, optionally performing a jump and then moving forward to cross it.

	The function first clears a movement result and determines a horizontal direction from the bot’s current position to the start of the specified reachability. It normalizes that vector and checks
   for obstructions. If the bot is within a very short distance (less than 9 units), it issues a jump command and then computes a forward direction from the reachability start to its end. It
   normalizes this vector, clamps the travel distance to a maximum of 60 units, computes a forward speed proportional to that distance (up to a maximum of 360 units per second), and issues a move
   command to apply momentum onto the barrier. If the bot is farther away, it similarly clamps the distance and moves forward along the initial heading with speed proportional to distance. Finally it
   records the chosen heading in the result and returns that structure.

	\param ms The bot’s current movement state used to read position and to issue movement commands.
	\param reach The reachability structure that describes the barrier to be traversed, including its start and end positions.
	\return A bot_moveresult_t structure describing the performed movement, such as the direction moved.
*/
bot_moveresult_t BotTravel_BarrierJump( bot_movestate_t* ms, aas_reachability_t* reach )
{
	float			 dist, speed;
	vec3_t			 hordir;
	bot_moveresult_t result;

	BotClearMoveResult( &result );
	// walk straight to reachability start
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	dist	  = VectorNormalize( hordir );
	//
	BotCheckBlocked( ms, hordir, qtrue, &result );

	// if pretty close to the barrier
	if( dist < 9 ) {
		EA_Jump( ms->client );

		// Ridah, do the movement also, so we have momentum to get onto the barrier
		hordir[0] = reach->end[0] - reach->start[0];
		hordir[1] = reach->end[1] - reach->start[1];
		hordir[2] = 0;
		VectorNormalize( hordir );

		dist  = 60;
		speed = 360 - ( 360 - 6 * dist );
		EA_Move( ms->client, hordir, speed );
		// done.

	} else {
		if( dist > 60 ) {
			dist = 60;
		}

		speed = 360 - ( 360 - 6 * dist );
		EA_Move( ms->client, hordir, speed );
	}

	VectorCopy( hordir, result.movedir );
	//
	return result;
}

/*!
	\brief Completes a barrier jump by moving the bot horizontally toward the reachability end when near the top or falling.

	The function starts by clearing the movement result structure. It checks the bot’s vertical velocity; if the bot is close to the top of the jump or descending (velocity below 250), it extends the
   target reachability end a short distance outward to ensure the ledge is cleared. A horizontal direction vector is calculated from the bot’s current position to this adjusted endpoint, normalized,
   and its magnitude is capped at 60 units. The movement speed is set proportionally to this distance (6 units per distance step), and an EA_Move command is issued with that direction and speed. The
   BotCheckBlocked routine is called to detect any obstacles that might block the movement, potentially altering the result before it is returned. If the velocity condition is not met, the function
   simply returns an empty result.

	The returned bot_moveresult_t contains the executed movement direction and any blocking information that may have been detected.

	\param ms Pointer to the bot’s current movement state, including position, velocity and client context.
	\param reach Pointer to the reachability defining the start and end points of the jump target.
	\return An initialized bot_moveresult_t describing the movement performed and any blocking status.
*/
bot_moveresult_t BotFinishTravel_BarrierJump( bot_movestate_t* ms, aas_reachability_t* reach )
{
	float			 dist, speed;
	vec3_t			 hordir;
	bot_moveresult_t result;

	BotClearMoveResult( &result );

	// if near the top or going down
	if( ms->velocity[2] < 250 ) {
		// Ridah, extend the end point a bit, so we strive to get over the ledge more
		vec3_t end;

		VectorSubtract( reach->end, reach->start, end );
		end[2] = 0;
		VectorNormalize( end );
		VectorMA( reach->end, 32, end, end );
		hordir[0] = end[0] - ms->origin[0];
		hordir[1] = end[1] - ms->origin[1];
		//		hordir[0] = reach->end[0] - ms->origin[0];
		//		hordir[1] = reach->end[1] - ms->origin[1];
		// done.
		hordir[2] = 0;
		dist	  = VectorNormalize( hordir );
		//
		BotCheckBlocked( ms, hordir, qtrue, &result );

		//
		if( dist > 60 ) {
			dist = 60;
		}

		speed = 400 - ( 400 - 6 * dist );
		EA_Move( ms->client, hordir, speed );
		VectorCopy( hordir, result.movedir );
	}

	//
	return result;
}

/*!
	\brief Calculates a swim movement result toward the start of a reachability.

	The function starts by clearing a result structure. It computes a vector from the bot's current origin to the reachability's start point and normalises it. A blocked check is performed in that
   direction. An EA_Move action is issued with the normalized direction and a speed of 400 units. The result is populated with the movement direction, converted to view angles for the ideal
   orientation, and a flag marking a swim view is set. Finally the populated result is returned.

	\param ms pointer to the bot’s movement state, including current origin and client handle
	\param reach pointer to the reachability structure representing the target swim region
	\return A bot_moveresult_t structure containing the computed movement direction, ideal view angles, and flags indicating swim view
*/
bot_moveresult_t BotTravel_Swim( bot_movestate_t* ms, aas_reachability_t* reach )
{
	vec3_t			 dir;
	bot_moveresult_t result;

	BotClearMoveResult( &result );
	// swim straight to reachability end
	VectorSubtract( reach->start, ms->origin, dir );
	VectorNormalize( dir );
	//
	BotCheckBlocked( ms, dir, qtrue, &result );
	// elemantary actions
	EA_Move( ms->client, dir, 400 );
	//
	VectorCopy( dir, result.movedir );
	Vector2Angles( dir, result.ideal_viewangles );
	result.flags |= MOVERESULT_SWIMVIEW;
	//
	return result;
}

/*!
	\brief Calculates a movement direction and view angles for a bot to perform a water jump toward a reachability endpoint.

	Starting from the bot’s current position, the function computes a vector pointing from the bot towards the reachability’s end point. The horizontal component of this vector is used to determine a
   proximity test, while the vertical component is increased by a base value plus a small random range to simulate a jump. After normalizing the vector, the bot issues a forward move command; if the
   horizontal distance to the target is small, it also issues an upward move command. The resulting direction and ideal view angles are stored in a bot_moveresult_t, and a flag is set to indicate that
   the movement view has been established.

	\param ms Pointer to the bot’s current movement state, containing its origin and client identifier.
	\param reach Pointer to the target reachability, providing the endpoint to which the bot should move.
	\return A bot_moveresult_t structure that includes the movement direction, ideal view angles, and flags indicating the movement behavior.
*/
bot_moveresult_t BotTravel_WaterJump( bot_movestate_t* ms, aas_reachability_t* reach )
{
	vec3_t			 dir, hordir;
	float			 dist;
	bot_moveresult_t result;

	BotClearMoveResult( &result );
	// swim straight to reachability end
	VectorSubtract( reach->end, ms->origin, dir );
	VectorCopy( dir, hordir );
	hordir[2] = 0;
	dir[2] += 15 + crandom() * 40;
	// botimport.Print(PRT_MESSAGE, "BotTravel_WaterJump: dir[2] = %f\n", dir[2]);
	VectorNormalize( dir );
	dist = VectorNormalize( hordir );
	// elemantary actions
	// EA_Move(ms->client, dir, 400);
	EA_MoveForward( ms->client );

	// move up if close to the actual out of water jump spot
	if( dist < 40 ) {
		EA_MoveUp( ms->client );
	}

	// set the ideal view angles
	Vector2Angles( dir, result.ideal_viewangles );
	result.flags |= MOVERESULT_MOVEMENTVIEW;
	//
	VectorCopy( dir, result.movedir );
	//
	return result;
}

/*!
	\brief Steers a bot through the final phase of a water jump toward a target point while updating movement and view parameters.

	The function first clears a movement result structure. If the bot’s movement flags still contain a water‑jump flag, it returns immediately because the jump process is already considered finished.
   Otherwise it verifies that the bot is still inside a liquid volume (water, lava, or slime) by checking a point slightly below its current origin; if it has exited the liquid the function does
   nothing.

	When the bot remains submerged, the function calculates a direction vector from the bot’s current origin to the end point of the supplied reachability. It adds a small random horizontal
   perturbation and a fixed upward component to model a realistic jump arc, then normalises the vector. The EA_Move call issues this movement command to the bot controller.

	Finally, it converts the final direction vector into ideal view angles, stores them along with the movement direction into the result structure, flags the move result to indicate that the view
   should follow the movement, and returns the filled result.

	This routine is used by the travel logic when a bot has just left the water and needs to propel itself to the destination point at the surface.

	\param ms Current state of the bot including position, movement flags, and a client handle
	\param reach Reachability structure whose end point defines the surface target of the water jump
	\return A bot_moveresult_t containing the movement direction, ideal view angles, and flags for following the movement
*/
bot_moveresult_t BotFinishTravel_WaterJump( bot_movestate_t* ms, aas_reachability_t* reach )
{
	vec3_t			 dir, pnt;
	float			 dist;
	bot_moveresult_t result;

	// botimport.Print(PRT_MESSAGE, "BotFinishTravel_WaterJump\n");
	BotClearMoveResult( &result );

	// if waterjumping there's nothing to do
	if( ms->moveflags & MFL_WATERJUMP ) {
		return result;
	}

	// if not touching any water anymore don't do anything
	// otherwise the bot sometimes keeps jumping?
	VectorCopy( ms->origin, pnt );
	pnt[2] -= 32; // extra for q2dm4 near red armor/mega health

	if( !( AAS_PointContents( pnt ) & ( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER ) ) ) {
		return result;
	}

	// swim straight to reachability end
	VectorSubtract( reach->end, ms->origin, dir );
	dir[0] += crandom() * 10;
	dir[1] += crandom() * 10;
	dir[2] += 70 + crandom() * 10;
	dist = VectorNormalize( dir );
	// elemantary actions
	EA_Move( ms->client, dir, 400 );
	// set the ideal view angles
	Vector2Angles( dir, result.ideal_viewangles );
	result.flags |= MOVERESULT_MOVEMENTVIEW;
	//
	VectorCopy( dir, result.movedir );
	//
	return result;
}

/*!
	\brief Determines the horizontal direction and speed for a bot to walk off a ledge from a given reachability

	The function first clears a move result structure and checks whether an obstacle blocks the path from the bot’s current position to the start of the reachability. It then computes the horizontal
   difference between the reachability’s start and end points to determine the overall direction of the ledge. Based on the distance to the start point and the dot product of directional vectors, the
   routine chooses whether to walk directly toward the start or to steer toward the end. The chosen horizontal vector is normalized and used to calculate an appropriate walking speed: small steps use
   a speed of around 200, longer steps up to 400 with additional scaling for distances below 128 units. The bot is instructed to crouch before moving, which helps the ledge transition look smoother.
   Finally, an elementary move command is issued with the computed direction and speed, the result’s direction is stored, and the populated result structure is returned.

	\param ms Pointer to the bot’s current movement state
	\param reach Pointer to the reachability describing the ledge to walk off
	\return A bot_moveresult_t structure containing the computed movement direction and status information
*/
bot_moveresult_t BotTravel_WalkOffLedge( bot_movestate_t* ms, aas_reachability_t* reach )
{
	vec3_t			 hordir, dir;
	float			 dist, speed, reachhordist;
	bot_moveresult_t result;

	BotClearMoveResult( &result );
	// check if the bot is blocked by anything
	VectorSubtract( reach->start, ms->origin, dir );
	VectorNormalize( dir );
	BotCheckBlocked( ms, dir, qtrue, &result );
	// if the reachability start and end are practially above each other
	VectorSubtract( reach->end, reach->start, dir );
	dir[2]		 = 0;
	reachhordist = VectorLength( dir );
	// walk straight to the reachability start
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	dist	  = VectorNormalize( hordir );
	// if pretty close to the start focus on the reachability end

	// Ridah, tweaked this
#if 0

	if ( dist < 48 ) {
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
#else

	if( ( dist < 72 ) && ( DotProduct( dir, hordir ) < 0 ) ) { // walk in the direction of start -> end
		// hordir[0] = reach->end[0] - ms->origin[0];
		// hordir[1] = reach->end[1] - ms->origin[1];
		// VectorNormalize( dir );
		// VectorMA( hordir, 48, dir, hordir );
		// hordir[2] = 0;

		VectorCopy( dir, hordir );
#endif
		VectorNormalize( hordir );

		//
		if( reachhordist < 20 ) {
			// speed = 100;
			speed = 200; // RF, increased this to speed up travel speed down steps

		} else if( !AAS_HorizontalVelocityForJump( 0, reach->start, reach->end, &speed ) ) {
			speed = 400;
		}

		// looks better crouching off a ledge
		EA_Crouch( ms->client );

	} else {
		if( reachhordist < 20 ) {
			if( dist > 64 ) {
				dist = 64;
			}

			// speed = 400 - (256 - 4 * dist);
			speed = 400 - ( 256 - 4 * dist ) * 0.5; // RF changed this for steps

		} else {
			speed = 400;

			// Ridah, tweaked this
			if( dist < 128 ) {
				speed *= 0.5 + 0.5 * ( dist / 128 );
			}
		}
	}

	//
	BotCheckBlocked( ms, hordir, qtrue, &result );
	// elemantary action
	EA_Move( ms->client, hordir, speed );
	VectorCopy( hordir, result.movedir );
	//
	return result;
}

/*!
	\brief Computes an air control vector and speed for a bot to reach a goal point under gravity.

	The function simulates the bot's motion for up to 50 steps by applying a scaled velocity and gravity. If the simulated trajectory would cross below the vertical position of the goal while moving
   downward, it adjusts the velocity to land exactly at the goal, computes the direction toward the goal, clamps the distance to 32 units, and then determines a speed based on that distance. When a
   suitable trajectory is found, the function outputs the direction, speed, and returns true. If no such trajectory is found after the simulation, it outputs a zero direction, sets a default speed of
   400, and returns false.

	\param origin Starting position of the bot.
	\param velocity Current velocity of the bot; used as a base for the simulation.
	\param goal Target position the bot aims to reach.
	\param dir Output vector giving the direction to move; set only if a viable trajectory is found.
	\param speed Output speed to use; set based on distance if a trajectory is found, otherwise set to 400.
	\return Returns an integer value indicating success (true) or failure (false) of finding an air‑controlled path to the goal.
*/
int BotAirControl( vec3_t origin, vec3_t velocity, vec3_t goal, vec3_t dir, float* speed )
{
	vec3_t org, vel;
	float  dist;
	int	   i;

	VectorCopy( origin, org );
	VectorScale( velocity, 0.1, vel );

	for( i = 0; i < 50; i++ ) {
		vel[2] -= sv_gravity * 0.01;

		// if going down and next position would be below the goal
		if( vel[2] < 0 && org[2] + vel[2] < goal[2] ) {
			VectorScale( vel, ( goal[2] - org[2] ) / vel[2], vel );
			VectorAdd( org, vel, org );
			VectorSubtract( goal, org, dir );
			dist = VectorNormalize( dir );

			if( dist > 32 ) {
				dist = 32;
			}

			*speed = 400 - ( 400 - 13 * dist );
			return qtrue;

		} else {
			VectorAdd( org, vel, org );
		}
	}

	VectorSet( dir, 0, 0, 0 );
	*speed = 400;
	return qfalse;
}

/*!
	\brief Completes the final movement step for a bot walking off a ledge

	The function first calculates the directional vector to the reachability end point and uses that to check whether the path is blocked. It then selects a landing point a short distance from the
   endpoint and attempts to compute an air control vector and speed; if air control is not possible it defaults to moving straight toward the endpoint at a fixed speed. The bot is commanded to crouch
   and a move action is issued with the chosen heading and speed. The resulting movement state, including the selected heading, is returned in a bot_moveresult_t structure.

	\param ms pointer to the bot's movement state, containing current position, velocity and client information
	\param reach pointer to the reachability structure that defines the ledge the bot is navigating toward
	\return a bot_moveresult_t structure describing the movement performed
*/
bot_moveresult_t BotFinishTravel_WalkOffLedge( bot_movestate_t* ms, aas_reachability_t* reach )
{
	vec3_t			 dir, hordir, end, v;
	float			 dist, speed;
	bot_moveresult_t result;

	BotClearMoveResult( &result );
	//
	VectorSubtract( reach->end, ms->origin, dir );
	BotCheckBlocked( ms, dir, qtrue, &result );
	//
	VectorSubtract( reach->end, ms->origin, v );
	v[2] = 0;
	dist = VectorNormalize( v );

	if( dist > 16 ) {
		VectorMA( reach->end, 16, v, end );

	} else {
		VectorCopy( reach->end, end );
	}

	//
	if( !BotAirControl( ms->origin, ms->velocity, end, hordir, &speed ) ) {
		// go straight to the reachability end
		VectorCopy( dir, hordir );
		hordir[2] = 0;
		//
		dist  = VectorNormalize( hordir );
		speed = 400;
	}

	//
	// looks better crouching off a ledge
	EA_Crouch( ms->client );
	EA_Move( ms->client, hordir, speed );
	VectorCopy( hordir, result.movedir );
	//
	return result;
}

/*!
	\brief Calculates and initiates a bot’s jump movement towards a reachability target.

	The function first determines a horizontal start point for the jump based on the provided reachability area and normalizes the direction vector toward that point. It then checks for gaps and
   adjusts the run start position accordingly. By comparing the bot’s current position with the run start and reach start, it decides whether to perform a normal or delayed jump and applies the
   appropriate jump elementary action (EA_Jump or EA_DelayedJump). If the bot is still approaching the start, it moves directly toward the run start point with a speed limiting at 80 units; otherwise,
   it moves toward the final reach end point. The function records the chosen movement direction in the result structure and updates the bot’s jump reach state. It also sets the result’s movedir field
   before returning.

	\param ms Pointer to the bot’s current movement state and information such as origin and reach area number.
	\param reach Pointer to the target reachability structure containing start and end positions and area number.
	\return The resulting bot movement decision structure containing the movement direction and other flags set by the function.
*/
bot_moveresult_t BotTravel_Jump( bot_movestate_t* ms, aas_reachability_t* reach )
{
	vec3_t			 hordir, dir1, dir2, start, end, runstart;
	//	vec3_t runstart, dir1, dir2, hordir;
	float			 dist1, dist2, speed;
	bot_moveresult_t result;

	BotClearMoveResult( &result );
	//
	AAS_JumpReachRunStart( reach, runstart );
	//*
	hordir[0] = runstart[0] - reach->start[0];
	hordir[1] = runstart[1] - reach->start[1];
	hordir[2] = 0;
	VectorNormalize( hordir );
	//
	VectorCopy( reach->start, start );
	start[2] += 1;
	VectorMA( reach->start, 80, hordir, runstart );

	// check for a gap
	for( dist1 = 0; dist1 < 80; dist1 += 10 ) {
		VectorMA( start, dist1 + 10, hordir, end );
		end[2] += 1;

		if( AAS_PointAreaNum( end ) != ms->reachareanum ) {
			break;
		}
	}

	if( dist1 < 80 ) {
		VectorMA( reach->start, dist1, hordir, runstart );
	}

	//
	VectorSubtract( ms->origin, reach->start, dir1 );
	dir1[2] = 0;
	dist1	= VectorNormalize( dir1 );
	VectorSubtract( ms->origin, runstart, dir2 );
	dir2[2] = 0;
	dist2	= VectorNormalize( dir2 );

	// if just before the reachability start
	if( DotProduct( dir1, dir2 ) < -0.8 || dist2 < 5 ) {
		//		botimport.Print(PRT_MESSAGE, "between jump start and run start point\n");
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		VectorNormalize( hordir );

		// elemantary action jump
		if( dist1 < 24 ) {
			EA_Jump( ms->client );

		} else if( dist1 < 32 ) {
			EA_DelayedJump( ms->client );
		}

		EA_Move( ms->client, hordir, 600 );
		//
		ms->jumpreach = ms->lastreachnum;

	} else {
		//		botimport.Print(PRT_MESSAGE, "going towards run start point\n");
		hordir[0] = runstart[0] - ms->origin[0];
		hordir[1] = runstart[1] - ms->origin[1];
		hordir[2] = 0;
		VectorNormalize( hordir );

		//
		if( dist2 > 80 ) {
			dist2 = 80;
		}

		speed = 400 - ( 400 - 5 * dist2 );
		EA_Move( ms->client, hordir, speed );
	}

	VectorCopy( hordir, result.movedir );
	//
	return result;
}

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotFinishTravel_Jump( bot_movestate_t* ms, aas_reachability_t* reach )
{
	vec3_t			 hordir, hordir2;
	float			 speed, dist;
	bot_moveresult_t result;

	BotClearMoveResult( &result );

	// if not jumped yet
	if( !ms->jumpreach ) {
		return result;
	}

	// go straight to the reachability end
	hordir[0] = reach->end[0] - ms->origin[0];
	hordir[1] = reach->end[1] - ms->origin[1];
	hordir[2] = 0;
	dist	  = VectorNormalize( hordir );
	//
	hordir2[0] = reach->end[0] - reach->start[0];
	hordir2[1] = reach->end[1] - reach->start[1];
	hordir2[2] = 0;
	VectorNormalize( hordir2 );

	//
	if( DotProduct( hordir, hordir2 ) < -0.5 && dist < 24 ) {
		return result;
	}

	// always use max speed when traveling through the air
	speed = 800;
	//
	EA_Move( ms->client, hordir, speed );
	VectorCopy( hordir, result.movedir );
	//
	return result;
}

/*!
	\brief Computes the bot’s movement and view angles to traverse a ladder, including approach to the ladder base and fine adjustments to stay centered.

	When the bot is already against a ladder or not on the ground, the function sets the view to face the ladder horizontally and issues climbing movement commands. It also checks the bot’s
   lateral position relative to the ladder and issues left or right moves to keep the bot near the ladder’s center. If the bot is not yet against the ladder, it first moves to a position behind
   the ladder base, then, once within a short distance, switches to climbing toward the ladder top. During this approach, it calculates a horizontal movement direction and speed based on its
   distance from the base, and commands the bot to move accordingly. The movement direction is recorded, an ideal view angle is computed, and a movement view flag is set in the returned structure.

	\param ms Pointer to the bot’s current movement state
	\param reach Pointer to the ladder reachability in the AAS graph
	\return A bot_moveresult_t containing the chosen movement direction, ideal view angles, and flags indicating a movement view.
*/
bot_moveresult_t BotTravel_Ladder( bot_movestate_t* ms, aas_reachability_t* reach )
{
	// float dist, speed;
	vec3_t			 dir, viewdir, hordir, pos, p, v1, v2, vec, right;
	vec3_t			 origin = { 0, 0, 0 };
	//	vec3_t up = {0, 0, 1};
	bot_moveresult_t result;
	float			 dist, speed;

	// RF, heavily modified, wolf has different ladder movement

	BotClearMoveResult( &result );

	//
	if( ( ms->moveflags & MFL_AGAINSTLADDER )
		// NOTE: not a good idea for ladders starting in water
		|| !( ms->moveflags & MFL_ONGROUND ) ) {
		// botimport.Print(PRT_MESSAGE, "against ladder or not on ground\n");
		//  RF, wolf has different ladder movement
		VectorSubtract( reach->end, reach->start, dir );
		VectorNormalize( dir );
		// set the ideal view angles, facing the ladder up or down
		viewdir[0] = dir[0];
		viewdir[1] = dir[1];
		viewdir[2] = dir[2];

		if( dir[2] < 0 ) { // going down, so face the other way (towards ladder)
			VectorInverse( viewdir );
		}

		viewdir[2] = 0; // straight forward goes up
		VectorNormalize( viewdir );
		Vector2Angles( viewdir, result.ideal_viewangles );
		// elemantary action
		EA_Move( ms->client, origin, 0 );

		if( dir[2] < 0 ) { // going down, so face the other way
			EA_MoveBack( ms->client );

		} else {
			EA_MoveForward( ms->client );
		}

		// check for sideways adjustments to stay on the center of the ladder
		VectorMA( ms->origin, 18, viewdir, p );
		VectorCopy( reach->start, v1 );
		v1[2] = ms->origin[2];
		VectorCopy( reach->end, v2 );
		v2[2] = ms->origin[2];
		VectorSubtract( v2, v1, vec );
		VectorNormalize( vec );
		VectorMA( v1, -32, vec, v1 );
		VectorMA( v2, 32, vec, v2 );
		ProjectPointOntoVector( p, v1, v2, pos );
		VectorSubtract( pos, p, vec );

		if( VectorLength( vec ) > 2 ) {
			AngleVectors( result.ideal_viewangles, NULL, right, NULL );

			if( DotProduct( vec, right ) > 0 ) {
				EA_MoveRight( ms->client );

			} else {
				EA_MoveLeft( ms->client );
			}
		}

		// set movement view flag so the AI can see the view is focussed
		result.flags |= MOVERESULT_MOVEMENTVIEW;

	} else {
		// botimport.Print(PRT_MESSAGE, "moving towards ladder base\n");
		//  find a postion back away from the base of the ladder
		VectorSubtract( reach->end, reach->start, hordir );
		hordir[2] = 0;
		VectorNormalize( hordir );
		VectorMA( reach->start, -24, hordir, pos );
		// project our position onto the vector
		ProjectPointOntoVector( ms->origin, pos, reach->start, p );
		VectorSubtract( p, ms->origin, dir );
		// make sure the horizontal movement is large anough
		VectorCopy( dir, hordir );
		hordir[2] = 0;
		dist	  = VectorNormalize( hordir );

		if( dist < 8 ) { // within range, go for the end
			// botimport.Print(PRT_MESSAGE, "found base, moving towards ladder top\n");
			VectorSubtract( reach->end, ms->origin, dir );
			// make sure the horizontal movement is large anough
			VectorCopy( dir, hordir );
			hordir[2] = 0;
			dist	  = VectorNormalize( hordir );
			// set the ideal view angles, facing the ladder up or down
			viewdir[0] = dir[0];
			viewdir[1] = dir[1];
			viewdir[2] = dir[2];

			if( dir[2] < 0 ) { // going down, so face the other way
				VectorInverse( viewdir );
			}

			viewdir[2] = 0;
			VectorNormalize( viewdir );
			Vector2Angles( viewdir, result.ideal_viewangles );
			result.flags |= MOVERESULT_MOVEMENTVIEW;
		}

		//
		dir[0] = hordir[0];
		dir[1] = hordir[1];
		//		if (dist < 48) {
		//			if (dir[2] > 0) dir[2] = 1;
		//			else dir[2] = -1;
		//		} else {
		dir[2] = 0;

		//		}
		if( dist > 50 ) {
			dist = 50;
		}

		speed = 400 - ( 300 - 6 * dist );
		EA_Move( ms->client, dir, speed );
	}

	// save the movement direction
	VectorCopy( dir, result.movedir );
	//
	return result;
}

/*!
	\brief Moves a bot toward the center of a teleport reachability area when not already teleported, optionally setting a swim view flag

	The function first checks whether the bot has already been moved by a teleport; if so, it returns an empty move result. Otherwise it computes the vector from the bot’s current location to the
   reachability’s start point, zeroing the vertical component when the bot is not swimming. This vector is normalized and used to check for blocked paths. Depending on the distance to the target,
   an EA_Move command is issued with a shorter or longer speed. If the bot is swimming, a swim view flag is added to the result. The final movedirection is stored in the result structure and
   returned.

	\param ms Pointer to the current bot movement state, which includes the bot’s position, flags, and client reference
	\param reach Pointer to the reachability structure that defines the teleport area
	\return A bot_moveresult_t value describing the performed move, including move direction and any special flags such as swim view
*/
bot_moveresult_t BotTravel_Teleport( bot_movestate_t* ms, aas_reachability_t* reach )
{
	vec3_t			 hordir;
	float			 dist;
	bot_moveresult_t result;

	BotClearMoveResult( &result );

	// if the bot is being teleported
	if( ms->moveflags & MFL_TELEPORTED ) {
		return result;
	}

	// walk straight to center of the teleporter
	VectorSubtract( reach->start, ms->origin, hordir );

	if( !( ms->moveflags & MFL_SWIMMING ) ) {
		hordir[2] = 0;
	}

	dist = VectorNormalize( hordir );
	//
	BotCheckBlocked( ms, hordir, qtrue, &result );

	if( dist < 30 ) {
		EA_Move( ms->client, hordir, 200 );

	} else {
		EA_Move( ms->client, hordir, 400 );
	}

	if( ms->moveflags & MFL_SWIMMING ) {
		result.flags |= MOVERESULT_SWIMVIEW;
	}

	VectorCopy( hordir, result.movedir );
	return result;
}

/*!
	\brief Moves a bot toward an elevator platform, handling standing on the elevator, moving toward its center or end, and waiting when it is still moving up.

	The routine first checks if the bot is currently on the elevator platform. If it is, the bot either moves toward the elevator's end point when it is vertically close enough, or toward the
   platform's center otherwise, while performing barrier checks to avoid obstacles. If the bot is not on the elevator, the function measures how close it is to the reachability endpoint and issues
   travel commands at a speed scaled to that distance; swimming bots receive a swim view flag. When the elevator is still moving up, the bot waits and the result type is set to indicate an
   elevator still up. The function also covers the case where the elevator is down, moving the bot toward the reachability start or the platform center based on distance and direction, and checks
   for blockages. Throughout the routine the movement result direction, flags, and type are set appropriately before returning.

	\param ms pointer to the bot's movement state structure
	\param reach pointer to range data describing the elevator platform reachability
	\return a bot_moveresult_t with the chosen movement direction, result type and flags for the bot
*/
bot_moveresult_t BotTravel_Elevator( bot_movestate_t* ms, aas_reachability_t* reach )
{
	vec3_t			 dir, dir1, dir2, hordir, bottomcenter;
	float			 dist, dist1, dist2, speed;
	bot_moveresult_t result;

	BotClearMoveResult( &result );

	// if standing on the plat
	if( BotOnMover( ms->origin, ms->entitynum, reach ) ) {
#ifdef DEBUG_ELEVATOR
		botimport.Print( PRT_MESSAGE, "bot on elevator\n" );
#endif // DEBUG_ELEVATOR

		// if vertically not too far from the end point
		if( fabs( ms->origin[2] - reach->end[2] ) < sv_maxbarrier ) {
#ifdef DEBUG_ELEVATOR
			botimport.Print( PRT_MESSAGE, "bot moving to end\n" );
#endif // DEBUG_ELEVATOR
	   // move to the end point
			VectorSubtract( reach->end, ms->origin, hordir );
			hordir[2] = 0;
			VectorNormalize( hordir );

			if( !BotCheckBarrierJump( ms, hordir, 100 ) ) {
				EA_Move( ms->client, hordir, 400 );
			}

			VectorCopy( hordir, result.movedir );
		}

		// if not really close to the center of the elevator
		else {
			MoverBottomCenter( reach, bottomcenter );
			VectorSubtract( bottomcenter, ms->origin, hordir );
			hordir[2] = 0;
			dist	  = VectorNormalize( hordir );

			//
			if( dist > 10 ) {
#ifdef DEBUG_ELEVATOR
				botimport.Print( PRT_MESSAGE, "bot moving to center\n" );
#endif // DEBUG_ELEVATOR

				// move to the center of the plat
				if( dist > 100 ) {
					dist = 100;
				}

				speed = 400 - ( 400 - 4 * dist );
				//
				EA_Move( ms->client, hordir, speed );
				VectorCopy( hordir, result.movedir );
			}
		}

	} else {
#ifdef DEBUG_ELEVATOR
		botimport.Print( PRT_MESSAGE, "bot not on elevator\n" );
#endif // DEBUG_ELEVATOR
	   // if very near the reachability end
		VectorSubtract( reach->end, ms->origin, dir );
		dist = VectorLength( dir );

		if( dist < 64 ) {
			if( dist > 60 ) {
				dist = 60;
			}

			speed = 360 - ( 360 - 6 * dist );

			//
			if( ( ms->moveflags & MFL_SWIMMING ) || !BotCheckBarrierJump( ms, dir, 50 ) ) {
				if( speed > 5 ) {
					EA_Move( ms->client, dir, speed );
				}
			}

			VectorCopy( dir, result.movedir );

			//
			if( ms->moveflags & MFL_SWIMMING ) {
				result.flags |= MOVERESULT_SWIMVIEW;
			}

			// stop using this reachability
			ms->reachability_time = 0;
			return result;
		}

		// get direction and distance to reachability start
		VectorSubtract( reach->start, ms->origin, dir1 );

		if( !( ms->moveflags & MFL_SWIMMING ) ) {
			dir1[2] = 0;
		}

		dist1 = VectorNormalize( dir1 );

		// if the elevator isn't down
		if( !MoverDown( reach ) ) {
#ifdef DEBUG_ELEVATOR
			botimport.Print( PRT_MESSAGE, "elevator not down\n" );
#endif // DEBUG_ELEVATOR
			dist = dist1;
			VectorCopy( dir1, dir );
			//
			BotCheckBlocked( ms, dir, qfalse, &result );

			//
			if( dist > 60 ) {
				dist = 60;
			}

			speed = 360 - ( 360 - 6 * dist );

			//
			if( !( ms->moveflags & MFL_SWIMMING ) && !BotCheckBarrierJump( ms, dir, 50 ) ) {
				if( speed > 5 ) {
					EA_Move( ms->client, dir, speed );
				}
			}

			VectorCopy( dir, result.movedir );

			//
			if( ms->moveflags & MFL_SWIMMING ) {
				result.flags |= MOVERESULT_SWIMVIEW;
			}

			// this isn't a failure... just wait till the elevator comes down
			// result.failure = qtrue;
			result.type = RESULTTYPE_ELEVATORUP;
			result.flags |= MOVERESULT_WAITING;
			return result;
		}

		// get direction and distance to elevator bottom center
		MoverBottomCenter( reach, bottomcenter );
		VectorSubtract( bottomcenter, ms->origin, dir2 );

		if( !( ms->moveflags & MFL_SWIMMING ) ) {
			dir2[2] = 0;
		}

		dist2 = VectorNormalize( dir2 );

		// if very close to the reachability start or
		// closer to the elevator center or
		// between reachability start and elevator center
		if( dist1 < 20 || dist2 < dist1 || DotProduct( dir1, dir2 ) < 0 ) {
#ifdef DEBUG_ELEVATOR
			botimport.Print( PRT_MESSAGE, "bot moving to center\n" );
#endif // DEBUG_ELEVATOR
			dist = dist2;
			VectorCopy( dir2, dir );

		} else { // closer to the reachability start
#ifdef DEBUG_ELEVATOR
			botimport.Print( PRT_MESSAGE, "bot moving to start\n" );
#endif // DEBUG_ELEVATOR
			dist = dist1;
			VectorCopy( dir1, dir );
		}

		//
		BotCheckBlocked( ms, dir, qfalse, &result );

		//
		if( dist > 60 ) {
			dist = 60;
		}

		speed = 400 - ( 400 - 6 * dist );

		//
		if( !( ms->moveflags & MFL_SWIMMING ) && !BotCheckBarrierJump( ms, dir, 50 ) ) {
			EA_Move( ms->client, dir, speed );
		}

		VectorCopy( dir, result.movedir );

		//
		if( ms->moveflags & MFL_SWIMMING ) {
			result.flags |= MOVERESULT_SWIMVIEW;
		}
	}

	return result;
}

/*!
	\brief Finalizes elevator movement for a bot by choosing the direction with the smallest vertical displacement and issuing a move event

	The function clears a move result container then obtains the bottom center point of the elevator reachability. It constructs two direction vectors: one towards the elevator bottom and one
   towards the elevator’s end point. By comparing the absolute vertical components of these vectors, it selects the shorter vertical path. The chosen direction is normalized and a move event is
   issued for the bot client at a fixed speed of 300 units. The function returns the cleared result structure, usually containing no pending move actions.

	\param ms Current state of the bot, including its origin and associated client controller
	\param reach Reachability data describing the elevator’s geometry and endpoints
	\return Cleared bot move result object, typically containing no pending move actions
*/
bot_moveresult_t BotFinishTravel_Elevator( bot_movestate_t* ms, aas_reachability_t* reach )
{
	vec3_t			 bottomcenter, bottomdir, topdir;
	bot_moveresult_t result;

	BotClearMoveResult( &result );
	//
	MoverBottomCenter( reach, bottomcenter );
	VectorSubtract( bottomcenter, ms->origin, bottomdir );
	//
	VectorSubtract( reach->end, ms->origin, topdir );

	//
	if( fabs( bottomdir[2] ) < fabs( topdir[2] ) ) {
		VectorNormalize( bottomdir );
		EA_Move( ms->client, bottomdir, 300 );

	} else {
		VectorNormalize( topdir );
		EA_Move( ms->client, topdir, 300 );
	}

	return result;
}

/*!
	\brief Computes start, end, and origin vectors for a reachability area based on model data and spawn flags.

	The function uses the reachability structure to locate the model associated with the current reachability. It first extracts the model number from the lower 16 bits of facenum. If an entity
   with that model number is found, the function obtains its bounding box and computes the middle point; this point is copied into the start and end parameters. The upper 16 bits of facenum encode
   spawn flags. Depending on these flags, the function interprets the two 16‑bit halves of edgenum as signed indices for the start and end vectors along one of the XYZ axes, and it also adjusts
   the origin vector accordingly, placing it at the computed middle with an offset along the selected axis.

	If no entity with the expected model number can be located, the function logs an error, zeroes start and end, and exits.

	The result is that start and end represent the extremities of the reachability along the chosen axis, while origin is positioned at the model’s center with a shift along that axis.

	\param reach reachability data structure containing facenum and edgenum fields used to derive model number, spawn flags and edge indices
	\param start output vector initialized to the middle of the model’s bounding box and then adjusted based on spawn flags
	\param end output vector initialized similarly to start and later adjusted along an axis specified by spawn flags
	\param origin input vector overwritten with the position of the entity’s origin, then shifted along the axis indicated by spawn flags
*/
void BotFuncBobStartEnd( aas_reachability_t* reach, vec3_t start, vec3_t end, vec3_t origin )
{
	int	   spawnflags, modelnum;
	vec3_t mins, maxs, mid, angles = { 0, 0, 0 };
	int	   num0, num1;

	modelnum = reach->facenum & 0x0000FFFF;

	if( !AAS_OriginOfEntityWithModelNum( modelnum, origin ) ) {
		botimport.Print( PRT_MESSAGE, "BotFuncBobStartEnd: no entity with model %d\n", modelnum );
		VectorSet( start, 0, 0, 0 );
		VectorSet( end, 0, 0, 0 );
		return;
	}

	AAS_BSPModelMinsMaxsOrigin( modelnum, angles, mins, maxs, NULL );
	VectorAdd( mins, maxs, mid );
	VectorScale( mid, 0.5, mid );
	VectorCopy( mid, start );
	VectorCopy( mid, end );
	spawnflags = reach->facenum >> 16;
	num0	   = reach->edgenum >> 16;

	if( num0 > 0x00007FFF ) {
		num0 |= 0xFFFF0000;
	}

	num1 = reach->edgenum & 0x0000FFFF;

	if( num1 > 0x00007FFF ) {
		num1 |= 0xFFFF0000;
	}

	if( spawnflags & 1 ) {
		start[0] = num0;
		end[0]	 = num1;
		//
		origin[0] += mid[0];
		origin[1] = mid[1];
		origin[2] = mid[2];

	} else if( spawnflags & 2 ) {
		start[1] = num0;
		end[1]	 = num1;
		//
		origin[0] = mid[0];
		origin[1] += mid[1];
		origin[2] = mid[2];

	} else {
		start[2] = num0;
		end[2]	 = num1;
		//
		origin[0] = mid[0];
		origin[1] = mid[1];
		origin[2] += mid[2];
	}
}

/*!
	\brief Computes the bot's movement when traversing a moving platform (func_bobbing), selecting direction and speed while handling special cases such as being on the platform, near the
   platform's center or end, swimming, and obstacle avoidance.

	The function first clears the output result and obtains the bobbing platform's start, end, and current origin positions. It then checks whether the bot is standing on the moving platform. If
   so, it drives the bot toward the platform's end point when close enough, or toward the center of the platform otherwise, adjusting the speed based on distance. If the bot is not on the platform
   it attempts to move toward the reachability end or start, swimming or jumping as needed. The routine performs barrier‑jump checks, adjusts for vertical motion, and adds swim‑view or waiting
   flags depending on the situation. It ultimately returns a bot_moveresult_t containing a movement direction vector, flags, and movement type information.

	\param ms pointer to the bot's current movement state
	\param reach pointer to the reachability data describing the func_bobbing platform
	\return A bot_moveresult_t record describing the bot’s movement direction, any relevant flags (e.g., swim view, waiting), and the type of movement taken during the func_bobbing travel.
*/
bot_moveresult_t BotTravel_FuncBobbing( bot_movestate_t* ms, aas_reachability_t* reach )
{
	vec3_t			 dir, dir1, dir2, hordir, bottomcenter, bob_start, bob_end, bob_origin;
	float			 dist, dist1, dist2, speed;
	bot_moveresult_t result;

	BotClearMoveResult( &result );
	//
	BotFuncBobStartEnd( reach, bob_start, bob_end, bob_origin );

	// if standing ontop of the func_bobbing
	if( BotOnMover( ms->origin, ms->entitynum, reach ) ) {
#ifdef DEBUG_FUNCBOB
		botimport.Print( PRT_MESSAGE, "bot on func_bobbing\n" );
#endif
		// if near end point of reachability
		VectorSubtract( bob_origin, bob_end, dir );

		if( VectorLength( dir ) < 24 ) {
#ifdef DEBUG_FUNCBOB
			botimport.Print( PRT_MESSAGE, "bot moving to reachability end\n" );
#endif
			// move to the end point
			VectorSubtract( reach->end, ms->origin, hordir );
			hordir[2] = 0;
			VectorNormalize( hordir );

			if( !BotCheckBarrierJump( ms, hordir, 100 ) ) {
				EA_Move( ms->client, hordir, 400 );
			}

			VectorCopy( hordir, result.movedir );
		}

		// if not really close to the center of the elevator
		else {
			MoverBottomCenter( reach, bottomcenter );
			VectorSubtract( bottomcenter, ms->origin, hordir );
			hordir[2] = 0;
			dist	  = VectorNormalize( hordir );

			//
			if( dist > 10 ) {
#ifdef DEBUG_FUNCBOB
				botimport.Print( PRT_MESSAGE, "bot moving to func_bobbing center\n" );
#endif

				// move to the center of the plat
				if( dist > 100 ) {
					dist = 100;
				}

				speed = 400 - ( 400 - 4 * dist );
				//
				EA_Move( ms->client, hordir, speed );
				VectorCopy( hordir, result.movedir );
			}
		}

	} else {
#ifdef DEBUG_FUNCBOB
		botimport.Print( PRT_MESSAGE, "bot not ontop of func_bobbing\n" );
#endif
		// if very near the reachability end
		VectorSubtract( reach->end, ms->origin, dir );
		dist = VectorLength( dir );

		if( dist < 64 ) {
#ifdef DEBUG_FUNCBOB
			botimport.Print( PRT_MESSAGE, "bot moving to end\n" );
#endif

			if( dist > 60 ) {
				dist = 60;
			}

			speed = 360 - ( 360 - 6 * dist );

			// if swimming or no barrier jump
			if( ( ms->moveflags & MFL_SWIMMING ) || !BotCheckBarrierJump( ms, dir, 50 ) ) {
				if( speed > 5 ) {
					EA_Move( ms->client, dir, speed );
				}
			}

			VectorCopy( dir, result.movedir );

			//
			if( ms->moveflags & MFL_SWIMMING ) {
				result.flags |= MOVERESULT_SWIMVIEW;
			}

			// stop using this reachability
			ms->reachability_time = 0;
			return result;
		}

		// get direction and distance to reachability start
		VectorSubtract( reach->start, ms->origin, dir1 );

		if( !( ms->moveflags & MFL_SWIMMING ) ) {
			dir1[2] = 0;
		}

		dist1 = VectorNormalize( dir1 );
		// if func_bobbing is Not it's start position
		VectorSubtract( bob_origin, bob_start, dir );

		if( VectorLength( dir ) > 16 ) {
#ifdef DEBUG_FUNCBOB
			botimport.Print( PRT_MESSAGE, "func_bobbing not at start\n" );
#endif
			dist = dist1;
			VectorCopy( dir1, dir );
			//
			BotCheckBlocked( ms, dir, qfalse, &result );

			//
			if( dist > 60 ) {
				dist = 60;
			}

			speed = 360 - ( 360 - 6 * dist );

			//
			if( !( ms->moveflags & MFL_SWIMMING ) && !BotCheckBarrierJump( ms, dir, 50 ) ) {
				if( speed > 5 ) {
					EA_Move( ms->client, dir, speed );
				}
			}

			VectorCopy( dir, result.movedir );

			//
			if( ms->moveflags & MFL_SWIMMING ) {
				result.flags |= MOVERESULT_SWIMVIEW;
			}

			// this isn't a failure... just wait till the func_bobbing arrives
			result.type = RESULTTYPE_ELEVATORUP;
			result.flags |= MOVERESULT_WAITING;
			return result;
		}

		// get direction and distance to func_bob bottom center
		MoverBottomCenter( reach, bottomcenter );
		VectorSubtract( bottomcenter, ms->origin, dir2 );

		if( !( ms->moveflags & MFL_SWIMMING ) ) {
			dir2[2] = 0;
		}

		dist2 = VectorNormalize( dir2 );

		// if very close to the reachability start or
		// closer to the elevator center or
		// between reachability start and func_bobbing center
		if( dist1 < 20 || dist2 < dist1 || DotProduct( dir1, dir2 ) < 0 ) {
#ifdef DEBUG_FUNCBOB
			botimport.Print( PRT_MESSAGE, "bot moving to func_bobbing center\n" );
#endif
			dist = dist2;
			VectorCopy( dir2, dir );

		} else { // closer to the reachability start
#ifdef DEBUG_FUNCBOB
			botimport.Print( PRT_MESSAGE, "bot moving to reachability start\n" );
#endif
			dist = dist1;
			VectorCopy( dir1, dir );
		}

		//
		BotCheckBlocked( ms, dir, qfalse, &result );

		//
		if( dist > 60 ) {
			dist = 60;
		}

		speed = 400 - ( 400 - 6 * dist );

		//
		if( !( ms->moveflags & MFL_SWIMMING ) && !BotCheckBarrierJump( ms, dir, 50 ) ) {
			EA_Move( ms->client, dir, speed );
		}

		VectorCopy( dir, result.movedir );

		//
		if( ms->moveflags & MFL_SWIMMING ) {
			result.flags |= MOVERESULT_SWIMVIEW;
		}
	}

	return result;
}

/*!
	\brief Adjusts a bot’s position after travelling through a func_bobbing platform and reports the movement performed

	The function calculates the remaining distance to the end of the bobbing effect by examining the reachability’s start, end and origin points.  If the bot is within 16 units of the bob’s end,
   it aligns the bot toward the platform’s goal and issues a move command whose speed scales with the remaining distance but is capped.  If the bot is farther away, it targets the centre of the
   moving platform, similarly normalising direction, clamping distances, and setting a movement speed proportional to distance.  Swimming conditions are respected by zeroing the vertical component
   when not swimming, and a flag indicating a swimming view is set when appropriate.  The movement direction and any flags are stored in a bot_moveresult_t and returned.

	The routine does not perform any I/O other than through the EA_Move call and does not throw exceptions.

	\param ms Pointer to the bot’s current movement state, providing its origin position and movement flags such as swimming
	\param reach Pointer to the reachability information for the func_bobbing entity, used to retrieve the start, end, and bob origin for calculations
	\return A bot_moveresult_t struct that contains the movement direction vector chosen and any flags that were set (e.g., indicating swimming view).
*/
bot_moveresult_t BotFinishTravel_FuncBobbing( bot_movestate_t* ms, aas_reachability_t* reach )
{
	vec3_t			 bob_origin, bob_start, bob_end, dir, hordir, bottomcenter;
	bot_moveresult_t result;
	float			 dist, speed;

	BotClearMoveResult( &result );
	//
	BotFuncBobStartEnd( reach, bob_start, bob_end, bob_origin );
	//
	VectorSubtract( bob_origin, bob_end, dir );
	dist = VectorLength( dir );

	// if the func_bobbing is near the end
	if( dist < 16 ) {
		VectorSubtract( reach->end, ms->origin, hordir );

		if( !( ms->moveflags & MFL_SWIMMING ) ) {
			hordir[2] = 0;
		}

		dist = VectorNormalize( hordir );

		//
		if( dist > 60 ) {
			dist = 60;
		}

		speed = 360 - ( 360 - 6 * dist );

		//
		if( speed > 5 ) {
			EA_Move( ms->client, dir, speed );
		}

		VectorCopy( dir, result.movedir );

		//
		if( ms->moveflags & MFL_SWIMMING ) {
			result.flags |= MOVERESULT_SWIMVIEW;
		}

	} else {
		MoverBottomCenter( reach, bottomcenter );
		VectorSubtract( bottomcenter, ms->origin, hordir );

		if( !( ms->moveflags & MFL_SWIMMING ) ) {
			hordir[2] = 0;
		}

		dist = VectorNormalize( hordir );

		//
		if( dist > 5 ) {
			// move to the center of the plat
			if( dist > 100 ) {
				dist = 100;
			}

			speed = 400 - ( 400 - 4 * dist );
			//
			EA_Move( ms->client, hordir, speed );
			VectorCopy( hordir, result.movedir );
		}
	}

	return result;
}

/*!
	\brief Determines the current status of a bot’s grapple hook relative to a reachability.

	The function scans all AAS entities for a grapple hook model and checks whether the hook is still in flight or has successfully hooked a surface. It uses a static model index to locate the
   hook entity and, for each candidate, compares its origin with the original visible origin. If the origin has not changed it calculates the distance to the reachability end point; a distance
   less than 32 units indicates the hook has attached, so 2 is returned. If the origin differs, the hook is still being fired, yielding 1. If no valid hook is found, 0 is returned. The function
   also respects the optional laserhook configuration, although that path is currently commented out.

	\param ms Pointer to the bot’s movement state structure.
	\param reach Pointer to the reachability data used for calculating distances.
	\return An integer: 0 for no visible hook, 1 for a hook that is still flying, 2 for a hook that has successfully attached to a surface.
*/
int GrappleState( bot_movestate_t* ms, aas_reachability_t* reach )
{
	static int		 grapplemodelindex;
	static libvar_t* laserhook;
	int				 i;
	vec3_t			 dir;
	aas_entityinfo_t entinfo;

	if( !laserhook ) {
		laserhook = LibVar( "laserhook", "0" );
	}

	if( !laserhook->value && !grapplemodelindex ) {
		grapplemodelindex = AAS_IndexFromModel( "models/weapons/grapple/hook/tris.md2" );
	}

	for( i = AAS_NextEntity( 0 ); i; i = AAS_NextEntity( i ) ) {
		if( ( !laserhook->value && AAS_EntityModelindex( i ) == grapplemodelindex )
			//			|| (laserhook->value && (AAS_EntityRenderFX(i) & RF_BEAM))
		) {
			AAS_EntityInfo( i, &entinfo );

			// if the origin is equal to the last visible origin
			if( VectorCompare( entinfo.origin, entinfo.lastvisorigin ) ) {
				VectorSubtract( entinfo.origin, reach->end, dir );

				// if hooked near the reachability end
				if( VectorLength( dir ) < 32 ) {
					return 2;
				}

			} else {
				// still shooting hook
				return 1;
			}
		}
	}

	// no valid grapple at all
	return 0;
}

/*!
	\brief Resets the bot's grapple state when it is no longer using a grapple hook.

	The function retrieves the reachability associated with the bot's last reach number. If that reachability is not of type TRAVEL_GRAPPLEHOOK and the bot currently has an active grapple or a
   remaining grapple visible time, it sends the "hook off" command to the bot’s client, clears the MFL_ACTIVEGRAPPLE flag, and resets the grapple visible time. A debug message may be printed when
   DEBUG_GRAPPLE is enabled.

	\param ms Pointer to the bot's movement state structure, from which the last reach number, movement flags and grapple visibility timer are accessed.
*/
void BotResetGrapple( bot_movestate_t* ms )
{
	aas_reachability_t reach;

	AAS_ReachabilityFromNum( ms->lastreachnum, &reach );

	// if not using the grapple hook reachability anymore
	if( reach.traveltype != TRAVEL_GRAPPLEHOOK ) {
		if( ( ms->moveflags & MFL_ACTIVEGRAPPLE ) || ms->grapplevisible_time ) {
			EA_Command( ms->client, CMD_HOOKOFF );
			ms->moveflags &= ~MFL_ACTIVEGRAPPLE;
			ms->grapplevisible_time = 0;
#ifdef DEBUG_GRAPPLE
			botimport.Print( PRT_MESSAGE, "reset grapple\n" );
#endif // DEBUG_GRAPPLE
		}
	}
}

/*!
	\brief Manages a bot’s grapple travel, handling activation, state progression, movement toward the target, and release conditions.

	When a grapple reset flag is active the bot is forced to hook off and the active grapple flag cleared. If the bot is already grabbing, the function checks the grapple state and distance; if
   too close or no progress is made, it releases the grapple and resets the state. If the grapple becomes invisible after a timeout it is also released. When the grapple is inactive the bot
   computes a direction to the grapple target, activates the grapple when within a small distance and the view is aligned, otherwise it moves toward the target with a speed modulated by distance,
   checking for blocked movement. The reachability status is updated if the bot enters a different area. All actions are recorded in a bot_moveresult_t which indicates movement direction, ideal
   view angles and relevant flags.

	\param ms Pointer to the bot’s current movement state
	\param reach Pointer to the reachability information defining the grapple route
	\return A bot_moveresult_t describing the movement direction, view angles, and movement flags generated during the grapple operation.
*/
bot_moveresult_t BotTravel_Grapple( bot_movestate_t* ms, aas_reachability_t* reach )
{
	bot_moveresult_t result;
	float			 dist, speed;
	vec3_t			 dir, viewdir, org;
	int				 state, areanum;

#ifdef DEBUG_GRAPPLE
	static int debugline;

	if( !debugline ) {
		debugline = botimport.DebugLineCreate();
	}

	botimport.DebugLineShow( debugline, reach->start, reach->end, LINECOLOR_BLUE );
#endif // DEBUG_GRAPPLE

	BotClearMoveResult( &result );

	//
	if( ms->moveflags & MFL_GRAPPLERESET ) {
		EA_Command( ms->client, CMD_HOOKOFF );
		ms->moveflags &= ~MFL_ACTIVEGRAPPLE;
		return result;
	}

	//
	if( ms->moveflags & MFL_ACTIVEGRAPPLE ) {
#ifdef DEBUG_GRAPPLE
		botimport.Print( PRT_MESSAGE, "BotTravel_Grapple: active grapple\n" );
#endif // DEBUG_GRAPPLE
	   //
		state = GrappleState( ms, reach );
		//
		VectorSubtract( reach->end, ms->origin, dir );
		dir[2] = 0;
		dist   = VectorLength( dir );

		// if very close to the grapple end or
		// the grappled is hooked and the bot doesn't get any closer
		if( state && dist < 48 ) {
			if( ms->lastgrappledist - dist < 1 ) {
				EA_Command( ms->client, CMD_HOOKOFF );
				ms->moveflags &= ~MFL_ACTIVEGRAPPLE;
				ms->moveflags |= MFL_GRAPPLERESET;
				ms->reachability_time = 0; // end the reachability
#ifdef DEBUG_GRAPPLE
				botimport.Print( PRT_ERROR, "grapple normal end\n" );
#endif // DEBUG_GRAPPLE
			}
		}

		// if no valid grapple at all, or the grapple hooked and the bot
		// isn't moving anymore
		else if( !state || ( state == 2 && dist > ms->lastgrappledist - 2 ) ) {
			if( ms->grapplevisible_time < AAS_Time() - 0.4 ) {
#ifdef DEBUG_GRAPPLE
				botimport.Print( PRT_ERROR, "grapple not visible\n" );
#endif // DEBUG_GRAPPLE
				EA_Command( ms->client, CMD_HOOKOFF );
				ms->moveflags &= ~MFL_ACTIVEGRAPPLE;
				ms->moveflags |= MFL_GRAPPLERESET;
				ms->reachability_time = 0; // end the reachability
				// result.failure = qtrue;
				// result.type = RESULTTYPE_INVISIBLEGRAPPLE;
				return result;
			}

		} else {
			ms->grapplevisible_time = AAS_Time();
		}

		// remember the current grapple distance
		ms->lastgrappledist = dist;

	} else {
#ifdef DEBUG_GRAPPLE
		botimport.Print( PRT_MESSAGE, "BotTravel_Grapple: inactive grapple\n" );
#endif // DEBUG_GRAPPLE
	   //
		ms->grapplevisible_time = AAS_Time();
		//
		VectorSubtract( reach->start, ms->origin, dir );

		if( !( ms->moveflags & MFL_SWIMMING ) ) {
			dir[2] = 0;
		}

		VectorAdd( ms->origin, ms->viewoffset, org );
		VectorSubtract( reach->end, org, viewdir );
		//
		dist = VectorNormalize( dir );
		Vector2Angles( viewdir, result.ideal_viewangles );
		result.flags |= MOVERESULT_MOVEMENTVIEW;

		//
		if( dist < 5 && fabs( AngleDiff( result.ideal_viewangles[0], ms->viewangles[0] ) ) < 2 && fabs( AngleDiff( result.ideal_viewangles[1], ms->viewangles[1] ) ) < 2 ) {
#ifdef DEBUG_GRAPPLE
			botimport.Print( PRT_MESSAGE, "BotTravel_Grapple: activating grapple\n" );
#endif // DEBUG_GRAPPLE
			EA_Command( ms->client, CMD_HOOKON );
			ms->moveflags |= MFL_ACTIVEGRAPPLE;
			ms->lastgrappledist = 999999;

		} else {
			if( dist < 70 ) {
				speed = 300 - ( 300 - 4 * dist );

			} else {
				speed = 400;
			}

			//
			BotCheckBlocked( ms, dir, qtrue, &result );
			// elemantary action move in direction
			EA_Move( ms->client, dir, speed );
			VectorCopy( dir, result.movedir );
		}

		// if in another area before actually grappling
		areanum = AAS_PointAreaNum( ms->origin );

		if( areanum && areanum != ms->reachareanum ) {
			ms->reachability_time = 0;
		}
	}

	return result;
}

/*!
	\brief Executes a rocket‑jump to reach a target, selecting the rocket launcher, moving toward the destination, and attacking.

	The routine first calculates the horizontal vector from the bot’s current location to the start point of the specified reachability. If the bot is within 5 units of that start point, it
   instead targets the reachability’s end point and triggers a short jump combined with an attack and forward movement. For more distant cases it limits the distance to 80 units, derives a speed
   that increases with distance, and commands the bot to move in that direction at the computed speed. After issuing movement commands, the function sets the bot’s ideal view to match the travel
   direction, forces a 90‑degree pitch to look straight down, and records that the view was set. It then guarantees the rocket launcher is selected and records this choice as the movement weapon.
   Finally, it populates the result struct with the movement direction, view angles, weapon, and relevant flags before returning it.

	\param ms Current state of the bot, containing origin coordinates and client information for issuing commands
	\param reach Reachability structure describing the target segment, including start and end coordinates
	\return A bot_moveresult_t object that records the movement direction, ideal view angles, chosen weapon, and flags indicating that the view and weapon were set for movement.
*/
bot_moveresult_t BotTravel_RocketJump( bot_movestate_t* ms, aas_reachability_t* reach )
{
	vec3_t			 hordir;
	float			 dist, speed;
	bot_moveresult_t result;

	// botimport.Print(PRT_MESSAGE, "BotTravel_RocketJump: bah\n");
	BotClearMoveResult( &result );
	//
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	//
	dist = VectorNormalize( hordir );

	//
	if( dist < 5 ) {
		//		botimport.Print(PRT_MESSAGE, "between jump start and run start point\n");
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		VectorNormalize( hordir );
		// elemantary action jump
		EA_Jump( ms->client );
		EA_Attack( ms->client );
		EA_Move( ms->client, hordir, 800 );
		//
		ms->jumpreach = ms->lastreachnum;

	} else {
		if( dist > 80 ) {
			dist = 80;
		}

		speed = 400 - ( 400 - 5 * dist );
		EA_Move( ms->client, hordir, speed );
	}

	// look in the movement direction
	Vector2Angles( hordir, result.ideal_viewangles );
	// look straight down
	result.ideal_viewangles[PITCH] = 90;
	// set the view angles directly
	EA_View( ms->client, result.ideal_viewangles );
	// view is important for the movment
	result.flags |= MOVERESULT_MOVEMENTVIEWSET;
	// select the rocket launcher
	EA_SelectWeapon( ms->client, WEAPONINDEX_ROCKET_LAUNCHER );
	// weapon is used for movement
	result.weapon = WEAPONINDEX_ROCKET_LAUNCHER;
	result.flags |= MOVERESULT_MOVEMENTWEAPON;
	//
	VectorCopy( hordir, result.movedir );
	//
	return result;
}

/*!
	\brief Initializes and returns an empty bot move result for a BFG jump

	The function currently creates a bot_moveresult_t structure, clears it using BotClearMoveResult, and returns it without performing any movement logic or modifying the movement state or
   reachability data. This appears to be a placeholder for future implementation.

	\param ms Pointer to the bot's current movement state
	\param reach Pointer to the reachability data for the destination
	\return A cleared bot_moveresult_t indicating no movement action performed
*/
bot_moveresult_t BotTravel_BFGJump( bot_movestate_t* ms, aas_reachability_t* reach )
{
	bot_moveresult_t result;

	BotClearMoveResult( &result );
	//
	return result;
}

/*!
	\brief Finishes a weapon jump by issuing a horizontal move toward the destination defined by the reachability.

	If the bot has not yet performed its jump, the function returns an empty move result. Otherwise it calculates a horizontal direction vector from the bot's current position to the end point of
   the provided reachability, normalises this vector, and sends an EA_Move command with a maximum speed of 800 units per second. The resulting movement direction is stored in the move result
   before it is returned.

	\param ms Pointer to the bot's movement state, containing the current origin, client reference, and jump status.
	\param reach Pointer to an AAS reachability structure, providing the target end location for the jump.
	\return A move result structure with the computed direction that the bot should use to finish the weapon jump.
*/
bot_moveresult_t BotFinishTravel_WeaponJump( bot_movestate_t* ms, aas_reachability_t* reach )
{
	vec3_t			 hordir;
	bot_moveresult_t result;

	BotClearMoveResult( &result );

	// if not jumped yet
	if( !ms->jumpreach ) {
		return result;
	}

	// go straight to the reachability end
	hordir[0] = reach->end[0] - ms->origin[0];
	hordir[1] = reach->end[1] - ms->origin[1];
	hordir[2] = 0;
	VectorNormalize( hordir );
	// always use max speed when traveling through the air
	EA_Move( ms->client, hordir, 800 );
	VectorCopy( hordir, result.movedir );
	//
	return result;
}

/*!
	\brief Computes a bot movement result by walking horizontally toward a jump pad’s start point at a fixed speed.

	The function begins by clearing the move result structure. It then calculates a horizontal direction vector from the bot’s current origin to the start point of the provided reachability,
   normalizes the vector, and records the distance. It checks whether any obstacles block the path using BotCheckBlocked. If the path is clear, it issues a move command to the bot client in the
   calculated direction at a constant speed of 400 units using EA_Move. The horizontal direction vector is copied into the result’s movedir field. The resulting bot_moveresult_t, which may include
   failure status set by the blocked‑check, is returned.

	\param ms Pointer to the bot movement state information
	\param reach Pointer to the reachability structure containing the jump pad start point
	\return A bot_moveresult_t object that holds the movement direction and any status information produced during the move
*/
bot_moveresult_t BotTravel_JumpPad( bot_movestate_t* ms, aas_reachability_t* reach )
{
	float			 dist, speed;
	vec3_t			 hordir;
	bot_moveresult_t result;

	BotClearMoveResult( &result );
	// first walk straight to the reachability start
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	dist	  = VectorNormalize( hordir );
	//
	BotCheckBlocked( ms, hordir, qtrue, &result );
	speed = 400;
	// elemantary action move in direction
	EA_Move( ms->client, hordir, speed );
	VectorCopy( hordir, result.movedir );
	//
	return result;
}

/*!
	\brief Computes and executes the final movement toward a jump pad, determining direction and speed with air control or a fallback, checking for obstacles, and recording the resulting move.

	The function first clears any previous move result. It attempts to use air control to adjust the bot's velocity so that it reaches the specified reachability endpoint. If air control fails, it
   defaults to a horizontal direction from the bot's current position to the target endpoint and sets a fixed speed of 400. Before issuing the move, it checks whether that direction is blocked,
   recording any failures in the result structure. The move is then issued via EA_Move, and the chosen direction is stored in result.movedir. Finally the completed result is returned.

	\param ms Current movement state of the bot, including its position, velocity, and client information.
	\param reach Destination reachability node that represents the jump pad target location.
	\return A bot_moveresult_t structure containing the executed move's direction, any set failure flags, and other related data.
*/
bot_moveresult_t BotFinishTravel_JumpPad( bot_movestate_t* ms, aas_reachability_t* reach )
{
	float			 speed;
	vec3_t			 hordir;
	bot_moveresult_t result;

	BotClearMoveResult( &result );

	if( !BotAirControl( ms->origin, ms->velocity, reach->end, hordir, &speed ) ) {
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		VectorNormalize( hordir );
		speed = 400;
	}

	BotCheckBlocked( ms, hordir, qtrue, &result );
	// elemantary action move in direction
	EA_Move( ms->client, hordir, speed );
	VectorCopy( hordir, result.movedir );
	//
	return result;
}

/*!
	\brief Calculates the number of ticks before a reachability becomes invalid based on its travel type.

	The function examines the travel type of the given reachability and returns a predefined timeout value expressed in ticks.
	- WALK, CROUCH, BARRIERJUMP, WALKOFFLEDGE, JUMP, SWIM, WATERJUMP, TELEPORT all return 5 ticks.
	- LADDER returns 6 ticks.
	- ELEVATOR returns 10 ticks.
	- GRAPPLEHOOK returns 8 ticks.
	- ROCKETJUMP returns 6 ticks.
	- JUMPPAD and FUNCBOB each return 10 ticks.
	If the travel type is unrecognized, an error is printed via botimport.Print and a default value of 8 ticks is returned.
	The returned value is intended to be added to the function AAS_Time() to set a reachability timeout.


	\param reach Pointer to the reachability structure whose traveltype field determines the timeout value.
	\return The timeout duration in ticks.
*/
int BotReachabilityTime( aas_reachability_t* reach )
{
	switch( reach->traveltype ) {
		case TRAVEL_WALK:
			return 5;

		case TRAVEL_CROUCH:
			return 5;

		case TRAVEL_BARRIERJUMP:
			return 5;

		case TRAVEL_LADDER:
			return 6;

		case TRAVEL_WALKOFFLEDGE:
			return 5;

		case TRAVEL_JUMP:
			return 5;

		case TRAVEL_SWIM:
			return 5;

		case TRAVEL_WATERJUMP:
			return 5;

		case TRAVEL_TELEPORT:
			return 5;

		case TRAVEL_ELEVATOR:
			return 10;

		case TRAVEL_GRAPPLEHOOK:
			return 8;

		case TRAVEL_ROCKETJUMP:
			return 6;

		// case TRAVEL_BFGJUMP: return 6;
		case TRAVEL_JUMPPAD:
			return 10;

		case TRAVEL_FUNCBOB:
			return 10;

		default: {
			botimport.Print( PRT_ERROR, "travel type %d not implemented yet\n", reach->traveltype );
			return 8;
		} // end case
	}
}

/*!
	\brief Moves the bot directly toward a goal area by calculating direction, speed and travel type.

	The function computes a vector from the bot’s current position to the goal’s origin, normalises it to obtain a unit direction, and determines whether the bot should walk or swim based on its
   movement flags. The distance to the goal is capped at 100 units unless the goal disables slowing, from which a speed is derived; speeds below 10 units are set to zero to avoid negligible
   motion. Obstacle checks are performed, a move action is issued with the computed direction and speed, and the resulting move data is stored. For swimming movement, the function also updates
   view angles and sets an appropriate flag. Finally, the bot state is updated with bookkeeping information such as the last area and time of the move.

	\param ms Current movement state of the bot, to be updated with new position and move information.
	\param goal Target goal area the bot should move toward.
	\return A bot_moveresult_t object describing the performed movement, including direction, speed, travel type, and any special flags such as swim view.
*/
bot_moveresult_t BotMoveInGoalArea( bot_movestate_t* ms, bot_goal_t* goal )
{
	bot_moveresult_t result;
	vec3_t			 dir;
	float			 dist, speed;

#ifdef DEBUG
	// botimport.Print(PRT_MESSAGE, "%s: moving straight to goal\n", ClientName(ms->entitynum-1));
	// AAS_ClearShownDebugLines();
	// AAS_DebugLine(ms->origin, goal->origin, LINECOLOR_RED);
#endif // DEBUG
	BotClearMoveResult( &result );
	// walk straight to the goal origin
	dir[0] = goal->origin[0] - ms->origin[0];
	dir[1] = goal->origin[1] - ms->origin[1];

	if( ms->moveflags & MFL_SWIMMING ) {
		dir[2]			  = goal->origin[2] - ms->origin[2];
		result.traveltype = TRAVEL_SWIM;

	} else {
		dir[2]			  = 0;
		result.traveltype = TRAVEL_WALK;
	} // endif

	//
	dist = VectorNormalize( dir );

	if( dist > 100 || ( goal->flags & GFL_NOSLOWAPPROACH ) ) {
		dist = 100;
	}

	speed = 400 - ( 400 - 4 * dist );

	if( speed < 10 ) {
		speed = 0;
	}

	//
	BotCheckBlocked( ms, dir, qtrue, &result );
	// elemantary action move in direction
	EA_Move( ms->client, dir, speed );
	VectorCopy( dir, result.movedir );

	//
	if( ms->moveflags & MFL_SWIMMING ) {
		Vector2Angles( dir, result.ideal_viewangles );
		result.flags |= MOVERESULT_SWIMVIEW;
	}

	// if (!debugline) debugline = botimport.DebugLineCreate();
	// botimport.DebugLineShow(debugline, ms->origin, goal->origin, LINECOLOR_BLUE);
	//
	ms->lastreachnum	= 0;
	ms->lastareanum		= 0;
	ms->lastgoalareanum = goal->areanum;
	VectorCopy( ms->origin, ms->lastorigin );
	ms->lasttime = AAS_Time();
	//
	return result;
}

int			 AAS_AreaRouteToGoalArea( int areanum, vec3_t origin, int goalareanum, int travelflags, int* traveltime, int* reachnum );
extern float VectorDistance( vec3_t v1, vec3_t v2 );
void		 BotMoveToGoal( bot_moveresult_t* result, int movestate, bot_goal_t* goal, int travelflags )
{
	int				   reachnum = 0, lastreachnum, foundjumppad, ent; // TTimo: init
	aas_reachability_t reach, lastreach;
	bot_movestate_t*   ms;
	// vec3_t mins, maxs, up = {0, 0, 1};
	// bsp_trace_t trace;
	// static int debugline;

	BotClearMoveResult( result );
	//
	ms = BotMoveStateFromHandle( movestate );

	if( !ms ) {
		return;
	}

	// reset the grapple before testing if the bot has a valid goal
	// because the bot could loose all it's goals when stuck to a wall
	BotResetGrapple( ms );

	//
	if( !goal ) {
#ifdef DEBUG
		botimport.Print( PRT_MESSAGE, "client %d: movetogoal -> no goal\n", ms->client );
#endif // DEBUG
		result->failure = qtrue;
		return;
	}

	// botimport.Print(PRT_MESSAGE, "numavoidreach = %d\n", ms->numavoidreach);
	// remove some of the move flags
	ms->moveflags &= ~( MFL_SWIMMING | MFL_AGAINSTLADDER );

	// set some of the move flags
	// NOTE: the MFL_ONGROUND flag is also set in the higher AI
	if( AAS_OnGround( ms->origin, ms->presencetype, ms->entitynum ) ) {
		ms->moveflags |= MFL_ONGROUND;
	}

	//

	if( ms->moveflags & MFL_ONGROUND ) {
		int modeltype, modelnum;

		ent = BotOnTopOfEntity( ms );

		if( ent != -1 ) {
			modelnum = AAS_EntityModelindex( ent );

			if( modelnum >= 0 && modelnum < MAX_MODELS ) {
				modeltype = modeltypes[modelnum];

				if( modeltype == MODELTYPE_FUNC_PLAT ) {
					AAS_ReachabilityFromNum( ms->lastreachnum, &reach );

					// if the bot is Not using the elevator
					if( reach.traveltype != TRAVEL_ELEVATOR ||
						// NOTE: the face number is the plat model number
						( reach.facenum & 0x0000FFFF ) != modelnum ) {
						reachnum = AAS_NextModelReachability( 0, modelnum );

						if( reachnum ) {
							// botimport.Print(PRT_MESSAGE, "client %d: accidentally ended up on func_plat\n", ms->client);
							AAS_ReachabilityFromNum( reachnum, &reach );
							ms->lastreachnum	  = reachnum;
							ms->reachability_time = AAS_Time() + BotReachabilityTime( &reach );

						} else {
							if( bot_developer ) {
								botimport.Print( PRT_MESSAGE, "client %d: on func_plat without reachability\n", ms->client );
							}

							result->blocked		= qtrue;
							result->blockentity = ent;
							result->flags |= MOVERESULT_ONTOPOFOBSTACLE;
							return;
						}
					}

					result->flags |= MOVERESULT_ONTOPOF_ELEVATOR;

				} else if( modeltype == MODELTYPE_FUNC_BOB ) {
					AAS_ReachabilityFromNum( ms->lastreachnum, &reach );

					// if the bot is Not using the func bobbing
					if( reach.traveltype != TRAVEL_FUNCBOB ||
						// NOTE: the face number is the func_bobbing model number
						( reach.facenum & 0x0000FFFF ) != modelnum ) {
						reachnum = AAS_NextModelReachability( 0, modelnum );

						if( reachnum ) {
							// botimport.Print(PRT_MESSAGE, "client %d: accidentally ended up on func_bobbing\n", ms->client);
							AAS_ReachabilityFromNum( reachnum, &reach );
							ms->lastreachnum	  = reachnum;
							ms->reachability_time = AAS_Time() + BotReachabilityTime( &reach );

						} else {
							if( bot_developer ) {
								botimport.Print( PRT_MESSAGE, "client %d: on func_bobbing without reachability\n", ms->client );
							}

							result->blocked		= qtrue;
							result->blockentity = ent;
							result->flags |= MOVERESULT_ONTOPOFOBSTACLE;
							return;
						}
					}

					result->flags |= MOVERESULT_ONTOPOF_FUNCBOB;
				}

				/* Ridah, disabled this, or standing on little fragments causes problems
				else
				{
				  result->blocked = qtrue;
				  result->blockentity = ent;
				  result->flags |= MOVERESULT_ONTOPOFOBSTACLE;
				  return;
				}
				*/
			}
		}
	}

	// if swimming
	if( AAS_Swimming( ms->origin ) ) {
		ms->moveflags |= MFL_SWIMMING;
	}

	// if against a ladder
	if( AAS_AgainstLadder( ms->origin, ms->areanum ) ) {
		ms->moveflags |= MFL_AGAINSTLADDER;
	}

	// if the bot is on the ground, swimming or against a ladder
	if( ms->moveflags & ( MFL_ONGROUND | MFL_SWIMMING | MFL_AGAINSTLADDER ) ) {
		// botimport.Print(PRT_MESSAGE, "%s: onground, swimming or against ladder\n", ClientName(ms->entitynum-1));
		//
		AAS_ReachabilityFromNum( ms->lastreachnum, &lastreach );
		// reachability area the bot is in
		// ms->areanum = BotReachabilityArea(ms->origin, (lastreach.traveltype != TRAVEL_ELEVATOR));
		ms->areanum = BotFuzzyPointReachabilityArea( ms->origin );

		// if the bot is in the goal area
		if( ms->areanum == goal->areanum ) {
			*result = BotMoveInGoalArea( ms, goal );
			return;
		}

		// assume we can use the reachability from the last frame
		reachnum = ms->lastreachnum;
		// if there is a last reachability

		// Ridah, best calc it each frame, especially for Zombie, which moves so slow that we can't afford to take a long route
		//		reachnum = 0;
		// done.
		if( reachnum ) {
			AAS_ReachabilityFromNum( reachnum, &reach );

			// check if the reachability is still valid
			if( !( AAS_TravelFlagForType( reach.traveltype ) & travelflags ) ) {
				reachnum = 0;
			}

			// special grapple hook case
			else if( reach.traveltype == TRAVEL_GRAPPLEHOOK ) {
				if( ms->reachability_time < AAS_Time() || ( ms->moveflags & MFL_GRAPPLERESET ) ) {
					reachnum = 0;
				}
			}

			// special elevator case
			else if( reach.traveltype == TRAVEL_ELEVATOR || reach.traveltype == TRAVEL_FUNCBOB ) {
				if( ( result->flags & MOVERESULT_ONTOPOF_FUNCBOB ) || ( result->flags & MOVERESULT_ONTOPOF_FUNCBOB ) ) {
					ms->reachability_time = AAS_Time() + 5;
				}

				// if the bot was going for an elevator and reached the reachability area
				if( ms->areanum == reach.areanum || ms->reachability_time < AAS_Time() ) {
					reachnum = 0;
				}

			} else {
#ifdef DEBUG

				if( bot_developer ) {
					if( ms->reachability_time < AAS_Time() ) {
						botimport.Print( PRT_MESSAGE, "client %d: reachability timeout in ", ms->client );
						AAS_PrintTravelType( reach.traveltype );
						botimport.Print( PRT_MESSAGE, "\n" );
					}

					/*
					if (ms->lastareanum != ms->areanum)
					{
					  botimport.Print(PRT_MESSAGE, "changed from area %d to %d\n", ms->lastareanum, ms->areanum);
					} //end if*/
				}

#endif // DEBUG

				// if the goal area changed or the reachability timed out
				// or the area changed
				if( ms->lastgoalareanum != goal->areanum || ms->reachability_time < AAS_Time() || ms->lastareanum != ms->areanum ||
					( ( ms->lasttime > ( AAS_Time() - 0.5 ) ) && ( VectorDistance( ms->origin, ms->lastorigin ) < 20 * ( AAS_Time() - ms->lasttime ) ) ) ) {
					reachnum = 0;
					// botimport.Print(PRT_MESSAGE, "area change or timeout\n");
				}
			}
		}

		// if the bot needs a new reachability
		if( !reachnum ) {
			// if the area has no reachability links
			if( !AAS_AreaReachability( ms->areanum ) ) {
#ifdef DEBUG

				if( bot_developer ) {
					botimport.Print( PRT_MESSAGE, "area %d no reachability\n", ms->areanum );
				}

#endif // DEBUG
			}

			// get a new reachability leading towards the goal
			reachnum = BotGetReachabilityToGoal(
				ms->origin, ms->areanum, ms->entitynum, ms->lastgoalareanum, ms->lastareanum, ms->avoidreach, ms->avoidreachtimes, ms->avoidreachtries, goal, travelflags, travelflags );
			// the area number the reachability starts in
			ms->reachareanum = ms->areanum;
			// reset some state variables
			ms->jumpreach = 0;					// for TRAVEL_JUMP
			ms->moveflags &= ~MFL_GRAPPLERESET; // for TRAVEL_GRAPPLEHOOK

			// if there is a reachability to the goal
			if( reachnum ) {
				AAS_ReachabilityFromNum( reachnum, &reach );
				// set a timeout for this reachability
				ms->reachability_time = AAS_Time() + BotReachabilityTime( &reach );
				//
#ifdef AVOIDREACH
				// add the reachability to the reachabilities to avoid for a while
				BotAddToAvoidReach( ms, reachnum, AVOIDREACH_TIME );
#endif // AVOIDREACH
			}
#ifdef DEBUG

			else if( bot_developer ) {
				botimport.Print( PRT_MESSAGE, "goal not reachable\n" );
				memset( &reach, 0, sizeof( aas_reachability_t ) ); // make compiler happy
			}

			if( bot_developer ) {
				// if still going for the same goal
				if( ms->lastgoalareanum == goal->areanum ) {
					if( ms->lastareanum == reach.areanum ) {
						botimport.Print( PRT_MESSAGE, "same goal, going back to previous area\n" );
					}
				}
			}

#endif // DEBUG
		}

		//
		ms->lastreachnum	= reachnum;
		ms->lastgoalareanum = goal->areanum;
		ms->lastareanum		= ms->areanum;

		// if the bot has a reachability
		if( reachnum ) {
			// get the reachability from the number
			AAS_ReachabilityFromNum( reachnum, &reach );
			result->traveltype = reach.traveltype;
			//
#ifdef DEBUG_AI_MOVE
			AAS_ClearShownDebugLines();
			AAS_PrintTravelType( reach.traveltype );
			AAS_ShowReachability( &reach );
#endif // DEBUG_AI_MOVE
	   //
#ifdef DEBUG
			// botimport.Print(PRT_MESSAGE, "client %d: ", ms->client);
			// AAS_PrintTravelType(reach.traveltype);
			// botimport.Print(PRT_MESSAGE, "\n");
#endif // DEBUG

			switch( reach.traveltype ) {
				case TRAVEL_WALK:
					*result = BotTravel_Walk( ms, &reach );
					break;

				case TRAVEL_CROUCH:
					*result = BotTravel_Crouch( ms, &reach );
					break;

				case TRAVEL_BARRIERJUMP:
					*result = BotTravel_BarrierJump( ms, &reach );
					break;

				case TRAVEL_LADDER:
					*result = BotTravel_Ladder( ms, &reach );
					break;

				case TRAVEL_WALKOFFLEDGE:
					*result = BotTravel_WalkOffLedge( ms, &reach );
					break;

				case TRAVEL_JUMP:
					*result = BotTravel_Jump( ms, &reach );
					break;

				case TRAVEL_SWIM:
					*result = BotTravel_Swim( ms, &reach );
					break;

				case TRAVEL_WATERJUMP:
					*result = BotTravel_WaterJump( ms, &reach );
					break;

				case TRAVEL_TELEPORT:
					*result = BotTravel_Teleport( ms, &reach );
					break;

				case TRAVEL_ELEVATOR:
					*result = BotTravel_Elevator( ms, &reach );
					break;

				case TRAVEL_GRAPPLEHOOK:
					*result = BotTravel_Grapple( ms, &reach );
					break;

				case TRAVEL_ROCKETJUMP:
					*result = BotTravel_RocketJump( ms, &reach );
					break;

				// case TRAVEL_BFGJUMP:
				case TRAVEL_JUMPPAD:
					*result = BotTravel_JumpPad( ms, &reach );
					break;

				case TRAVEL_FUNCBOB:
					*result = BotTravel_FuncBobbing( ms, &reach );
					break;

				default: {
					botimport.Print( PRT_FATAL, "travel type %d not implemented yet\n", reach.traveltype );
					break;
				} // end case
			}

		} else {
			result->failure = qtrue;
			memset( &reach, 0, sizeof( aas_reachability_t ) );
		}

#ifdef DEBUG

		if( bot_developer ) {
			if( result->failure ) {
				botimport.Print( PRT_MESSAGE, "client %d: movement failure in ", ms->client );
				AAS_PrintTravelType( reach.traveltype );
				botimport.Print( PRT_MESSAGE, "\n" );
			}
		}

#endif // DEBUG

	} else {
		int	   i, numareas, areas[16];
		vec3_t end;

		// special handling of jump pads when the bot uses a jump pad without knowing it
		foundjumppad = qfalse;
		VectorMA( ms->origin, -2 * ms->thinktime, ms->velocity, end );
		numareas = AAS_TraceAreas( ms->origin, end, areas, NULL, 16 );

		for( i = numareas - 1; i >= 0; i-- ) {
			if( AAS_AreaJumpPad( areas[i] ) ) {
				// botimport.Print(PRT_MESSAGE, "client %d used a jumppad without knowing, area %d\n", ms->client, areas[i]);
				foundjumppad = qtrue;
				lastreachnum = BotGetReachabilityToGoal(
					end, areas[i], ms->entitynum, ms->lastgoalareanum, ms->lastareanum, ms->avoidreach, ms->avoidreachtimes, ms->avoidreachtries, goal, travelflags, TFL_JUMPPAD );

				if( lastreachnum ) {
					ms->lastreachnum = lastreachnum;
					ms->lastareanum	 = areas[i];
					// botimport.Print(PRT_MESSAGE, "found jumppad reachability\n");
					break;

				} else {
					for( lastreachnum = AAS_NextAreaReachability( areas[i], 0 ); lastreachnum; lastreachnum = AAS_NextAreaReachability( areas[i], lastreachnum ) ) {
						// get the reachability from the number
						AAS_ReachabilityFromNum( lastreachnum, &reach );

						if( reach.traveltype == TRAVEL_JUMPPAD ) {
							ms->lastreachnum = lastreachnum;
							ms->lastareanum	 = areas[i];
							// botimport.Print(PRT_MESSAGE, "found jumppad reachability hard!!\n");
							break;
						}
					}

					if( lastreachnum ) {
						break;
					}
				}
			}
		}

		if( bot_developer ) {
			// if a jumppad is found with the trace but no reachability is found
			if( foundjumppad && !ms->lastreachnum ) {
				botimport.Print( PRT_MESSAGE, "client %d didn't find jumppad reachability\n", ms->client );
			}
		}

		//
		if( ms->lastreachnum ) {
			// botimport.Print(PRT_MESSAGE, "%s: NOT onground, swimming or against ladder\n", ClientName(ms->entitynum-1));
			AAS_ReachabilityFromNum( ms->lastreachnum, &reach );
			result->traveltype = reach.traveltype;
#ifdef DEBUG
			// botimport.Print(PRT_MESSAGE, "client %d finish: ", ms->client);
			// AAS_PrintTravelType(reach.traveltype);
			// botimport.Print(PRT_MESSAGE, "\n");
#endif // DEBUG

			//
			switch( reach.traveltype ) {
				case TRAVEL_WALK:
					*result = BotTravel_Walk( ms, &reach );
					break; // BotFinishTravel_Walk(ms, &reach); break;

				case TRAVEL_CROUCH: /*do nothing*/
					break;

				case TRAVEL_BARRIERJUMP:
					*result = BotFinishTravel_BarrierJump( ms, &reach );
					break;

				case TRAVEL_LADDER:
					*result = BotTravel_Ladder( ms, &reach );
					break;

				case TRAVEL_WALKOFFLEDGE:
					*result = BotFinishTravel_WalkOffLedge( ms, &reach );
					break;

				case TRAVEL_JUMP:
					*result = BotFinishTravel_Jump( ms, &reach );
					break;

				case TRAVEL_SWIM:
					*result = BotTravel_Swim( ms, &reach );
					break;

				case TRAVEL_WATERJUMP:
					*result = BotFinishTravel_WaterJump( ms, &reach );
					break;

				case TRAVEL_TELEPORT: /*do nothing*/
					break;

				case TRAVEL_ELEVATOR:
					*result = BotFinishTravel_Elevator( ms, &reach );
					break;

				case TRAVEL_GRAPPLEHOOK:
					*result = BotTravel_Grapple( ms, &reach );
					break;

				case TRAVEL_ROCKETJUMP:
					*result = BotFinishTravel_WeaponJump( ms, &reach );
					break;

				// case TRAVEL_BFGJUMP:
				case TRAVEL_JUMPPAD:
					*result = BotFinishTravel_JumpPad( ms, &reach );
					break;

				case TRAVEL_FUNCBOB:
					*result = BotFinishTravel_FuncBobbing( ms, &reach );
					break;

				default: {
					botimport.Print( PRT_FATAL, "(last) travel type %d not implemented yet\n", reach.traveltype );
					break;
				} // end case
			}

#ifdef DEBUG

			if( bot_developer ) {
				if( result->failure ) {
					botimport.Print( PRT_MESSAGE, "client %d: movement failure in finish ", ms->client );
					AAS_PrintTravelType( reach.traveltype );
					botimport.Print( PRT_MESSAGE, "\n" );
				}
			}

#endif // DEBUG
		}
	}

	// FIXME: is it right to do this here?
	if( result->blocked ) {
		ms->reachability_time -= 10 * ms->thinktime;
	}

	// copy the last origin
	VectorCopy( ms->origin, ms->lastorigin );
	ms->lasttime = AAS_Time();

	// RF, try to look in the direction we will be moving ahead of time
	if( reachnum > 0 && !( result->flags & ( MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) ) {
		vec3_t dir;
		int	   ftraveltime, freachnum, straveltime, ltraveltime;

		AAS_ReachabilityFromNum( reachnum, &reach );

		if( reach.areanum != goal->areanum ) {
			if( AAS_AreaRouteToGoalArea( reach.areanum, reach.end, goal->areanum, travelflags, &straveltime, &freachnum ) ) {
				ltraveltime = 999999;

				while( AAS_AreaRouteToGoalArea( reach.areanum, reach.end, goal->areanum, travelflags, &ftraveltime, &freachnum ) ) {
					// make sure we are not in a loop
					// jmarshall: aas loop fix.
					if( ftraveltime >= ltraveltime ) {
						break;
					}

					ltraveltime = ftraveltime;
					//
					AAS_ReachabilityFromNum( freachnum, &reach );

					if( reach.areanum == goal->areanum ) {
						VectorSubtract( goal->origin, ms->origin, dir );
						VectorNormalize( dir );
						vectoangles( dir, result->ideal_viewangles );
						result->flags |= MOVERESULT_FUTUREVIEW;
						break;
					}

					if( straveltime - ftraveltime > 120 ) {
						VectorSubtract( reach.end, ms->origin, dir );
						VectorNormalize( dir );
						vectoangles( dir, result->ideal_viewangles );
						result->flags |= MOVERESULT_FUTUREVIEW;
						break;
					}
				}
			}

		} else {
			VectorSubtract( goal->origin, ms->origin, dir );
			VectorNormalize( dir );
			vectoangles( dir, result->ideal_viewangles );
			result->flags |= MOVERESULT_FUTUREVIEW;
		}
	}

	// return the movement result
	return;
}

void BotResetAvoidReach( int movestate )
{
	bot_movestate_t* ms;

	ms = BotMoveStateFromHandle( movestate );

	if( !ms ) {
		return;
	}

	memset( ms->avoidreach, 0, MAX_AVOIDREACH * sizeof( int ) );
	memset( ms->avoidreachtimes, 0, MAX_AVOIDREACH * sizeof( float ) );
	memset( ms->avoidreachtries, 0, MAX_AVOIDREACH * sizeof( int ) );

	// RF, also clear movestate stuff
	ms->lastareanum		= 0;
	ms->lastgoalareanum = 0;
	ms->lastreachnum	= 0;
}

void BotResetLastAvoidReach( int movestate )
{
	int				 i, latest;
	float			 latesttime;
	bot_movestate_t* ms;

	ms = BotMoveStateFromHandle( movestate );

	if( !ms ) {
		return;
	}

	latesttime = 0;
	latest	   = 0;

	for( i = 0; i < MAX_AVOIDREACH; i++ ) {
		if( ms->avoidreachtimes[i] > latesttime ) {
			latesttime = ms->avoidreachtimes[i];
			latest	   = i;
		}
	}

	if( latesttime ) {
		ms->avoidreachtimes[latest] = 0;

		if( ms->avoidreachtries[i] > 0 ) {
			ms->avoidreachtries[latest]--;
		}
	}
}

void BotResetMoveState( int movestate )
{
	bot_movestate_t* ms;

	ms = BotMoveStateFromHandle( movestate );

	if( !ms ) {
		return;
	}

	memset( ms, 0, sizeof( bot_movestate_t ) );
}

int BotSetupMoveAI()
{
	BotSetBrushModelTypes();
	sv_maxstep	  = LibVarValue( "sv_step", "18" );
	sv_maxbarrier = LibVarValue( "sv_maxbarrier", "32" );
	sv_gravity	  = LibVarValue( "sv_gravity", "800" );
	return BLERR_NOERROR;
}

void BotShutdownMoveAI()
{
	int i;

	for( i = 1; i <= MAX_CLIENTS; i++ ) {
		if( botmovestates[i] ) {
			FreeMemory( botmovestates[i] );
			botmovestates[i] = NULL;
		}
	}
}
