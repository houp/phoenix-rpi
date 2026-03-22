# Current Step

## Metadata

- Step ID: `STEP-0358`
- Title: Implement the smallest cleanup step for the stale `create_dev` probes
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- restore a clean automated Pi 4 shell-smoke signal by removing the stale
  kernel `create_dev` probes that are no longer diagnostically needed

## Scope

In scope:

- removing the old `create_dev` probe helpers and their call sites
- preserving the current runtime behavior
- validating through:
  - generic shell smoke
  - Pi 4 shell smoke
  - Pi 4 HDMI smoke if needed

Out of scope:

- SD-image export or checksum refresh
- manual hardware execution
- xHCI, PCIe, or USB-host runtime behavior changes
- `pl011-tty`, `psh`, or namespace logic changes
- broad probe cleanup outside the active `create_dev` spam source

## Expected Repositories

- coordination repo
- `phoenix-rtos-kernel`

## Expected Files Or Subsystems

- `sources/phoenix-rtos-kernel/syscalls.c`
- `docs/status.md`
- `docs/source-artifacts.md`
- `docs/testing-automation.md`
- `tracking/current-step.md`
- `tracking/step-history.md`
- `manifests/`

## Acceptance Criteria

- the generic shell smoke still passes
- the Pi 4 shell smoke passes cleanly again without stale `create_dev` prompt
  interleaving
- generic shell smoke stays green
- the cleanup stays limited to obsolete probe removal

## Validation Plan

- fresh `aarch64a72-generic-rpi4b` build from the copied VM-local buildroot in
  `phoenix-dev`
- `./scripts/qemu-shell-smoke.sh generic`
- `./scripts/qemu-shell-smoke.sh rpi4b`
- `/bin/bash /Users/witoldbolt/phoenix-rpi/scripts/qemu-rpi4b-hdmi-smoke.sh`

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-pi4-shell-smoke-cleanup-scope.md`

## Notes

- Risks:
  do not hide a live functional bug behind a blind log cleanup; remove only the
  probes already shown to be stale
- Dependencies:
  completed `STEP-0357` Pi 4 shell-smoke cleanup scope
- User-visible control point before next step:
  after this step lands, the Pi 4 `raspi4b` shell-smoke helper should be clean
  and deterministic again
