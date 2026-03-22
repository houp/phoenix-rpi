# Current Step

## Metadata

- Step ID: `STEP-0370`
- Title: Implement the smallest xHCI roothub control/request step
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- implement the first usable xHCI roothub request subset needed before the live
  Pi 4 image can safely stage `/sbin/usb`

## Scope

In scope:

- special-casing `usb_isRoothub(pipe->dev)` in `xhci_transferEnqueue()`
- implementing the smallest roothub request subset needed by early enumeration:
  - hub descriptor
  - port status read
  - minimal set/clear port feature handling
- keeping the step pre-full-enumeration and pre-keyboard claims

Out of scope:

- non-roothub transfer support
- USB keyboard device bring-up
- staging `/sbin/usb` or `/sbin/usbkbd` on the Pi 4 image in this step
- SD-image export or checksum refresh
- manual hardware execution
- unrelated shell, console, or PCIe changes

## Expected Repositories

- coordination repo
- `phoenix-rtos-devices`

## Expected Files Or Subsystems

- `sources/phoenix-rtos-devices/usb/xhci/xhci.c`
- `sources/phoenix-rtos-usb/usb/usb.c`
- `docs/status.md`
- `docs/source-artifacts.md`
- `tracking/current-step.md`
- `tracking/step-history.md`
- `manifests/`

## Acceptance Criteria

- the xHCI path now contains a bounded roothub request handler for the minimal
  early-enumeration subset
- the step stays pre-full-device-enumeration and pre-keyboard claims
- the full `aarch64a72-generic-rpi4b` build still succeeds

## Validation Plan

- fresh `aarch64a72-generic-rpi4b` build from the copied VM-local buildroot in
  `phoenix-dev`
- preserve the staged Pi 4 image composition until the roothub path and xHCI
  init are stronger

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-xhci-roothub-scope.md`

## Notes

- Risks:
  avoid widening directly into full USB enumeration or image staging before the
  roothub contract exists
- Dependencies:
  completed `STEP-0369` xHCI roothub scope
- User-visible control point before next step:
  the next implementation step should answer the concrete readiness question:
  whether `/sbin/usb` is any closer to being stageable on the Pi 4 image
