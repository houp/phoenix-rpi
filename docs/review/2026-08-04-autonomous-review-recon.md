# Full-repo code-review recon — 2026-08-04 (autonomous)

Read-only candidate list for plan task **G1**. Produced by a 4-auditor recon pass,
cross-checked against the prior pre-publication review
(`docs/review/2026-07-06-pre-publication/`) and the TD tracking checklist. **These are
CANDIDATES — verify each before fixing.** Owner-policy calls (licensing choice, whether
bring-up harnesses ship) are NOT auto-decided; see PENDING-USER-TASKS.

## Execution ordering for autonomous work

**Tier A — safe now, text-only, no behavior change (do first; a `--scope core` build
to confirm syntax, no Pi boot needed for correctness):**
- Fix factually WRONG comments: `lwip/port/genet-rxcache-bench.c:18-21` (`dc ivac` →
  `dc civac`); `usb/mem.c:191-197` (refs deleted `port/diag-udp.c`); `usb/usb.c:526-530`
  (refs non-existent `runStateSelftest`); `usb/usb.c:504-508` (soften REFUTED
  per-process inbound-DMA silicon story to the real PoC-embedding reason).
- Reword stale `TODO(#NNN)` markers that misrepresent FINISHED code as unfinished:
  `usb/hub.c:37,321-372` (5× #129), `usb/xhci/xhci.c:1847`, `pcie/server/pcie.c:783,957-962,1013`,
  `storage/bcm2711-emmc/sdstorage_dev.c:162`, `sdcard.c:40-49`, `pl011-tty.c:1168-1171` (#127).
- Trim stale-history comments: kernel `hal/aarch64/_init.S:887-894,98-102`, `main.c:151`
  (SMP-D-5 label); plo `_init.S:187-188,286-294`, `hal.c:144,375-380`, `video.c:204-241`;
  project `phoenix-armstub8-rpi4.S:116-126,372-379`, `board_config.h:48-57`.
- Remove stale TD marker `pl011-tty.c:629 TODO(TD-14-pl011-retry)` (checklist: superseded).

**Tier B — remove disproved/dead diagnostic code (needs `--scope core` build + ONE Pi
boot-verify per repo; keep load-bearing recovery logic — only strip instrumentation):**
- `devices/storage/bcm2711-emmc/sdcard.c`: `#ifdef SDCARD_DIAG_CLOCKSWEEP` (1077-1350, call 1592);
  default-on `SDREADDIAG` (464-512, calls 817/850/898/2040) — hypothesis disproved (card-specific).
- `devices/pcie/server/pcie.c`: ~14 `debug()` sites + `diag-outbound` block (827-853).
- `devices/usb/xhci/bcm2711-pcie.c:640-657,695-703,965-1146` (USB-FIX-11/8 readbacks);
  `xhci.c:871-935,1352-1378` (USB-DBG), `1818-1840` (#129 wedged-dump).
- `devices/audio/rpi4-audio/rpi4-audio.c:451-453,417-419` (register dumps). **Boot tone
  484-503: gate behind a flag, don't silently delete.**
- `tools/v3d-driver-port/v3d_phoenix_winsys.c:674-722,1088-1090` (#67 bincrc — #67 RESOLVED).
- `tools/vkquake-port/platform/pl_phoenix_main.c:143-175` (loop heartbeat prints),
  `pl_phoenix_vk_vid.c:259-340,1197-1219` (bring-up counters).
- plo `_init.S:302-420` (dead `secondary_smoke_entry`); project armstub `216-236,286-315`
  + `phoenix-kernel8-reloc.S:61-123` (TR/marker `uart_putc`); kernel `console.c:125,50-55`.
- **plo `_init.S:619-668,540-617` (HIGHEST plo item):** the live EL1/2/3 vector table is a
  print-and-halt diagnostic; real `_exceptions_dispatch` is dead-wired → plo has NO real
  fault handling. Wiring real dispatch is a FIX, not just cleanup — treat carefully.

**Tier C — licensing (publication blockers; low behavior risk but verify policy):**
- Add missing Phoenix SPDX/`%LICENSE%` headers: `tools/v3d-driver-port/phoenix_mesa_compat.h`,
  `test_ident_decode.c`, `tools/x11-port/launcher/mouseprobe.c`, `tools/dbg-probe/dbg.h`.
- **Delete** dead `tools/x11-port/ddx/fbdev_stub.c` (superseded by `fbdev.c`).
- `kernel/hal/aarch64/_memset.S:1-14`: verify ARM optimized-routines provenance; if derived,
  add the ARM MIT attribution like its sibling `_memcpy.S:7,20-22`.
- Verify publish tooling substitutes `%LICENSE%` → Phoenix BSD in ~15 v3d/x11 glue files.

**Tier D — NOT removable yet / owner decisions (do not touch autonomously):**
- `usb/mem.c:24-170,322,411` #121 free-list forensic apparatus — #121 root cause
  UNCONFIRMED; keep as forensic hook, note in release notes.
- Whether bring-up harnesses/probes ship: `v3dv_harness`, `gl_det_harness`,
  `gl_frontend_smoke`, `rpi4-ipcprobe`, `mouseprobe`, `dbg-probe`, `test_ident_decode`.
- GPL-vs-permissive choice for author-written glue + fork relocation (PENDING-USER).
- Debug facilities exposing ABI/syscall surface: `pctl_watchpoint` (exceptions.c:222-271).
- Genuine unimplemented TODOs (keep): `xhci.c:2151,2169,3367` (topology/speed),
  `sdcard.c:29-31` (zynq dedupe, pre-upstream), `sdcard.c:283`, `sdstorage_dev.c:140`,
  `quakespasm pl_phoenix_vid.c:176-189` (gamma root-cause), `_memset.S:24 TODO(TD-20)`,
  TD-14 devfs markers, `bcm-genet.c:22 TODO(TD-Eth-LinkIRQ)`.
- Non-Pi4 generic TODOs (out of scope): plo `cpu.c:41`, `interrupts_gicv2.c:335`,
  `_exceptions.S:382`, `pmap.c:280,464`; shared `ephy.c`.

## Notes / anchors
- Verified already-fixed (no action): main.c B4 SMP link-break (gated on `__aarch64__`),
  fbdev GPL keycode, Quake/vkQuake GPL headers (now `GPL-2.0-or-later`).
- Verified clean: plo `phoenix-armstub8-rpi4.S` (BSD-3 + RPi attribution), kernel
  `dtb.c`/`cache.c`/`bcm-genet.c`, `teken/` (FreeBSD headers), libphoenix aarch64.
- `kernel/log/log.c:34-38,411-438`: `RPI4_LOG_TO_FILE` defaults 0 → klog→UART mirror ON
  by default for ALL boards, not just RPi4 (prior review flagged). Consider opt-in.
- Prior low items to spot-check still-present: `rpi4-vcmbox` MBOX_EMPTY mislabel,
  `fs-nfs` indent block, libphoenix `wchar.c:94` wcstombs truncation.
