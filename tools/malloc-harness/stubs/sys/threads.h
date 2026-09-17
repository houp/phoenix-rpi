/* Host stub for libphoenix <sys/threads.h>.
 *
 * The allocator lock used to be a no-op here, because the harness was
 * single-threaded. It is not any more: the 2026-09-17 evidence (guards firing in
 * multithreaded apps while 2.4M single-threaded ops stayed clean) puts the open
 * question on the CONCURRENT path, so the lock has to be real or an MT run would
 * prove nothing about mutual exclusion.
 *
 * Backed by a real pthread mutex, one per handle, from a small fixed table --
 * the allocator creates exactly one, and a table keeps the API honest without a
 * dynamic allocation inside the allocator's own dependency.
 */
#ifndef HZ_SYS_THREADS_H
#define HZ_SYS_THREADS_H

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef int handle_t;

#define HZ_MUTEX_MAX 4096

static pthread_mutex_t hz_mutexes[HZ_MUTEX_MAX];
static int hz_mutexCount = 0;

static inline int mutexCreate(handle_t *h)
{
	if (hz_mutexCount >= HZ_MUTEX_MAX) {
		fprintf(stderr, "harness: out of mutex slots\n");
		abort();
	}
	if (pthread_mutex_init(&hz_mutexes[hz_mutexCount], NULL) != 0) {
		fprintf(stderr, "harness: pthread_mutex_init failed\n");
		abort();
	}
	*h = ++hz_mutexCount; /* 1-based: 0 stays "no mutex" */
	return 0;
}

static inline int mutexLock(handle_t h)
{
	if ((h <= 0) || (h > hz_mutexCount)) {
		/* An allocator call before _malloc_init() would be a real defect on
		 * target too -- say so rather than silently running unlocked. */
		fprintf(stderr, "harness: mutexLock on uninitialised handle %d\n", h);
		abort();
	}
	return pthread_mutex_lock(&hz_mutexes[h - 1]);
}

static inline int mutexUnlock(handle_t h)
{
	if ((h <= 0) || (h > hz_mutexCount)) {
		fprintf(stderr, "harness: mutexUnlock on uninitialised handle %d\n", h);
		abort();
	}
	return pthread_mutex_unlock(&hz_mutexes[h - 1]);
}

#endif
