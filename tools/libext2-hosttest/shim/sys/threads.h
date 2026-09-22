/* Host shim for Phoenix's thread primitives.
 *
 * By default the locks are no-ops: every harness except the concurrency one is
 * single-threaded, and no-ops keep those runs simple and fast.
 *
 * Build with -DSHIM_REAL_MUTEX to get REAL pthread mutexes. That matters
 * because libext2 is genuinely driven concurrently in production -- the umass
 * driver dispatches filesystem messages with UMASS_N_MSG_THREADS = 2 -- so
 * "concurrency is untestable on the host" was only true of the shim, not of
 * the code.
 */
#ifndef _SHIM_SYS_THREADS_H
#define _SHIM_SYS_THREADS_H

#include <sys/types.h>
#include <phoenix/types.h>

#ifdef SHIM_REAL_MUTEX

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define SHIM_MAX_MUTEX 4096

extern pthread_mutex_t shimMutex[SHIM_MAX_MUTEX];
extern int shimMutexCount;
extern pthread_mutex_t shimMutexAlloc;

static inline int mutexCreate(handle_t *h)
{
	pthread_mutex_lock(&shimMutexAlloc);
	if (shimMutexCount >= SHIM_MAX_MUTEX) {
		pthread_mutex_unlock(&shimMutexAlloc);
		fprintf(stderr, "shim: out of mutexes\n");
		abort();
	}
	int id = shimMutexCount++;
	pthread_mutex_unlock(&shimMutexAlloc);

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	/* ERRORCHECK, not RECURSIVE: Phoenix mutexes are not recursive, so a
	 * recursive acquire is a bug we want reported rather than silently
	 * tolerated. */
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	pthread_mutex_init(&shimMutex[id], &attr);
	pthread_mutexattr_destroy(&attr);

	*h = id + 1; /* 0 stays "unset" */
	return 0;
}

static inline int mutexLock(handle_t h)
{
	if (h <= 0 || h > shimMutexCount) {
		fprintf(stderr, "shim: mutexLock on an invalid handle %d\n", h);
		abort();
	}
	int e = pthread_mutex_lock(&shimMutex[h - 1]);
	if (e != 0) {
		fprintf(stderr, "shim: mutexLock(%d) failed: %d (EDEADLK=35 means a recursive acquire)\n", h, e);
		abort();
	}
	return 0;
}

static inline int mutexUnlock(handle_t h)
{
	if (h <= 0 || h > shimMutexCount) {
		fprintf(stderr, "shim: mutexUnlock on an invalid handle %d\n", h);
		abort();
	}
	int e = pthread_mutex_unlock(&shimMutex[h - 1]);
	if (e != 0) {
		fprintf(stderr, "shim: mutexUnlock(%d) failed: %d\n", h, e);
		abort();
	}
	return 0;
}

static inline int resourceDestroy(handle_t h) { (void)h; return 0; }

#else /* no-op locks */

static inline int mutexCreate(handle_t *h) { *h = 1; return 0; }
static inline int mutexLock(handle_t h) { (void)h; return 0; }
static inline int mutexUnlock(handle_t h) { (void)h; return 0; }
static inline int resourceDestroy(handle_t h) { (void)h; return 0; }

#endif

static inline int condCreate(handle_t *h) { *h = 1; return 0; }
static inline int condWait(handle_t c, handle_t m, time_t t) { (void)c; (void)m; (void)t; return 0; }
static inline int condSignal(handle_t c) { (void)c; return 0; }
static inline int condBroadcast(handle_t c) { (void)c; return 0; }

#endif
