# Current Step

## Metadata

- Step ID: `STEP-0046`
- Title: Implement first common public AArch64 timer file
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add the first common public AArch64 timer implementation file on top of the existing `gtimer_backend` helper layer

## Scope

In scope:

- add `hal/aarch64/gtimer_timer.c`
- provide the required public timer entry points from that file
- use the existing backend-state, conversion, wakeup, and IRQ helpers
- validate compilation through `AARCH64_TIMER_IMPL_OVERRIDE` in `phoenix-dev`

Out of scope:

- adding a new QEMU target
- changing the default runtime timer implementation for any target
- changing target definitions in `phoenix-rtos-build` or `phoenix-rtos-project`
- adding PL011 console code
- Raspberry Pi-specific code

## Expected Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Expected Files Or Subsystems

- `hal/aarch64/gtimer_timer.c`
- tracking files and manifest updates for this step

## Acceptance Criteria

- the new common file provides the required public timer entry points
- the file builds successfully when selected through `AARCH64_TIMER_IMPL_OVERRIDE`
- the default `aarch64a53-zynqmp-qemu` selection remains unchanged outside the override validation lane

## Validation Plan

- Build:
  refresh the copied buildroot and rebuild `TARGET=aarch64a53-zynqmp-qemu` with `AARCH64_TIMER_IMPL_OVERRIDE='$(addprefix $(PREFIX_O)hal/aarch64/, gtimer_timer.o)'`
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-public-timer-file-scope.md`

## Notes

- Risks:
  the step must stay within one new common file and must not also switch default targets or widen into project/build target work
- Dependencies:
  completed scoping step `STEP-0045`
- User-visible control point before next step:
  after this step lands, the next slice can decide whether to validate runtime behavior on a new common timer lane or keep staying compile-only
