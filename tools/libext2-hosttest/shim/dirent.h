/* Host shim: Phoenix's struct dirent carries d_namlen (BSD-style), which
 * glibc's does not. ext2's _ext2_dir_read() fills it. */
#ifndef _SHIM_DIRENT_H
#define _SHIM_DIRENT_H
#include <sys/types.h>
#include <stdint.h>

/* From libphoenix's dirent.h -- fixed values, kept on flash by some FSes. */
#define DT_UNKNOWN 0
#define DT_FIFO    1
#define DT_CHR     2
#define DT_DIR     4
#define DT_BLK     6
#define DT_REG     8
#define DT_LNK     10
#define DT_SOCK    12
#define DT_WHT     14
struct dirent {
	ino_t d_ino;
	off_t d_off;
	uint16_t d_reclen;
	uint16_t d_namlen;
	uint8_t d_type;
	char d_name[];
};
#endif
