#ifndef _SHIM_THREADS_H
#define _SHIM_THREADS_H
#include <stdint.h>
typedef int handle_t;
static inline int mutexCreate(handle_t *h) { *h = 1; return 0; }
static inline int mutexLock(handle_t h) { (void)h; return 0; }
static inline int mutexUnlock(handle_t h) { (void)h; return 0; }
static inline int resourceDestroy(handle_t h) { (void)h; return 0; }
#endif
