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
these four show is the **contained** half: the guard leaks the block and the process survives, which
is why every one of these runs otherwise graded clean.

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

## Next step that would make the next occurrence decisive

The guard prints `caller=` only. Record one more return address (the caller of `operator delete`)
and these 71 events become attributable instead of pointing at the C++ runtime. That is a
libphoenix change in the same place the `caller=`/footer print already lives.
