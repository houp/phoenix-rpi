# Current Step

## Metadata

- Step ID: `STEP-0306`
- Title: Scope the smallest post-HDMI-text Pi 4 shell-smoke regression step
- Status: `in_progress`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- keep the HDMI text-console milestone recorded, then define the smallest next
  technical step that explains why a fresh `./scripts/qemu-shell-smoke.sh rpi4b`
  rerun no longer completes even though the Pi 4 HDMI smoke still passes

## Scope

In scope:

- tracking and documentation updates for the HDMI-text milestone and deferred
  SD export
- narrowing the next regression-analysis step for the `rpi4b` shell helper
- keeping the current validated Pi 4 HDMI smoke baseline explicit

Out of scope:

- SD-image export or checksum refresh
- manual hardware execution
- USB keyboard support
- broad framebuffer-console redesign

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `docs/status.md`
- `docs/manual-operator-instructions.md`
- `docs/testing-automation.md`
- `docs/source-artifacts.md`
- `tracking/current-step.md`
- `tracking/step-history.md`
- `manifests/`

## Acceptance Criteria

- the repo no longer points at the paused SD-export step as active work
- the operator runbook reflects the validated HDMI text-console signature
- the next implementation step is explicitly scoped around the current
  `rpi4b` shell-smoke mismatch rather than a stale artifact-export goal

## Validation Plan

- documentation consistency review
- preserved current validation facts:
  - `/bin/bash /Users/witoldbolt/phoenix-rpi/scripts/qemu-rpi4b-hdmi-smoke.sh`
    passes
  - `./scripts/qemu-shell-smoke.sh rpi4b` currently needs re-verification

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-rpi4b-hdmi-text-console.md`

## Notes

- Risks:
  do not silently resume artifact refresh after the user explicitly deferred it
- Dependencies:
  completed `STEP-0303` Pi 4 HDMI text console validation
- User-visible control point before next step:
  after this step lands, the repo should clearly state that HDMI text is ready,
  SD export remains deferred, and the next bounded task is shell-smoke
  re-verification
