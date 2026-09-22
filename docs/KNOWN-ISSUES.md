# Known open issues

Every issue we know about and have **not** fixed, in one list. Nothing else.

- **Open only.** A fixed issue leaves this file. Closed rows are kept, verbatim, in
  [`done/closed-issues-archive.md`](done/closed-issues-archive.md) so nothing is re-litigated.
- **No decisions recorded here.** Fix / postpone / won't-fix is the owner's call, made from this
  list; earlier verdicts have been stripped deliberately.
- **No history.** Each row says what is wrong and what we last measured. Analyses live in
  `docs/misc/` and `docs/done/`.
- Open `TD-NN` transitional-debt items are issues too and are listed here.
  [`TEMPORARY-FIXES-AND-FUTURE-CLEANUP.md`](TEMPORARY-FIXES-AND-FUTURE-CLEANUP.md) keeps their
  engineering detail; anything marked RESOLVED there is not repeated here.

Verified against the tree, the git history and the boot-log archive on **2026-09-22**. Where a
row's old claim turned out to be stale, the correction is noted in the row.

## 1. Correctness — crashes and data loss

| # | Issue | Last measured |
|---|---|---|
| C1 | **Heap corruption: allocator guards fire, usually without a crash.** One family, four guard messages — `free() of a corrupt chunk header`, `large-bin lookup returned a non-chunk`, `large-bin lookup returned a wild pointer`, `chunk handed out twice`. The corrupt value is a valid heap address with **garbage in the upper 32 bits** (`0x80000001_0000d000`, always a `0xd000` bin-14 heap). Root cause open: the double-BO-ownership route was fixed and measured clean (0 overlapping reuses in 2 827 BO events), so it does **not** explain the residue. Affects at least STK, Quake II, Quake III and the X desktop — not one app. | **Still live. 5 fires on 2026-09-22** in the six-app showcase gate (`gate-stk`, `why=5`, 0 faults, 1189 frames — the run still passed visually). Other signatures last seen 09-17, 09-21, 09-16. Non-fatal in every recent case: the guard leaks the block and the app keeps running. |
| C2 | **A process that exhausts its 1 MiB stack takes down the kernel.** `hal_cpuPushSignal` (aarch64) writes the signal frame to the now-unmapped user stack and double-faults at EL1, with no recovery. Reproducer: `tools/stack-bomb/`. | **Open, unguarded.** `hal/aarch64/cpu.c:92-117` still does an unconditional `hal_memcpy` onto `signalCtx->sp` with no bounds or mapping check, and its only caller adds none. ⚠ The upstream signal rework (kernel `a63760d3`, 2026-09-16) changed this path and was **not** re-tested against the reproducer. Last reproduction 2026-08-22. |
| C3 | **Two libc globals read back zero, killing the process at `main`.** `stderr` and psh's applet list are both unset, so `fprintf` dereferences a NULL `FILE*` (`far=0x30`). Both live in one 4 KiB `.bss` page. A source review gave a clean negative on every kernel mapping path, so "those writes definitely execute" is the assumption the next occurrence has to test. | **2 sightings (2026-09-07, 2026-09-15), none since.** Impact limited to `ntpclient`, which only sets the clock. The label added to make a recurrence self-explaining (`applet list is empty`) has **never fired** in the whole archive. |
| C4 | **Intermittent V3D binner wedge on long GPU runs.** Auto-recovered by a GPU reset that reports loudly (`RENDER TIMEOUT` / `DROPPED job`). Not an MMU fault — a CT0 front-end stall with zero error status, `ct0ca` parked inside a valid BCL BO. `q3dm7` is the documented reproducer. | **Not reproduced since 2026-09-09** (and that was an experiment-labelled run; last sighting on a shipping build 2026-08-22). 0 wedge signatures in every log 09-10 → 09-22, including 15 Quake III runs and both six-app gates. Diagnostics are armed, so the next occurrence self-explains. |
| C5 | **PWM audio DMA stalls at startup on some boots**, which used to hang Quake II inside `SDL_OpenAudio()`. Captured three times: the channel is alive but parked for a DREQ that never comes, ring full, 0 samples moved. Mechanism open. Blast radius is contained — the driver now grades the channel by progress, re-arms, and degrades to a paced null sink. | **Mechanism unfixed; symptom quiet.** 25/25 Quake II runs since 2026-09-18 reached `SDL audio initialized` and drew frames; 0 stall aborts in any log 09-19 → 09-22. ⚠ Timing-sensitive: the rate has moved across functionally neutral relinks, so short clean runs bound it rather than clear it. |
| C6 | **A 56-year NTP clock step can land inside an app's startup and silently kill it.** psh forks `ntpclient` fire-and-forget on session start, so the step can arrive anywhere in a ~2-minute window. An app timing its own startup across it sees a negative duration and may never start. | **Open; no product fix.** `pshapp.c:1641-1657` is unchanged (last clock-path commit 2026-09-02). Mitigation is entirely test-harness-side (`psh-interact.py`, 2026-09-19) and is bounded — it can time out and proceed. Historical rate of a *visible* failure: ~1 in 174 Quake III runs. |

## 2. Graphics and UI

| # | Issue | Last measured |
|---|---|---|
| G1 | **Windowed GL in X11 is bandwidth-bound, ~10 fps.** A GL app drawing into a window copies pixels GPU → CPU → socket → CPU → GPU every frame: 96 ms/frame at 640×480, of which `XPutImage` is 58 ms. No DRI3/DMA-BUF equivalent exists, so a client can only hand the server pixels, never a buffer reference. Full-screen GL is unaffected (the games bypass X). | Architectural. All cheaper mitigations measured and closed; a point fix was half-proven on hardware with a ceiling of only ~1.7×. |
| G2 | **xterm resize artefacts while dragging** on the GPU X desktop (owner-reported). | **Unreproduced.** No new evidence since 2026-09-14; resize *correctness* is verified once settled. The missing measurement is a deliberate drag test with the mouse released. |
| G3 | **A stale Mesa shader-cache blob renders as green speckle** over an otherwise-valid frame. Phoenix has no ELF build-id, so the cache keys on shader source only and a host toolchain change invalidates nothing. | **Mitigated, mechanism intact.** Both invalidation controls verified present 2026-09-22, and the `rsync --delete` hole that made the host-side one inert was closed 2026-09-16. No speckle report since. ⚠ The SD image ships **no** warm cache, so the first GL app on a freshly flashed card pays the full recompile. |

## 3. Platform limitations

| # | Issue | Last measured |
|---|---|---|
| P1 | **Only the 4 GB Pi 4B is validated, and 2/8 GB boards are known to be mis-mapped** — plo's syspage memory map is still hardcoded. The DTB parser also assumes a single interrupt controller. (TD-06, TD-15 remainder) | Open. Do not simply try an 8 GB board. Also in TD-15: VC4 quiesce + mailbox-move (low value), and drivers use identity va2pa (⚠ do not naively wire `dtb_armToBus` — it would break GENET). |
| P2 | **Asynchronous SError is masked in early kernel paths** because a live PCIe/VL805 USB external-abort SError is not root-caused; unmasking regresses boot. A dump-and-halt handler is implemented and armed. ⚠ This is why an MMIO read of an unclocked block **hangs silently** instead of aborting. (TD-10) | Open, HW-gated. |
| P3 | **`dc zva` fast path disabled in `hal_memset` on the Cortex-A72**, pending proof of the EL2 DC-ZVA trap state (does not reproduce in QEMU). Performance-only, correctness-safe. (TD-20) | Open, HW-gated. |
| P4 | **The generic aarch64 early-console path hardcodes the UART at VA `0xffffffffffe00000`**, because it must print before the DTB-discovered base is available. On the Pi 4B this alias equals the discovered pl011 base. Matters only when porting to another aarch64 board. | Open for a future cross-board port; inert on this board. |
| P5 | **Netboot NFS-root: one narrow boot-order race.** plo launches psh as a sibling of the NFS takeover without gating on it, so a command issued at the prompt can still hit the pre-takeover RAM root and report `not found`. The clean fix is a plo boot-order gate. | Open; invisible in practice because the test harness waits for the takeover marker. Confirmed still ungated in `user.plo.yaml` on 2026-09-22. |
| P6 | **`echo > file` from a shell script is ~50× slower than it should be on real storage.** bash's builtin output reaches the filesystem in very small writes, so cost scales with byte count: 5000 ms/file vs 100 ms for `cp` or `dd` on the same 512 bytes. Invisible on a RAM filesystem. | Open, ports-level (bash output buffering). The filesystem is not at fault. Workaround: prefer `cp`/`dd` or one redirect of a whole command's output. |

## 4. Incomplete features

| # | Issue | Last measured |
|---|---|---|
| F1 | **WiFi works but does not join automatically.** ⚠ *Corrected 2026-09-22 — the old "the lwip netif is not wired up" claim is stale.* The `wifi43455` netif is in the shipping plo launch line and registers on every boot; the driver joins WPA2-PSK, completes the 4-way handshake and gets a full DHCP lease. What remains: every boot prints `waiting for /dev/wifidata (start the rpi4-wifi daemon)`, so sockets do not use WiFi unless `rpi4-wifi` is started by hand. Last automatic DHCP-over-WiFi evidence is 2026-09-02. | Open: the join is manual. Use wired Ethernet for everyday networking. |
| F2 | **Bluetooth is driver-level only.** `/dev/hci0` comes up, patchram loads (323/323), a real BD_ADDR is read and an HCI Inquiry completes. There is **no host stack** — no L2CAP, SDP, GATT or RFCOMM anywhere in the tree, so no pairing, profiles or audio. | Open. Last `bt/` commit 2026-09-04 was packaging. |
| F3 | **No I²C, SPI, general-purpose PWM, camera (CSI-2) or DSI display drivers for the BCM2711.** ⚠ *Corrected 2026-09-22 — USB mass storage was on this list and works*: `/dev/umass0`/`umass1` enumerate at SuperSpeed and mount ext2, `e2fsck`-clean. `rpi4-audio`'s internal PWM1 use is a driver-internal path, not a general-purpose PWM driver. | Open, not started. |
| F4 | **PWM audio has had no audible sign-off on real headphones.** The path is verified end to end in software (DMA ring, mixer backend, self-test feeds 8960 samples). | Open; needs an owner at the hardware. The support matrix agrees. |

## 5. Debt and cleanup

| # | Issue | Last measured |
|---|---|---|
| D1 | **`RPI4AUDIO_ARMTRIALS` is a permanent diagnostic ABI in a published header** — an ioctl plus struct in `rpi4-audio.h` for a facility that can block the driver's only message thread. (TD-23) | Open by design; removal was tied to C6 closing, which has not happened. Still present at `rpi4-audio.h:65`. |
| D2 | **psh's `/dev/console` open-retry budget deviates from upstream** — 50 × 10 ms against upstream's 20 × 100 ms. (TD-14-psh-retry) | Open. Marker live at `pshapp.c:73,78`; unchanged. Close by restoring the upstream default once devfs registration is fast. |
| D3 | **libphoenix has no `swprintf`/`vswprintf`**, so SuperTuxKart ships an Irrlicht shim patch. (TD-STK-SWPRINTF) | Open. Zero implementation or declaration in libphoenix as of 2026-09-22. |
| D4 | **plo still boots with a debug exception vector table** — every slot prints a tag char and halts, in place of the real table. (TD-diag) | Open. Still at `plo/hal/aarch64/generic/_init.S:479`; self-described as temporary. |
| D5 | **VL805 firmware-load race workarounds in xhci** — config-space/BAR0 access races the boot ROM (the bridge returns `0xdead…`), so the code waits, capped at 300 ms; plus a pmap workaround pre-creating the xhci MMIO mapping because a late `mmap` empirically reads `0xdead`. (TD-USB, TD-USB-pmap, 4 sites) | Open; real workarounds, not cosmetic. |
| D6 | **`vkCmdSetDepthBias` is a no-op in the vkQuake Vulkan trampoline.** Deliberate: V3DV leaves `CmdSetDepthBias2EXT` unpopulated, so the indirect call jumped to garbage. Cost is polygon-offset z-fighting cosmetics. Proper fix: populate that entry in V3DV's dispatch. | Open; vkQuake renders correctly in the gate. |
| D7 | **Four TD-14 residuals are marked "likely still active" and have never been re-verified**: `tty0-nonfatal`, `ttyopen-nonfatal`, `console-alias`, `psh-ttyopen-errno`. | Open, unverified — the status is a guess in the registry, not a measurement. |
| D8 | **Three intentional upstream deviations are still carried**: `TD-14-devfs-direct` (devfs fast-path predicate), `TD-14-console-open-fastpath` (strdup short-circuit for `/dev/console`), `TD-14-tiocspgrp-pgrp` (correct semantics, upstreamable as-is). | Open only in the sense that they are deviations; all three work as designed. |
| D9 | **22 stale local branches across 12 sibling repos.** 16 are fully merged into master and safe to delete; six are unmerged work — `devices/wip/rpi4-wifi-glom`, `devices/agent/umass-umount-drain` (C3's WIP fix) and four on lwip (`agent/rpi4-genet`, `full-history-backup`, `rpi4-port-clean`, `wifi-wip`). (TD-Git-Branches) | Open housekeeping; nothing is lost and nothing blocks. |
| D10 | **Residual debug prints from the bring-up era** remain in the kernel and plo, to be removed case-by-case. (TD-05 residual) | Open, low priority. |
