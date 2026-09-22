# libext2: ten defects in one day, and the harness that found them

**2026-09-22.** Defects 12-21. Four of them destroyed data. None was found on
hardware; all were found by a host harness written that morning.

This is the write-up a reviewer needs, separate from the weekly log's
chronology. See `manifests/2026-09-22-ext2-defects-12-21-and-short-read.md` for
the exact tested SHAs.

---

## 1. Why hardware was the wrong place to look

Before this, an ext2 defect cost a **5-10 minute Pi cycle** per attempt: build,
boot, run a workload, `dd` the device back over NFS, `e2fsck` host-side. Eleven
defects had been found that way, and the result was recorded as:

> "Eleven defects fixed, `e2fsck -fn` on a full 1 GiB read-back is completely
> clean across create / write / delete / umount / remount."

**That claim was true, and it was also the problem.** It described one
workload — a sequential create-write-delete script. Such a script never
produces a **hole**, and holes turned out to be where the data loss lived.

## 2. The harness

libext2 reaches storage through two plain callbacks:

```c
typedef ssize_t (*dev_read)(id_t, off_t, char *, size_t);
typedef ssize_t (*dev_write)(id_t, off_t, const char *, size_t);
```

Nothing about it is Phoenix-specific at the device boundary, so `pread`/`pwrite`
on a `mke2fs` image is a complete substitute. Only the *headers* needed shims
(`tools/libext2-hosttest/shim/`): no-op mutexes, `oid_t`/`handle_t`, a
compile-only `storage_t` (the harness always uses the legacy callbacks), Phoenix's
`d_namlen` dirent, and the `at*`/`ot*` enums. `libext2.c` — the `msg_t`
dispatcher — is excluded; the harness replicates `libext2_mount()`'s sequence and
calls the public API directly.

Result: **the real libext2, under ASan+UBSan, with `e2fsck` as oracle, in under a
second.** `run-all.sh` runs every harness at both block sizes in ~2 minutes.

## 3. The defects

| # | defect | consequence |
|---|---|---|
| 12 | `truncate` never decremented `i_blocks` for freed **indirect** blocks (allocation counted them) | `e2fsck`: "i_blocks is 18, should be 0" |
| 13 | `inode_init()` **and** `inode_sync()` derived `fs->gdt[group]` *before* validating `ino` | heap over-read → plausible-looking wrong `inodeTbl`, silently; `_sync` is the **write** path |
| **14** | `truncate` mishandled **holes**: a hole set `lbno = 0`, so `lbno + 1 - n` underflowed uint32 | read past `fs->gdt[]` (SEGV in the driver) **and freed blocks belonging to other files** |
| **15** | `truncate` left the surviving remainder of the **last partial block** | deleted content became readable again after re-extending — a data *leak* |
| **16** | `block_sync()` flushed a hole-terminated run using `i` instead of `block + i` (one of four sites missing the base) | data lost at the offset written **and overwrote whoever owned that block** |
| 17 | freeing an indirect block left it in `obj->ind[]`; eviction/teardown wrote the stale copy back | overwrote a block since **reallocated to another file** |
| 18 | `block_sync()`'s discontiguity branch set `lbno = 0`, leaving the new run's first block unrecorded | a discontiguous pair written **as if contiguous** |
| 19 | `unlink` adjusted directory link counts in memory only — no `OFLAG_DIRTY`, no parent sync | on-disk count one too high for every directory that had held a subdirectory |
| 20 | `objs_destroy()` freed the root object while `fs->root` still pointed at it | teardown read freed memory (the checks already tolerated a NULL root) |
| **21** | `setattr(atSize)` reached `_ext2_file_truncate()` by a **second route**, skipping `ext2_truncate()`'s `-EISDIR` | **returned SUCCESS and freed a directory's contents**: 0 of 40 entries left |

## 4. What actually found them

**Ask what the current test cannot see.** Every defect from 12 on came from
that question, and each blind spot had a *reason* the old test could not reach it:

| the question | what it found |
|---|---|
| a sequential writer never makes a **hole** | 12, 14, 15, 16, 17, 18 |
| a file whose links reach 0 is destroyed, so a stale count never shows — only a **surviving directory** keeps a wrong number | 19 |
| **hard links** leave a file alive with a count that must be right on disk | 20 |
| `setattr` was on the "uncovered" list; reading it showed a **second route** into truncate | 21 |

Three areas came back **clean**, and are recorded as such so nobody re-searches
them: directory growth beyond one block (`bigdir`, 800 entries / 23 blocks),
device nodes and FIFOs/sockets (`devnode`), and `statfs` (`attrtest`).

Still uncovered: **multi-process concurrency** and **power-loss consistency**.
Both need more than a host harness.

## 5. Mistakes worth not repeating

* **A use-after-free names the toucher, not the lifetime owner.** ASan said
  `ext2_inode_sync`; the freed object was the *root*, behind a validation that
  function performs. Dumping the live structure (`fs->objs->used`, every
  object's id/refs/links/flags) narrowed 200 lines to one. Do that before
  theorising.
* **Two of my own fixes created a second defect.** The backwards walk in
  `iblock_destroy` (conditions written for a forward walk), and defect 19's
  `obj` dirty flag (which reached defect 20's use-after-free and was briefly
  backed out). Both were caught by the harness; neither would have been caught
  by hardware.
* **`addr2line` the PC before reading the registers.** An SD-lane Data Abort was
  blamed on a read-coalescing change for a whole session — same session, same
  stressed path, and an `x19` that looked like a plausible byte count. One
  command named a different function entirely, and the cause was defect 14.
* **Read registers as an equation.** `x19 = 0xFFFFE0` ÷ `sizeof(ext2_gd_t)` gave
  524287, which appeared *independently* in `x25`; back-solving the group gave
  `0xFFFFE001` = `1 - 8192`. Two registers agreeing is proof; one that "looks
  about right" is not.
* **A measurement artefact looks exactly like a defect.** Twice: `bs=1M` cannot
  address the last 0.72 MiB of a 1026.72 MiB partition, so `dd` stopped early,
  printed no summary, and `e2fsck` then claimed the partition table was corrupt.
  Check for `dd`'s `records out` line before trusting an image.

## 6. Related fixes found along the way

* **libcache no longer writes back a line whose contents did not change**
  (`corelibs a46399c`). An in-place ext2 overwrite costs 3 device writes, **2 of
  which store bytes already present** (the superblock, and the inode — `time()`
  has 1-second resolution). Hardware: **12.6x** on partial-line rewrites, with
  the warm-cache confound controlled, and 3.33 ms per avoided device write
  against an independently measured 3.1 ms per command.
* **`bcm2711-emmc` returned `-EINVAL` where POSIX wants a short read**
  (`devices 822e933`). Reading a device to EOF failed, which is what made two
  `e2fsck` read-backs short and nearly produced a bogus "truncated partition"
  report. Gate: `dd` with no count now reports `1026+1 records` and a byte-exact
  1076594688-byte image.
* **The SD root can never be unmounted**, so a power-cut cycle leaks 2 inodes and
  2 blocks. Ending the cycle with `/usr/bin/sync` leaves the free counts
  unchanged (measured: delta 0/0 vs −2/−2).

## 7. Open, deliberately

* **Lock-order inversion**: `ext2_lookup`/`ext2_unlink` hold `dir->lock` then take
  `objs->lock`; `ext2_objs_destroy` holds `objs->lock` and reaches `obj->lock`.
  Reachability depends on whether unmount can race an in-flight request, which
  lives in the storage framework. Not fixed — concurrency is untestable with this
  harness, and a locking mistake would not be caught by it.
* **`storage_write()`** has the same `-EINVAL` pattern as the read path. No
  measured symptom, and a short write on a storage device silently truncates the
  caller's data, so the right behaviour is a design call.
* **`ext2_statfs()`** reports `f_fsid = (unsigned long)fs` — a heap pointer handed
  to userspace. Carries a `TODO` already.
