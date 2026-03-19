# Current Step

## Metadata

- Step ID: `STEP-0014`
- Title: Define the first runtime-oriented kernel step after DTB preparation
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- define and bound the first runtime-oriented kernel change needed after the current DTB preparation series, so the next implementation step stays small and explicitly chooses between a generic ARM timer path and broader AArch64 target/platform work

## Scope

In scope:

- update `phoenix-rtos-kernel/hal/aarch64/dtb.c`
- inspect the current AArch64 kernel runtime dependencies after the completed DTB parser steps
- compare the narrowest next-step options:
  - a generic ARM architectural timer implementation path
  - a broader generic AArch64 target or platform split
- select one small next implementation step with explicit touched files, validation lane, and acceptance criteria
- keep this as a planning and scoping step only

Out of scope:

- implementation of the selected runtime step itself
- adding a new QEMU target
- adding PL011 console code
- changing `plo`
- Raspberry Pi-specific code

## Expected Repositories

- coordination repo
- likely `phoenix-rtos-kernel`

## Expected Files Or Subsystems

- AArch64 kernel HAL timer and console paths
- `hal/aarch64/Makefile`
- `hal/aarch64/zynqmp/timer.c`
- `hal/aarch64/zynqmp/console.c`
- tracking files and manifest updates for the chosen next step

## Acceptance Criteria

- the next runtime-oriented kernel step is explicitly scoped with exact touched files, rationale, validation command, and success criteria
- the selected next step is narrow enough to implement and validate in one controlled follow-up session

## Validation Plan

- Build:
  not applicable for this planning step
- Emulator:
  inspect the current QEMU `virt` timer and console dependencies as needed to choose the narrowest next step
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-dtb-timer-metadata.md`

## Notes

- Risks:
  the next runtime step is the first one that can widen materially, so it must be explicitly bounded before any new code lands
- Dependencies:
  completed DTB preparation series for GIC, serial, and timer metadata, plus a working AArch64 validation toolchain in `phoenix-dev`
- User-visible control point before next step:
  present the exact selected runtime step before moving into timer implementation, console implementation, or target-definition work
