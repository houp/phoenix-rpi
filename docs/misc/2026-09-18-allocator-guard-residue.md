# The allocator guard residue: the grid is intact, so the application did not smash it

Source review of the silent heap-guard firings behind `allocator-double-free`
(`docs/KNOWN-ISSUES.md`). No build, no Pi cycle — the archive plus
`sources/libphoenix/stdlib/malloc_dl.c`.

## Premise correction: no archived fire carries a `why=` code

The `why=` guard landed 2026-09-17 **15:55** (libphoenix `8659311`); the `lheap?=`/`freed?=`
fields at **17:38** (`e4f7c65`). The latest fire in the archive is `botrace-now`, 2026-09-17
**13:57**. Every `why=` string under `artifacts/rpi4b-uart/` belongs to the self-test
(`mutexsmp`, `mtheavy`, `whyverify`, all ≥ 17:19). **The armed guard has never fired in the
field**, so everything below is deduced from the printed fields, not read off a code.

## Two disjoint families

**(A) Pre-fix noise, 2026-09-10 → 09-12 — do not mix it in.** ~5 300 corrupt-header reports per
run across `allfix-T*`/`relguard-T*`, every one at chunk offset exactly `0x10`, heaps marching
page-by-page — the `released[]` false-positive era, closed by `f8f7ff2`. Likewise the
`sbinfix-T*` runs (~390 each), closed by `46f456a`.

**(B) The residue — four logs, 99 + 1 events.** 23 (`w38-postfix-stk`), 1 (`w38-upstream-vkq`,
a different guard), 5 (`w38-postloop-stk`), 71 (`botrace-now`).

## Which invariant fails

Across all 99: `chunk` canonical and 8-aligned (not code 1) · `heap` page-aligned and inside
`[heapLo, heapHi)` (not 3, not 4) · `size & 3 == 3`, and `size & ~3` 8-aligned and ≥ 40 in 99/99
(not 7) · `heapSizeValid` bounds only against `heapHi`, so 5 would need a wild `heap->size`.

⇒ **code 6 (or 8): the chunk lies outside the extent its own heap claims.** The 09-17 doc had
this as "2, 5, or 6/8".

## ★ The failing region is an intact chunk grid

Sorting the events by address within each heap:

| log | heap | n | span from heap base | exactly adjacent | overlaps |
|---|---|---|---|---|---|
| 09-15 | `0x0c88b000` | 15 | `0x8398 .. 0xbef0` | 9/14 | **0** |
| 09-15 | `0x0c99c000` | 8 | `0x8800 .. 0xc918` | 2/7 | **0** |
| 09-16 | `0x0cae6000` | 5 | `0xc6c0 .. 0xccd8` | 1/4 | **0** |
| 09-17 | `0x0b805000` | 71 | `0x8088 .. 0xcf70` | 61/70 | **0** |

73 of 95 consecutive pairs start exactly where the previous block ends; the rest are gaps, never
overlaps. **These are blocks this allocator really carved.** That retires, on log evidence, the
whole application-corruption class: a header size/alignment mismatch, a userspace overrun, a free
of an alignment-adjusted pointer, a foreign-allocator free.

It also means the 09-17 doc's proposed next step — name the `operator delete` caller — is aimed at
the wrong half of the problem. (It was separately impossible on this target: `-fomit-frame-pointer`
is on for aarch64.)

Bullet is cleared in one look: `btAlignedAllocator.cpp:23-34` routes to plain `malloc`/`free`, and
the aligned path (`:63-96`) stores the real pointer at `ret[-1]` and frees *that*. The observed
caller `0x1b28f2c` is `operator delete`, not `btAlignedFree`.

## ★ Every residue heap is a `0xd000` (bin-14) heap

Legal heap sizes are exactly `CEIL(sizeof(heap_t) + lookup[idx], 4096)` (`malloc_dl.c:1068`, table
`:1235-1240`): `0x1000, 0x2000, 0x3000, 0x4000, 0x5000, 0x7000, 0x9000, 0xd000, 0x11000, …`
(confirmed empirically — the vkQuake event printed `hsize = 0x181000` = `lookup[24]`). Each grid
runs past `0xcf70`, and `0xd000` is the unique legal size in `[0xcf70, 0x11000)`.

The 2026-09-12 **fatal** signature-A event
([`2026-09-12-two-corruption-signatures.md`](2026-09-12-two-corruption-signatures.md):18) was also
a `0xd000` heap. Same heap class on both halves of the family — a link neither earlier doc records.

Meanwhile the reported extent must be ≤ the smallest failing offset, `0x8088` — and `0x8000` is
**not** a legal heap size, so the reported value is either `0x7000` (bin 12) or an arbitrary
clobber. The archive gives only the right edge of that interval.

**Plainly: a heap that is really `0xd000` reports an extent ≤ `0x8088`, while a live, intact,
in-use chunk grid carrying that heap's own base sits above the reported end.**

## Ranked hypotheses

**H1 — the extent shrank in place under a live heap (or the base was re-initialised).** The only
alternative is that 70+ blocks of a perfect grid all carry a wrong `->heap`, but `->heap` is
written only by `malloc_chunkInit` (`:499-507`) from `_malloc_heapAlloc` (`:1125`, correct by
construction) or inherited by `_malloc_chunkSplit` (`:929`) — so a wrong grid needs a parent chunk
created when B's extent *did* cover it. Both readings converge on "the extent shrank between carve
and free". Against: the only writer of `heap->size` is `malloc_heapInit` (`:1058-1062`), and the
writer was not found in the source.

**H2 — a smaller heap was re-`mmap`'d over a released `0xd000` one**, the pages above the new end
still mapped and holding the old unzeroed grid. It explains `freed?=0` exactly
(`_malloc_heapAlloc:1090-1104` clears the `released[]` record when the new heap overlaps). Against,
and this is a source+log argument: release (`:1576-1643`) has refused to release a heap that is not
one chunk since `cdea6dc` (09-10) and refused if the chunk did not leave its bin since `54549e6`
(09-12 12:18) — **both predate all three residue logs, and neither refusal message appears in any
of them.** With 71 live `CUSED` blocks in `[0x8088, 0xcf70]`, that heap was not one chunk.

**H3 — cross-heap coalesce.** Largely closed: `_malloc_chunkJoin:1038-1048` computes the boundary
from the trusted reference heap (`165439a`) and prints on disagreement (`:1029-1034`, `:964-998`).
**None of those prints appears in the residue logs.**

**H4 — underflow or a thread race.** Not supported: every public entry takes the lock
(`:1392, 1422, 1483, 1673, 1819`); the host harness is clean at 2.4 M single-thread and 3.2 M
concurrent ops, and `phoenix_mutex_mutual_exclusion` passes on target.

## The discriminating experiment — landed, unbuilt

Not another return address: `why=`/`lheap?=`/`freed?=` print identically for H1 and H2. The
corrupt-header branch now also prints `hsize`, `hfree` and `hend?` for codes ≥ 5 (libphoenix,
committed 2026-09-18; the double-free branch at `:1540` already printed the first two, and the
heap pointer has passed its gates by then, so no new dereference).

- **`hend?=1`** — the next live heap begins exactly where this one now ends ⇒ **H2**.
- **`hend?=0`**, with `hfree` inconsistent against `hsize` ⇒ **H1**.

Either way `hsize` turns the events into arithmetic anyone can check by eye, and confirms or kills
the `0xd000`-class claim directly.

## ★ The unchecked store that would produce exactly this

`_malloc_heapAlloc()` (`malloc_dl.c:1065`) tests the address `mmap` returned against `released[]`
and **nothing else** — then calls `malloc_heapInit(heap, heapSize)`, whose whole body is
`heap->size = size; heap->freesz = size - sizeof(heap_t);`.

So if the kernel ever hands back a region overlapping a heap that is **still live**, that one store
shrinks the live heap's recorded extent in place. Its chunks above the new end stay mapped, stay in
use, and still name that base in `->heap` — and every one of their frees then reports code 6/8.
That is the residue signature exactly, including why the grid is intact: nobody wrote over the
chunks, only over the heap header in front of them.

This is H1 with a named mechanism, and it is Phoenix-specific (the kernel's anonymous-mmap address
selection), which is where the standing rule says to look.

**Landed 2026-09-18 (libphoenix, unbuilt):** `_malloc_heapAlloc()` now walks `live[]` and reports
`new`/`nsize`/`live`/`lsize` if the new mapping overlaps a live heap. It reports and carries on — a
`NULL` return here would turn a contained corruption into an immediate crash, and if the print never
fires the hypothesis is dead. Host harness: 12/12 `why` codes, 0 detector failures, randomized
stress OK on every seed, **no false positive** over ~3 000 mmap/munmap cycles per seed.

⚠ **The check can go blind.** `live[]` holds 256 entries and the host harness reaches **226 live
heaps on a single 100k-op seed**, so a wrapped ring is realistic on a real workload — and a wrapped
ring also invalidates the `lheap?` verdict. `lovfl` is now printed in the corrupt-header branch too,
so a reader can tell. A wrapped ring can only *miss* an overlap, never invent one.

## Kernel side: the overlap cannot come from `mmap` — H2 is effectively dead

Source trace of `_map_find` (`vm/map.c:175`), the gap search behind every anonymous
`mmap(NULL, …)`:

- It returns only from a node whose relevant child is `NULL` (`:198`, `:208`, `:236`), where
  `lmaxgap`/`rmaxgap` are the **exact** gap to the in-order neighbour, not a subtree maximum.
- Every writer of those fields was traced — insertion, entry growth, removal/shrink, rotations.
  Where augmentation is skipped (a leaf `rb_transplant`, `lib/rb.c:239`), the stale value is **too
  small**. Stale bookkeeping loses free space; it cannot invent room that is not there.
- `malloc` passes `NULL`, which `_map_find:188-190` clamps to `map->start`, so the `max()` at `:208`
  is the identity — the one shape that *can* overlap needs a non-`MAP_FIXED` **hint** (now TD-22).

And fresh anonymous pages are **eagerly zeroed** on this target: `process->lazy = 0` on MMU builds
(`proc/process.c:226-230`), so `_vm_mmap` forces every page at mmap time (`:623-628`) and
`amap_page` memsets it (`vm/amap.c:297`). That cuts against H2 directly — a smaller heap mapped over
a live one would *replace* the PTEs below its new end, destroying the grid there rather than
shadowing it.

⚠ Two honest limits on that: it is "no path found by tracing", not a proof, and the zeroing argument
is not testable from the archive — the guard only prints **failing** frees, and blocks below the
reported extent free successfully and silently. So "no events below `0x8000`" is not evidence that
no chunks live there.

★ **A third mechanism needs no kernel bug at all.** `_map_find` is lowest-fit from `map->start` and
same-flag anonymous neighbours **merge** into one map entry (`vm/map.c:265-266`), so heaps pack
back-to-back. A chunk at `base + 0x8088` whose `->heap` names the *preceding* heap reads identically
to "this heap claims too small an extent" — intact header, no overlap, no kernel involvement. The
`hend?` field already separates it: **`hend?=1` is exactly that adjacency.**

### Landed alongside (both unbuilt)

- **kernel** — `_map_map()` discarded `_map_add()`'s result, and that is `-EEXIST` precisely when
  the new entry overlaps one already in the tree: an overlap left the entry outside the tree while
  the address went back to the caller. Now reported (bounded to 8).
- **libphoenix** — the release path discarded `munmap()`'s result. A failure there would leave the
  region mapped while `released[]` said it was gone.
- **TD-22** — the one place in `_map_find` where a returned range really can overlap, with the
  leaf-only fix written out. Unreachable today; held for its own gate.

## Separate, and not enough data

`w38-upstream-vkq`'s single `chunk handed out twice` (`:1153-1171`) has a fully consistent header
and heap (`hsize=0x181000`, offset `0x145968` legitimately inside): a live block still linked in a
free bin. One sample, different defect.

## Incidental, not implicated

`realloc`'s grow-in-place (`:1709-1714`) never calls `malloc_chunkSetFooter(chunk)` after
`chunk->size += malloc_chunkSize(next)`. Latent-safe only because `_malloc_allocFrom` sets
`CHUNK_PUSED` on the following chunk, so `malloc_chunkPrev` never consults the stale footer. Worth
a comment or a fix for upstreamability.
