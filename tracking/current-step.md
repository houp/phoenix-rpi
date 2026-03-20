# Current Step

## Metadata

- Step ID: `STEP-0169`
- Title: Scope first Pi 4 `hal_cpuJump()` / EL-exit visibility step
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest next visibility step that distinguishes whether the Pi 4 official-DTB lane blocks before entering `hal_cpuJump()`, inside `hal_cpuJump()`, or only after `hal_exitToEL1()` begins the exception-level handoff

## Scope

In scope:

- review the narrow jump path:
  - `plo/hal/aarch64/generic/hal.c`
  - `plo/hal/aarch64/generic/_init.S`
- select one bounded visibility step that exposes the first silent boundary after `go: jump`
- update manifests and docs with the scoped next step

Out of scope:

- code changes
- changing Pi 4 image layout
- changing DTB content or selection
- kernel-side changes
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `plo` jump / EL-exit handoff notes
- Pi 4 QEMU handoff boundary notes
- manifests and tracking updates for this planning step

## Acceptance Criteria

- the reviewed jump path is explicitly recorded
- the next implementation step is narrowed to one `hal_cpuJump()` or EL-exit visibility change
- the scoped next step is specific enough to divide `hal_cpuJump()` C code from the assembly handoff path

## Validation Plan

- Review:
  inspect `hal_cpuJump()` and `hal_exitToEL1()`
- Build:
  not applicable
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-rpi4b-go-visibility.md`

## Notes

- Risks:
  avoid widening into kernel instrumentation or board-specific hacks before the generic jump path is explicitly split
- Dependencies:
  completed `STEP-0168` filtered post-`go!` visibility
- User-visible control point before next step:
  after this step lands, the next bounded move should be a single jump-path visibility patch rather than a broader Pi 4 kernel change
