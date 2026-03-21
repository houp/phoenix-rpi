# Current Step

## Metadata

- Step ID: `STEP-0339`
- Title: Scope the smallest post-notify xHCI readiness refinement
- Status: `in_progress`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest xHCI controller-readiness refinement that should follow
  the new Pi 4 firmware-notify baseline

## Scope

In scope:

- reviewing the remaining gap between the new xHCI reset plus firmware-notify
  baseline and the first controller state needed for later root-hub work
- selecting the smallest next xHCI refinement after reset and firmware notify
- preserving the current compile-valid, unstaged USB-host baseline on the Pi 4

Out of scope:

- SD-image export or checksum refresh
- manual hardware execution
- staged runtime USB host support
- command/event ring setup
- interrupt setup
- root-hub modeling or enumeration
- changes to the existing `usbkbd` or `pl011-tty` logic
- broad BCM2711 PCIe host-bridge redesign

## Expected Repositories

- coordination repo
- `phoenix-rtos-devices`

## Expected Files Or Subsystems

- `sources/phoenix-rtos-devices/usb/xhci/`
- `docs/status.md`
- `docs/source-artifacts.md`
- `tracking/current-step.md`
- `tracking/step-history.md`
- `manifests/`

## Acceptance Criteria

- the next xHCI refinement is explicitly bounded against the new reset plus
  firmware-notify baseline
- the selected next step clearly states why it comes before rings, interrupts,
  root-hub work, or staged `/sbin/usb`
- the next step still avoids staged runtime USB support and enumeration

## Validation Plan

- source inspection against:
  - `sources/phoenix-rtos-devices/usb/xhci/xhci.c`
  - `sources/phoenix-rtos-usb/usb/hcd.c`
  - `external/circle/lib/usb/xhcidevice.cpp`
  - `external/circle/include/circle/usb/xhci.h`

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-rpi4b-xhci-firmware-notify.md`

## Notes

- Risks:
  do not widen the next xHCI step into ring, interrupt, or root-hub work just
  because the firmware-notify seam is now in place
- Dependencies:
  completed `STEP-0338` Pi 4 firmware `notify xHCI reset`
- User-visible control point before next step:
  after this step lands, the repo should show the next smallest xHCI-internal
  readiness move beyond the new Pi 4 firmware hook
