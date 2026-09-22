/* Host test for libcache's WRITE-BACK path, including the ranged flush.
 *
 * The ranged write-back (dirtyStart/dirtyEnd + cache_setFlushGranularity)
 * cut ext2 metadata write amplification 21.02x -> 1.00x on hardware, but its
 * only validation was an end-to-end e2fsck on the Pi. This checks the two
 * things that change could plausibly get wrong, independently of hardware:
 *   1. CORRECTNESS -- after a flush, does the device hold exactly what was
 *      written? A too-narrow range would silently drop part of a write.
 *   2. AMPLIFICATION -- how many bytes actually reach the device per byte
 *      written? That is the number the 21.02x -> 1.00x claim is about.
 */
#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define DEV_SIZE  (8u * 1024u * 1024u)
#define LINE_SIZE (64u * 1024u)
#define LINES     16u

static unsigned char *dev, *shadow;   /* shadow = what the device SHOULD hold */
static unsigned long long wrBytes, wrCalls;

static ssize_t devRead(uint64_t off, void *buf, size_t n, cache_devCtx_t *c)
{ (void)c; if (off + n > DEV_SIZE) return -EIO; memcpy(buf, dev + off, n); return (ssize_t)n; }
static ssize_t devWrite(uint64_t off, const void *buf, size_t n, cache_devCtx_t *c)
{ (void)c; if (off + n > DEV_SIZE) return -EIO; memcpy(dev + off, buf, n); wrBytes += n; wrCalls++; return (ssize_t)n; }

int main(int argc, char **argv)
{
    size_t gran = (argc > 1) ? (size_t)strtoul(argv[1], NULL, 0) : 0;
    dev = calloc(DEV_SIZE, 1); shadow = calloc(DEV_SIZE, 1);

    cache_ops_t ops = { .readCb = devRead, .writeCb = devWrite, .ctx = NULL };
    cachectx_t *c = cache_init(DEV_SIZE, LINE_SIZE, LINES, &ops);
    if (c == NULL) { printf("cache_init FAILED\n"); return 1; }
    if (gran != 0) {
        if (cache_setFlushGranularity(c, gran) != 0) { printf("setFlushGranularity(%zu) REJECTED\n", gran); return 1; }
        printf("flush granularity: %zu bytes\n", gran);
    } else {
        printf("flush granularity: default (whole %u KiB line)\n", LINE_SIZE / 1024u);
    }

    /* The ext2 metadata pattern: many small scattered writes, write-back. */
    unsigned long long userBytes = 0;
    srand(12345);
    for (int i = 0; i < 400; i++) {
        uint64_t off = ((uint64_t)(rand() % (DEV_SIZE / 512))) * 512u;   /* 512-aligned */
        unsigned char buf[128];
        for (size_t k = 0; k < sizeof(buf); k++) buf[k] = (unsigned char)(i * 7 + k);
        if (cache_write(c, off, buf, sizeof(buf), LIBCACHE_WRITE_BACK) != (ssize_t)sizeof(buf)) {
            printf("  [FAIL] cache_write short at i=%d\n", i); return 1;
        }
        memcpy(shadow + off, buf, sizeof(buf));
        userBytes += sizeof(buf);
    }
    if (cache_flush(c, 0, DEV_SIZE) < 0) { printf("  [FAIL] cache_flush\n"); return 1; }

    int bad = 0;
    for (size_t i = 0; i < DEV_SIZE; i++) {
        if (dev[i] != shadow[i]) { printf("  [FAIL] device differs at %zu (got %02x want %02x)\n", i, dev[i], shadow[i]); bad = 1; break; }
    }
    if (!bad) printf("  [ ok ] device matches what was written, byte for byte\n");

    printf("  user bytes written : %llu\n", userBytes);
    printf("  device bytes written: %llu in %llu calls\n", wrBytes, wrCalls);
    printf("  AMPLIFICATION: %.2fx\n", (double)wrBytes / (double)userBytes);
    printf("\n%s\n", bad ? "RESULT: write path BROKEN" : "RESULT: write path correct");
    return bad;
}
