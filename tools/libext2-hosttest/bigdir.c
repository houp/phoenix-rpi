/* A directory large enough to span many blocks.
 *
 * dirstress uses a dozen entries, so every directory it makes fits in ONE
 * block and the growth path is never taken. A directory that spans blocks
 * exercises _ext2_dir_add()'s allocation, and past ~12 blocks the directory's
 * own indirect block too -- the same machinery that held five defects on the
 * file side.
 *
 *   ./bigdir <img> <entries>
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
static void um(ext2_t *fs) { ext2_objs_destroy(fs); ext2_gdt_destroy(fs); ext2_sb_destroy(fs); free(fs); }
static int fails;
static void ck(const char *w, int ok) { printf("  [%s] %s\n", ok ? " ok " : "FAIL", w); if (!ok) fails++; }

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    const char *img = argv[1];
    int n = (argc > 2) ? atoi(argv[2]) : 800;

    devFd = open(img, O_RDWR);
    ext2_t *fs = mnt();
    if (!fs) { printf("mount FAILED\n"); return 2; }
    printf("bigdir: %d entries, blocksz=%u\n", n, fs->blocksz);

    id_t dir;
    ck("create the directory", ext2_create(fs, ROOT_INO, "big", 3, NULL, S_IFDIR | 0755, &dir) >= 0);

    /* Fill it. Names are long enough that the directory must span many blocks. */
    int made = 0;
    for (int i = 0; i < n; i++) {
        char nm[48];
        int l = snprintf(nm, sizeof(nm), "entry-%06d-padding", i);
        id_t f;
        if (ext2_create(fs, dir, nm, (size_t)l, NULL, S_IFREG | 0644, &f) < 0) break;
        made++;
    }
    printf("  created %d of %d entries\n", made, n);
    ck("created them all", made == n);

    long long dsize = 0;
    ext2_getattr(fs, dir, atSize, &dsize);
    printf("  directory size = %lld bytes = %lld block(s)\n", dsize, dsize / fs->blocksz);
    ck("the directory really spans >1 block", dsize > (long long)fs->blocksz);

    /* Every entry must still be found -- including the ones past block 0. */
    int found = 0;
    for (int i = 0; i < made; i++) {
        char nm[48];
        int l = snprintf(nm, sizeof(nm), "entry-%06d-padding", i);
        oid_t res, dev;
        if (ext2_lookup(fs, dir, nm, (size_t)l, &res, &dev) >= 0) found++;
    }
    printf("  lookup found %d of %d\n", found, made);
    ck("every entry is found by lookup", found == made);

    /* Remove every other one, then re-check the survivors. */
    int removed = 0;
    for (int i = 0; i < made; i += 2) {
        char nm[48];
        int l = snprintf(nm, sizeof(nm), "entry-%06d-padding", i);
        if (ext2_unlink(fs, dir, nm, (size_t)l) >= 0) removed++;
    }
    printf("  removed %d\n", removed);

    int stillThere = 0, wronglyThere = 0;
    for (int i = 0; i < made; i++) {
        char nm[48];
        int l = snprintf(nm, sizeof(nm), "entry-%06d-padding", i);
        oid_t res, dev;
        int ok = (ext2_lookup(fs, dir, nm, (size_t)l, &res, &dev) >= 0);
        if ((i % 2) == 1 && ok) stillThere++;
        if ((i % 2) == 0 && ok) wronglyThere++;
    }
    printf("  survivors found %d (want %d), removed-but-still-found %d (want 0)\n",
           stillThere, made - removed, wronglyThere);
    ck("all survivors still found", stillThere == made - removed);
    ck("no removed entry is still found", wronglyThere == 0);

    um(fs); fsync(devFd); close(devFd);
    printf("%s\n", fails ? "BIGDIR: checks failed" : "BIGDIR: all checks passed");
    return fails ? 1 : 0;
}
