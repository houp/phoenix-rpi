# Current Step

## Metadata

- Step ID: `STEP-0283`
- Title: Implement the first Pi 4 `plo` mailbox-framebuffer visibility step
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- implement the smallest Pi 4-specific non-UART visibility step that can show
  HDMI-side life signs before widening into full display or networking support

## Scope

In scope:

- add minimal Raspberry Pi mailbox property-call support in `plo`
- allocate one simple framebuffer on the Pi 4 path
- produce one visible framebuffer-side signal under `raspi4b` QEMU if possible
- keep the step limited to early visibility, not full runtime display support

Out of scope:

- runtime framebuffer console support
- real hardware execution
- broad graphics, USB, or network bring-up

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `docs/manual-operator-instructions.md`
- `docs/testing-automation.md`
- `docs/platforms/raspberry-pi-4.md`
- current Pi 4 source and reference notes for mailbox, framebuffer, and early
  network possibilities
- `sources/plo`
- current Pi 4 `board_config.h`
- `docs/status.md`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- the Pi 4 path performs one minimal mailbox property transaction in `plo`
- the step produces one visible HDMI-side sign of life or one tightly bounded
  negative result under `raspi4b` QEMU
- no Phoenix upstream repo changes are introduced

## Validation Plan

- build the Pi 4 image after the change
- validate first in `raspi4b` QEMU
- use GDB-first debugging if the expected visible path does not appear
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-rpi4b-no-uart-observability-scope.md`

## Notes

- Risks:
  avoid widening into full framebuffer console plumbing or general graphics
  support
- Dependencies:
  completed `STEP-0282` no-UART observability scoping
- User-visible control point before next step:
  after this step lands, the next bounded move can either refine the mailbox
  framebuffer signal or start threading that graphmode data into later runtime
  consumers
