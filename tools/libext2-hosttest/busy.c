/* Can libext2 tell whether the filesystem is still in use?
 *
 * This is the question behind issue C3: umount during a write threw the data
 * away and returned success. The Linux answer is EBUSY, which needs a reliable
 * "is anything open?" and nothing else. Checked here rather than on hardware
 * because a wrong answer in EITHER direction is bad -- a false "busy" makes
 * umount never work, a false "idle" is the data-loss bug again.
 *
 *   ./busy <img>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "ext2.h"
#include "sb.h"
#include "gdt.h"
#include "obj.h"

#define ROOT_INO 2
static int devFd = -1;
static int fails = 0;
static ssize_t hr(id_t i, off_t o, char *b, size_t l) { (void)i; return pread(devFd, b, l, o); }
static ssize_t hw(id_t i, off_t o, const char *b, size_t l) { (void)i; return pwrite(devFd, b, l, o); }

static void ck(const char *what, int cond)
{
	printf("  [%s] %s\n", cond ? " ok " : "FAIL", what);
	if (!cond) { fails++; }
}

int main(int argc, char **argv)
{
	ext2_t *fs;
	id_t a, b;

	if (argc < 2) { fprintf(stderr, "usage: busy <img>\n"); return 2; }
	devFd = open(argv[1], O_RDWR);
	if (devFd < 0) { perror("open"); return 2; }

	fs = calloc(1, sizeof(ext2_t));
	fs->sectorsz = 512; fs->strg = NULL;
	fs->legacy.devId = 0; fs->legacy.read = hr; fs->legacy.write = hw; fs->port = 1;
	if ((ext2_sb_init(fs) < 0) || (ext2_gdt_init(fs) < 0) || (ext2_objs_init(fs) < 0)) {
		fprintf(stderr, "mount failed\n"); return 2;
	}
	fs->root = ext2_obj_get(fs, ROOT_INO);
	if (fs->root == NULL) { fprintf(stderr, "no root\n"); return 2; }

	ck("a freshly mounted filesystem is idle", ext2_objs_busy(fs) == 0);

	ck("create a file", ext2_create(fs, ROOT_INO, "one", 3, NULL, S_IFREG | 0644, &a) >= 0);
	/* create leaves nothing open */
	ck("...and creating it does not leave it open", ext2_objs_busy(fs) == 0);

	ck("open it", ext2_open(fs, a) == 0);
	ck("NOW it is busy", ext2_objs_busy(fs) == 1);

	ck("create+open a second file", ext2_create(fs, ROOT_INO, "two", 3, NULL, S_IFREG | 0644, &b) >= 0
		&& ext2_open(fs, b) == 0);
	ck("busy counts BOTH", ext2_objs_busy(fs) == 2);

	ck("close the first", ext2_close(fs, a) == 0);
	ck("busy drops to 1", ext2_objs_busy(fs) == 1);

	ck("close the second", ext2_close(fs, b) == 0);
	ck("idle again", ext2_objs_busy(fs) == 0);

	/* A closed-but-cached object must NOT read as busy -- that is the false
	 * positive that would make umount permanently fail. */
	{
		char buf[64];
		ck("read through a closed file (populates the LRU)",
			ext2_read(fs, a, 0, buf, sizeof(buf)) >= 0);
		ck("a cached-but-closed object is NOT busy", ext2_objs_busy(fs) == 0);
	}

	/* Directories too: the root is always present and must not count. */
	ck("the root alone never makes it busy", ext2_objs_busy(fs) == 0);

	ext2_objs_destroy(fs); ext2_gdt_destroy(fs); ext2_sb_destroy(fs); free(fs);
	close(devFd);
	printf("%s\n", fails ? "BUSY: checks failed" : "BUSY: all checks passed");
	return fails ? 1 : 0;
}
