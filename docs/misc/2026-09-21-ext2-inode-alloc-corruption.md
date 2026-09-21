# libext2 hands out an in-use inode on first allocation (confirmed, data-losing)

**Status: CONFIRMED on hardware, root cause NOT yet found. Do not write to a
freshly-created large ext2 filesystem until it is.**

## The symptom, unambiguously

On `/dev/umass1` (27.5 GiB ext2, 4 KiB blocks), creating the very first file
turned `lost+found` into a 0-byte regular file. Verified with individually
tagged output, so UART interleaving cannot be the explanation:

```
TAG-mount rc=0
TAG-lostfound-type: regular empty file      <- was  drwx------ 2 root root 16384
TAG-lostfound-size: 0
TAG-phoenix-type:   directory               <- intact
TAG-checksums:      5
TAG-checksum-fail:  0
```

Before the write (previous boot, same partition): `lost+found` read as a
directory and all five checksums passed. So the filesystem was consistent, and
one `touch` broke it. `lost+found` is **inode 11** — the first inode after
ext2's ten reserved ones — so the allocator handed out an inode that was in use.

File *data* survived; this destroys directory inodes, quietly.

## What has been ruled out by reading the code

- **Inode-table offset arithmetic** (`inode.c:31`, `:46`). The block index uses
  `((ino-1) % groupInodes) / inodesPerBlock` while the in-block offset uses
  `(ino-1) % inodesPerBlock`. Those differ only when `inodesPerBlock` does not
  divide `groupInodes`; here 4096/256 = 16 divides 8192, and on the 1 KiB
  partition 1024/256 = 4 divides 4096. **Correct for both geometries.**
- **1-based bitmap indexing** is used consistently: `ext2_findzerobit(bmp,
  groupInodes, 0)` → `ext2_togglebit(bmp, ino)` → `group * groupInodes + ino`,
  and `ext2_inode_destroy` mirrors it with `(ino-1) % groupInodes + 1`.
- **`ext2_gdt_syncone`'s block number** for group 0: `fstBlock + 0 + 1` — block 1
  on the 4 KiB fs (fstBlock 0) and block 2 on the 1 KiB fs (fstBlock 1). Both
  land on the GDT, not on data.
- **The >4 GiB offset overflow** — already fixed, and reads are now correct
  (5/5 checksums), so block addressing itself is sound.

## Why it is probably geometry-dependent

`umass0` (2 GiB, 1 KiB blocks, 256 groups, 4096 inodes/group) has had files
created on it repeatedly with `lost+found` surviving. `umass1` (27.5 GiB, 4 KiB
blocks, 220 groups, 8192 inodes/group) broke on the first write. The SD card's
ext2 root also creates files every boot without corrupting.

## The decisive next step

Instrument `ext2_inode_create`: print the chosen `group`, the returned `ino`,
`fs->gdt[group].inodeBmp`, and the first 4 bytes of the bitmap block just read.
If those first bytes are not `0xff 0x07` (inodes 1..11 used) on group 0, the
**bitmap read is landing on the wrong block** and the GDT is the suspect; if
they are correct and `findzerobit` still returns 11, the **bit search** is.

That is one build and one boot, and it separates the two remaining candidates.
