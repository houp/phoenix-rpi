# libext2 freed live inodes, corrupting root and lost+found (RESOLVED)

**Status: ROOT-CAUSED AND FIXED, 2026-09-21.** Fix:
`phoenix-rtos-filesystems` `ccd11e4`. HW-verified. The title below is the
original framing and is kept for the record; see "Resolution" first -- the
allocator was NOT at fault.

## Resolution (read this first)

`_ext2_obj_create()` is called two ways: with `inode == NULL` to allocate a new
inode, and from `ext2_obj_get()` with an *existing* inode to pull it into the
object cache. In the second case `ino = pino` names a **live file**, but the
error path ran unconditionally:

```c
free(inode);
ext2_inode_destroy(fs, ino, mode);   /* <- releases an inode we never allocated */
```

Any failure in that block -- the `MAX_OBJECTS` (512) LRU eviction returning
`-ENOENT` when every object is held, `malloc`, or `mutexCreate` -- marked an
in-use inode free. The inodes pulled into the object cache most often are the
root directory (2) and `lost+found` (11), which is exactly what went missing:
group 0's inode bitmap read `0x000003fd` where a healthy filesystem reads
`0x000007ff` (bits 1 and 10 clear).

One defect explains all three symptoms:

* **`lost+found` becoming a 0-byte regular file** -- inode 11 was freed, then
  reallocated to the next file created.
* **Every regular-file create returning `EINVAL`** -- once inode 2 was free the
  allocator handed the *root directory* to the next create and the object layer
  refused it. `mkdir` kept working because Orlov places directories in a
  different block group, whose bitmap was intact.
* **The intermittency** -- it needs object-cache pressure, so it depended on how
  much of the tree had been walked.

The fix releases the inode number only when that call allocated it. `free(inode)`
stays unconditional, because the object takes ownership on success, so the buffer
must be released on failure either way. `ext2_inode_destroy()` was additionally
hardened: it rejected only inode 1, and now refuses the whole ext2-reserved range
1..10. `ext2_inode_sync()` is deliberately unchanged -- syncing root is
legitimate.

Verified on a freshly restored 1 GiB 4 KiB-block ext2 on the USB stick. Before:
every create `EINVAL`, allocator printing `ino=2 bmp[0]=000003fd`. After:
`touch`, shell redirect, `dd` and `mkdir` all succeed, allocator prints
`ino=12 bmp[0]=000007ff`, 800/800 creates under object-cache pressure with zero
failures, and root, `lost+found` and an 8 MiB reference file all survive a
remount intact (sha256 `c95413d29beacf15`).

### Two readings retracted along the way

* **"The EINVAL reproduces deterministically"** -- wrong. One reproduction in two
  valid runs; a third was clean. Pressure-dependent, consistent with the LRU
  trigger.
* **"Heavy `umass0` traffic is required"** -- wrong. It reproduced on a pristine
  filesystem with `umass0` never mounted. The two-partition framing was an
  artefact of how the first runs happened to be ordered.

## Bug #2, found afterwards: every block on a >1 KiB ext2 was off by one

Fixing #1 let creates succeed, which exposed a second, independent defect: a
large write silently overwrote an existing file.

A group's block bitmap is indexed from the filesystem's **first data block**,
which ext2 defines as **1 for 1 KiB blocks and 0 for every larger block size**.
libext2's bitmap helpers take a 1-based bit index, and the block number was

```c
bno = group * groupBlocks + offset;      /* correct only if s_first_data_block == 1 */
```

so the allocator reserved bit N and handed out block **N+1**. Inside a free run
that is invisible -- each allocation reserves the previous block. At the **end**
of a free run it hands out the first block of the next, in-use run.

Group 0's free runs on the test image (`585-623, 672-703, 896-1023, 1280-1535,
3072-...`) are each followed by an extent of `data.bin`, so one large write
clobbered exactly **624, 704, 1024, 1536** -- the first block of every extent and
nothing else. That is also why a 4 MiB and a 64 MiB write produced the *same*
bad checksum. `e2fsck -fn` on a full read-back shows both halves at once:

```
Multiply-claimed block(s) in inode 12:    624 704 1024 1536   <- the new file
Multiply-claimed block(s) in inode 49154: 624 704 1024 1536   <- data.bin
Block bitmap differences: -585 -672 -896 -1280 -3072 +19018
```

The negative entries are the first block of each **free** run: reserved, never
handed to anyone.

**Scope: every ext2 with a block size above 1 KiB** -- mke2fs's default for
anything but a small volume. It was never seen on this project because the SD
rootfs is built `mke2fs -b 1024` (`scripts/build-rpi4b-rootfs-ext2.sh`), where
the arithmetic is accidentally correct. Fix: one conversion pair in `block.c`
used at all six mapping sites; for `fstBlock == 1` it reduces symbol-for-symbol
to the old expressions, so 1 KiB filesystems are bit-for-bit unaffected.

After the fix, `e2fsck` on a read-back reports **no multiply-claimed blocks and
no block bitmap differences**, and an 8 MiB non-zero payload written alongside
reads back byte-identical.

### Method note: two ways these runs lied

* `dd if=/dev/umass1 skip=<large>` fails with `cannot fstat` and leaves a
  **0-byte** file. Check dump sizes before drawing conclusions from them.
* A 1 GiB restore stopped at 255 MiB while still printing a normal `copied`
  line. An earlier conclusion ("the medium is untouched") came from that run and
  was wrong. Confirm the byte count.

---

---

## Original report (kept for the record; the allocator was exonerated)

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
