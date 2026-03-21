# Current Step

## Metadata

- Step ID: `STEP-0341`
- Title: Scope the smallest next xHCI structural-capability refinement
- Status: `in_progress`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest next xHCI structural-capability refinement that should
  follow the new post-notify readiness baseline

## Scope

In scope:

- reviewing the remaining structural capability gaps after version, reset,
  firmware notify, page-size validation, and basic port-count extraction
- selecting the smallest next xHCI capability-state move before any root-hub,
  ring, or interrupt work
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

- the next xHCI step is explicitly bounded to one more structural-capability
  refinement beyond page-size and port-count checks
- the selected next step clearly states why it comes before root-hub, ring, or
  interrupt work
- the next step still avoids staged runtime USB support and enumeration

## Validation Plan

- source inspection against:
  - `sources/phoenix-rtos-devices/usb/xhci/xhci.c`
  - `sources/phoenix-rtos-usb/usb/hcd.c`
  - `external/circle/lib/usb/xhcidevice.cpp`
  - `external/circle/include/circle/usb/xhci.h`

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-xhci-post-notify-readiness.md`

## Notes

- Risks:
  do not widen the next xHCI step into root-hub, ring, or interrupt work just
  because more capability bits are nearby in the spec
- Dependencies:
  completed `STEP-0340` post-notify xHCI readiness implementation
- User-visible control point before next step:
  after this step lands, the repo should show the next smallest xHCI structural
  move beyond the current readiness baseline without yet claiming host
  operation
