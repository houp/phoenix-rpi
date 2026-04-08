# Current Step

## Metadata

- Step ID: `STEP-0419`
- Title: Await the next Pi 4 board retry on the expanded armstub image
- Status: `in_progress`
- Date: `2026-04-08`
- Milestone / phase: `Phase 1`

## Objective

- hold the project at the next justified real-hardware boundary and collect the
  first board evidence from the refreshed Pi 4 SD image that now includes the
  expanded custom armstub EL3 timer and GIC preparation

## Scope

In scope:

- the next real Pi 4 SD-card boot on the refreshed exported image
- recording the exact HDMI, LED, keyboard, and reboot behavior
- mapping that evidence to the next smallest firmware-handoff or earliest-entry
  step

Out of scope:

- speculative source changes before the next board retry result exists
- wider refactors unrelated to the observed next hardware symptom

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `docs/pi4-first-hardware-trial.md`
- `docs/manual-operator-instructions.md`
- `docs/status.md`
- `tracking/step-history.md`
- `manifests/`

## Acceptance Criteria

- the refreshed Pi 4 SD image has been handed off with its exact checksum
- the next board retry result has been captured
- the next active implementation step is chosen from real hardware evidence,
  not guesswork

## Validation Plan

- use the exported image:
  - `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- verify with:
  - `/Users/witoldbolt/phoenix-rpi/scripts/verify-rpi4b-sdimg.sh`
- flash and boot according to:
  - `/Users/witoldbolt/phoenix-rpi/docs/pi4-first-hardware-trial.md`

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-04-08-pi4-armstub-el3-gic-prep.md`

## Notes

- Current exported image SHA-256:
  `16c4f7f5e313266bdb197a9ddc4d3dc81a080fffb6bea631ab7016dbbb741590`
- Trigger:
  the earlier corrected-GIC-address image still left the board black and
  silent, and the next bounded response has now expanded the custom Pi 4
  armstub with Circle-style EL3 timer and GIC preparation.
