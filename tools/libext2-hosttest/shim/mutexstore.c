/* Storage for the real-mutex shim (see shim/sys/threads.h). */
#ifdef SHIM_REAL_MUTEX
#include <pthread.h>
#define SHIM_MAX_MUTEX 4096
pthread_mutex_t shimMutex[SHIM_MAX_MUTEX];
int shimMutexCount = 0;
pthread_mutex_t shimMutexAlloc = PTHREAD_MUTEX_INITIALIZER;
#endif
