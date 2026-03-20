# Current Step

## Metadata

- Step ID: `STEP-0167`
- Title: Scope first post-`go!` Pi 4 handoff visibility step
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest next visibility step that distinguishes whether the Pi 4 official-DTB lane blocks inside `cmd_go()`, inside `hal_done()`, inside `hal_cpuJump()`, or immediately after the jump attempt

## Scope

In scope:

- review the narrow post-`go!` handoff path:
  - `plo/cmds/go.c`
  - `plo/hal/aarch64/generic/hal.c`
- select one bounded visibility step that exposes the first silent boundary after `call: exec go!`
- update manifests and docs with the scoped next step

Out of scope:

- loader code changes
- changing Pi 4 image layout
- changing DTB content or selection
- kernel-side changes
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `plo` post-`go!` handoff notes
- Pi 4 QEMU handoff boundary notes
- manifests and tracking updates for this planning step

## Acceptance Criteria

- the reviewed handoff path is explicitly recorded
- the next implementation step is narrowed to one post-`go!` visibility change
- the scoped next step is specific enough to divide `cmd_go()` from `hal_cpuJump()` / post-jump behavior

## Validation Plan

- Review:
  inspect `cmd_go()` and the generic AArch64 jump path
- Build:
  not applicable
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-rpi4b-call-visibility.md`

## Notes

- Risks:
  avoid widening into kernel instrumentation before the post-`go!` loader handoff is explicitly split
- Dependencies:
  completed `STEP-0166` filtered loader call visibility
- User-visible control point before next step:
  after this step lands, the next bounded move should be a single post-`go!` visibility patch rather than a broader loader or kernel change
