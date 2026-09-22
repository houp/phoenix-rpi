/* Host shim: the harness is single-threaded, so locks are no-ops. */
#ifndef _SHIM_SYS_THREADS_H
#define _SHIM_SYS_THREADS_H
#include <sys/types.h>
#include <phoenix/types.h>
static inline int mutexCreate(handle_t *h) { *h = 1; return 0; }
static inline int mutexLock(handle_t h) { (void)h; return 0; }
static inline int mutexUnlock(handle_t h) { (void)h; return 0; }
static inline int resourceDestroy(handle_t h) { (void)h; return 0; }
static inline int condCreate(handle_t *h) { *h = 1; return 0; }
static inline int condWait(handle_t c, handle_t m, time_t t) { (void)c; (void)m; (void)t; return 0; }
static inline int condSignal(handle_t c) { (void)c; return 0; }
static inline int condBroadcast(handle_t c) { (void)c; return 0; }
#endif
