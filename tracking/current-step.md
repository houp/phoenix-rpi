# Current Implementation Step

## Step: Re-map `_hal_syspageCopied` as Normal Non-Cacheable to bypass the BCM2711 boot-handoff cache anomaly

**Status**: PLANNED (next implementation iteration)

**Date**: 2026-04-29

**Phase**: Phase A (close TD-04 prerequisite — program relocation)

This step replaces the previous "Analyse E1 probe data" framing. The
analysis is done; the picture is much bigger than a single iter-8 hang
in `syspage_init`. See sections below.

### What we now know (verified)

1. **plo on rpi4 runs cache-off the entire time.** Confirmed by
   reading `sources/plo/hal/aarch64/generic/_init.S`:
   - line 106–107: `SCTLR_EL3 = 0x30c50838` (M=0, C=0, I=0)
   - line 118–119: `SCTLR_EL2 = 0x30c00838` (M=0, C=0, I=0)
   - line 120: `SCTLR_EL1 = 0`
   - inline comment at line 183 explicitly says "plo runs cache-off
     (SCTLR.{M,C,I}=0)".

   So plo's stores go directly to DDR via the bus. There are *no*
   plo-dirty cache lines to flush at handoff. The earlier mental model
   ("plo writes are stuck in cache, kernel reads stale") that drove
   most of the pre-2026-04-29 probe iterations was wrong.

2. **The kernel-side copy logic is correct.** Same kernel image,
   same probe, in QEMU 10.2.2 raspi4b: `s{0} l{0} v{0} d0{0} d100{0}
   d200{0045700000000002}` — every offset reads back what plo wrote.
   On real Pi 4 hardware: `s{0} l{X} v{X} d0{0} d100{0}
   d200{0045700000000002}` where `X` is per-boot randomized garbage at
   offsets ≥ ~0x208. Same compiled instruction stream, divergent
   behaviour.

3. **The corruption pattern fits an external coherent-master
   write**, not a stale plo cache line. Deterministic offset
   threshold, non-deterministic value across boots. Pure cache
   staleness from a single producer would be deterministic.

### Why this is a class-of-problem, not a single bug

Phoenix-RTOS supports many platforms (zynqmp, zynq7000, imx6ull,
stm32 family, imxrt, riscv64, ia32, etc.) and the *same shared
`hal/aarch64/_init.S`* runs on ZynqMP without this corruption.
Three structural differences make BCM2711 the platform where the
bug surfaces:

| Property | ZynqMP / typical port | Real Pi 4 (BCM2711) |
|---|---|---|
| **Always-running non-coherent master** | None | **VideoCore VI continues DMA into DRAM after handoff** |
| **Multi-stage pre-plo firmware** | 1 stage (FSBL/u-boot) | **bootcode → start4.elf → armstub → plo (3 producers of stale L2 lines)** |
| **Cluster L2 topology** | Various, well-tested | **A72 unified L2, partly inclusive of L1; set/way maintenance on a single core not sufficient** |

The shared kernel boot code is fine on both. The Pi-4-specific
boot environment is where the coherency rule we relied on
(implicit on simpler SoCs) breaks.

### Linux's general answer to this class of problem

ARM64 Linux Boot Protocol
([Documentation/arch/arm64/booting.rst](https://docs.kernel.org/arch/arm64/booting.html))
specifies the contract:

- MMU off at kernel entry
- D-cache off at kernel entry
- Kernel image (and DTB) cleaned to PoC by VA before the jump
- DMA quiesced

Pi 4 firmware's stock `armstub8.S` honors this exactly: it sets
`SCTLR_EL2 = 0x30c50830` (M=C=I=0) and does no maintenance, on the
assumption that whoever wrote bytes to DDR already cleaned them.
Linux on Pi 4 works because (a) both ends meet the contract and (b)
Linux contains explicit Pi-4 platform init that quiesces VPU/mailbox
DMA before relying on ordinary memory accesses.

Phoenix-rpi4 has neither piece. plo's `eret` leaves caches in
whatever state, and there's no platform init that touches the VPU.

### Why other Phoenix aarch64 ports don't hit it

- **ZynqMP plo**: same SCTLR pattern (cache-off through plo runtime).
- **ZynqMP boot**: single-stage, no multi-firmware producers of
  stale L2.
- **ZynqMP system**: no equivalent of an always-running
  non-coherent VPU.

So the shared aarch64 kernel code works there because the Pi-4
aggravators are absent.

### Captured probe data, for the record

Real Pi 4 hardware, three consecutive boots of the same image:

```
s{0000000000000000}    src[0x310] = 0  (matches plo's pre-jump probe)
l{ba79ec73…}/{2286619f…}/{2286419f…}  dst[0x310] low PA — different garbage every boot
v{ba79ec73…}/{2286619f…}/{2286419f…}  dst[0x310] high VA — same as l{} (mappings agree)
d0{0}                              dst[0x000] = 0
s0{0}                              src[0x000] = 0
d100{0}                            dst[0x100] = 0
s100{0}                            src[0x100] = 0
d200{0045700000000002}             dst[0x200] = correct (matches src)
s200{0045700000000002}             src[0x200] = correct
```

Same kernel image in QEMU 10.2.2 raspi4b:

```
s{0} l{0} v{0} d0{0} s0{0} d100{0} s100{0} d200{0045700000000002} s200{0045700000000002}
```

### What was tried and rejected

- **Pre-copy `_clean_inval_dcache_range` over the dest range**:
  hangs the kernel on real hardware at marker `V` (line 247, before
  the X1 MMU-setup marker), works fine in QEMU. The hang itself is
  diagnostic — `dsb sy` inside the flush function waits for all
  in-flight memory ops to drain and would block if a firmware-side
  DMA channel is wedged. Reverted; the kernel `_init.S` carries a
  do-not-redo comment marker referencing this file.
- **Post-copy `_clean_inval_dcache_range` over the dest range**:
  no effect on real hardware; bytes still read back as garbage. As
  expected: if the dest cache line is *clean* (because plo's writes
  went to DDR not cache, and the kernel's str fills cache fresh), a
  later VA-by-VA flush is a no-op except for invalidating, which
  doesn't help because the post-flush read goes to DDR — and DDR
  *also* has bad bytes now. Confirms the writer is *outside the
  A72 D-cache*, not in it.

### Recommendation — change the shape of the fix

#### Step 1 (next implementation): make `_hal_syspageCopied`
   **Normal Non-Cacheable**.

The MAIR table the kernel installs already has slot 1 = Normal
Non-Cacheable (`MAIR_EL1_VALUE = 0x444FF`, byte 1 = `0x44`). Re-map
just the one 4 KiB page containing `_hal_syspageCopied` so its
TTBR1 TTL3 entry uses `AttrIndx = 1` instead of `0`.

Both writes during the kernel-side copy and reads in `syspage_init`
then bypass cache entirely and hit DDR directly. There is no
A72-cache-line for an external master to corrupt or for a stale L2
line to evict onto.

This is the same general technique used for any region shared
between two cache-incoherent agents (DMA descriptor rings, GIC
distributor regions, mailbox windows). Cost on syspage walks is
~5% performance — irrelevant for boot.

The change is small and contained:
- a new TTL3 entry in `_init.S` written with attrs `0x705`
  (AttrIndx=1, AP=00, SH=11, AF=1) for the page containing
  `_hal_syspageCopied`'s low PA
- nothing else changes — copy code, probe code, syspage_init all
  remain identical
- testing per the new probe-parity rule: build → QEMU smoke → real
  HW → diff outputs side-by-side in this file

#### Step 2 (only if Step 1 leaves residual corruption)

If even an uncached destination shows wrong bytes on real
hardware, *something is actively writing to that DDR location
between plo and the kernel*. Candidates, ranked:

1. **VideoCore VI** continuing to access mailbox/framebuffer
   regions via a non-coherent path. Resolution: relocate
   `_hal_syspageCopied` above the firmware-reserved DRAM range
   (read `/memreserve/` and `/reserved-memory/` from the DTB plo
   passes us), or quiesce the VPU via a mailbox call before plo
   finishes.
2. **Other A72 cores**. Currently parked in `_other_core_trap`
   *after* the syspage copy in `_init.S`; trapping them earlier
   (in the armstub or via a barrier in plo) closes that window.
3. **In-flight firmware DMA** that hadn't drained when plo took
   control. The pre-copy `dc civac` hang above is consistent with
   this hypothesis.

The bug pattern (deterministic offset, non-deterministic value)
strongly fits VPU writes into a fixed DRAM mailbox window that
overlaps `_hal_syspageCopied`'s physical placement. We can confirm
or rule it out by reading the firmware-passed DTB `/memreserve/`
node before any code change.

### Exit criteria

- After Step 1: real-hardware probe markers match QEMU markers
  byte-for-byte at every offset (s/l/v/d0/d100/d200/s0/s100/s200).
  Three consecutive boots produce identical output.
- After Step 1, the C-level iter trace's `B{...}` syspage-region
  hash is bit-identical across boots.
- iter-7/8 entry pointers either show a clean linked-list
  termination (matching plo's emitted list) *or* expose a different
  bug (e.g. genuinely shorter list than the kernel walks). Either
  way the cache anomaly stops being the dominant noise.

### Rollback

- Worktree `dazzling-joliot-cd9889`.
- Sibling repo `phoenix-rtos-kernel` branch `agent/rpi4-program-reloc`,
  ahead of tag `known-good/2026-04-19-map-relocation-complete`.
- `plo` branch `codex/common-aarch64-platform-makefiles`.
- The reverted-pre-flush experiment is documented with a comment
  marker in `_init.S`; do not re-introduce it.

### References

- ARM64 Linux Boot Protocol —
  https://docs.kernel.org/arch/arm64/booting.html
- raspberrypi/tools `armstubs/armstub8.S` —
  https://github.com/raspberrypi/tools/blob/master/armstubs/armstub8.S
- u-boot `cleanup_before_linux` arm64 sequence (cache-disable in asm) —
  doc/arch/arm64.rst, README.arm-caches in u-boot tree
- ARM tf-issues #205 (set/way only safe for power-down, not handoff) —
  https://github.com/ARM-software/tf-issues/issues/205
- Cortex-A72 TRM r0p3 — relevant for cluster L2 inclusivity behaviour

### Notes

- This step is the first one applying the **probe-parity rule**
  added to `AGENTS.md` and `docs/testing-automation.md` on
  2026-04-29: every probe must run in QEMU first, then on real HW,
  with both outputs diffed. The rule was learned from this
  investigation — the QEMU comparison is what proved the copy logic
  was correct and reframed the search away from "code bug" toward
  "boot environment problem".
- Netboot infra works for the iteration loop; bridge wedge with
  crossover cable is the residual annoyance, expected to disappear
  once the user wires the GigE switch (TD-09).
