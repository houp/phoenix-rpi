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


---

# Update: the ALLOCATOR IS EXONERATED. Suspect is the FAILED-create path.

## What the instrumentation showed

`ext2_inode_create` was made to print what it actually sees:

```
umass0 (1 KiB): group=1 ino=1  bmpBlk=8459 bmp[0]=00000000  -> inode 4097, lost+found intact
umass1 (4 KiB): group=0 ino=13 bmpBlk=1026 bmp[0]=00000fff  -> inode 13, correct
```

`bmp[0]=0x00000fff` marks inodes 1..12 used, and the allocator correctly returned
13. **The bit search, the bitmap block number and the group choice are all
right**, and this run corrupted nothing — `lost+found` was *already* a regular
file when it started.

So "libext2 hands out an in-use inode" was the wrong framing. Retracted.

## The timeline points somewhere much more specific

| time | event | `lost+found` |
|---|---|---|
| 00:35 `u1ss` | full read, `sha256sum -c` 5/5 | `drwx------ … 16384` **healthy** |
| 01:34 `wrprobe` | three `open(O_CREAT)` **failed with `EINVAL`** | — |
| 01:50 `wramp` | `touch` succeeded | **regular empty file** |

The only thing that touched `umass1` between a healthy reading and a corrupt one
is **three failed file creations**. So the suspect is the **error/rollback path
of a failed create**, not normal allocation.

Note `ext2_inode_create`'s own rollback: on a `gdt_syncone` failure it re-toggles
the bitmap bit and adjusts counters, but the inode it already wrote is not
cleaned up, and the outer create path (`ext2_create` / `ext2_link`) has its own
unwind. That interaction is where to look.

## Why `open(O_CREAT)` returned EINVAL at all is the other half

`touch` on the same partition succeeded 16 minutes later, so EINVAL was not a
property of the filesystem. In `wrprobe` the failing creates came **after**
heavy write traffic on `umass0` in the same boot; in `wramp` they succeeded
after `umass0` was unmounted first. State left behind by the other partition is
the obvious thing to test.

## Next step: test against a KNOWN-CLEAN filesystem

`umass1`'s filesystem is now damaged, so it cannot serve as a baseline. A clean
1 GiB ext2 image with **4 KiB blocks** (the geometry that matters) is staged on
the NFS export as `/clean4k.img`, containing `/phoenix/data.bin` (8 MiB) and its
`SHA256SUMS`. Write it over the partition from the Pi itself:

```
/usr/bin/dd if=/clean4k.img of=/dev/umass1 bs=1M
```

then mount it, confirm `lost+found` is a directory and the checksum passes, and
only then try creating files — first with `umass0` untouched, then after heavy
`umass0` traffic, to reproduce the EINVAL deliberately.
