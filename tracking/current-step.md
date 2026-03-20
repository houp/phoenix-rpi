# Current Step

## Metadata

- Step ID: `STEP-0024`
- Title: Add targeted AArch64 SGI helper for future timer updates
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- expose a targeted AArch64 SGI helper so later timer-path work can notify CPU 0 directly when a non-CPU0 context needs the wakeup timer reprogrammed

## Scope

In scope:

- add a targeted AArch64 SGI helper alongside the existing broadcast helper
- update the common CPU interface declaration used by the kernel to include that helper
- preserve existing behavior and validate the current ZynqMP build lane
- validate the existing `aarch64a53-zynqmp-qemu` build in `phoenix-dev`

Out of scope:

- adding a new QEMU target
- using the new helper from the timer or scheduler code
- implementing the common generic timer runtime backend itself
- changing timer wakeup semantics
- adding PL011 console code
- Raspberry Pi-specific code

## Expected Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Expected Files Or Subsystems

- `hal/cpu.h`
- `hal/aarch64/interrupts_gicv2.c`
- tracking files and manifest updates for this step

## Acceptance Criteria

- AArch64 exposes a targeted SGI helper in addition to the existing broadcast helper
- the helper is limited to generic SGI send plumbing and does not yet change timer or scheduler behavior
- the existing `aarch64a53-zynqmp-qemu` build still succeeds in `phoenix-dev`

## Validation Plan

- Build:
  refresh the copied buildroot and rebuild `TARGET=aarch64a53-zynqmp-qemu` with `./phoenix-rtos-build/build.sh clean host core project`
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-gicv2-ppi-safe-handler-registration.md`

## Notes

- Risks:
  the step must not widen into new SGI users or timer behavior changes
- Dependencies:
  completed PPI-safe handler-registration step from `STEP-0023`
- User-visible control point before next step:
  after this helper lands, re-scope the first timer runtime step around explicit CPU0-directed wakeup-update notification instead of trying to drop in the full architectural timer backend at once
