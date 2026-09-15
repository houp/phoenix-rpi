# W38, 2026-09-15 — the working detail behind §3 of the weekly log

Moved out of `docs/inprogress/WEEK-2026-W38.md` once each item was closed and recorded in
`docs/KNOWN-ISSUES.md`. The weekly log keeps a one-line verdict per item; this is the evidence.

---

## 3b. STK crash hunt — redirected to the allocator (2026-09-15)

⛔ **The STK font path is cleared.** The faulting `ldrb` is `font_with_face.cpp:787` (collect loop);
ports patch `0019` touches loops starting at 834, and its own guard `STK-FONT-OOB` fired **0 times in
28 post-fix logs** — so 0019 fixes a real bug that was never live here, and neither caused nor cured
this crash. There is also **no out-of-bounds 4-byte store anywhere in `render()`**, and the
`m_cached_gls` dangling-reference theory has no invalidation path. ⇒ the buffer was wrong, not the
store: **the allocator handed the same memory to two owners.** Same root cause as
`freebin-corruption`; the fix belongs in the allocator/VM.

**12-run STK soak: 12/12 started, 0 faults**, ~1850–2000 frames each. With the earlier post-fix runs,
**23 runs, 0 faults since 2026-09-12** — p ≈ 0.09 vs the old ~1-in-10, i.e. suggestive, **not
significant**. Not closed.
ⓘ 2 *contained* allocator events in the 23. ⓘ **1 run in 12 burned its whole 300 s window compiling
shaders** (19 frames vs ~1850) with the Mesa cache present — cold GPU starts are intermittent, not
just first-run, so warm each app **twice** before presenting.
🔧 Shipped so the next occurrence is decisive (ports `d7a3740`): `0020` re-reads the bad pair through
a **`volatile`** pointer and dumps the element, its neighbours and the vector's base/size/capacity;
`0021` NULL-checks `FontDrawer::addGlyph`'s texture (a reachable NULL deref).
Review: [`docs/misc/2026-09-15-stk-font-path-cleared.md`](../misc/2026-09-15-stk-font-path-cleared.md).

## 3c. `libc-uninit-main` — new row, one sighting, no demo impact

The "ntpclient stack-overflow" reading was wrong. `far=0x30` is `FILE.lock`: it is `fwrite()` on a
**NULL stream** (`stdio/file.c:777`), and the stack dump carries the formatted string verbatim —
`psh: ntpclient: unknown command`, i.e. `psh.c:211`. `stderr` **and** psh's applet list were unset at
once; both are set by libc init. ⚠ Two readings — libc init never ran, or the page lost its writes —
and they point at different subsystems, so the row does not pick one. Needs a second occurrence.
🔧 Hardened (libphoenix `c996bec`): `_file_init()` never checked its three `calloc()`s.

## 3d. Verification of this turn — all green

libc `stdio` **82/0/1** · `posixsrv` **16/0** · `unix-socket` **38/0** · quakespasm `Host_Init`
**2.276 s** · SuperTuxKart with 0020/0021 **0 faults, 2014 frames, 9 fps**, neither probe fired.
Manifest `manifests/2026-09-15-stk-font-probes.md`.
## 3e. ✅ The `fbcon up` row was lying — fixed (2026-09-15)

**The HDMI console is fine.** An early boot frame shows the whole Phoenix boot log, the `(psh)%`
prompt and the build-versions table rendered on HDMI. What was broken was the *marker*:
`pl011_writeRaw()` poked the PL011 data register directly while the kernel console drove the same
UART, so the bytes were lost **about two boots in three** — `fbcon: ok` survived in only **220 of 715**
recent logs. `uart-summary.sh` therefore reported `fbcon up: NO` on perfectly good boots, and two real
`tty0` error messages vanished entirely whenever they fired.
🔧 devices `3174143`: `pl011_note()` writes the HDMI copy directly (it must — `pl011_thr` has not
started yet) and the UART/log copy through **stderr**, which is drained properly. `pl011_writeRaw` had
no other callers and is gone.
**Measured: `fbcon: ok` in 3/3 boots (was ~1/3), 0 faults, unix-socket 38/0 ×3.** Manifest
`manifests/2026-09-15-fbcon-marker-fix.md`.
⚠ My "not seen since 2026-08-14" note yesterday was wrong — an unsorted `head -3` on the match list.
It was intermittent, not absent.
🔧 Follow-on: **9 of the 19 boot-stage rows read `[NO ]` on a healthy boot.** Two were stale patterns
with a live equivalent (`pcie running`, `init thread spawn`); the other seven are debug-build markers
and now render `[ - ]` instead of looking like failures. A stock netboot log is now `[YES]` on every
non-debug row, and a truncated log still shows `[NO ]`. `[YES]` lines are unchanged, so
`test-cycle-bench.sh`'s counting is unaffected.
ⓘ Same frame confirms `ntpclient` works (`System time set to UTC Tue Sep 15 06:49:21 2026`), so the
`libc-uninit-main` sighting really is a one-off.

## 3f. ✅ `#66` stale X lock — FIXED, and it was not what the row said

The X server refusing to restart after an unclean exit was **not** a broken liveness probe. A new
libc test settles that: `kill(pid, 0)` answers **ESRCH** for a reaped pid and **0** for a live one
without delivering anything — `test-libc-signal -g liveness`, **5/5 on hardware** (tests `cefdfd5`;
that path had no coverage at all, the old signal tests only send real signals to self).
The real mechanism is **PID REUSE**: the kernel hands out the *lowest* free pid, so the pid a dead X
server released is likely already held by something else by the time the next launch reads
`/tmp/.X0-lock`, and Xorg's `LockServer()` then refuses with "Server is already active".
🔧 `xlaunch` now removes a leftover lock before starting the server (coord `ce5f50873`) — it is the
only launcher, on a board with one display, about to start a fresh server.
**Verified:** with a planted lock it logs `removed leftover /tmp/.X0-lock (see #66)` and X comes up;
with no lock the unlink is a silent no-op and **two back-to-back X sessions in one boot both start**.

## 3g. ★★★ The allocator corruption has a MEASURED fix, not an inferred one (2026-09-15)

`freebin-corruption` / `stk-highbits-pointer` come from a BO's CPU mapping having **two live owners**,
so a page the GPU still uses is handed to `malloc`. That was fixed by devices **`7a1e3db`** + mesa
**`274ee5abea9`** on **2026-09-12 at 20:51** — and the archive's last such fault is 2026-09-12 **16:38**,
four hours earlier. ⇒ **the 23 clean SuperTuxKart runs attach to this fix, not to the font patch 0019.**

Confirmed directly rather than by absence of crashes: a `V3D_BO_TRACE=1` STK run on the current build
scores **0 overlapping reuses in 2880 BO events**.
⚠ **Read the right number.** The same run shows **117 CPU addresses reused by more than one handle** —
*expected* once closed BOs are really unmapped, because the address genuinely returns to the process.
The historical "56" was measured with unmapping **off**, where any reuse meant two live owners. Only
*temporal* overlap is a defect; scoring the raw count would have reported a 2× regression on a healthy
build. New `scripts/analyze-bo-trace.py` bakes that distinction in and prints PASS/FAIL.
⏭ Not closed: the mechanism check is one run, and signature B (the stale-bin hand-out, `hbase?=0`)
still fires ~2 runs in 23 — separate, and contained.
ⓘ `docs/misc/2026-09-08-x-soak-and-v3d-bo-residue.md` ("do not fix before the demo") is marked
superseded: the daemon has reaped dead clients' BOs since devices `1eb8608` (2026-09-08).

---

# Later the same day — measurement work moved out of the weekly log

## 3c. `V3D-binner-wedge` — the row's "named next step" was stale; done properly (2026-09-15)

**6 more q3dm7 runs: 0 wedges, 0 faults, 58 123 frames.** Total now **9 q3dm7 runs, ~85 700 frames,
0 wedges**; last actually observed **2026-08-22**.

The source diff the row asked for (`TILE_BINNING_MODE_CFG`, tile sizing, CT0QMA/QMS, CPU→GPU
coherency, ours vs Mesa/Linux) **had already been done on 08-22** — the row never said so. Re-verified
against today's tree, and three of the four arms are structurally impossible:
`TILE_BINNING_MODE_CFG` **cannot differ — we emit no field of it** (upstream Mesa does, and no Phoenix
commit touches those files); tile_alloc uses **Mesa's own formula**, whose headroom term saturates by
~16 draws, so q3dm1 and q3dm7 get the *same* ~570 KiB — and the wedge dump showed 520 KiB still free;
**no unflushed CPU→GPU write exists** (every binner-input BO is Normal-NC, per-frame writes drained by
`dsb sy`).
⓵ **One real divergence, new:** we write `CTL_MISCCFG=0x05` (`QRMAXCNT=2`, `OVRTMUOUT`) at init;
**Linux writes that register only for ver < 41 — never on 4.2** — and our own cold probe reads the
silicon default as `0x06`. `QRMAXCNT=2` was A/B-tuned; the default has never been benched.
⚠ **Deliberately not changing it:** the bug has not reproduced, so an A/B would destroy the only signal.
⏭ Next step is an **observation, not a change**: at the next recurrence read `CL@ct0ca`. Foreign
content ⇒ BO aliasing survived the three fixes that landed after the last sighting → re-run
`V3D_BO_TRACE=1` + `analyze-bo-trace.py` for *temporal* overlap. A valid draw item with `FDBGS` stall
bits ⇒ a real front-end starve, and *then* MISCCFG parity is the A/B.
ⓘ Aside, unverified and not this bug: `_pmap_cacheOpAfterChange` flushes by **VA** while
`_pmap_writeEntry` edits page tables through `scratch_tt` — i.e. possibly a non-current pmap. V3D
self-maps so it is unaffected; noted so it is not lost.

## 3d. ✅ Full libc sweep — 21 suites, 1171 tests, 0 failures; one real coverage gap closed

Ran **every** `test-libc-*` on the tree the demo image came from: **21 suites, 1171 tests, 0 failures,
28 ignored, 0 faults.** That is the broad health statement behind "stable enough to present".

★ The sweep found a gap worth fixing: **`inet-socket` had 2 cases** against `unix-socket`'s 38, and
**`socket()` — the call whose kernel path we changed this week — had no coverage at all.** Added five
(tests `4e0da18`, 2 → 7, all PASS on hardware), aimed squarely at what the new `/dev/netsocket` oid
cache can get wrong: 64 sequential creates must all succeed **and be distinct** (a cache handing back
one shared object would still return a valid fd); a mix of families/types through the same cached
port, with `AF_INET6` allowed to be unsupported but not to crash or return a v4 socket; a **failed**
`socket()` in between must not poison the cache for the next good one; `bind(port 0)` must actually
assign a port; and a UDP loopback round-trip must deliver the bytes with the source attributed to
loopback. Manifest `manifests/2026-09-15-libc-sweep-inet.md`.

## 3e. ✅ Demo-length X soak — 10 minutes, clean

Longest run we had ever verified was ~5 minutes; a presentation is longer than that, so the gap was
real. `startx_gpu --quit-after 600 action` — the full 6-client desktop (Window Maker + GPU window +
xterm + xbill + xclock) for **600 s**, then a clean `--quit-after` shutdown:

**0 faults · 0 GPU wedges · 0 allocator events · 0 lock warnings**, and `session ended` reached
normally.

ⓘ Memory across one full X lifecycle: **91 204 KB → 246 336 KB** (202 → 501 map entries). That is the
known per-session GPU/X residue, now measured at demo length instead of 150 s. On a 4 GB board it is
fine for the one or two desktop starts a presentation does; it is not fine for dozens, and it stays on
the post-demo list.
