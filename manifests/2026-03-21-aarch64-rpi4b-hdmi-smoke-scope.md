# 2026-03-21: scope the smallest automated Pi 4 HDMI visibility smoke

## Scope

- Step: `STEP-0284`
- Goal: define the smallest automated-friendly regression check for the current
  Pi 4 `plo` HDMI marker path

## Current Baseline

- `STEP-0283` already proved the Pi 4 `plo` path can:
  - allocate a framebuffer through the mailbox/property interface
  - paint a visible screen
  - show a bright top-left marker rectangle under `raspi4b` QEMU
- the validation method is currently manual and session-shaped:
  - run QEMU with a display backend
  - connect to the QEMU monitor
  - `screendump` to a PPM
  - sample pixels by hand or with ad hoc Python

## Decision

The smallest useful next step is:

- add one coordination-repo helper script for the current `raspi4b` QEMU lane
- keep it build-output based; do not fold a rebuild into the helper
- make it validate only the current early HDMI marker signature:
  - expected image size `1024 x 768`
  - top-left marker sample points should be bright
  - background sample points should match the current filled color

## Why This Is The Smallest Useful Shape

- it turns the newly working HDMI path into a repeatable regression check
- it does not widen into full runtime display support
- it does not require real hardware
- it matches the current no-UART observability goal directly
- it fits the existing coordination-repo pattern already used by
  `scripts/qemu-shell-smoke.sh`

## Out Of Scope

- framebuffer console support
- keyboard or mouse interaction over HDMI
- real-hardware HDMI capture
- generalized screenshot tooling across multiple targets

## Next Step

- implement `STEP-0285`: add a Pi 4 HDMI smoke helper that runs the current
  QEMU lane, captures one framebuffer dump, validates a few pixel samples, and
  exits non-zero on regression
