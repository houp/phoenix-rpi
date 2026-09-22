/* Minimal repro hunt for the unmount use-after-free. Each case is a separate
 * process so one abort does not mask the others. */
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
static ssize_t hr(id_t i, off_t o, char *b, size_t l) { (void)i; return pread(devFd, b, l, o); }
static ssize_t hw(id_t i, off_t o, const char *b, size_t l) { (void)i; return pwrite(devFd, b, l, o); }
static ext2_t *mnt(void)
{
    ext2_t *fs = calloc(1, sizeof(ext2_t));
    fs->sectorsz = 512; fs->strg = NULL;
    fs->legacy.devId = 0; fs->legacy.read = hr; fs->legacy.write = hw; fs->port = 1;
    if (ext2_sb_init(fs) < 0 || ext2_gdt_init(fs) < 0 || ext2_objs_init(fs) < 0) return NULL;
    fs->root = ext2_obj_get(fs, ROOT_INO); return fs->root ? fs : NULL;
}
int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    int which = atoi(argv[2]);
    devFd = open(argv[1], O_RDWR);
    ext2_t *fs = mnt();
    if (!fs) { printf("mount FAILED\n"); return 2; }
    id_t id;
    if (which == 1) {                       /* plain file, unlink to 0 links */
        ext2_create(fs, ROOT_INO, "a", 1, NULL, S_IFREG | 0644, &id);
        ext2_write(fs, id, 0, "x", 1);
        ext2_unlink(fs, ROOT_INO, "a", 1);
    }
    else if (which == 2) {                  /* SHORT symlink (target in inode) */
        ext2_create(fs, ROOT_INO, "s", 1, NULL, S_IFLNK | 0777, &id);
        ext2_write(fs, id, 0, "/short", 6);
        ext2_unlink(fs, ROOT_INO, "s", 1);
    }
    else if (which == 3) {                  /* LONG symlink (target in a block) */
        char t[200]; memset(t, 'L', sizeof(t)); t[0] = '/'; t[sizeof(t)-1] = 0;
        ext2_create(fs, ROOT_INO, "l", 1, NULL, S_IFLNK | 0777, &id);
        ext2_write(fs, id, 0, t, strlen(t));
        ext2_unlink(fs, ROOT_INO, "l", 1);
    }
    else if (which == 4) {                  /* directory, rmdir */
        ext2_create(fs, ROOT_INO, "d", 1, NULL, S_IFDIR | 0755, &id);
        ext2_unlink(fs, ROOT_INO, "d", 1);
    }
    else if (which == 5) {                  /* hard link, remove ONE name */
        ext2_create(fs, ROOT_INO, "h1", 2, NULL, S_IFREG | 0644, &id);
        ext2_write(fs, id, 0, "x", 1);
        ext2_link(fs, ROOT_INO, "h2", 2, id);
        ext2_unlink(fs, ROOT_INO, "h2", 2);
    }
    ext2_objs_destroy(fs); ext2_gdt_destroy(fs); ext2_sb_destroy(fs); free(fs);
    fsync(devFd); close(devFd);
    printf("case %d: completed\n", which);
    return 0;
}
