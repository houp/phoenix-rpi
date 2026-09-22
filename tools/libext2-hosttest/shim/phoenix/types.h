/* Host shim: only the Phoenix types the ext2 headers need. Deliberately NOT
 * a copy of the kernel's types.h, which redefines off_t/time_t/handle_t and
 * collides with the host libc. */
#ifndef _SHIM_PHOENIX_TYPES_H
#define _SHIM_PHOENIX_TYPES_H
#include <stdint.h>
#include <sys/types.h>
typedef struct _oid_t {
	uint32_t port;
	id_t id;
} oid_t;

typedef int handle_t;

/* From the kernel's include/file.h -- the attribute selectors ext2.c switches on. */
enum { atMode = 0, atUid, atGid, atSize, atBlocks, atIOBlock, atType, atPort, atPollStatus,
       atEventMask, atCTime, atMTime, atATime, atLinks, atDev };
#endif
