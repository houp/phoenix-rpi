# Manifest: Generic AArch64 `plo` Secondary-Core Containment Scope

- Date: `2026-03-20`
- Step: `STEP-0117`
- Status: `completed`

## Goal

- choose the smallest generic AArch64 `plo` patch that can contain non-primary cores on the Pi 4 QEMU `plo.elf` lane without widening into full SMP bring-up

## Evidence

From the completed Pi 4 QEMU `plo.elf` handoff:

- the board now reaches visible loader output
- the run then falls into an early exception storm with heavily interleaved UART output

From local QEMU `10.2.2` source:

- `hw/arm/boot.c`
  - raw images are treated as Linux, ELF images are not
  - the custom secondary CPU boot stub is only installed on the Linux-classified path
- `hw/arm/raspi.c`
  - `raspi4b` does have a custom AArch64 spin-table secondary boot stub, but only when the Linux path above is active

From current Phoenix sources:

- `sources/plo/hal/aarch64/generic/_init.S`
  - has no non-primary-core trap or parking loop
- `sources/plo/hal/aarch64/generic/hal.c`
  - has no `hal_coreJumpFlag`
- `sources/plo/hal/aarch64/zynqmp/_init.S`
  - already contains the exact containment pattern needed conceptually:
    - detect non-zero MPIDR core ID
    - park non-primary cores in `wfe`
    - release them only at kernel handoff
- `sources/plo/hal/aarch64/zynqmp/hal.c`
  - sets `hal_coreJumpFlag = 1` immediately before `hal_exitToEL1()`

## Selected Patch Shape

- keep the patch in generic AArch64 `plo` only:
  - `sources/plo/hal/aarch64/generic/_init.S`
  - `sources/plo/hal/aarch64/generic/hal.c`
- add a generic `hal_coreJumpFlag`
- trap non-zero MPIDR cores after the shared EL setup and stack setup, before `_startc`
- release trapped secondary cores only through `hal_cpuJump()`
- keep the generic fast path for CPU 0 unchanged

## Why This Is The Smallest Safe Step

- it addresses the earliest observed Pi 4 QEMU failure directly
- it is generic AArch64 cleanup, not a Pi-4-only hack
- it reuses an existing Phoenix loader pattern instead of inventing a new SMP policy
- it does not require kernel changes, DT changes, or a QEMU-specific image redesign

## Step Boundary

- this step does not attempt:
  - secondary CPU startup policy after kernel handoff
  - full SMP enablement
  - a real-hardware SMP contract
  - a broader QEMU boot wrapper redesign

## Selected Next Step

- implement the generic AArch64 `plo` secondary-core containment patch and validate it on:
  - the existing generic `virt` build lane
  - the Pi 4 `raspi4b` `plo.elf` QEMU smoke
