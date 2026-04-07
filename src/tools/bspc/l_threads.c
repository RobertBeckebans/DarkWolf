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

//===========================================================================
//
// Name:				l_threads.c
// Function:		multi-threading
// Programmer:		Mr Elusive (MrElusive@demigod.demon.nl)
// Last update:	1999-05-14
// Tab Size:		3
//===========================================================================

#include "l_cmd.h"
#include "l_threads.h"
#include "l_log.h"
#include "l_mem.h"

#define MAX_THREADS 64

// #define THREAD_DEBUG

int		 dispatch;
int		 workcount;
int		 oldf;
qboolean pacifier;
qboolean threaded;
void ( *workfunction )( int );

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int GetThreadWork()
{
	int r;
	int f;

	ThreadLock();

	if( dispatch == workcount ) {
		ThreadUnlock();
		return -1;
	}

	f = 10 * dispatch / workcount;
	if( f != oldf ) {
		oldf = f;
		if( pacifier ) {
			printf( "%i...", f );
		}
	}

	r = dispatch;
	dispatch++;
	ThreadUnlock();

	return r;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadWorkerFunction( int threadnum )
{
	int work;

	while( 1 ) {
		work = GetThreadWork();
		if( work == -1 ) {
			break;
		}
		// printf ("thread %i, work %i\n", threadnum, work);
		workfunction( work );
	}
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void RunThreadsOnIndividual( int workcnt, qboolean showpacifier, void ( *func )( int ) )
{
	if( numthreads == -1 ) {
		ThreadSetDefault();
	}
	workfunction = func;
	RunThreadsOn( workcnt, showpacifier, ThreadWorkerFunction );
}

//===================================================================
//
// WIN32
//
//===================================================================

#if defined( WIN32 ) || defined( _WIN32 )

	#define USED

	#include <windows.h>

typedef struct thread_s {
	HANDLE			 handle;
	int				 threadid;
	int				 id;
	struct thread_s* next;
} thread_t;

thread_t*		 firstthread;
thread_t*		 lastthread;
int				 currentnumthreads;
int				 currentthreadid;

int				 numthreads = 1;
CRITICAL_SECTION crit;
HANDLE			 semaphore;
static int		 enter;
static int		 numwaitingthreads = 0;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void			 ThreadSetDefault()
{
	SYSTEM_INFO info;

	if( numthreads == -1 ) { // not set manually
		GetSystemInfo( &info );
		numthreads = info.dwNumberOfProcessors;
		if( numthreads < 1 || numthreads > 32 ) {
			numthreads = 1;
		}
	}
	qprintf( "%i threads\n", numthreads );
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadLock()
{
	if( !threaded ) {
		Error( "ThreadLock: !threaded" );
		return;
	}
	EnterCriticalSection( &crit );
	if( enter ) {
		Error( "Recursive ThreadLock\n" );
	}
	enter = 1;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadUnlock()
{
	if( !threaded ) {
		Error( "ThreadUnlock: !threaded" );
		return;
	}
	if( !enter ) {
		Error( "ThreadUnlock without lock\n" );
	}
	enter = 0;
	LeaveCriticalSection( &crit );
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadSetupLock()
{
	Log_Print( "Win32 multi-threading\n" );
	InitializeCriticalSection( &crit );
	threaded		  = true; // Stupid me... forgot this!!!
	currentnumthreads = 0;
	currentthreadid	  = 0;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadShutdownLock()
{
	DeleteCriticalSection( &crit );
	threaded = false; // Stupid me... forgot this!!!
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadSetupSemaphore()
{
	semaphore = CreateSemaphore( NULL, 0, 99999999, "bspc" );
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void ThreadShutdownSemaphore()
{
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadSemaphoreWait()
{
	WaitForSingleObject( semaphore, INFINITE );
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadSemaphoreIncrease( int count )
{
	ReleaseSemaphore( semaphore, count, NULL );
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void RunThreadsOn( int workcnt, qboolean showpacifier, void ( *func )( int ) )
{
	int	   threadid[MAX_THREADS];
	HANDLE threadhandle[MAX_THREADS];
	int	   i;
	int	   start, end;

	Log_Print( "Win32 multi-threading\n" );
	start	  = I_FloatTime();
	dispatch  = 0;
	workcount = workcnt;
	oldf	  = -1;
	pacifier  = showpacifier;
	threaded  = true;

	if( numthreads == -1 ) {
		ThreadSetDefault();
	}

	if( numthreads < 1 || numthreads > MAX_THREADS ) {
		numthreads = 1;
	}
	//
	// run threads in parallel
	//
	InitializeCriticalSection( &crit );

	numwaitingthreads = 0;

	if( numthreads == 1 ) { // use same thread
		func( 0 );
	} else {
		//		printf("starting %d threads\n", numthreads);
		for( i = 0; i < numthreads; i++ ) {
			threadhandle[i] = CreateThread( NULL, // LPSECURITY_ATTRIBUTES lpsa,
				0,								  // DWORD cbStack,
				( LPTHREAD_START_ROUTINE )func,	  // LPTHREAD_START_ROUTINE lpStartAddr,
				( LPVOID )i,					  // LPVOID lpvThreadParm,
				0,								  //   DWORD fdwCreate,
				&threadid[i] );
			//			printf("started thread %d\n", i);
		}

		for( i = 0; i < numthreads; i++ ) {
			WaitForSingleObject( threadhandle[i], INFINITE );
		}
	}
	DeleteCriticalSection( &crit );

	threaded = false;
	end		 = I_FloatTime();
	if( pacifier ) {
		printf( " (%i)\n", end - start );
	}
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AddThread( void ( *func )( int ) )
{
	thread_t* thread;

	if( numthreads == 1 ) {
		if( currentnumthreads >= numthreads ) {
			return;
		}
		currentnumthreads++;
		func( -1 );
		currentnumthreads--;
	} else {
		ThreadLock();
		if( currentnumthreads >= numthreads ) {
			ThreadUnlock();
			return;
		}
		// allocate new thread
		thread = GetMemory( sizeof( thread_t ) );
		if( !thread ) {
			Error( "can't allocate memory for thread\n" );
		}

		//
		thread->threadid = currentthreadid;
		thread->handle	 = CreateThread( NULL, // LPSECURITY_ATTRIBUTES lpsa,
			  0,							   // DWORD cbStack,
			  ( LPTHREAD_START_ROUTINE )func,  // LPTHREAD_START_ROUTINE lpStartAddr,
			  ( LPVOID )thread->threadid,	   // LPVOID lpvThreadParm,
			  0,							   // DWORD fdwCreate,
			  &thread->id );

		// add the thread to the end of the list
		thread->next = NULL;
		if( lastthread ) {
			lastthread->next = thread;
		} else {
			firstthread = thread;
		}
		lastthread = thread;
		//
	#ifdef THREAD_DEBUG
		qprintf( "added thread with id %d\n", thread->threadid );
	#endif // THREAD_DEBUG
		   //
		currentnumthreads++;
		currentthreadid++;
		//
		ThreadUnlock();
	}
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void RemoveThread( int threadid )
{
	thread_t *thread, *last;

	// if a single thread
	if( threadid == -1 ) {
		return;
	}
	//
	ThreadLock();
	last = NULL;
	for( thread = firstthread; thread; thread = thread->next ) {
		if( thread->threadid == threadid ) {
			if( last ) {
				last->next = thread->next;
			} else {
				firstthread = thread->next;
			}
			if( !thread->next ) {
				lastthread = last;
			}
			//
			FreeMemory( thread );
			currentnumthreads--;
	#ifdef THREAD_DEBUG
			qprintf( "removed thread with id %d\n", threadid );
	#endif // THREAD_DEBUG
			break;
		}
		last = thread;
	}
	if( !thread ) {
		Error( "couldn't find thread with id %d", threadid );
	}
	ThreadUnlock();
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void WaitForAllThreadsFinished()
{
	HANDLE handle;

	ThreadLock();
	while( firstthread ) {
		handle = firstthread->handle;
		ThreadUnlock();

		WaitForSingleObject( handle, INFINITE );

		ThreadLock();
	}
	ThreadUnlock();
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int GetNumThreads()
{
	return currentnumthreads;
}

#endif

//===================================================================
//
// OSF1
//
//===================================================================

#if defined( __osf__ )

	#define USED

	#include <pthread.h>

typedef struct thread_s {
	pthread_t		 thread;
	int				 threadid;
	int				 id;
	struct thread_s* next;
} thread_t;

thread_t*		firstthread;
thread_t*		lastthread;
int				currentnumthreads;
int				currentthreadid;

int				numthreads = 1;
pthread_mutex_t my_mutex;
pthread_attr_t	attrib;
static int		enter;
static int		numwaitingthreads = 0;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void			ThreadSetDefault()
{
	if( numthreads == -1 ) { // not set manually
		numthreads = 1;
	}
	qprintf( "%i threads\n", numthreads );
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadLock()
{
	if( !threaded ) {
		Error( "ThreadLock: !threaded" );
		return;
	}
	if( my_mutex ) {
		pthread_mutex_lock( my_mutex );
	}
	if( enter ) {
		Error( "Recursive ThreadLock\n" );
	}
	enter = 1;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadUnlock()
{
	if( !threaded ) {
		Error( "ThreadUnlock: !threaded" );
		return;
	}
	if( !enter ) {
		Error( "ThreadUnlock without lock\n" );
	}
	enter = 0;
	if( my_mutex ) {
		pthread_mutex_unlock( my_mutex );
	}
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadSetupLock()
{
	pthread_mutexattr_t mattrib;

	Log_Print( "pthread multi-threading\n" );

	if( !my_mutex ) {
		my_mutex = GetMemory( sizeof( *my_mutex ) );
		if( pthread_mutexattr_create( &mattrib ) == -1 ) {
			Error( "pthread_mutex_attr_create failed" );
		}
		if( pthread_mutexattr_setkind_np( &mattrib, MUTEX_FAST_NP ) == -1 ) {
			Error( "pthread_mutexattr_setkind_np failed" );
		}
		if( pthread_mutex_init( my_mutex, mattrib ) == -1 ) {
			Error( "pthread_mutex_init failed" );
		}
	}

	if( pthread_attr_create( &attrib ) == -1 ) {
		Error( "pthread_attr_create failed" );
	}
	if( pthread_attr_setstacksize( &attrib, 0x100000 ) == -1 ) {
		Error( "pthread_attr_setstacksize failed" );
	}

	threaded		  = true;
	currentnumthreads = 0;
	currentthreadid	  = 0;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadShutdownLock()
{
	threaded = false;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void RunThreadsOn( int workcnt, qboolean showpacifier, void ( *func )( int ) )
{
	int					i;
	pthread_t			work_threads[MAX_THREADS];
	pthread_addr_t		status;
	pthread_attr_t		attrib;
	pthread_mutexattr_t mattrib;
	int					start, end;

	Log_Print( "pthread multi-threading\n" );

	start	  = I_FloatTime();
	dispatch  = 0;
	workcount = workcnt;
	oldf	  = -1;
	pacifier  = showpacifier;
	threaded  = true;

	if( numthreads < 1 || numthreads > MAX_THREADS ) {
		numthreads = 1;
	}

	if( pacifier ) {
		setbuf( stdout, NULL );
	}

	if( !my_mutex ) {
		my_mutex = GetMemory( sizeof( *my_mutex ) );
		if( pthread_mutexattr_create( &mattrib ) == -1 ) {
			Error( "pthread_mutex_attr_create failed" );
		}
		if( pthread_mutexattr_setkind_np( &mattrib, MUTEX_FAST_NP ) == -1 ) {
			Error( "pthread_mutexattr_setkind_np failed" );
		}
		if( pthread_mutex_init( my_mutex, mattrib ) == -1 ) {
			Error( "pthread_mutex_init failed" );
		}
	}

	if( pthread_attr_create( &attrib ) == -1 ) {
		Error( "pthread_attr_create failed" );
	}
	if( pthread_attr_setstacksize( &attrib, 0x100000 ) == -1 ) {
		Error( "pthread_attr_setstacksize failed" );
	}

	for( i = 0; i < numthreads; i++ ) {
		if( pthread_create( &work_threads[i], attrib, ( pthread_startroutine_t )func, ( pthread_addr_t )i ) == -1 ) {
			Error( "pthread_create failed" );
		}
	}

	for( i = 0; i < numthreads; i++ ) {
		if( pthread_join( work_threads[i], &status ) == -1 ) {
			Error( "pthread_join failed" );
		}
	}

	threaded = false;

	end = I_FloatTime();
	if( pacifier ) {
		printf( " (%i)\n", end - start );
	}
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AddThread( void ( *func )( int ) )
{
	thread_t* thread;

	if( numthreads == 1 ) {
		if( currentnumthreads >= numthreads ) {
			return;
		}
		currentnumthreads++;
		func( -1 );
		currentnumthreads--;
	} else {
		ThreadLock();
		if( currentnumthreads >= numthreads ) {
			ThreadUnlock();
			return;
		}
		// allocate new thread
		thread = GetMemory( sizeof( thread_t ) );
		if( !thread ) {
			Error( "can't allocate memory for thread\n" );
		}
		//
		thread->threadid = currentthreadid;

		if( pthread_create( &thread->thread, attrib, ( pthread_startroutine_t )func, ( pthread_addr_t )thread->threadid ) == -1 ) {
			Error( "pthread_create failed" );
		}

		// add the thread to the end of the list
		thread->next = NULL;
		if( lastthread ) {
			lastthread->next = thread;
		} else {
			firstthread = thread;
		}
		lastthread = thread;
		//
	#ifdef THREAD_DEBUG
		qprintf( "added thread with id %d\n", thread->threadid );
	#endif // THREAD_DEBUG
		   //
		currentnumthreads++;
		currentthreadid++;
		//
		ThreadUnlock();
	}
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void RemoveThread( int threadid )
{
	thread_t *thread, *last;

	// if a single thread
	if( threadid == -1 ) {
		return;
	}
	//
	ThreadLock();
	last = NULL;
	for( thread = firstthread; thread; thread = thread->next ) {
		if( thread->threadid == threadid ) {
			if( last ) {
				last->next = thread->next;
			} else {
				firstthread = thread->next;
			}
			if( !thread->next ) {
				lastthread = last;
			}
			//
			FreeMemory( thread );
			currentnumthreads--;
	#ifdef THREAD_DEBUG
			qprintf( "removed thread with id %d\n", threadid );
	#endif // THREAD_DEBUG
			break;
		}
		last = thread;
	}
	if( !thread ) {
		Error( "couldn't find thread with id %d", threadid );
	}
	ThreadUnlock();
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void WaitForAllThreadsFinished()
{
	pthread_t*	   thread;
	pthread_addr_t status;

	ThreadLock();
	while( firstthread ) {
		thread = &firstthread->thread;
		ThreadUnlock();

		if( pthread_join( *thread, &status ) == -1 ) {
			Error( "pthread_join failed" );
		}

		ThreadLock();
	}
	ThreadUnlock();
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int GetNumThreads()
{
	return currentnumthreads;
}

#endif

//===================================================================
//
// LINUX
//
//===================================================================

#if defined( LINUX )

	#define USED

	#include <pthread.h>
	#include <semaphore.h>

typedef struct thread_s {
	pthread_t		 thread;
	int				 threadid;
	int				 id;
	struct thread_s* next;
} thread_t;

thread_t*		firstthread;
thread_t*		lastthread;
int				currentnumthreads;
int				currentthreadid;

int				numthreads = 1;
pthread_mutex_t my_mutex   = PTHREAD_MUTEX_INITIALIZER;
pthread_attr_t	attrib;
sem_t			semaphore;
static int		enter;
static int		numwaitingthreads = 0;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void			ThreadSetDefault()
{
	if( numthreads == -1 ) { // not set manually
		numthreads = 1;
	}
	qprintf( "%i threads\n", numthreads );
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadLock()
{
	if( !threaded ) {
		Error( "ThreadLock: !threaded" );
		return;
	}
	pthread_mutex_lock( &my_mutex );
	if( enter ) {
		Error( "Recursive ThreadLock\n" );
	}
	enter = 1;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadUnlock()
{
	if( !threaded ) {
		Error( "ThreadUnlock: !threaded" );
		return;
	}
	if( !enter ) {
		Error( "ThreadUnlock without lock\n" );
	}
	enter = 0;
	pthread_mutex_unlock( &my_mutex );
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadSetupLock()
{
	pthread_mutexattr_t mattrib;

	Log_Print( "pthread multi-threading\n" );

	threaded		  = true;
	currentnumthreads = 0;
	currentthreadid	  = 0;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadShutdownLock()
{
	threaded = false;
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void ThreadSetupSemaphore()
{
	sem_init( &semaphore, 0, 0 );
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void ThreadShutdownSemaphore()
{
	sem_destroy( &semaphore );
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void ThreadSemaphoreWait()
{
	sem_wait( &semaphore );
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void ThreadSemaphoreIncrease( int count )
{
	int i;

	for( i = 0; i < count; i++ ) {
		sem_post( &semaphore );
	}
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void RunThreadsOn( int workcnt, qboolean showpacifier, void ( *func )( int ) )
{
	int					i;
	pthread_t			work_threads[MAX_THREADS];
	void*				pthread_return;
	pthread_attr_t		attrib;
	pthread_mutexattr_t mattrib;
	int					start, end;

	Log_Print( "pthread multi-threading\n" );

	start	  = I_FloatTime();
	dispatch  = 0;
	workcount = workcnt;
	oldf	  = -1;
	pacifier  = showpacifier;
	threaded  = true;

	if( numthreads < 1 || numthreads > MAX_THREADS ) {
		numthreads = 1;
	}

	if( pacifier ) {
		setbuf( stdout, NULL );
	}

	for( i = 0; i < numthreads; i++ ) {
		if( pthread_create( &work_threads[i], NULL, ( void* )func, ( void* )i ) == -1 ) {
			Error( "pthread_create failed" );
		}
	}

	for( i = 0; i < numthreads; i++ ) {
		if( pthread_join( work_threads[i], &pthread_return ) == -1 ) {
			Error( "pthread_join failed" );
		}
	}

	threaded = false;

	end = I_FloatTime();
	if( pacifier ) {
		printf( " (%i)\n", end - start );
	}
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AddThread( void ( *func )( int ) )
{
	thread_t* thread;

	if( numthreads == 1 ) {
		if( currentnumthreads >= numthreads ) {
			return;
		}
		currentnumthreads++;
		func( -1 );
		currentnumthreads--;
	} else {
		ThreadLock();
		if( currentnumthreads >= numthreads ) {
			ThreadUnlock();
			return;
		}
		// allocate new thread
		thread = GetMemory( sizeof( thread_t ) );
		if( !thread ) {
			Error( "can't allocate memory for thread\n" );
		}
		//
		thread->threadid = currentthreadid;

		if( pthread_create( &thread->thread, NULL, ( void* )func, ( void* )thread->threadid ) == -1 ) {
			Error( "pthread_create failed" );
		}

		// add the thread to the end of the list
		thread->next = NULL;
		if( lastthread ) {
			lastthread->next = thread;
		} else {
			firstthread = thread;
		}
		lastthread = thread;
		//
	#ifdef THREAD_DEBUG
		qprintf( "added thread with id %d\n", thread->threadid );
	#endif // THREAD_DEBUG
		   //
		currentnumthreads++;
		currentthreadid++;
		//
		ThreadUnlock();
	}
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void RemoveThread( int threadid )
{
	thread_t *thread, *last;

	// if a single thread
	if( threadid == -1 ) {
		return;
	}
	//
	ThreadLock();
	last = NULL;
	for( thread = firstthread; thread; thread = thread->next ) {
		if( thread->threadid == threadid ) {
			if( last ) {
				last->next = thread->next;
			} else {
				firstthread = thread->next;
			}
			if( !thread->next ) {
				lastthread = last;
			}
			//
			FreeMemory( thread );
			currentnumthreads--;
	#ifdef THREAD_DEBUG
			qprintf( "removed thread with id %d\n", threadid );
	#endif // THREAD_DEBUG
			break;
		}
		last = thread;
	}
	if( !thread ) {
		Error( "couldn't find thread with id %d", threadid );
	}
	ThreadUnlock();
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void WaitForAllThreadsFinished()
{
	pthread_t* thread;
	void*	   pthread_return;

	ThreadLock();
	while( firstthread ) {
		thread = &firstthread->thread;
		ThreadUnlock();

		if( pthread_join( *thread, &pthread_return ) == -1 ) {
			Error( "pthread_join failed" );
		}

		ThreadLock();
	}
	ThreadUnlock();
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int GetNumThreads()
{
	return currentnumthreads;
}

#endif // LINUX

//===================================================================
//
// IRIX
//
//===================================================================

#ifdef _MIPS_ISA

	#define USED

	#include <task.h>
	#include <abi_mutex.h>
	#include <sys/types.h>
	#include <sys/prctl.h>

typedef struct thread_s {
	int				 threadid;
	int				 id;
	struct thread_s* next;
} thread_t;

thread_t*  firstthread;
thread_t*  lastthread;
int		   currentnumthreads;
int		   currentthreadid;

int		   numthreads = 1;
static int enter;
static int numwaitingthreads = 0;

abilock_t  lck;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void	   ThreadSetDefault()
{
	if( numthreads == -1 ) {
		numthreads = prctl( PR_MAXPPROCS );
	}
	printf( "%i threads\n", numthreads );
	//@@
	usconfig( CONF_INITUSERS, numthreads );
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadLock()
{
	spin_lock( &lck );
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadUnlock()
{
	release_lock( &lck );
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadSetupLock()
{
	init_lock( &lck );

	Log_Print( "IRIX multi-threading\n" );

	threaded		  = true;
	currentnumthreads = 0;
	currentthreadid	  = 0;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadShutdownLock()
{
	threaded = false;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void RunThreadsOn( int workcnt, qboolean showpacifier, void ( *func )( int ) )
{
	int i;
	int pid[MAX_THREADS];
	int start, end;

	start	  = I_FloatTime();
	dispatch  = 0;
	workcount = workcnt;
	oldf	  = -1;
	pacifier  = showpacifier;
	threaded  = true;

	if( numthreads < 1 || numthreads > MAX_THREADS ) {
		numthreads = 1;
	}

	if( pacifier ) {
		setbuf( stdout, NULL );
	}

	init_lock( &lck );

	for( i = 0; i < numthreads - 1; i++ ) {
		pid[i] = sprocsp( ( void ( * )( void*, size_t ) )func, PR_SALL, ( void* )i, NULL, 0x100000 );
		//		pid[i] = sprocsp ( (void (*)(void *, size_t))func, PR_SALL, (void *)i
		//			, NULL, 0x80000);
		if( pid[i] == -1 ) {
			perror( "sproc" );
			Error( "sproc failed" );
		}
	}

	func( i );

	for( i = 0; i < numthreads - 1; i++ ) {
		wait( NULL );
	}

	threaded = false;

	end = I_FloatTime();
	if( pacifier ) {
		printf( " (%i)\n", end - start );
	}
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AddThread( void ( *func )( int ) )
{
	thread_t* thread;

	if( numthreads == 1 ) {
		if( currentnumthreads >= numthreads ) {
			return;
		}
		currentnumthreads++;
		func( -1 );
		currentnumthreads--;
	} else {
		ThreadLock();
		if( currentnumthreads >= numthreads ) {
			ThreadUnlock();
			return;
		}
		// allocate new thread
		thread = GetMemory( sizeof( thread_t ) );
		if( !thread ) {
			Error( "can't allocate memory for thread\n" );
		}
		//
		thread->threadid = currentthreadid;

		thread->id = sprocsp( ( void ( * )( void*, size_t ) )func, PR_SALL, ( void* )thread->threadid, NULL, 0x100000 );
		if( thread->id == -1 ) {
			perror( "sproc" );
			Error( "sproc failed" );
		}

		// add the thread to the end of the list
		thread->next = NULL;
		if( lastthread ) {
			lastthread->next = thread;
		} else {
			firstthread = thread;
		}
		lastthread = thread;
		//
	#ifdef THREAD_DEBUG
		qprintf( "added thread with id %d\n", thread->threadid );
	#endif // THREAD_DEBUG
		   //
		currentnumthreads++;
		currentthreadid++;
		//
		ThreadUnlock();
	}
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void RemoveThread( int threadid )
{
	thread_t *thread, *last;

	// if a single thread
	if( threadid == -1 ) {
		return;
	}
	//
	ThreadLock();
	last = NULL;
	for( thread = firstthread; thread; thread = thread->next ) {
		if( thread->threadid == threadid ) {
			if( last ) {
				last->next = thread->next;
			} else {
				firstthread = thread->next;
			}
			if( !thread->next ) {
				lastthread = last;
			}
			//
			FreeMemory( thread );
			currentnumthreads--;
	#ifdef THREAD_DEBUG
			qprintf( "removed thread with id %d\n", threadid );
	#endif // THREAD_DEBUG
			break;
		}
		last = thread;
	}
	if( !thread ) {
		Error( "couldn't find thread with id %d", threadid );
	}
	ThreadUnlock();
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void WaitForAllThreadsFinished()
{
	ThreadLock();
	while( firstthread ) {
		ThreadUnlock();

		// wait (NULL);

		ThreadLock();
	}
	ThreadUnlock();
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int GetNumThreads()
{
	return currentnumthreads;
}

#endif //_MIPS_ISA

//=======================================================================
//
// SINGLE THREAD
//
//=======================================================================

#ifndef USED

int	 numthreads		   = 1;
int	 currentnumthreads = 0;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadSetDefault()
{
	numthreads = 1;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadLock()
{
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadUnlock()
{
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadSetupLock()
{
	Log_Print( "no multi-threading\n" );
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadShutdownLock()
{
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadSetupSemaphore()
{
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void ThreadShutdownSemaphore()
{
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadSemaphoreWait()
{
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ThreadSemaphoreIncrease( int count )
{
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void RunThreadsOn( int workcnt, qboolean showpacifier, void ( *func )( int ) )
{
	int start, end;

	Log_Print( "no multi-threading\n" );
	dispatch  = 0;
	workcount = workcnt;
	oldf	  = -1;
	pacifier  = showpacifier;
	start	  = I_FloatTime();
	#ifdef NeXT
	if( pacifier ) {
		setbuf( stdout, NULL );
	}
	#endif
	func( 0 );

	end = I_FloatTime();
	if( pacifier ) {
		printf( " (%i)\n", end - start );
	}
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AddThread( void ( *func )( int ) )
{
	if( currentnumthreads >= numthreads ) {
		return;
	}
	currentnumthreads++;
	func( -1 );
	currentnumthreads--;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void RemoveThread( int threadid )
{
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void WaitForAllThreadsFinished()
{
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int GetNumThreads()
{
	return currentnumthreads;
}

#endif // USED
