# Current Step

## Metadata

- Step ID: `STEP-0118`
- Title: Implement generic AArch64 `plo` secondary-core containment
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add the smallest generic AArch64 `plo` startup guard that traps non-primary cores until CPU 0 transfers control to the kernel

## Scope

In scope:

- `sources/plo/hal/aarch64/generic/_init.S`
- `sources/plo/hal/aarch64/generic/hal.c`
- add a generic `hal_coreJumpFlag`
- trap non-primary cores by MPIDR in generic startup
- release them only at `hal_cpuJump()` handoff
- validate both generic build viability and the Pi 4 `plo.elf` QEMU lane

Out of scope:

- broad Pi 4 peripheral-debug work
- broad SMP bring-up or kernel SMP work
- kernel or project changes
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration
- changing the existing QEMU boot-media model beyond the already approved `plo.elf` smoke

## Expected Repositories

- `sources/plo`
- coordination repo

## Expected Files Or Subsystems

- `sources/plo/hal/aarch64/generic/_init.S`
- `sources/plo/hal/aarch64/generic/hal.c`
- relevant Pi 4 QEMU smoke notes
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- generic AArch64 `plo` now traps non-primary cores before `_startc`
- `hal_cpuJump()` releases trapped secondary cores through the new flag
- the existing generic build remains green
- the Pi 4 `plo.elf` QEMU lane no longer fails immediately with the same multi-core startup storm and yields a more stable early-boot signal

## Validation Plan

- Review:
  confirm that the patch stays localized and reuses the existing ZynqMP containment pattern
- Build:
  rebuild affected generic AArch64 targets in `phoenix-dev`
- Emulator:
  rerun the Pi 4 `raspi4b` `plo.elf` QEMU smoke and compare the result against `STEP-0116`
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-generic-plo-secondary-containment-scope.md`

## Notes

- Risks:
  keep the secondary containment contract minimal and loader-local
- Dependencies:
  completed `STEP-0117`
- User-visible control point before next step:
  after this step lands, the next bounded move should come from the new Pi 4 QEMU behavior rather than from abstract SMP theory
