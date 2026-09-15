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

---

# Hardening and publication debt, later on 09-15

## 3d. ✅ `#64` closed — and it had one real, unapplied fix inside it

`#64` was a one-line row ("SD-side filesystem stack pressure"). Its SD/ext2 half was **already fixed
and is the reference fix** (`#120`: storage pool → 64 KB). But the 2026-06-08 pool-thread audit that
re-checked every Pi 4 server left **one RISK row that was never applied**: the **USB msg and status
threads on 2 KB stacks**, running msgRecv/status → device enumeration → in-process class drivers
(usbkbd/usbmouse) → DMA pool, on adjacent `.bss` with **no guard page** — so an overflow does not
fault, it silently clobbers the neighbour. That is this port's own history: `#120` was misdiagnosed as
an ext2 bug for exactly that reason.

🔧 usb `f9f5723`: **2 KB → 32 KB** (top of the audit's 16–32 KB range, ~60 KB of `.bss`).
**Verified on hardware:** `usbkbd` **and** `usbmouse` both enumerate (`/dev/kbd0`, `/dev/mouse0`),
0 faults, unix-socket **38/0**, quakespasm `Host_Init` **2.228 s**, every non-debug boot-stage row
green. Manifest `manifests/2026-09-15-usb-stack-32k.md`.
⚠ **Not in the `16ad56f9` demo image** — that was cut before this. The image's USB works (it is what
every gate ran on); this removes a silent-corruption margin, it does not fix an observed failure.

## 3e. ✅ Publication debt from the 07-06 review — swept, two items were still live

Re-checked the pre-publication review's open findings against today's source. **Already fixed:** B4's
cross-arch link break (the SMP block is now `#if (NUM_CPUS != 1) && defined(__aarch64__)`), B10 (a53
GIC base), the stale genet header comment, the leftover `pcie.c` debug include. The sdcard `#154`
sweep is documented dead-but-useful, which is fine.

**Two were still live, both "port debt that changes behaviour for every Phoenix board":**
- 🔧 kernel `bb05353f` — six unconditional `hal_consolePrint(ATTR_USER, "hi: ...")` bring-up markers in
  the **shared** `main.c`. Kept as a capability, not deleted: back with
  `KERNEL_DIAG='-DKERNEL_BOOT_TRACE'`. Verified **both ways** so the flag cannot be dead: stock
  `loader.disk` has **0** `hi: ` strings, the flag build has all **6**.
- 🔧 kernel `53cd40d6` + project `871b4fa` — the klog→console mirror was `#if !RPI4_LOG_TO_FILE`, i.e.
  **on by default for every board**. Now opt-in (`KLOG_CONSOLE_MIRROR`, default 0); both RPi4 targets
  opt in from `board_config.h`. `RPI4_LOG_TO_FILE` still suppresses it on top; the panic path stays
  ungated.

**Verified on hardware after each:** kernel messages still reach the console, unix-socket **38/0**,
quakespasm `Host_Init` **2.25 s**, 0 faults, every non-debug boot-stage row green.
Manifest `manifests/2026-09-15-publication-debt.md`.

---

# The W38 bug work in full (moved out of the weekly log 2026-09-15)

## 3. ✅✅✅ `premain-hang` — ROOT-CAUSED AND FIXED (2026-09-15)

**`socket()` deadlocked the process that owns `/`.** The kernel (`posix/inet.c`) and libphoenix
(`sys/socket.c`, via `getaddrinfo`) resolved `/dev/netsocket` with a **path lookup on every call** —
and a path lookup is answered by the filesystem that owns `/`, which single-threaded `nfs-fs` **is**.
libnfs opens a fresh socket whenever it must reconnect, so a dropped server stopped the whole
filesystem and every process blocked on its first request. A launch hung while opening its own binary.
⇒ It was never crt0, libc, exec, `vfork` or the scheduler — each was eliminated correctly; the
launched process was a victim.

Fixes: kernel `7c021702` (cache the oid) + `4e473590` (same for `/dev/posix/pipes`, which `open()`
also paid for on the hot path) · libphoenix `47dde32` · ports `a44623a` (libnfs sync-call deadline,
defence in depth) · filesystems `427aabd`, `d5db203`.
**Gate: `./scripts/test-nfs-recovery.sh`** — restarts the host nfsd under a running Pi and requires
the reclaim to complete AND a fresh process to launch. **PASS** on the clean build; the same scenario
wedged **2/2** before. Manifests `2026-09-15-premain-hang-fixed.md`, `2026-09-15-pipesrv-cache.md`.
**Regression question:** the per-call lookup is **old** — not from the recent merges. Our own NFS-root
takeover (2026-06-28) made it reachable and the NFSv4 lease keepalive (2026-08-03) made it periodic.
ⓘ **Netboot-only** — an SD root has no `nfs-fs`, so the demo image cannot hit this.
ⓘ Post-fix six-app gate: **6/6 content-graded, 0 faults, 6/6 launched**; torches **16/16 LIT**.
Full hunt: [`docs/done/2026-09-15-premain-hang-hunt.md`](../done/2026-09-15-premain-hang-hunt.md).

## 3b. Closed this week — verdict per item (evidence in [`docs/done/2026-09-15-w38-fixes-detail.md`](../done/2026-09-15-w38-fixes-detail.md))

| item | verdict |
|---|---|
| ★★★ **allocator corruption** (`freebin-corruption` / `stk-highbits-pointer`) | Cause is a BO CPU mapping with **two live owners**; fixed by devices `7a1e3db` + mesa `274ee5abea9` (2026-09-12 20:51 — four hours *after* the archive's last such fault). **Measured, not inferred: 0 overlapping reuses in 2880 BO events** via `V3D_BO_TRACE=1` + new `scripts/analyze-bo-trace.py`. 23 clean STK runs attach to **this** fix, not to font patch 0019. ⚠ Raw "addresses reused by >1 handle" is **117 and healthy** — only *temporal* overlap is a defect; the historical "56" was measured with unmapping off. Believed fixed, watching. |
| ⛔ **STK font path** | Cleared as the origin. The faulting `ldrb` is in the collect loop (`font_with_face.cpp:787`); 0019 patches loops from 834, and its own guard `STK-FONT-OOB` fired **0 times in 28 logs**. No out-of-bounds 4-byte store exists in `render()`; the `m_cached_gls` dangling-reference theory has no invalidation path. Probes `0020`/`0021` shipped so the next occurrence is decisive. |
| ✅ **`#66` stale X lock** | Fixed (coord `ce5f50873`). Not a broken liveness probe — new `test-libc-signal -g liveness` proves `kill(pid,0)` is correct, **5/5**. It is **pid reuse** (kernel hands out the lowest free pid). `xlaunch` now drops a leftover lock; verified both with and without one. |
| ✅ **`fbcon up` row was lying** | The HDMI console was always fine; `pl011_writeRaw` raced the kernel console and lost the marker ~2 boots in 3. Fixed (devices `3174143`): **3/3** now. Follow-on: 9 of 19 boot-stage rows were false negatives — two stale patterns fixed, seven debug-only rows now render `[ - ]`. |
| ✅ **`#67` vkQuake torches** | **Closed — 6/6 by rate, twice.** `torch67`: 6 of 6 gradeable trials PRESENT, 0 absent, 0 inconclusive, 16–19 at-viewpoint frames each, **599/553 lit px vs a threshold of 8**, 0 faults. Repeats the 09-14 6/6 on a different build ⇒ **12 trials, two builds, no absence**. No intermittency survives a correct measurement; the historical reports were scored without a controlled viewpoint. The check runs inside the showcase gate, so a regression is caught automatically. |
| ★ **`libc-uninit-main` narrowed from the ELF** | The two globals that read back zero — psh's applet list and `stderr` — share **one 4 KiB page** of `.bss` (`0x438198` and `0x4383c8`, page `0x438000`), and the fault's own registers were on it (`x24` = `psh_common` exactly, `x25` = the page base). ⇒ **one page reading back zero explains both**, which is much smaller than "libc init never ran": those writes *do* happen, so they are not there. Same shape as `atexit-null-head` (".data did not reach the process intact"). Both surviving readings are **mapping** bugs, not startup bugs. 🔧 A recurrence now arrives labelled: psh's unknown-command path reports via `write(2)` (it used to die on the error itself) and names an empty applet list as lost `.bss`. |
| ⓘ **`libc-uninit-main`** | New row, **one** sighting, no demo impact. `far=0x30` is `FILE.lock` — `fwrite()` on a NULL stream. Two readings (libc init never ran / the page lost its writes) point at different subsystems, so the row picks neither. libphoenix `c996bec` hardens `_file_init()`'s unchecked `calloc()`s. |

🔧 Also shipped: libphoenix `c5a4251` — the bin-corruption report that actually fires in the field (the
large-bin lookup one) printed only `chunk/want/hbase?=`, so every real occurrence came back unable to
tell a stale pointer into a released heap from a live heap base from a look-alike payload. It now
prints the same discriminators as the long report. Containment unchanged.

**Verification of the above, all green:** libc `stdio` 82/0/1 · `posixsrv` 16/0 · `unix-socket` 38/0 ·
`signal -g liveness` 5/0 · `stdlib` 93/0 · quakespasm `Host_Init` **2.23–2.28 s** · STK **0 faults, 2014 frames** ·
six-app gate **6/6 content-graded, 0 faults**. Manifests `2026-09-15-*`.

## 3c. Measured this week, nothing to change

| measurement | result |
|---|---|
| **Full libc sweep** (all 21 suites) | **1171 tests, 0 failures**, 28 ignored, 0 faults. Found and closed one real gap: `inet-socket` had 2 cases and `socket()` — the call whose kernel path changed this week — had none; now 7, aimed at what the `/dev/netsocket` oid cache can get wrong (tests `4e0da18`). |
| **`V3D-binner-wedge`** | 6 more q3dm7 runs, **0 wedges, 58 123 frames** → **9 runs, ~85 700 frames, 0 wedges**; last sighting 2026-08-22. The row's "named next step" (diff our binner setup vs Mesa/Linux) **had already been done on 08-22**; re-verified, and 3 of its 4 arms are *structurally* impossible — we emit no `TILE_BINNING_MODE_CFG` field at all, tile_alloc is Mesa's own formula that saturates by ~16 draws (so q3dm1 and q3dm7 get the same ~570 KiB), and no unflushed CPU→GPU write exists. ⓵ One real divergence found: we write `CTL_MISCCFG` at init, **Linux never does on 4.2**, and the silicon default differs in two fields. **Deliberately not changed** — the bug has not reproduced, so an A/B now destroys the only signal. Next step is an *observation*: read `CL@ct0ca` at the next recurrence. |
| **Demo-length X soak** | `startx_gpu --quit-after 600 action` — the full 6-client desktop for **10 minutes**, clean shutdown: **0 faults, 0 GPU wedges, 0 allocator events, 0 lock warnings**. Longest previously verified run was ~5 min, shorter than a presentation. ⓘ Per-lifecycle residue at demo length: 91 204 → 246 336 KB — fine for the one or two desktop starts a talk needs, still on the post-demo list. |

Detail: [`docs/done/2026-09-15-w38-fixes-detail.md`](../done/2026-09-15-w38-fixes-detail.md).

## 3d. Hardening + publication debt (2026-09-15) — detail in [`docs/done/2026-09-15-w38-fixes-detail.md`](../done/2026-09-15-w38-fixes-detail.md)

| change | why |
|---|---|
| **`#64` closed** — USB msg/status thread stacks **2 KB → 32 KB** (usb `f9f5723`) | Its SD/ext2 half was already the reference fix (`#120`), but the 2026-06-08 pool-thread audit left **one RISK row unapplied**: those two threads ran enumeration → in-process class drivers → DMA pool on 2 KB of adjacent `.bss` with **no guard page**, so an overflow silently clobbers the neighbour. That is exactly how `#120` got misdiagnosed as an ext2 bug. Verified: `usbkbd` **and** `usbmouse` enumerate, 0 faults. |
| **`main.c` bring-up markers gated** (kernel `bb05353f`) | Six unconditional `hal_consolePrint(ATTR_USER, "hi: …")` in the **shared** `main.c` printed on every Phoenix board. Kept as a capability — `KERNEL_DIAG='-DKERNEL_BOOT_TRACE'`. Verified **both ways** so the flag cannot be dead: stock loader has **0** `hi: ` strings, flag build has **6**. |
| **`usb/mem` #121 archaeology retired** (usb `e8e1092`, −94 lines) | #121 was root-caused and fixed (stale dirty cache lines evicted over the pool; the `dc civac` in `usb_allocBuffer`, usb `12c4fe8`) — and the instrumentation said so itself: *"Remove once the writer is pinned and fixed."* Gone: the alloc/free rings, the two record helpers called on **every** `usb_alloc()`/`usb_free()`, and the caller-PC walk. **Kept: detection, not investigation** — `usb_chunkSane()` still refuses a wild free-list pointer and leaks rather than crashes, and a hit still dumps the corrupt node's bytes. Silence is not evidence. |
| **klog console mirror made opt-in** (kernel `53cd40d6` + project `871b4fa`) | It was `#if !RPI4_LOG_TO_FILE`, i.e. **on by default for every board**. Now `KLOG_CONSOLE_MIRROR`, default 0, with both RPi4 targets opting in from `board_config.h`. Panic path still ungated. |

Also swept and found **already fixed**: B4's cross-arch link break, B10 (a53 GIC base), the stale genet
header comment, the leftover `pcie.c` debug include, and both "moderate correctness" items the review
left catalogued (vcmbox `MBOX_WRFULL`, the un-gated `SDREADDIAG` printf). TD-21 (syscall-table divergence) is **RESOLVED and
HW-verified since 2026-09-04** — not a pending owner action.
**Verified on hardware after each:** unix-socket **38/0**, quakespasm `Host_Init` **2.23–2.26 s**,
0 faults, every non-debug boot-stage row green. Manifests `2026-09-15-usb-stack-32k`,
`2026-09-15-publication-debt`.

## 3e. ⓘ Netboot's mirror-image guard: exists, and it discriminates

The SD gate I added covers "an SD image carrying a netboot loader". The opposite footgun — a
`--variant sd` build overwriting the **TFTP** loader, so every netboot cycle silently tests nothing —
is already guarded by `scripts/check-netboot-blob.sh`, wired into both netboot cycle scripts.
Checked rather than assumed: against the live blob it prints `rootfs: nfsroot` and exits 0; pointed at
the SD image's loader it prints `rootfs: sd`, refuses with rc=3 and names the rebuild command.
⇒ Nothing to add here. Recorded so the next sweep does not re-implement it.

## 3f. 🐞 HEAP-CANARY fired for the FIRST TIME — a zeroed page in the AF_UNIX fork path

`test-libc-unix-socket` aborted at `dgram_sock_msg_fork` (2026-09-15 15:32, `w38-resync`):

```
HEAP-CANARY at child-after-recv: off=0 (page off 0) got=0x00 want=0xa5 run=187 zeros=1 base=0x5020
```

This is **pre-existing instrumentation in the test suite**, whose own comment says what the shape
means: *"a zeroed PAGE is the COW signature, a single flipped byte is something else entirely."*
**It has never fired before — 1 occurrence in the entire archive.**

Decoded: `run=187` is not a short run. The canary is `0xa5 ^ (i*31)`, which equals `0x00` at exactly
`i=187` (187·31 mod 256 = 0xa5), so the comparison stops there **because the pattern itself is zero**.
⇒ the region is **all zeros from the buffer base**, i.e. the COW signature, not a stray byte.

⚠ **Regression-first, per the standing rule:** the unix-socket suite passed **38/0** twice earlier the
same afternoon, and the only source change between those runs and this one is a **comment** in
`rc.psh`. So this is far more likely a rare intermittent that the canary has simply never caught than
a regression from today's work — but that is a claim, and a 6-trial bench is running to test it.
ⓘ It lands in the exact path a previously-fixed bug lived in — `map_pageFault` dropping `PROT_USER`
on EL1 user-copy faults, which produced a **COW storm on AF_UNIX recv into a forked buffer**. Same
scenario, same signature class as `libc-uninit-main`/`atexit-null-head` ("a page not holding the
writes made to it").
**Bench result: 6/6 clean, 38/0 each — so it is INTERMITTENT, not a regression.** Roughly 1 in 7 runs
today, and 0 in the whole prior archive. It stays open as the first *live, in-a-test* capture of the
"page reads back zero" family — far more tractable than the one-off process faults, because a test can
be re-run.

⛔ **And the companion source review came back a CLEAN NEGATIVE on the mapping story** — worth as much
as a positive would have been:
- `libc-uninit-main` and `atexit-null-head` are **not the same bug**. psh's RW `PT_LOAD` splits into a
  file-backed COW page (`.data`, where `atexit_common` lives) and a 3-page anonymous demand-zero
  mapping (where `psh_common`/`stderr` live). Different populate paths.
- **No kernel path loses a write to that anon page**: `process->lazy` is 0 on MMU builds so exec
  eagerly forces every page; `pmap_switch` precedes `process_load`; faults serialize on `map->lock`
  and a populated amap slot returns early; there is no pageout to drop a filled slot; and
  `_pmap_cacheOpAfterChange`'s flush-by-VA never fires for a cached RW data page.
  ⇒ **"those writes definitely execute" is an assumption to TEST, not inherit** — crt0 / `_libc_init`
  ordering is back on the table.
- `atexit-null-head` is already attributed in-tree and the attribution holds (`vm/object.c:285-309`:
  `object_fetchCluster` silently zero-filled on a short/EOF NFS read).
🔧 kernel `0c22f8f7`: the `.bss` comment in `process_load` claimed the anon mapping is demand-paged.
It is not — that is what sent this review looking at fault paths at all.
ⓘ Incidental VM defects found and recorded, not fixed blind: `amap_putanons` discards
`amap_putanon`'s return (dangling `anon_t*`), `amap_clear` drops anons without putting them (page
leak), `_vm_mmap` discards `page_map`'s rc, `_pmap_remove` passes a stale entry/lvl to the cache op.

## 4. `unlock on not locked lock` — narrowed a long way (2026-09-15)

⚠ First, scope: this message is emitted only by `LIB_ASSERT_THREADS`, which is behind
`KERNEL_DIAG=-DDEBUG_THREADS`. **It does not exist in the shipped build** and the failed unlock just
returns `-EPERM`. No demo impact.

**What the 52 archived occurrences actually say:** the lock is **always** `user.mutex` and the pid is
**always the same one** (25) — the launched application, and the message lands **immediately after
`quakespasm: LOAD-TIME main->Host_Init`**, right behind SDL audio init. So it is one call site in one
program at one moment, not a scattered defect.

★ **A code path that provably produces exactly this** (not yet tied to this occurrence — labelled a
candidate): `semaphoreDown()` (libphoenix `sys/semaphore.c:52-66`) does
`mutexLock` → `condWait(...)` → `mutexUnlock`, i.e. it assumes `condWait` always returns with the
mutex held. **The kernel explicitly does not:** `proc_lockWait()` (kernel `proc/threads.c:2273-2281`)
re-acquires only when `err != -EINTR`, so on `-EINTR` — and when `_proc_lockClear()` itself fails —
the caller comes back **without** the mutex. `semaphoreDown` then reads `s->v` unlocked and finally
unlocks a mutex it does not hold. POSIX requires `pthread_cond_wait` to return with the mutex locked
in every case, so the kernel side is the one out of contract.
⏭ Deliberately **not** patched blind: the right fix is in `proc_lockWait`'s interruptible path, and the
`-EINTR` skip looks intentional. Needs a decision and a test that a thread still owns the mutex after
an interrupted `condWait`, not a speculative edit to a benign warning.
ⓘ The `STRAY-UNLOCK` probe's `user pc`/`lr` remain useless — they are kernel addresses, confirming
again that `thread->context` is the scheduler's last save, not the syscall frame.
