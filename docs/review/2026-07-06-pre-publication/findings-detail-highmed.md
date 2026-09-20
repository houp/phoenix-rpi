# Workflow findings — full detail (medium + high), for the fix pass

## [high/correctness] kernel-hal-mem — hal/aarch64/_init.S:1078
**Early-exception dump block references PL011_TTY_EARLY_VADDR unguarded, breaking the link for every aarch64 target that doesn't define it**

- detail: _early_exception_common and the early_putc_inline / early_puthex64_inline macros use `ldr x21, =(PL011_TTY_EARLY_VADDR + UARTFR_OFFSET)` / `ldr x21, =PL011_TTY_EARLY_VADDR` with NO #ifdef guard, and the unconditional _early_vector_table (line ~1113) plus its `msr vbar_el1` install (line ~445-446) always emit calls into _early_exception_common. PL011_TTY_EARLY_VADDR is defined ONLY in the two rpi4b board_config.h files (aarch64a72-generic-rpi4b, and NOT even aarch64a53-generic-rpi4b). _init.S is a single shared source built for ALL aarch64 targets (hal/aarch64/Makefile), including aarch64a53-generic-qemu, aarch64a53-generic-rpi4b, and the three aarch64a53-zynqmp-* targets, none of which define the symbol. GNU as still assembles (it emits a literal-pool relocation against the external symbol), so an assembly-only check passes; the failure is at LINK time: 'undefined reference to PL011_TTY_EARLY_VADDR'. This is a build-breaking regression for those targets.
- fix: Wrap the whole early-exception facility (_early_exception_common, early_putc_inline, early_puthex64_inline, _early_vector_table) and its `adr x0,_early_vector_table / msr vbar_el1` install under `#if defined(PL011_TTY_EARLY_VADDR)`, restoring the prior VBAR behavior (or a symbol-independent fallback) for targets that don't define the early UART VA.
- verify: The finding is real and empirically reproducible. In /home/houp/phoenix-rpi/sources/phoenix-rtos-kernel/hal/aarch64/_init.S the early-exception facility references PL011_TTY_EARLY_VADDR with NO #ifdef guard:

- Lines 1032-1041 (early_putc_inline) and 1043-1065 (early_puthex64_inline) emit `ldr x21, 

## [high/correctness] project — _projects/aarch64a72-generic-rpi4b/preinit.plo.yaml:2637
**ddr map boundary (0x3b400000 = gpu_mem 76MB) contradicts config.txt gpu_mem=128, handing 52MB of GPU-reserved RAM to the kernel as usable rwx**

- detail: preinit.plo.yaml maps `map ddr 0x00400000 0x3b400000 rwx`, with the comment stating the GPU carve-out is 0x3b400000-0x3fffffff (76MB, i.e. gpu_mem=76). But config.txt in the SAME project now sets `gpu_mem=128`. On the Pi 4 the firmware carves GPU RAM downward from 0x40000000, so gpu_mem=128 places the GPU/firmware-reserved region at base 0x38000000, not 0x3b400000. The map therefore extends 0x38000000..0x3b400000 (0x3400000 = exactly 52MB) into GPU-owned memory and exposes it to the kernel as usable rwx DRAM. This is internally corroborated: config.txt's own comment says gpu_mem=128 'cost is ~52 MB of ARM DRAM' — precisely the overlap. The kernel allocating into firmware/GPU-reserved RAM causes non-deterministic corruption once the GPU/firmware touches those pages. The 'map ddr' end was never lowered when gpu_mem was raised from 76 to 128.
- fix: Lower the ddr map end from 0x3b400000 to 0x38000000 to match gpu_mem=128, and update the preinit.plo.yaml comment (which still says gpu_mem=76 and 0x3b400000-0x3fffffff). Alternatively pin the boundary and gpu_mem to a single documented value.
- verify: Verified against the source project files and git history.

FILE FACTS (sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/):
- preinit.plo.yaml:12 has `- map ddr 0x00400000 0x3b400000 rwx`, with the comment block (lines 5-11) explicitly saying "layout (with config.txt gpu_mem=76)" and 

## [high/legal] tools-quakespasm — tools/quakespasm-port/platform/pl_phoenix_stubs.c:31
**net_drivers/net_landrivers tables are verbatim GPLv2 Quakespasm (net_bsd.c) source with no license header/attribution**

- detail: pl_phoenix_stubs.c lines 31-92 (the net_driver_t net_drivers[] and net_landriver_t net_landrivers[] initializer tables plus the Q_COUNTOF sizing) are a byte-for-byte copy of external/quakespasm/Quake/net_bsd.c lines 31-96, whose header is 'Copyright (C) 1996-1997 Id Software / (C) 2010-2014 QuakeSpasm developers ... GNU General Public License ... version 2 ... or (at your option) any later version'. The comment in the file even states provenance ('network driver tables (were in net_bsd.c, which this port excludes)'). More broadly ALL files under tools/quakespasm-port/platform/ are a GPLv2 Quakespasm derivative: every shim #includes quakedef.h and implements Quake's internal Sys_*/VID_*/IN_*/SNDDMA_* contract, IN_Move is documented as 'identical math to in_sdl.c' (GPL), yet none of the new files carry any license header. Publishing these in a permissive (BSD-style) coordination repo without GPLv2 headers/attribution is a licensing violation and a real legal-exposure problem for public release.
- fix: Add the GPLv2 + Id/QuakeSpasm attribution header to every file under tools/quakespasm-port/platform/ (they are derivative works of GPLv2 Quakespasm), and clearly delineate that this subtree is GPLv2-licensed and separate from the repo's permissive code. The net_drivers/net_landrivers tables specifically should retain the net_bsd.c copyright notice.
- verify: Verified directly against the code. (1) The specific claim: pl_phoenix_stubs.c lines 31-92 (net_drivers[]/net_landrivers[] initializer tables + Q_COUNTOF sizing) are a byte-for-byte copy of external/quakespasm/Quake/net_bsd.c lines 31-96 — same Loopback+Datagram driver entries, same UDP landriver en

## [high/legal] tools-x11-other — tools/x11-port/ddx/fbdev.c:627
**GPL-2.0 Linux kernel keycode table (usb_kbd_keycode) copied verbatim into this BSD-licensed, publicly-published repo**

- detail: The 256-entry hidToEvdev[256] array (lines 627-644) is byte-for-byte identical to usb_kbd_keycode[256] in the Linux kernel's drivers/hid/usbhid/usbkbd.c, which is licensed GPL-2.0-or-later (Copyright (c) 1999-2001 Vojtech Pavlik; SPDX-License-Identifier: GPL-2.0-or-later; MODULE_LICENSE("GPL")). The in-code comment at lines 624-625 states this outright: "Verbatim from the kernel's drivers/hid/usbhid/usbkbd.c usb_kbd_keycode[256]". I confirmed the identity against the local Linux clone at external/linux/drivers/hid/usbhid/usbkbd.c (lines 35-52). Phoenix-RTOS is permissively (BSD-style) licensed; importing a GPL table verbatim into a repo about to be published publicly is a real license-incompatibility exposure. Note: the switch-based pl_hid_key() mappings in the two pl_phoenix_in.c files are independent reimplementations and are NOT affected — only this verbatim array in fbdev.c is the problem.
- fix: Replace the verbatim table with an independently-authored HID-usage->evdev/X-keycode mapping (a switch or a table built from the USB HID Usage Tables spec, which is the actual normative source, not the kernel's C array), or obtain explicit permission/relicensing from the rights holder. Remove the "Verbatim from the kernel" comment. Do not ship as-is.
- verify: Verified by direct comparison. The hidToEvdev[256] array at tools/x11-port/ddx/fbdev.c:627-644 is byte-for-byte identical to usb_kbd_keycode[256] at external/linux/drivers/hid/usbhid/usbkbd.c:35-52 — all 16 rows match exactly (checked every value). The Linux source header (line 1) is SPDX-License-Id

## [medium/correctness] kernel-hal-rest — hal/aarch64/arch/interrupts.h:30
**TIMER_WAKEUP_IRQ defined as an enum constant but consumed with #ifdef, so the entire SMP timer-wakeup IPI facility is silently compiled out**

- detail: interrupts.h defines `enum { TIMER_WAKEUP_IRQ = 1U };`. The sole consumer, proc/threads.c, gates every use behind `#ifdef TIMER_WAKEUP_IRQ` (lines 61, 214, 296, 2128, 2155). An enum constant is NOT a preprocessor macro, so `#ifdef TIMER_WAKEUP_IRQ` is ALWAYS false. As a result the whole facility the author wrote never compiles in on aarch64: (1) `_threads_updateWakeup` never sends the coalescing IPI to CPU0 (line 219) and instead falls through to `_threads_programWakeup` directly on secondaries; (2) `threads_wakeupintr` is never registered as the SGI-1 handler (lines 2156-2159). aarch64 is the only platform that defines TIMER_WAKEUP_IRQ, and it is the only one that gets it wrong (as an enum, not a #define). On a NUM_CPUS=4 build, secondary cores therefore reprogram the timer via the shared `timer_common.state` bookkeeping (hal_gtimerStateSetWakeup) instead of delegating the deadline recompute to CPU0 — exactly the design the coalescing IPI was written to avoid. The code does not do what it was written to do.
- fix: Change line 30 to a preprocessor macro so the #ifdef guards fire: `#define TIMER_WAKEUP_IRQ 1U` (keep the SGI-1 value). Then rebuild --scope core and confirm the wakeupHandler is registered and the IPI path is exercised on a 4-core boot. If the intent was actually to leave the facility disabled, delete the dead symbol and the #ifdef blocks instead — but the surrounding code clearly intends it enabled.
- verify: Confirmed from source. hal/aarch64/arch/interrupts.h:30 defines `enum { TIMER_WAKEUP_IRQ = 1U };` — an enum constant, NOT a preprocessor macro. Its sole consumer, proc/threads.c, gates every use behind `#ifdef TIMER_WAKEUP_IRQ` (lines 61, 214, 296, 2128, 2155). The C preprocessor has no knowledge of

## [medium/hack] kernel-hal-rest — hal/aarch64/generic/config.h:27
**NUM_CPUS comment claims the scheduler runs on cpu0 only with secondaries parked — stale, contradicted by _init.S and MEMORY**

- detail: The multi-line comment above NUM_CPUS states "the scheduler currently runs on cpu0 only; the secondaries are parked behind hal_smpPrimaryReady until a reliable wakeup path lands." This is stale (comment dates to 2026-06-06). Current HEAD _init.S `_other_core_virtual` (lines 816-842) has secondaries pass the hal_smpPrimaryReady gate, run _set_up_vbar_and_stacks / _hal_interruptsInitPerCPU / _hal_cpuInit / _hal_timerInitPerCPU, unmask DAIF (`msr daifClr, #7`), and take timer PPIs so they contribute to scheduling — and the project memory explicitly records 4-core SMP scheduling now works and that the old cpu0-only state must NOT be cited as current. Shipping this comment in a new file will mislead upstream reviewers about the actual SMP state.
- fix: Rewrite the comment to describe the real current behaviour: all four A72 cores are enumerated AND run the scheduler (secondaries arm CNTV in _hal_timerInitPerCPU and re-arm at SYSTICK cadence). Keep only the still-true firmware/armstub-release caveat.
- verify: The config.h comment (lines 30-36) states the scheduler "currently runs on cpu0 only; the secondaries are parked behind hal_smpPrimaryReady until a reliable wakeup path lands." This is contradicted by the code at the same HEAD (master):

1. _init.S _other_core_virtual (lines 799-842): secondaries sp

## [medium/hack] kernel-hal-rest — hal/aarch64/generic/generic.c:133
**hal_cpuReboot is a spin-halt stub (never reboots) with no TODO(TD-nn) marker or explanatory comment**

- detail: hal_cpuReboot() just loops on hal_cpuHalt() (WFI) forever; it does not reset the SoC. It is reached from hal_platformctl's pctl_reboot/pctl_set path (line 45). A userspace reboot request therefore silently hangs the machine with no diagnostic print. Every other platform (armv7r/zynqmp, armv7a/zynq7000, armv7r/tda4vm, ia32, ...) implements a real reset (watchdog/reset register). This project tracks intentional shortcuts with TODO(TD-nn): markers, but this stub has neither a TD marker nor a comment noting it is unimplemented, so it looks like a finished function.
- fix: Either implement a real BCM2711 reset (e.g. PM_WDOG/PM_RSTC watchdog poke, matching the firmware's reset path) or add an explicit TODO(TD-nn): marker + a one-line comment that reboot is unimplemented and the loop is a deliberate hang, and register the debt item in TEMPORARY-FIXES-AND-FUTURE-CLEANUP.md.
- verify: Verified against the actual source. In hal/aarch64/generic/generic.c, hal_cpuReboot (lines 141-146, not 133 as stated — minor line-number drift) is `__attribute__((noreturn)) void hal_cpuReboot(void) { for (;;) { hal_cpuHalt(); } }` — a WFI spin-halt with no SoC reset. It is reached from hal_platfor

## [medium/quality] kernel-hal-rest — hal/aarch64/generic/console.c:32
**hal_consolePrint routes ALL output through the hardcoded early-VADDR putch, not the DTB-probed pl011; the 'Early' name misleads on the primary print path**

- detail: hal_consolePrint (the path every debug() syscall and kernel print lands on) writes via _hal_consoleEarlyPrint -> _hal_consoleEarlyPutch, which hardcodes 0xffffffffffe00000 (UART data) and 0xffffffffffe00018 (flag reg). Meanwhile hal_consolePutch (the klog mirror) uses the properly-initialized hal_pl011Putch(&console_common.uart, ...) from dtb_getConsoleSerial. So the two console entry points reach the UART by two different mechanisms. On RPi4 the hardcoded literal equals board_config.h's PL011_TTY_EARLY_VADDR (0xffffffffffe00000) and its +0x18 flag offset, and the early mapping (DEVICES_TTL3[0], _init.S:477-481) persists for the kernel's lifetime, so there is no functional bug on this board — but (a) the '_hal_consoleEarly*' naming is misleading for the permanent primary output path, and (b) the raw literal duplicates PL011_TTY_EARLY_VADDR and will silently break if that board value changes.
- fix: Either make hal_consolePrint emit through hal_pl011Putch(&console_common.uart, ...) like hal_consolePutch (dropping the hardcoded early path once the UART is initialized), or at minimum replace the two magic literals with (PL011_TTY_EARLY_VADDR) and (PL011_TTY_EARLY_VADDR + UARTFR_OFFSET) and rename the helpers to drop 'Early'.
- verify: All factual claims in the finding are verified against the code. In /home/houp/phoenix-rpi/sources/phoenix-rtos-kernel/hal/aarch64/generic/console.c, hal_consolePrint (line 88, the path every debug() syscall and kernel print reaches per its own comment at 63-72) routes ALL output through _hal_consol

## [medium/hack] kernel-vm-proc — proc/msg.c:359
**Leftover devfs IPC-timing diagnostic in proc_send() (td14 trace) must be removed before publication**

- detail: proc_send() carries a full diagnostic apparatus with NO TODO(TD-NN) marker: the msg_common.td14_lookupTrace counter, a 160-byte on-stack traceBuff, three time_t timestamps, traceLookupDevfs/sawReceived flags, the mtLookup=="devfs" sniff at the top, the mid-loop timestamp capture, and a hal_consolePrint of "td14: send devfs port=... total=... queued=... err=... state=..." for the first 16 devfs lookups. It also pulls in a new #include "hal/timer.h" purely for hal_timerGetUs(). This is disproved/closed investigation scaffolding (the TD-14 devfs race is now handled by the fast path in name.c) sitting in a hot IPC path and printing to the console on early boot. It is exactly the kind of diagnostic-only code the project says to strip before closing a step.
- fix: Delete all td14 tracing from proc_send() and the td14_lookupTrace field from msg_common; drop the now-unused #include "hal/timer.h". Keep only the genuine cleanup (int state = msg_rejected initialization is harmless to retain).
- verify: Every concrete claim in the finding is verified against the actual code at /home/houp/phoenix-rpi/sources/phoenix-rtos-kernel/proc/msg.c:

- Line 17: `#include "hal/timer.h"` — grep confirms hal_timerGetUs/hal/timer.h appear ONLY inside the td14 tracing; the include exists solely for the probe.
- Li

## [medium/hack] kernel-vm-proc — proc/threads.c:237
**Write-only SMP tick counter threads_smpTickCount adds an atomic to every timer ISR but is never read**

- detail: The volatile threads_smpTickCount[8] global is atomically incremented (hal_cpuAtomicInc) in threads_timeintr() on every timer tick on every CPU, but grep shows it is never read anywhere. The comment claims "Primary's main_initthr prints a snapshot after the syspage spawn loop" — no such reader exists, so this is dead Phase-D diagnostic code that also imposes an atomic RMW on the hot per-tick interrupt path of all cores for no observable output. The comment itself says it "can be removed entirely" once Phase D is closed.
- fix: Remove threads_smpTickCount, the myCpuId bounds check, and the hal_cpuAtomicInc from threads_timeintr(); keep myCpuId only if still used by the `myCpuId != 0U` scheduler check (it is — retain that single hal_cpuGetID()).
- verify: Verified directly against the code. In /home/houp/phoenix-rpi/sources/phoenix-rtos-kernel/proc/threads.c, `volatile unsigned int threads_smpTickCount[8]` (line 237) is written only in threads_timeintr(): a bounds check (line 251) and hal_cpuAtomicInc(&threads_smpTickCount[myCpuId]) (line 252), guard

## [medium/quality] kernel-vm-proc — proc/name.c:79
**Misleading dead name_trace* scaffolding: two no-op functions plus load-bearing predicates wearing 'trace' names**

- detail: name.c introduces a name_trace* layer that is half dead code, half misnamed live code. name_traceRegister() and name_traceDevfs() are pure no-ops ((void)arg; return;) called at several sites — dead scaffolding. name_traceDevfsLookup() and name_traceIs() are NOT diagnostics at all: they are the load-bearing predicates that gate the TD-14 devfs fast path (the comment on name_traceDevfsLookup explicitly warns that returning 0 breaks the fast path). Naming a live control predicate `name_trace...` is actively misleading and will confuse future readers/upstreamers. (The devfs fast path itself is TODO(TD-14-devfs-direct)-marked debt and returns devfsOid for both file and dev, which matches the existing dcache-hit path — that part is acceptable and correct.)
- fix: Delete name_traceRegister() and name_traceDevfs() and their call sites; rename name_traceDevfsLookup() to name_isDevfs() (and inline/rename name_traceIs accordingly). Remove the traceDevfs branch prints in proc_portLookup that only call the no-op name_traceDevfs().
- verify: All four claims verified directly against sources/phoenix-rtos-kernel/proc/name.c. (1) name_traceRegister() (lines 85-88) and name_traceDevfs() (lines 106-109) are pure no-ops — bodies are `(void)arg;` with no effect — yet are called at multiple live sites (131, 171 for Register; 273, 287, 364, 397,

## [medium/hack] plo — hal/aarch64/generic/_init.S:594
**Entire generic (rpi4) exception vector table is leftover diagnostic scaffolding shipping in a public release**

- detail: The generic target's _vector_table (line 723) wires 15 of 16 slots to `exc_tag` (print a single tag char then `b .` halt) and slot 0x200 to `_slot_e_dump` (dump ESR/ELR/FAR/etc then halt). There is NO real exception/IRQ dispatch: the file `#include`s ../_interrupts.S and ../_exceptions.S (providing `_exceptions_dispatch`), but unlike the zynqmp target — whose vector table branches slots 0x200/0x280/... to `_exceptions_dispatch` — none of the generic slots ever reach it. The code self-labels 'TD-diag' (line 582, not a tracked TD-NN debt marker) and `_slot_e_dump`'s comments are disproved-hypothesis debug text ('If these match... the I-cache is NOT the culprit', 'matches the SCTLR_EL2.SA=1 ... hypothesis'). This is diagnostic bring-up code that was never removed. It is not a live functional bug because DAIF stays masked from the start_el2 ERET onward and the generic target registers no interrupt handlers (timer is polled via cntpct, only device is polled ram-storage), so no vector ever executes at runtime — but it is exactly the leftover-probe class the project says to remove before closing a step, and it ships publicly.
- fix: Replace the diagnostic vector table + `_slot_e_dump` + `uart_put_hex64`/`uart_put_hexnibble`/`exc_tag` helpers with a real vector table that branches sync/IRQ/SError slots to the shared `_exceptions_dispatch` (mirroring hal/aarch64/zynqmp/_init.S). If any minimal 'halt on fault' behavior is intentionally desired for a bootloader, keep a single small handler with a real TD-NN marker and a short comment, and delete the disproved-hypothesis dump code.
- verify: All substantive claims verify against the code at /home/houp/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S:

1. The generic (rpi4) _vector_table (line 620) wires 15 of 16 slots to `exc_tag` (macro at 490: prints a tag char then `b .` halt) and slot 0x200 to `_slot_e_dump` (line 542: dumps ESR/

## [medium/correctness] dev-tty — tty/usbkbd/usbkbd.c:860
**Insertion error path leaks the fifo buffer and two kernel handles (lock+cond)**

- detail: On a failed insertion, usbkbd_handleInsertion does `_usbkbd_put(dev)` (rfcnt 1->0, which only calls idtree_remove) and then `free(dev)`. But _usbkbd_devAlloc already allocated dev->fifo (malloc), dev->lock (mutexCreate) and dev->cond (condCreate). The error path frees only the struct, leaking the fifo buffer plus two kernel handles per failed insertion. Repeated flaky-insertion cycles can exhaust handles. The deletion path uses the correct idiom (`if (_usbkbd_put(dev) == 0) usbkbd_free(dev);`).
- fix: Replace the error-path `_usbkbd_put(dev)` + trailing `free(dev)` with `if (_usbkbd_put(dev) == 0) { usbkbd_free(dev); }` and drop the separate `free(dev)`. usbkbd_free's remove(dev->path) is harmless when path is unset.
- verify: The defect is real and confirmed in sources/phoenix-rtos-devices/tty/usbkbd/usbkbd.c. _usbkbd_devAlloc (line 752) allocates dev->fifo via malloc (767), dev->lock via mutexCreate (776), dev->cond via condCreate (783), and sets rfcnt=1 (795). In usbkbd_handleInsertion's error path (lines 860-868), on 

## [medium/correctness] dev-tty — tty/usbmouse/usbmouse.c:613
**Insertion error path leaks the fifo buffer and two kernel handles (lock+cond)**

- detail: Identical bug to the usbkbd twin: on failed insertion usbmouse_handleInsertion does `_usbmouse_put(dev)` (which only idtree_remove's) then `free(dev)`, leaking dev->fifo, dev->lock and dev->cond allocated by _usbmouse_devAlloc.
- fix: Use `if (_usbmouse_put(dev) == 0) { usbmouse_free(dev); }` and remove the trailing `free(dev)`, matching usbmouse_handleDeletion.
- verify: Confirmed by tracing the code in /home/houp/phoenix-rpi/sources/phoenix-rtos-devices/tty/usbmouse/usbmouse.c. In usbmouse_handleInsertion, dev->rfcnt is 1 (set in _usbmouse_devAlloc line 548, never incremented in the insertion path). On any failure (usb_open ctrl/int-in, setConfiguration, setProtoco

## [medium/correctness] dev-storage — storage/bcm2711-emmc/sdcard.c:212
**Stale comments claim SDMA/DMA is disabled while SDCARD_ENABLE_DMA is actually defined and the DMA read path is live**

- detail: SDCARD_ENABLE_DMA is #define'd to 1 at line 50 and the SDMA read path is the active, HW-validated path (sdhost_allocDMA sets host->useDma when the buffer lands low; _sdcard_transferBlocks uses DMA for reads). But two comment blocks in the same file state the opposite: line 187 says "The staging buffer is moved by PIO (SDMA is disabled, see _sdio_cmdSend)", and lines 212-215 say "SDCARD_ENABLE_DMA is currently UNDEFINED: ... so DMA is gated OFF and PIO is the trusted path". These are leftovers from an earlier iteration where DMA was off. In code that is about to be published, this tells a maintainer the exact inverse of the runtime behavior on the DMA path and directly contradicts the line-50 comment. Not a runtime bug, but actively misleading.
- fix: Rewrite the line-187 and line-212-215 comments to match reality: SDCARD_ENABLE_DMA is defined, SDMA is the read data path (reads only; writes use PIO), and the useDma gate depends on the staging buffer being DMA-reachable (< 1 GiB).
- verify: Confirmed from the code. sdcard.c:50 defines `#define SDCARD_ENABLE_DMA 1` with a comment "SDMA read data path (HW-validated; reads only — writes use PIO)". The DMA path is genuinely live: line 216-217 `#ifdef SDCARD_ENABLE_DMA` sets `host->useDma = (dmaBufferPhys < 0x40000000ul)`; line 1980 `useDma

## [medium/duplication] dev-usb-pcie — pcie/server/pcie.c:172
**pcie.c gains a ~600-line BCM2711 bring-up block that is a dead, older, buggier duplicate of usb/xhci/bcm2711-pcie.c**

- detail: The whole PCI_EXPRESS_BCM2711_INDEXED_CFG block added to pcie.c (cfg backend, bcm2711PrepareHostBridge/LinkState, SetOutboundWindow0, SetRcBar2, ShapeRootBridge, ExposeDownstreamBridge, scanBus, main warm-up) is a near-verbatim copy of usb/xhci/bcm2711-pcie.c. But the standalone `pcie` daemon is NOT spawned on the only aarch64a72 target: _targets/Makefile.aarch64a72-generic lines 65-69 explicitly exclude it, and Pi4 does the bring-up in-process via libusbxhci's bcm2711_pcie_initVL805(). So this code is unreachable on the only board that defines the macro. Worse, it is the pre-fix copy: it lacks every inbound-DMA fix the live path required (UBUS_BAR2_ACCESS_EN enable, BAR1/BAR3 disable, endian VSR1, MISC_CTRL RCB/SCB0 sizing) and carries the BAR2-size truncation bug (see separate finding). Publishing two divergent copies of the same bridge bring-up, one silently broken, is a maintenance and correctness liability. Since Pi4 no longer uses it, the BCM2711 additions to pcie.c should be dropped, keeping pcie.c's generic ECAM path intact for other boards.
- fix: Remove the PCI_EXPRESS_BCM2711_INDEXED_CFG additions from pcie/server/pcie.c (and the pcie/server/Makefile -DPCI_EXPRESS_BCM2711_INDEXED_CFG hook) since no target builds the standalone pcie daemon with that macro; the single source of truth is usb/xhci/bcm2711-pcie.c.
- verify: All claims in the finding are verified against the code.

DUPLICATE + DEAD: pcie/server/pcie.c contains the full BCM2711 bring-up block guarded by PCI_EXPRESS_BCM2711_INDEXED_CFG (macro checks at lines 132, 203, 759, 996) with the same function set as usb/xhci/bcm2711-pcie.c: bcm2711PrepareHostBridg

## [medium/correctness] dev-usb-pcie — pcie/server/pcie.c:452
**bcm2711EncodeBar2Size in pcie.c truncates a 4 GiB window to 1 MB (the bug USB-FIX-12 fixed in bcm2711-pcie.c)**

- detail: pcie.c's bcm2711EncodeBar2Size starts shift=20 and counts trailing right-shifts, so for size=4 GiB (2^32) it returns shift-15 = 52-15 = 37; with BCM2711_PCIE_RC_BAR2_SIZE_MASK=0x1f (5 bits) that truncates to 5, which the bridge decodes as a 1 MB inbound window. VL805 DMA targets above 1 MB then have no valid PCIe-side destination and the transfer fails at the bridge. bcm2711-pcie.c's copy was fixed (starts shift=0, computes true log2, comment USB-FIX-12) but pcie.c's was not. Also pcie.c's bcm2711SetRcBar2 uses the pre-USB-FIX-12b address mask 0xfffffff0 (clobbers size bit 4) instead of the corrected 0xfffff000. Latent only because pcie.c is not spawned on Pi4, but it is a real bug should anyone re-enable the daemon.
- fix: If the pcie.c BCM2711 block is not removed per the duplication finding, port the USB-FIX-12/12b corrections (shift-from-0 log2 encode; 0xfffff000 address mask) from usb/xhci/bcm2711-pcie.c.
- verify: Verified all three sub-claims by direct code comparison of the two files.

1. pcie/server/pcie.c:478-493 bcm2711EncodeBar2Size starts `shift = 20` and counts trailing right-shifts (`while ((value > 1u) && ((value & 1u) == 0u))`). For size = 4 GiB = 2^32, value is right-shifted 32 times to reach 1, g

## [medium/hack] dev-usb-pcie — pcie/server/pcie.c:226
**Standalone pcie.c is saturated with diagnostic scaffolding: unbounded mailbox busy-waits, per-BAR/per-device debug() dumps, a diag-outbound MMIO probe, and a 30-iteration main() warm-up loop**

- detail: The added pcie.c code is bring-up scaffolding not fit for publication: bcm2711NotifyXhciReset busy-waits on the mailbox FULL/EMPTY bits with NO timeout (an unresponsive VideoCore firmware hangs the process forever); scanFunc/print_bars are peppered with inline `extern void debug(); char m[..]; snprintf(); debug()` register dumps; there is a diag-outbound mmap+read probe block; and main() runs a 30x100ms 'VL805 warm-up' read loop before exit. The file itself carries `TODO: remove this diagnostic include once VL805 BAR-programming is proven stable` over `#include <sys/debug.h>`. These are disproved-hypothesis / bring-up diagnostics the project's publication mandate says to remove.
- fix: Remove the diagnostic debug() dump blocks, the diag-outbound probe, the main() warm-up loop and the sys/debug.h include; if any mailbox wait is retained, bound it with a timeout instead of an infinite busy-wait.
- verify: Every specific claim in the finding is verified verbatim against /home/houp/phoenix-rpi/sources/phoenix-rtos-devices/pcie/server/pcie.c. (1) bcm2711NotifyXhciReset (starting ~line 219, debug-enter marker at 227-228 adjacent to the cited line 226) contains two unbounded busy-waits with NO timeout: li

## [medium/correctness] dev-video-gpio-audio-sensors — audio/rpi4-audio/rpi4-audio.c:133
**DRAM_BUS() silently masks the DMA source/CB address to the low 1 GB with no runtime bounds check, contradicting the comment that claims one exists**

- detail: The legacy-DMA 0xC0000000 alias only reaches physical addresses below 1 GB. DRAM_BUS(pa) = 0xc0000000u | ((uint32_t)(pa) & 0x3fffffffu) simply masks off any bits >= bit 30. The ring and control block are allocated via mmap(MAP_CONTIGUOUS | MAP_ANONYMOUS) (lines 360-363), and nothing constrains that allocation to physical < 0x40000000 on a 2/4/8 GB Pi 4. If ring_pa or cb_pa lands at or above 1 GB, DRAM_BUS wraps and the DMA engine reads a *different* physical DRAM region (the low-1GB alias of a truncated address) rather than the ring, feeding garbage duty words to the PWM FIFO — audible corruption or a wedged/erroring channel. Worse, the comment on line 115 explicitly claims the address is '(logged + checked at runtime)', but the only runtime treatment is the printf log on lines 399-400 (ring_pa is printed); there is no check anywhere that aborts or falls back to PIO when ring_pa/cb_pa >= 0x40000000. The comment is therefore false and masks the missing guard.
- fix: After computing ring_pa and cb_pa in audio_dmaStart(), verify both are addressable via the alias: if (((ring_pa | cb_pa) >> 30) != 0) { munmap the two buffers, leave ad.dma_active = 0 (PIO fallback), and log the out-of-range PA }. Then correct the line-115 comment to say the check actually exists (or drop the 'checked at runtime' claim).
- verify: The defect is real at the code level and the false-comment claim is definitively true.

CODE FACTS (verified):
- rpi4-audio.c:133 `DRAM_BUS(pa) = (0xc0000000u | ((uint32_t)(pa) & 0x3fffffffu))` masks off all bits >= bit 30, so any physical address >= 0x40000000 wraps to a low-1GB alias.
- Both DMA a

## [medium/hack] lwip — port/mbox.c:142
**Leftover 16-word raw-memory diagnostic dump (hypothesis-hunting for #121/#129) ships in a to-be-published repo**

- detail: The bounds guard added to mbox_tryfetch (checking ring==NULL / sz==0 / head>=sz before dereferencing) is a defensible survive-not-crash guard. But the block at lines 142-166 additionally prints the victim struct's physical address via va2pa() and dumps 16 u64 words around the struct with debug(), purely to localise an unconfirmed heap/DMA-overrun writer (#121/#129). #121 is documented as UNCONFIRMED (not resolved), and CLAUDE.md requires stripping diagnostic-only code whose hypothesis is unresolved before public publication. This is diagnostic hunting code, not a fix.
- fix: Keep the bounds guard (lines 135-141,167-169) but remove the PA-print + 16-word dump loop (lines 142-166), or gate the whole dump behind an explicit debug macro (e.g. #if LWIP_MBOX_CORRUPT_DEBUG) that is off by default.
- verify: Verified directly in /home/houp/phoenix-rpi/sources/phoenix-rtos-lwip/port/mbox.c (committed at HEAD dffa814, no pending edits, so it ships). Lines 142-166 in mbox_tryfetch contain exactly what the finding describes: a static corruptCount counter, a va2pa()-based PA/ringpa print via debug() (lines 1

## [medium/correctness] libphoenix — wchar/wchar.c:94
**wcstombs silently truncates wide chars > 255 instead of returning (size_t)-1**

- detail: The new wcstombs() does `s[i] = (char)pwcs[i]` with no range check, so a wchar_t outside [0,255] is silently truncated to a low byte and the function still succeeds. Per C/POSIX, a wide character that cannot be represented in the current locale must make wcstombs return (size_t)-1 (and set errno EILSEQ). This is both a spec deviation and an internal inconsistency: the sibling wcsrtombs() in the very same file, and wcrtomb() here, and wctomb() in posix/stubs.c, all correctly reject values > 0xff. The NULL-buffer counting path in wcstombs likewise omits the check that wcsrtombs performs. Data loss is silent for callers that pass non-Latin-1 wchar_t.
- fix: In both the NULL-dst counting loop and the copy loop, add `if ((unsigned long)pwcs[i] > 0xffUL) { errno = EILSEQ; return (size_t)-1; }` before storing, matching wcsrtombs() in the same file.
- verify: Verified directly in /home/houp/phoenix-rpi/sources/libphoenix/wchar/wchar.c. wcstombs (lines 94-112) stores s[i] = (char)pwcs[i] at line 108 with no range check, and its NULL-buffer counting path (lines 99-101) also omits any check. A wchar_t outside [0,255] is silently truncated to a low byte and 

## [medium/correctness] usb-stack — usb/hub.c:364
**Per-port give-up counter never clears for a device that never enumerated, permanently disabling the port after replug**

- detail: hub_connectstatus clears portEnumFails only inside `if (hub->devs[port-1] != NULL)` (line 351-354). But a device that fails enumeration is disconnected via hub_devConnected without ever being recorded in hub->devs (devs[port-1] stays NULL). Once such a device hits HUB_ENUM_GIVEUP consecutive failures, unplugging it does NOT clear the counter (devs[port-1] is NULL, so the clear branch is skipped), and the early `if (portEnumFails >= HUB_ENUM_GIVEUP) return;` fires before hub_portDebounce, so the disconnect is never even observed. Physically re-plugging any device (even a good one) on that port is then permanently ignored. This directly contradicts the code comment's promise 'a disconnect clears the counter'. The counter's primary purpose (stopping the enum reboot-loop) still works; only replug recovery is broken.
- fix: Detect physical disconnect independently of hub->devs tracking: before the give-up early-return, run/inspect the port connection bit (or move the counter clear to trigger on any transition to disconnected). E.g. only apply the give-up return while the port still reports CONNECTION, and reset portEnumFails[port-1]=0 whenever the port reads as disconnected — regardless of whether devs[port-1] was ever set.
- verify: Traced the concrete failure path in /home/houp/phoenix-rpi/sources/phoenix-rtos-usb/usb/hub.c and confirmed the defect.

Failure path:
1. A device that repeatedly fails enumeration is disconnected via hub_devConnected (line 335, hub_devDisconnect) and is NEVER recorded in hub->devs[port-1] — the els

## [medium/hack] usb-stack — usb/mem.c:24
**~150 lines of pure diagnostic reporting apparatus for #121 (alloc/free rings, hex dumps, caller PC) still present in a pre-publication review**

- detail: The project rule is to remove diagnostic-only code once its hypothesis is settled before closing a step. mem.c mixes two things: defensive guards (usb_chunkSane, usb_bufSane, leak-don't-crash paths) that are legitimate hardening, and pure reporting apparatus — struct usb_freelog_ent/usb_alloclog_ent, the freelog/alloclog rings, freelogPos/alloclogPos, usb_freelogRecord/usb_alloclogRecord, usb_memReportCorrupt with its hex+ASCII dumps and __builtin_return_address(0) capture. That reporting apparatus is diagnostic-only and is being published. Notably the dev.c ctrlBuf-sizing fix in this same diff resolves one of the concrete overflow hypotheses (config descriptor within 32 bytes of USBDEV_BUF_SIZE) the apparatus was built to chase, so at least part of the investigation it supports is now closed.
- fix: Before publication, strip the reporting rings and usb_memReportCorrupt (and their call sites/records), keeping only the usb_chunkSane/usb_bufSane bounds guards and the log-and-leak recovery if that hardening is intended to stay. Or gate the whole apparatus behind a build-time diagnostic flag.
- verify: Verified directly against the committed code at HEAD (usb 12c4fe8) in /home/houp/phoenix-rpi/sources/phoenix-rtos-usb/usb/mem.c. The finding's inventory is accurate and present: struct usb_freelog_ent (L47-50), struct usb_alloclog_ent with caller PC (L59-63), the freelog/alloclog rings + freelogPos/

## [medium/quality] usb-stack — usb/mem.c:211
**Unconditional aarch64 `dc civac` inline asm + hardcoded 64-byte line size injected into a shared cross-arch USB file**

- detail: usb/mem.c is generic upstream Phoenix USB code built for multiple targets (ia32, arm-imx, etc.). The added cache-eviction block in usb_allocUncached uses raw `dc civac`/`dsb sy` aarch64 instructions and a hardcoded 64-byte cache-line stride. This will not compile on non-aarch64 targets and hurts upstreamability. Architecturally the maintenance itself is sound (PIPT D-cache on the A72 evicts by PA regardless of the uncached VA mapping), so this is not a correctness bug — it is a portability/layering issue.
- fix: Guard the eviction behind an arch/target #ifdef (or a HAL/platform cache-clean helper), and use a cache-line-size constant from the platform rather than a hardcoded 64. Keep the generic file free of arch-specific asm.
- verify: Verified directly in /home/houp/phoenix-rpi/sources/phoenix-rtos-usb/usb/mem.c. Lines 211-218 (usb_allocUncached) contain an unconditional aarch64 inline-asm cache-eviction loop: `uintptr_t a = (uintptr_t)res & ~63UL; ... a += 64UL ... __asm__ volatile("dc civac, %0"...); __asm__ volatile("dsb sy"..

## [medium/correctness] usb-stack — usb/hub.c:508
**hub_conf port-power error path frees hub->devs but not the just-allocated hub->portEnumFails**

- detail: In the `hub_setPortPower` failure branch (hub.c:506-510) the code `free(hub->devs); return -EINVAL;` without freeing hub->portEnumFails, which was allocated a few lines above at line 499. Also neither pointer is NULLed, so a later usb_devFree could double-free devs. This is an error path so low severity, but inconsistent with the ENOMEM path above it which does clean up.
- fix: Free (and NULL) both hub->devs and hub->portEnumFails on this error path, matching the ENOMEM handling; ideally NULL them so teardown does not double-free.
- verify: Verified against sources/phoenix-rtos-usb/usb/hub.c and dev.c.

At hub.c:505-510, the hub_setPortPower failure branch does `free(hub->devs); return -EINVAL;`. It does NOT free hub->portEnumFails (calloc'd at line 499) — confirmed leak. It also does not NULL hub->devs. `hub->devs` is assigned only on

## [medium/correctness] corelibs-posixsrv — special.c:103
**Shared static random_hwrngFd is read/written by concurrent worker threads without a lock**

- detail: posixsrv runs a pool of worker threads that all dispatch random_read_op on srvPort concurrently (srv.c:56-57 spawns multiple posixsrv_threadMain, and posixsrv.c:297 calls the read handler with no lock held). random_hwrngFd is a plain static mutated by these threads with no synchronization. Two hazards: (1) the lazy-init check-then-act (if fd == -2 { open; print; }) can run in two threads at once, leaking one fd and printing the banner twice; (2) on the hwrng runtime-failure path a thread does close(random_hwrngFd) and sets it to -1 while another thread is mid read(random_hwrngFd, ...), closing the fd underneath it — and if a subsequent open() reuses that descriptor number, the in-flight reader reads from an unrelated fd. In the normal steady state (hwrng present and delivering) the close path never runs, so this is a latent race rather than a bug that fires in normal operation, hence medium.
- fix: Guard the init and close/-1 transitions with a mutex (posixsrv_common.lock already exists), or open /dev/hwrng once in posixsrv_init before the worker threads are spawned. The lazy-open rationale (hwrng node registers after posixsrv starts) argues for the mutex as the minimal fix rather than moving the open to startup.
- verify: The finding is confirmed against the code.

Concurrency model (verified): srv.c:28 declares stacks[4]; srv.c:54 spawns thread 0 on eventPort; srv.c:56-58 spawns threads 1,2,3 on srvPort — 3 concurrent workers dispatching /dev/urandom reads. In posixsrv.c:71-96 posixsrv_object_get takes posixsrv_comm

## [medium/correctness] tools-v3d — tools/v3d-driver-port/gl_stubs.c:123
**posix_memalign ignores the requested alignment and returns plain malloc()**

- detail: posix_memalign() discards `alignment` entirely and returns malloc(size), which Phoenix only guarantees 16-byte aligned. The comment admits this. Any caller requesting a larger alignment (SIMD cache-line 64, page alignment, etc.) silently receives under-aligned memory. Mesa's own os_memory_aligned.h was patched to `#undef HAVE_POSIX_MEMALIGN` precisely because this stub corrupts util_sparse_array (NODE_ALLOC_ALIGN=64, tree level packed in the low 6 bits) -> so the primary consumer is worked around, but the broken symbol is still exported and callable by the harness or any future/other consumer, where it will silently corrupt. A stub that claims to honor alignment but doesn't is a latent landmine before public release.
- fix: Implement a real aligned allocator (over-allocate + store the original malloc pointer just before the aligned block, with a matching aligned_free), or if free()-compatibility is required use Phoenix's native aligned-alloc primitive. At minimum, if alignment > 16, do not silently succeed with a mis-aligned pointer — allocate `size + alignment` and align, storing a back-pointer.
- verify: Verified directly in /home/houp/phoenix-rpi/tools/v3d-driver-port/gl_stubs.c:123-135. posix_memalign() only floors `alignment` to sizeof(void*) and then never uses it — it returns plain malloc(size) and sets *memptr = p. The inline comment admits it ("Phoenix malloc returns 16-byte-aligned, enough f

## [medium/correctness] ext-mesa-v3d — src/gallium/drivers/v3d/v3d_resource.c:909
**Size-only heuristic (>=1024x768) forces RASTER + SCANOUT-backing + Y-flip on ANY render target, including large sampled RTTs**

- detail: The port introduces a pure size heuristic `(bind & PIPE_BIND_RENDER_TARGET) && width0>=1024 && height0>=768` as a stand-in for 'this is the single full-screen display FBO'. This same gate is copy-pasted in three coupled sites, all keyed to the same magic numbers: (1) v3d_resource_create_with_modifiers ~line 909 forces should_tile=false (linear); (2) v3d_resource_bo_alloc ~line 143 sets V3D_CREATE_BO_SCANOUT so the winsys backs the BO with the fixed HDMI framebuffer pages; (3) src/mesa/state_tracker/st_atom_framebuffer.c ~line 137 forces Y_0_TOP. The gate does NOT exclude PIPE_BIND_SAMPLER_VIEW, which directly contradicts the code's own comment ('small SAMPLED render targets ... stay tiled'). Any general-GL app that creates a large offscreen render-to-texture, shadow map, or post-processing target that is >=1024x768 and bound as a render target will be forced linear, marked SCANOUT (consequence: it is backed by the single fixed display framebuffer PA, so multiple such targets alias each other and clobber the display), and rendered upside-down by the forced Y_0_TOP. This does NOT affect the proven GLQuake path (its intermediate RTs are small), so it does not block the shipped use case, but it is a latent correctness bug for the driver as a general GL implementation.
- fix: Replace the three duplicated size heuristics with a single explicit predicate that identifies the display FBO by intent, not size — e.g. gate additionally on !(bind & PIPE_BIND_SAMPLER_VIEW) and on the resource being the winsys/display target (PIPE_BIND_DISPLAY_TARGET/SCANOUT or a winsys flag), and factor it into one shared helper so the three sites cannot drift. At minimum, exclude SAMPLER_VIEW-bound resources to match the stated intent.
- verify: All three claimed sites exist and match the finding exactly:

1. v3d_resource.c:909-911 (v3d_resource_create_with_modifiers) forces should_tile=false when `(tmpl->bind & PIPE_BIND_RENDER_TARGET) && tmpl->width0 >= 1024 && tmpl->height0 >= 768`.
2. v3d_resource.c:142-143 (v3d_resource_bo_alloc) sets 

## [medium/correctness] ext-quakespasm — Quake/gl_screen.c:982
**Divide-by-zero in SCR_CaptureTick when scr_capture is a fractional value in (0,1)**

- detail: scr_capture is a float cvar. The guard `scr_capture.value <= 0` admits any positive value, including fractions like 0.5. The subsequent modulo `frames++ % (int)scr_capture.value` truncates such a value to 0, producing an integer modulo by zero. On aarch64 this is silently defined (SDIV-by-0), but the harness's whole purpose is Pi-vs-host comparison and the host is an x86 build where integer divide-by-zero raises SIGFPE and crashes the process. Any user (or capture script) that sets `scr_capture 0.5` on the host reference run crashes it.
- fix: Truncate once and gate on the integer step: `int step = (int)scr_capture.value; if (step < 1 || !cls.demoplayback || cls.signon != SIGNONS) return;` then `if ((frames++ % step) != 0) return;`.
- verify: The code at /home/houp/phoenix-rpi/external/quakespasm/Quake/gl_screen.c:974-993 matches the finding exactly. scr_capture is a cvar_t whose .value is a float (line 114), registered as a normal user-settable cvar (line 429). SCR_CaptureTick guards only on `scr_capture.value <= 0` (line 979), which ad
