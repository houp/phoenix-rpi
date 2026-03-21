# Current Step

## Metadata

- Step ID: `STEP-0301`
- Title: Scope the first AArch64 framebuffer-graphmode exposure step for Pi 4 HDMI text
- Status: `in_progress`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest code step that exposes the already-populated Pi 4
  framebuffer geometry through the generic AArch64 kernel/user ABI, so HDMI
  text output can build on existing Phoenix framebuffer-console code

## Scope

In scope:

- identify the minimum generic AArch64 `platformctl` and syspage surface needed
  for framebuffer consumers
- keep the step below full text rendering or runtime app selection

Out of scope:

- SD-image refresh or export
- manual hardware execution
- USB keyboard support
- a full framebuffer-console implementation

## Expected Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Expected Files Or Subsystems

- `hal/aarch64/generic/generic.c`
- `include/arch/aarch64/generic/generic.h`
- `docs/status.md`
- `docs/source-artifacts.md`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- the next code change is narrowed to one reusable AArch64 graphmode exposure
  step
- the plan explicitly prefers reuse of Phoenix's existing framebuffer text code
  over a new font subsystem
- the step does not widen into SD-image, USB, or manual-trial work

## Validation Plan

- Build:
  not applicable for the scoping step
- Emulator:
  not applicable for the scoping step
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-rpi4b-staged-hdmi-progress.md`

## Notes

- Risks:
  avoid widening from graphmode exposure into a whole new tty or font design
- Dependencies:
  completed `STEP-0297` staged Pi 4 HDMI progress
- User-visible control point before next step:
  after this step lands, the first implementation patch should be a narrow
  generic AArch64 framebuffer metadata exposure change rather than more artifact
  work
