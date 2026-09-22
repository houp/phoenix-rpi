/* Round-trip the block <-> (group, offset) conversion helpers.
 *
 * The residual stress mismatches are 1 KiB-only, and 1 KiB is the only block
 * size where s_first_data_block == 1. If these helpers are not exact inverses
 * for fstBlock == 1, that is the defect.
 */
#include <stdio.h>
#include <stdint.h>

static uint32_t fstBlock, groupBlocks, blocks;

static uint32_t toGroup(uint32_t bno) { return (bno - fstBlock) / groupBlocks; }
static uint32_t toOffs(uint32_t bno)  { return ((bno - fstBlock) % groupBlocks) + 1; }
static uint32_t fromOffs(uint32_t group, uint32_t offs) { return fstBlock + (group * groupBlocks) + offs - 1; }

static int check(const char *label, uint32_t fb, uint32_t gb, uint32_t nblocks)
{
    fstBlock = fb; groupBlocks = gb; blocks = nblocks;
    int bad = 0;
    uint32_t firstBad = 0;
    for (uint32_t b = fb; b < nblocks; b++) {
        uint32_t g = toGroup(b), o = toOffs(b);
        uint32_t back = fromOffs(g, o);
        if (back != b) { if (!bad) firstBad = b; bad++; }
        if (o == 0 || o > gb) { printf("  %s: offs out of 1..%u at bno=%u (offs=%u)\n", label, gb, b, o); bad++; break; }
    }
    printf("  %-28s fstBlock=%u groupBlocks=%u: %s",
           label, fb, gb, bad ? "" : "round-trip exact for every block\n");
    if (bad) printf("%d mismatches, first at bno=%u\n", bad, firstBad);

    /* Do group boundaries land where ext2 says? Group g owns blocks
     * [fstBlock + g*groupBlocks, fstBlock + (g+1)*groupBlocks). */
    for (uint32_t g = 0; g < 4; g++) {
        uint32_t first = fb + g * gb, last = fb + (g + 1) * gb - 1;
        if (toGroup(first) != g || toGroup(last) != g) {
            printf("  %s: group %u boundary wrong (first->%u last->%u)\n", label, g, toGroup(first), toGroup(last));
            bad++;
        }
    }
    return bad;
}

int main(void)
{
    int bad = 0;
    printf("block <-> (group, offs) round-trip:\n");
    bad += check("1 KiB blocks", 1, 8192, 65536);
    bad += check("4 KiB blocks", 0, 32768, 16384);
    bad += check("2 KiB blocks", 0, 16384, 32768);
    printf("\n%s\n", bad ? "RESULT: conversion helpers are NOT exact inverses" : "RESULT: helpers are exact inverses at every fstBlock tested");
    return bad ? 1 : 0;
}
