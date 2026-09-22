/* Host harness: does cache_read() mishandle a LARGE count?
 * Models the SD lane: 64 KiB lines, and a backing "device" filled with a
 * position-derived pattern so any mis-assembled byte is detectable. */
#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define DEV_SIZE  (64u * 1024u * 1024u)   /* 64 MiB backing store */
#define LINE_SIZE (64u * 1024u)
#define LINES     64u                     /* cache HOLDS 4 MiB < the 16 MiB read */

static unsigned char *dev;
static unsigned long long readCalls, readBytes, maxReadLen;

static unsigned char pat(unsigned long long off) { return (unsigned char)((off * 31u) ^ (off >> 13)); }

static ssize_t devRead(uint64_t off, void *buf, size_t n, cache_devCtx_t *ctx)
{
    (void)ctx;
    if (off + n > DEV_SIZE) { fprintf(stderr, "!! devRead OUT OF RANGE off=%llu n=%zu\n", (unsigned long long)off, n); return -EIO; }
    readCalls++; readBytes += n; if (n > maxReadLen) maxReadLen = n;
    memcpy(buf, dev + off, n);
    return (ssize_t)n;
}
static ssize_t devWrite(uint64_t off, const void *buf, size_t n, cache_devCtx_t *ctx)
{ (void)ctx; if (off + n > DEV_SIZE) return -EIO; memcpy(dev + off, buf, n); return (ssize_t)n; }

static int check(const char *label, unsigned char *got, unsigned long long base, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        if (got[i] != pat(base + i)) {
            printf("  [FAIL] %-22s first bad byte at +%zu (got %02x want %02x)\n", label, i, got[i], pat(base + i));
            return 1;
        }
    }
    printf("  [ ok ] %-22s %zu bytes verified\n", label, n);
    return 0;
}

int main(void)
{
    dev = malloc(DEV_SIZE);
    for (unsigned long long i = 0; i < DEV_SIZE; i++) dev[i] = pat(i);

    cache_ops_t ops = { .readCb = devRead, .writeCb = devWrite, .ctx = NULL };
    cachectx_t *c = cache_init(DEV_SIZE, LINE_SIZE, LINES, &ops);
    if (c == NULL) { printf("cache_init FAILED\n"); return 1; }

    printf("cache: %u lines x %u KiB = %u KiB; reads are larger than the whole cache\n\n",
           LINES, LINE_SIZE / 1024u, LINES * LINE_SIZE / 1024u);

    int bad = 0;
    struct { const char *l; unsigned long long off; size_t n; } T[] = {
        { "1 block (1 KiB)",      0,        1024 },
        { "64 KiB, aligned",      0,        65536 },
        { "16 MiB CONTIGUOUS",    0,        16u*1024*1024 },      /* the faulting shape */
        { "16 MiB, unaligned",    1024,     16u*1024*1024 },
        { "16 MiB + 1000",        0,        16u*1024*1024 + 1000 },
        { "16 MiB - 1",           0,        16u*1024*1024 - 1 },
        { "1 KiB @ odd offset",   65536+37, 1024 },
    };
    for (size_t t = 0; t < sizeof(T)/sizeof(T[0]); t++) {
        unsigned char *buf = malloc(T[t].n + 64);
        memset(buf, 0xAA, T[t].n + 64);
        ssize_t r = cache_read(c, T[t].off, buf, T[t].n);
        if (r != (ssize_t)T[t].n) { printf("  [FAIL] %-22s cache_read returned %zd, wanted %zu\n", T[t].l, r, T[t].n); bad++; free(buf); continue; }
        bad += check(T[t].l, buf, T[t].off, T[t].n);
        for (int g = 0; g < 64; g++) if (buf[T[t].n + g] != 0xAA) { printf("  [FAIL] %-22s WROTE PAST the buffer at +%d\n", T[t].l, g); bad++; break; }
        free(buf);
    }
    printf("\ndevice reads: %llu calls, %llu bytes, largest single read = %llu bytes\n",
           readCalls, readBytes, maxReadLen);
    printf("\n%s\n", bad ? "RESULT: libcache MISBEHAVES on these shapes" : "RESULT: libcache is CLEAN on every shape tested");
    return bad ? 1 : 0;
}
