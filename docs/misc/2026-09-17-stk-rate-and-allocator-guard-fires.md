# SuperTuxKart crash rate before/after the BO-ownership fix, and what still fires

**2026-09-17.** Two questions, both answered from the UART archive with one instrument each, and
both with a negative control — the lesson from the same day's grader audit is that a detector which
cannot fire proves nothing.

## 1. STK: 30/242 → 0/131, measured with the same detector

`stk-highbits-pointer`'s row has said "dies ~1 run in 10" since 2026-09-12. That rate was never
re-measured after devices `7a1e3db` + mesa `274ee5abea9` (the BO single-owner fix, 2026-09-12 20:51).

**Method.** A run counts if its log contains `stk: DATADIR=` — the port launcher's own line, which
proves the engine started. A run is a *failure* if the log contains `Exception #` or `Data Abort`.
The signature proper is a `far=` whose top hex digit is 8-f (bit 63 set is never a valid aarch64
userspace pointer).

| window | STK engine starts | with an exception | with a bit63 `far` |
|---|---|---|---|
| 2026-09-08 → 09-12 16:38 (pre-fix) | 242 | **30** (12.4%) | **8** (3.3%) |
| 2026-09-12 20:51 → 09-17 (post-fix) | 131 | **0** | **0** |

p(0 in 131 | 12.4%) ≈ **3·10⁻⁸**; for the narrower signature, p(0 in 131 | 3.3%) ≈ **0.012**.

The pre-fix column is the negative control: the identical detector fires 30 times there, so a zero
in the post-fix column is a measurement, not a blind spot. ⚠ Some pre-fix events are deliberate
reproducers (`dfguard`, `guards2`, `addprobe`), which inflates the *old* rate but cannot affect the
post-fix zero.

**What this does and does not settle.** It is strong evidence that whatever killed STK ~1 run in 10
stopped on 2026-09-12. It does not identify the fix by itself — the BO ownership pair is the only
candidate that landed in that window, and the font patch 0019 predates it (see the row).

## 2. What still fires: 4 contained guard hits since the fix

Same window, all 1239 non-probe logs (labels containing `dfprobe`/`dfguard`/`probe`/`guards`
excluded, since those are deliberate):

| date | log | guard | events |
|---|---|---|---|
| 2026-09-15 | `w38-postfix-stk` | `free() of a corrupt chunk header` | 23 |
| 2026-09-16 | `w38-upstream-vkq` | `chunk handed out twice` | 1 |
| 2026-09-16 | `w38-postloop-stk` | `free() of a corrupt chunk header` | 5 |
| 2026-09-17 | `botrace-now` (STK) | `free() of a corrupt chunk header` | 71 |

**0** logs carry the fatal double-free report (`Double free detected` / exit 70) — so
`allocator-double-free`'s "no field occurrence since 2026-09-09" holds for *its* signature. What
these four show is the **contained** half: the guard leaks the block, the process survives, and the
run keeps rendering to the end of its window (checked: flipstat continues past the last fire in all
three STK runs), so nothing about the run *looks* wrong.

↩ **Correction — the tool was not blind, I was.** `uart-summary.sh` has carried these patterns in
its fault set since 2026-09-10, and re-grading the four logs with it now reports **23 / 1 / 5 / 71**
fault matches. They went unnoticed because none of these four runs was ever put through it: three
were ad-hoc verification runs and the fourth was graded only by `analyze-bo-trace.py`, whose PASS I
reported the same morning without checking the log for faults. The showcase gate's own narrower
regex genuinely could not have seen them — that is a separate defect, fixed today.

### The callers, symbolised

`caller=` resolves against the **unstripped** `.buildroot/_build/<target>/prog/supertuxkart`
(the shipped `/usr/bin/supertuxkart` is stripped — `nm` says "no symbols"; `prog.stripped/` is the
one that ships). Only addresses from a log whose build matches that binary are trustworthy.

- `2026-09-17 botrace-now`, all 71 events: `0x1b28f2c` = **`operator delete(void*)`**. The guard
  records its immediate caller, so for a C++ `delete` the real site is one frame further up and is
  **not currently captured**.
- `2026-09-16 postloop-stk`, the three non-`operator delete` callers:
  - `0x95bf94` = `btCompoundCollisionAlgorithm::~btCompoundCollisionAlgorithm()`
  - `0x942560` = `btUnionFind::reset(int)`
  - `0x941a90` = `btSimulationIslandManager::buildAndProcessIslands(...)`

⇒ **the surviving corruption in STK is concentrated in the Bullet physics teardown/reset path**, not
in the font path (cleared 2026-09-15) and not in the drive-graph (fixed).

### One constraint this adds

`botrace-now` is the same run that scored **0 overlapping BO reuses in 2827 events** — and it fired
the header guard **71 times**. So the residue is **not** explained by the BO double-ownership
mechanism: that one is fixed and instrumented, and the corruption happened anyway.

## What the printed fields already rule out

Pairing each event's fields correctly (the `ptr`/`heap` columns must be read per event, not as two
independent lists — reading them as lists produced one bogus "the chunk is below its heap base"
inference, retracted here), every fire looks like this:

| log | ptr | heap | offset into heap | size |
|---|---|---|---|---|
| 09-15 | `0x0c9a4810` | `0x0c99c000` | 0x8810 | 0x293 |
| 09-15 (last) | `0x0c896898` | `0x0c88b000` | 0xb898 | 0x153 |
| 09-17 | `0x0b80d5f8` | `0x0b805000` | 0x85f8 | 0x073 |

So in every case the **chunk header is plausible**: canonical, 8-aligned, a sane small size with sane
flag bits, at a 32–52 KiB offset inside a page-aligned heap that sits inside the `[heapLo, heapHi)`
window the process reports. That leaves only the checks that depend on the **heap** the chunk points
back to: code 2 (the heap was already released), 5 (`heap->size` is not sane), or 6/8 (the chunk is
outside the extent `heap->size` claims). ⇒ the open question is the **heap back-pointer / heap
extent**, not the chunk's own size — which is the opposite of what "corrupt chunk header" suggests.

## Landed: the report now says which check failed

libphoenix `8659311` (2026-09-17) splits `malloc_chunkValid()` into `malloc_chunkValidWhy()`
returning 1-8 and prints `why=` from both `free()` and `realloc()`. That alone separates the four
live candidates: **2** = the heap was released, **5** = `heap->size` is not sane, **6** = the chunk
is outside the extent the heap claims, **8** = it runs off the end. Every code is covered host-side
by `tools/malloc-harness` (13/13, each case failing exactly one check).
⏭ Optional refinement, deliberately NOT taken today to avoid a second rebuild+gate cycle for a small
gain: also print `hsize = heap->size` when the heap pointer itself passed (codes ≥ 5), which would
turn a code 6 or 8 into an arithmetic anyone can check by eye.
No behaviour change — the same chunks are accepted and rejected as before; verified by rebuild
(core + ports), libc string/stdlib/stdio 383 tests / 0 failures, stale census 0 of 354.

## Where the fault is NOT: the allocator's single-threaded bookkeeping

`tools/malloc-harness` compiles the **real** `malloc_dl.c` for the host (file-statics visible, its
own invariant checker walking every heap). Run 2026-09-17 at **8 seeds × 300 000 ops** — ~2.4 M
operations, **~75 000 heap mmap/munmap cycles**, up to 217 live heaps at once — with the checker
firing every 500 ops:

```
seed 0x…  ops 300411  check-every 500  -> OK      (8/8 seeds OK)
    coverage: mmap=9528 munmap=9528 chunksWalked=106327 freeChunks=39361
              realloc shrink-split=1184 grow-in-place=2967 | join back=14334 fwd=13727
```

No orphaned bin link, no `freesz` drift, no unmerged neighbours, no chunk pointing into a released
heap. The checker is known to be able to fire — its own self-test forges each violation first.

⇒ the defect is **not** in the single-threaded path, which is the only thing that harness can see.
The untested axis it leaves is **concurrency**, and both apps that fired (SuperTuxKart, vkQuake) are
heavily multithreaded. `tools/malloc-mt-stress` (new, staged as `bin/mtstress`) drives the same
allocator from N threads with a size mix that keeps heaps being created and released under each
other, tagging every block so user-data corruption is caught in the act.

## Next step that would make the next occurrence decisive

The guard prints `caller=` only. Record one more return address (the caller of `operator delete`)
and these 71 events become attributable instead of pointing at the C++ runtime. That is a
libphoenix change in the same place the `caller=`/footer print already lives.
