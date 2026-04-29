# Temporary Fixes and Future Cleanup

This document is the registry of transitional shortcuts and workarounds
accepted during the Raspberry Pi 4 bring-up. Each item has a stable ID
(`TD-NN`) used to link from source code, commits, and future cleanup steps.

Ordering rule: once the Pi 4 boots to a usable state, every item here becomes
mandatory cleanup. Until then, progress on the boot path takes priority.

## Conventions

- **IDs are stable.** Never re-number. If an item is merged into another,
  add a `merged into TD-NN` note rather than deleting the entry.
- **Status** is one of:
  - `PENDING` — shortcut still active in source
  - `IN-PROGRESS` — cleanup step open against it
  - `RESOLVED` — cleanup committed and validated, record kept as history
- **Linking from source.** Every transitional fix in upstream source should
  carry an inline marker: `TODO(TD-NN): <short hint>`. Grep for `TD-NN` to
  find all sites of a given shortcut.
- **Location snapshots may drift.** Line numbers in this file reflect state
  at the time the entry was written. Re-verify against current source before
  acting — the code changes faster than this doc.

---

## TD-01: SMP enable disabled on Cortex-A72

- **Status:** PENDING
- **First observed:** 2026-04 bring-up
- **Where:** `sources/phoenix-rtos-kernel/hal/aarch64/_init.S`, the
  `CPUECTLR_EL1` SMPEN block behind `__TARGET_AARCH64A72`.
- **What was done:** The SMP-enable MSR sequence is commented out and the
  only remaining effect is the debug markers around it.
- **Why:** Enabling SMP on A72 produced an early-boot hang; cause not
  diagnosed yet.
- **Risk accepted:** A72 coherency behavior with Inner-Shareable memory is
  undefined without this bit. Current code avoids Inner-Shareable in early
  boot, which is itself a related transitional compromise.
- **Resolution requirements:**
  - Reproduce the hang with a bounded diagnostic (GDB over QEMU gdbstub, or
    one minimal marker pair) and capture the fault.
  - Follow the Cortex-A72 TRM SMP enable sequence; compare against Circle OS
    and similar bare-metal references.
  - Re-enable SMP, then switch early boot back to Inner-Shareable and
    confirm boot on real hardware across multiple cold resets.

## TD-02: Pre-MMU cache invalidation disabled

- **Status:** PENDING
- **First observed:** 2026-04 bring-up
- **Where:** `sources/phoenix-rtos-kernel/hal/aarch64/_init.S`, the
  `PMAP_COMMON_KERNEL_TTL2 … PMAP_COMMON_STACK` `_inval_dcache_range` call
  before MMU enable.
- **What was done:** The pre-MMU data-cache invalidation sweep is commented
  out. The code now relies solely on the post-MMU-enable invalidation and
  on `dsb ish; isb` to make table writes visible.
- **Why:** Cache maintenance with the MMU disabled hung the board in
  observed runs. Linux arm64 performs this sweep unconditionally.
- **Risk accepted:** Speculatively loaded stale lines for the page-table
  regions can survive into early MMU walks. So far no observed corruption,
  but that is not a guarantee.
- **Resolution requirements:**
  - Identify the A72-specific precondition that makes the generic sequence
    hang (likely ordering or an earlier missing setup step).
  - Restore the invalidation, or document the exact reason a narrower form
    is correct for this platform.

## TD-03: Syspage copy / BSS mapping shortcut

- **Status:** PENDING
- **First observed:** 2026-04 bring-up
- **Where:** Interaction between `hal/aarch64/_init.S` (virtual syspage
  copy) and `syspage.c` (syspage access after MMU enable). BSS region is
  not reliably mapped in the early MMU page tables.
- **What was done:** Per `docs/status.md`, syspage access was stabilized by
  side-stepping the copied-into-BSS location and working with the original
  syspage. Intent and current source may diverge: **verify before acting.**
- **Why:** The early MMU page tables did not cover the BSS region into
  which the syspage was being copied.
- **Risk accepted:** Any code path that assumes the copied virtual syspage
  is authoritative may read stale data or wrong addresses.
- **Resolution requirements:**
  - Extend early MMU setup to map the BSS region (or move the syspage copy
    target to an already-mapped region).
  - Re-enable the canonical syspage copy path and validate that every
    consumer reads from the virtual location.
  - Add a syspage integrity check (size and a simple checksum) to the
    post-copy path.

## TD-04: BCM2711-specific syspage corruption at the plo→kernel handoff

- **Status:** PENDING (active step — see `tracking/current-step.md`)
- **First observed:** 2026-04 bring-up. Originally tracked under several
  narrower descriptions: "iter-8 hang in `syspage_init` entry sub-loop",
  "non-deterministic post-MMU markers", "circular-list relocation
  divergence". 2026-04-29 reframed all of those as one underlying
  cache-coherency / boot-handoff anomaly that only manifests on real
  BCM2711 silicon.
- **Where:** `sources/phoenix-rtos-kernel/hal/aarch64/_init.S` (the syspage
  copy loop and surrounding cache maintenance) and
  `sources/phoenix-rtos-kernel/syspage.c` (the C-level relocation loops
  that read the copied data). The shared aarch64 kernel handoff code,
  which works correctly on ZynqMP and on QEMU 10.2.2 raspi4b.
- **Verified facts (2026-04-29 E2 probe):**
  - Kernel reads source bytes correctly from plo's heap PA at every
    offset checked (0, 0x100, 0x200, 0x310). plo and kernel agree on
    source contents.
  - Kernel reads destination bytes correctly at offsets 0/0x100/0x200,
    incorrectly at 0x310 onward. The garbage value at 0x310 differs
    every boot (e.g. `0xba79ec73…`, `0x2286619f…`, `0x2286419f…` across
    three runs of the same image).
  - Both low-PA (`adrp + lo12`) and high-VA (literal pool) reads return
    the same garbage value, so the TTBR0/TTBR1 mappings agree on the
    physical address.
  - The same kernel image in QEMU produces correct values at every
    offset. The bug is real-Cortex-A72-silicon-only.
  - plo on rpi4 runs cache-off the entire time
    (`sources/plo/hal/aarch64/generic/_init.S`: SCTLR_EL3 = `0x30c50838`,
    SCTLR_EL2 = `0x30c00838`, SCTLR_EL1 = 0). Plo's stores go directly
    to DDR. So the corruption is *not* "stale plo cache lines".
- **Working theory:** an external coherent-master writes to the DRAM
  region containing `_hal_syspageCopied` between plo's stores and the
  kernel's reads. The top candidate is the BCM2711 VideoCore VI GPU
  continuing to access mailbox/framebuffer memory after the ARM kernel
  takes over (Linux on Pi 4 quiesces this with explicit platform init;
  Phoenix has no equivalent yet). Secondary candidates: stale L2 lines
  from the bootcode → start4.elf → armstub firmware chain, or in-flight
  DMA that hasn't drained at plo's `eret`.
- **Why other Phoenix platforms don't hit it:** ZynqMP plo also runs
  cache-off and shares the same kernel handoff code, but ZynqMP has
  no always-running non-coherent peripheral and a single-stage boot.
- **Why neither Linux nor other OSes hit it on Pi 4:** Linux on Pi 4
  meets the ARM64 Linux Boot Protocol contract (MMU off, D-cache off,
  kernel image cleaned to PoC, DMA quiesced) and contains explicit
  Pi-4 platform init that touches the VPU. Both halves are required.
- **Resolution path (current plan):**
  1. **Re-map `_hal_syspageCopied` as Normal Non-Cacheable** in TTBR1
     TTL3 using MAIR slot 1 (already configured as `0x44` =
     Normal NC). Both writes during the kernel-side copy and reads in
     `syspage_init()` then bypass the A72 D-cache entirely. This is
     the standard technique for memory shared with a cache-incoherent
     producer (DMA descriptor rings, GIC distributor regions, mailbox
     windows). If the residual corruption disappears, we know the
     class is fully addressed at this layer.
  2. **If Step 1 is insufficient** (i.e. an external master is still
     writing into that DRAM range), relocate `_hal_syspageCopied` above
     the firmware-reserved DRAM range advertised in the DTB's
     `/memreserve/` and `/reserved-memory/` nodes, and/or quiesce the
     VPU via mailbox before plo finishes. Either move closes the bug
     class for good rather than papering over individual symptoms.
  3. **Long term**, align plo's exit sequence with the ARM64 Linux
     Boot Protocol: clean kernel image and DTB to PoC, disable
     SCTLR.{M,C,I}, then `eret`. This is mostly stylistic given plo
     already runs cache-off, but it removes a class of "what state is
     this in?" ambiguity for future ARM64 ports.
- **Risks of doing nothing:** the iter-7/8 corruption blocks every
  attempt to validate program relocation, which blocks reaching
  `_hal_init()` from `syspage_init()`, which blocks the first full
  boot to userspace. This is the active blocker.
- **References:**
  - ARM64 Linux Boot Protocol —
    https://docs.kernel.org/arch/arm64/booting.html
  - raspberrypi/tools `armstubs/armstub8.S` (the contract Pi 4 firmware
    promises) — https://github.com/raspberrypi/tools
  - ARM tf-issues #205 — set/way safe only for power-down, not handoff
  - tracking/current-step.md — full probe data, QEMU comparison, plan

## TD-05: UART debug-marker scaffolding

- **Status:** PENDING
- **First observed:** 2026-04 bring-up (pervasive)
- **Where:** `hal/aarch64/_init.S`, `syspage.c`, `main.c`, and related
  boot-path files.
- **What was done:** Dozens of `uart_putc` and `uart` ring-buffer writes
  scattered through the early boot path to produce the
  `NYOPSTUZbcdeFGVWXabcdefgmklmno` progress trace.
- **Why:** The trace is how we locate hangs when no other diagnostic is
  available; there is no working console or fault reporting yet.
- **Risk accepted:** The markers affect boot timing, burn UART bandwidth,
  and make diffs noisy. Individual markers are easy to leave behind once
  they served their purpose.
- **Resolution requirements:**
  - Replace ad hoc markers with a compile-time-gated debug macro
    (`RPI4_BOOT_MARKER(c)`) so they can all be disabled in one place.
  - Establish a rule that every marker added to test a hypothesis is
    removed when the hypothesis is disproved (already in
    [code-quality-and-upstreaming.md](code-quality-and-upstreaming.md)).
  - Before upstreaming, strip or gate every remaining marker.

## TD-06: DTB handling assumptions

- **Status:** PENDING
- **First observed:** 2026-04 bring-up
- **Where:** `sources/phoenix-rtos-kernel/hal/aarch64/dtb.c`.
- **What was done:** Early parsing assumes a fixed memory layout, a single
  known interrupt controller, and limited error paths.
- **Why:** Early bring-up needed a DTB path with no surprises; robust
  parsing was not on the critical boot lane.
- **Risk accepted:** Any future board variant or firmware change silently
  reuses the fixed assumptions.
- **Resolution requirements:**
  - Drive memory layout from the actual DTB, not compile-time constants.
  - Validate required nodes at parse time and fail with a useful message.
  - Add multi-board support (Pi 4B variants, Pi 5) as the scope expands.

---

## TD-07: Update QEMU inside the phoenix-dev VM to a current stable

- **Status:** PENDING
- **Where:** apt-installed QEMU inside the Lima VM, used by
  `scripts/qemu-rpi4b-hdmi-smoke.sh` and `scripts/qemu-shell-smoke.sh`.
- **What was done:** an older QEMU release was good enough for early
  bring-up; never refreshed.
- **Why:** Pi 4 peripheral models in older QEMU are limited; some boot
  stages don't progress or behave differently than on real hardware.
- **Resolution requirements:**
  - Install upstream QEMU 11.x (or newer stable) inside the VM, either
    via apt-pinning a backports source, source build into `/opt/qemu/`,
    or a Lima provision script. Document the version + install method.
  - Verify the `raspi4b` machine model exists, has Cortex-A72 + GIC +
    PL011 working, and reproduces our SD-boot path far enough to be
    useful as an introspection target.

## TD-08: Re-test boot under QEMU + gdbstub for in-flight introspection

- **Status:** PENDING (depends on TD-07)
- **Where:** QEMU runner scripts and a gdb script we'll add under
  `scripts/`.
- **What was done:** real-hardware bring-up gives only UART markers as
  state. Memory contents at specific markers, register values right
  before the iter-7/8 corruption, MMU translation tables, and cache
  state are all opaque.
- **Why:** QEMU + gdbstub solves this — at the cost of not fully
  reproducing real-hardware cache/DDR/DMA timing.
- **Resolution requirements:**
  - `qemu-system-aarch64 ... -gdb tcp::1234 -S` against the same SD
    image we use on hardware; attach `gdb-multiarch` from outside.
  - Walk: pre-handoff syspage region in plo (0x280..0x340, SCTLR,
    TTBR0/1); post-handoff in kernel _init.S right after MMU enable
    (same region via low PA and high VA); inside `syspage_init`'s
    map-entry sub-loop around iter 7's `entry->next` read.
  - Even if the corruption itself doesn't reproduce in QEMU, validate
    the *logic* (list shape, struct layout, pointer arithmetic).

## TD-09: Replace en7 crossover cable with an unmanaged ethernet switch

- **Status:** PENDING
- **Where:** physical cabling between the Mac's en7 USB-C ethernet
  and the Pi 4 RJ45.
- **What was done:** direct crossover cable. Works electrically.
- **Why:** en7's link state mirrors the Pi's PHY directly. Every Pi
  power-cycle toggles en7 between `active` and `inactive`. socket_vmnet's
  BPF capture on en7 wedges on a non-trivial fraction of those toggles,
  and once wedged tends to stay wedged across one or more VM restarts.
  The watchdog + auto-recovery in `test-cycle-netboot.sh` handles the
  simple wedge case; a switch eliminates the trigger entirely.
- **Resolution requirements:**
  - Plug an unmanaged GigE switch between Mac and Pi. en7's link
    partner becomes the switch (always `active`); Pi power-cycles
    don't touch the bridge.
  - User has the switch on hand but is missing its PSU; install when
    found.

## Priority Ladder

**Blocks "first Pi 4 boots to userspace" milestone:**
- TD-04 (currently active — BCM2711 cache-coherency at plo→kernel handoff;
  next move: re-map `_hal_syspageCopied` as Normal Non-Cacheable)
- TD-03 (unblocks proper virtual syspage access)

**Blocks effective debugging:**
- TD-09 (netboot loop reliability — is the bottleneck right now)
- TD-07 → TD-08 (QEMU+gdb introspection of the iter-7/8 corruption)

**Blocks upstream-ready quality:**
- TD-05 (debug-marker strip/gate)
- TD-01 (SMP enable, required for anything beyond single-core)

**Medium-term:**
- TD-02 (cache invalidation correctness)
- TD-06 (DTB robustness, portability)

## Tracking Checklist

| ID | Status | Blocker? |
| --- | --- | --- |
| TD-01 | PENDING | multi-core work |
| TD-02 | PENDING | stability risk |
| TD-03 | PENDING | milestone |
| TD-04 | PENDING | active step |
| TD-05 | PENDING | upstream quality |
| TD-06 | PENDING | portability |
| TD-07 | PENDING | QEMU debugging |
| TD-08 | PENDING | QEMU+gdb debugging |
| TD-09 | PENDING | netboot loop reliability |

When resolving an item:

1. Create a `tracking/current-step.md` scoped to that single ID.
2. Remove the corresponding `TODO(TD-NN):` marker(s) from upstream source.
3. Commit the upstream repo change and snapshot an integration manifest.
4. Flip the status to `RESOLVED` in this file with the commit SHA and date.
