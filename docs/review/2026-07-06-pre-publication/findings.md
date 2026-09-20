# Findings — 2026-07-06 pre-publication review

Accumulated as passes complete. `[fixed <sha>]` = resolved. Severity: high/med/low.

## Legal / provenance (headline concern) — CLEAN

Manual cross-repo scan (vs `external/linux` GPL clone, `external/mesa` MIT clone):

- **No GPL/FSF/Linux license markers** in any changed file across the siblings.
- **bcm-genet.c** (GENET Ethernet): fresh code, own register naming (not Linux's `UMAC_*`);
  header documents *behavioral-only* references (Linux/U-Boot/Circle/FreeBSD) per CLAUDE.md.
  Not derived. ✓
- **bcm2711-emmc** (SD): pre-existing Phoenix driver (2023, Phoenix authors) adapted for
  BCM2711 SDIO backend. Not Linux-derived. ✓
- **teken** (vendored FreeBSD VT parser, tty): `.c`/`.h` retain FreeBSD BSD headers;
  `teken_wcwidth.h` retains the Markus Kuhn permission notice; `teken_state.h` is a generated
  table ("Generated file. Do not edit.", matches upstream). ✓
- Mesa/quakespasm/vkquake are forks of permissive (MIT) / GPL projects respectively —
  publishing our ports under their own licenses is fine (pending fork-diff review by the workflow).

Findings:
- **[low] teken_phoenix.h missing Phoenix `%LICENSE%` header** — it's a Phoenix-authored
  compat shim; every other Phoenix file carries the standard header. Add it.
  `sources/phoenix-rtos-devices/tty/pl011-tty/teken/teken_phoenix.h`

## (workflow findings appended below as they land + are verified)

## Prior-review B1–B14 status (re-checked 2026-07-06/07)

The 06-06 review deferred B1–B14 as NEEDS-HW. Current status:
- **B4 [high] LIVE** — `kernel/main.c:157` `#if NUM_CPUS != 1` block references
  `hal_smpPrimaryReady`/`hal_smpFirstIntervalUs`, defined ONLY in `hal/aarch64/*`. Shared
  main.c → **link break on sparcv8leon SMP targets (gr740/gr712rc)**. Also a leftover debug
  print `"hi: primary-ready set"`. Fix: gate the aarch64-specific block to the aarch64 HAL
  (or a HAL hook) + drop the print. (NOTE: workflow units don't cover main.c — manual only.)
- **B10 [med] LIVE** — `project/aarch64a53-generic-rpi4b/board_config.h:16-17` PLO GIC base
  `0x40041000/0x40042000` vs a72's correct `0xff841000/0xff842000`. a53-rpi4b IS a present
  target dir. APPLY-SAFE (copy a72 values) if a53 is kept; else consider removing the target.
- **B6 RESOLVED** — raw-VA UART probes (`0xffffffffffe00000`) gone from log.c/devices.
- B2/B3/B7/B8/B9/B11/B12/B13/B14 — status swept below / to verify against current code.

## Manual pass — port-introduced findings (complement workflow)

- **[high] B4 main.c cross-arch link break** (see above) + leftover debug print
  `hal_consolePrint(ATTR_USER, "hi: primary-ready set\n")` at kernel/main.c:172.
- **[med] bcm2711-emmc duplicates zynq7000-sdcard verbatim** — `sdcard.c`, `sdhost_defs.h`,
  `sdstorage_dev.{c,h}`, `sdstorage_srv.c` copied from `storage/zynq7000-sdcard` (documented
  TODO at sdcard.c:29 "de-duplicate into a shared lib before upstreaming"). DECISION: dedup is
  a structural refactor (shared SDHCI core lib) — quantify the diff, then decide (→ user task).
- **[low] genet bcm-genet.c:17 stale comment** — header still says "cyclic aliasing of 16
  unique pinned buffers" (the OLD buggy design); lines 98-109 correctly describe the fixed
  256-unique-buffer design. Fix the header comment.
- **[low] pcie/server/pcie.c:960** — leftover `#include <sys/debug.h>` diagnostic (TODO says
  remove once VL805 BAR stable). pcie-server is the inert/never-compiled copy, so low-risk, but
  it's dirty code that would publish.
- **[low] sdcard.c #154 self-test diag** — gated behind undefined `SDCARD_DIAG_CLOCKSWEEP`;
  the bug it validated is RESOLVED+HW-validated. Dead-but-documented; consider removing for
  publication cleanliness.
- **[low] teken_phoenix.h** — missing Phoenix `%LICENSE%` header (see legal section).

Note: many FIXME/TODO in posix/*, msg.c, libtty, signal.c, vm/map.c, unistd/* are PRE-EXISTING
UPSTREAM Phoenix debt (not port-introduced) — out of scope for this port review.

## Gap-file review (kernel top-level, workflow-uncovered) — mostly clean

- hal/cpu.h (+hal_cpuSendIPI), hal/timer.h (+hal_timerIrq): plain decls, fine.
- include/posix-fcntl.h: self-referential F_* macros for autoconf `#ifdef` probing —
  well-documented, values unchanged. Good.
- lib/lib.h: single-core aarch64 atomics (spinlock vs __atomic_), gated to
  `__aarch64__ && NUM_CPUS==1`, tracked TD-01/TD-11. Correct for single-core. OK.
- **[med] log/log.c shared-behavior change** — the klog→UART per-byte mirror is `#if
  !RPI4_LOG_TO_FILE` (default 0), so it's ON BY DEFAULT for EVERY Phoenix board, not just
  RPi4. Changes console behavior for all targets on upstream. Consider making it opt-IN
  (default off; RPi4 board_config.h opts in), and the ~20-line comment is longer than the
  project's short-comment norm.

## posix/ poll-readiness + AF_UNIX (workflow-uncovered — manual)

Design (readiness-woken poll with a 20ms timed fallback) is sound and well-documented.
Findings:
- **[low] unix.c:84 misleading comment** — claims "the wakeupPending sentinel covers the
  check-then-wait race", but NO such sentinel exists in the code. The race is actually
  bounded by the timeout fallback in unix_pollWait (proc_threadWaitInterruptible w/ deadline).
  Fix the comment to describe the real mechanism (anti-slop: comment must match code).
- **[low] unix_close doesn't broadcast pollQueue** — a peer-close POLLHUP wake for a blocked
  poller degrades to the ~20ms fallback instead of firing immediately. Not a hang; consider a
  broadcast on close for symmetry with accept/connect/recv/send.
- POLL_INTERVAL 100ms→20ms and the hasUnix path: correct, documented. OK.

NOTE: posix/ (posix.c, unix.c) is NOT covered by any workflow unit — reviewed manually here.

## external/vkquake (V3DV/Vulkan port) — WIP, workflow-uncovered

- vkQuake is explicitly WIP + OFF-by-default (`--with-vkquake`; not HW-validated). Its Phoenix
  edits (gl_screen.c, gl_rmisc.c) contain live **bring-up bisector diagnostics**: hardcoded
  magenta test rect (gl_screen.c:1125-1160), "copy-mechanism-vs-conchars-data discriminator"
  (:1222), per-shader diag (gl_rmisc.c:2156). These are legitimate for an in-progress port but
  are NOT publication-clean.
- **[decision → user]** Is vkQuake/V3DV in the public release? If yes it needs a diagnostic
  cleanup pass (or a clear "experimental/WIP" label); if it's excluded or shipped as explicitly
  experimental, the diag is acceptable. Not touched — it's your active debugging state.

## FIX LOG (pass 2 — 2026-07-07)

Workflow: 87 verified findings (4 high, 25 med, 58 low) — full list in
`workflow-findings-triage.md`. Fixing per repo with build tests; HW/legal/judgment
items → PENDING-USER-TASKS.

**kernel (committed c93298a9, build OK):**
- [fixed] B4 main.c cross-arch SMP link break → gated on __aarch64__ + dropped "hi:" print.
- [fixed] _init.S early-exception facility + VBAR install → guarded on PL011_TTY_EARLY_VADDR
  (workflow HIGH #1; fixes link on 5/6 aarch64 targets).
- [fixed] pmap.c:923 collapsed-line typo.

**kernel diagnostics (staged, build-testing):**
- [fixed] msg.c td14 devfs-lookup IPC-timing trace removed from proc_send (field + 5 sites).
- [fixed] threads.c write-only threads_smpTickCount removed (reader was already deleted).

**HIGH still open (routed to PENDING-USER):**
- fbdev.c GPL keycode table (legal — needs your call + HW kbd test).
- quakespasm-port/vkquake-port GPL headers + repo LICENSE structure (legal decision).
- preinit.plo.yaml gpu_mem/ddr-map (needs boot validation).

**NEXT batches (by repo, each build-tested):** usb-stack (hub.c counter+leaks, mem.c diag),
dev-tty (usbkbd/usbmouse insertion leaks = B9), dev-storage (sdcard stale DMA comments),
dev-pcie (dead pcie-server duplicate + B1 + diag scaffolding), libphoenix (wcstombs),
lwip (mbox.c diag dump), audio (DRAM_BUS mask), corelibs (special.c race),
externals (quakespasm SCR_CaptureTick div0, mesa v3d heuristic — cautious, proven path),
+ the 58 low (comments/quality) last.

### Fix batches committed (pass 2, build-verified each)
- kernel c93298a9: B4 main.c + _init.S cross-arch link breaks + pmap typo.
- kernel 49e41298: msg.c td14 trace + threads.c write-only SMP counter removed.
- usb 3c7fdb2: hub.c give-up-counter replug recovery + hub_conf error-path leak.
- devices 30eaafb: usbkbd/usbmouse insertion-path resource leaks (B9).
- devices (build-testing): audio DRAM_BUS >=1GB PA -> PIO fallback + false-comment fix;
  bcm2711-emmc sdcard.c stale "DMA disabled" comments corrected.

### Remaining medium (next batches)
- dev-usb-pcie: pcie-server ~600-line dead BCM2711 duplicate of usb/xhci/bcm2711-pcie.c
  (incl. B1 BAR2-size bug + diag scaffolding) — it's never-compiled dead code; DECISION:
  delete the dead block vs keep pcie-server (→ likely delete, flag to user).
- libphoenix wchar.c:94 wcstombs truncation (>255 → should return (size_t)-1).
- lwip port/mbox.c:142 leftover raw-memory diag dump.
- corelibs special.c:103 random_hwrngFd concurrent-access race (needs a lock).
- tools-v3d gl_stubs.c:123 posix_memalign ignores alignment.
- ext-quakespasm gl_screen.c:982 SCR_CaptureTick divide-by-zero (fractional scr_capture).
- ext-mesa v3d_resource.c:909 size-only scanout heuristic too broad (CAUTION: proven render
  path + relates to the flicker area the user is testing — review, likely flag not change).
- kernel-hal-rest: config.h stale NUM_CPUS comment, generic.c hal_cpuReboot stub (no marker),
  console.c misleading Early-VADDR name, interrupts.h TIMER_WAKEUP_IRQ enum-vs-#ifdef.
- plo _init.S:594 leftover exception-vector diagnostic scaffolding.
- + 58 low (comments/quality).

### Pass 2 complete — HIGH + MEDIUM triaged (2026-07-07)
Fixed + build-verified + committed (9 batches): kernel c93298a9/49e41298/04eda12b,
usb 3c7fdb2, devices 30eaafb/7379e73, libphoenix b740469, lwip 5452478, quakespasm e0109fc.
Flagged to PENDING-USER (HW/legal/render-path/concurrency/decision): fbdev GPL table,
quakespasm+vkquake GPL licensing + top LICENSE, preinit gpu_mem, posixsrv race, interrupts.h
SMP enable/delete, pcie-server dead-code deletion, posix_memalign, mesa scanout heuristic,
console.c/B5, vkquake scope, SD flicker boot, lighttpd, NFS durable fix.
REMAINING: 58 low (comment/quality/typo) in findings-low.md — batching per-repo next; plo
_init.S:594 diag vector + kernel name.c trace-named predicates (assess in the low pass).

### Low pass progress (2026-07-07)
- dev-tty 528eefe: dead `woke` local + teken provenance/Phoenix headers.
- kernel fdce4eac: _vm_init boot traces + disproved-hypothesis SMP narrative.
- kernel f0bbf82d: all 8 #43 TEMP-NOMEM-DIAG probes removed from the exec path.
Kernel low/hack findings now fixed except deferred cpu.h:89 (FIQ comment) + syscalls.c:849
(err churn) + the flagged watchpoint cluster. Continuing per-repo: plo diag vector, dev-usb-pcie
diag (part of the flagged pcie-server dead-code deletion), fs-nfs/lwip/ports/project/utils lows.

### Low pass — repos completed (2026-07-07)
Fixed+committed: dev-tty (528eefe), kernel vm/generic (fdce4eac), kernel process.c #43 diag
(f0bbf82d), kernel cpu.h/syscalls (6bd75283), usb dev.c portEnumFails leak (3779d7d),
devices sdcard/sdstorage comments (e6ab289), utils nfs-smoke leak (b759bf5), lwip MDIO/MAC
(3526d62), devices audio settle (adf084a).

### Remaining low items (catalogued, not yet applied) — findings-low.md
- Moderate correctness/hack (worth doing, need care): dev-misc vcmbox MBOX_EMPTY mislabel on
  write-FULL (add MBOX_WRFULL); dev-storage un-gated SDREADDIAG printf (gate behind the diag
  macro); dev-misc ipcprobe unchecked write(PONG) (diagnostic util); usb mem.c #121 free-guard
  only validates head; nfs-fs '.'/'..' id-node aliasing.
- Boot-critical (FLAGGED, need HW): project armstub SMP-D-3 marker block; project
  kernel8-reloc forward-copy direction; plo _init.S diagnostic vector table.
- Minor comment/quality (safe, low value): libphoenix termios.h c_oflag #if claim; plo-rest
  ldt "1GB" header, _startc.c heap-zero TD-05 label; plo cache.c SCTLR dup + hal.c signed cmp;
  ports xterm/windowmaker/libnfs comments + windowmaker -D_SC_LINE_MAX; build port_internal
  HOST_TARGET suffix; project user.plo.yaml commented TEMP lines.
- Legal (VERIFY): audio rpi4-audio.c header credits rpi4os.com tutorial — confirm no code lifted.
- Tools/externals (need showcase/GPU build): tools-v3d posix_memalign (flagged), tools-quakespasm
  /x11 + ext-mesa/quakespasm lows.
