# Current Step

## Metadata

- Step ID: `STEP-0376`
- Title: Implement the smallest xHCI `Enable Slot` step
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- add the first bounded internal xHCI child-device operation after the roothub:
  submit `Enable Slot`, parse completion, and capture the slot ID

## Scope

In scope:

- implementing the smallest internal `Enable Slot` command path
- extending command completion handling just enough to validate and capture a
  returned slot ID
- keeping the step pre-`Address Device` and pre-real control transfers

Out of scope:

- endpoint-0 transfer support
- `Address Device`
- broad xHCI enumeration implementation
- staging `/sbin/usb` or `/sbin/usbkbd` on the Pi 4 image
- SD-image export or checksum refresh
- manual hardware execution
- unrelated shell, HDMI, or PCIe changes

## Expected Repositories

- coordination repo
- `phoenix-rtos-devices`

## Expected Files Or Subsystems

- `sources/phoenix-rtos-devices/usb/xhci/xhci.c`
- `docs/status.md`
- `docs/source-artifacts.md`
- `tracking/current-step.md`
- `tracking/step-history.md`
- `manifests/`

## Acceptance Criteria

- the xHCI path now contains a bounded internal `Enable Slot` command path
- the step stays pre-`Address Device` and pre-real control transfers
- the full `aarch64a72-generic-rpi4b` build still succeeds

## Validation Plan

- fresh `aarch64a72-generic-rpi4b` build from the copied VM-local buildroot in
  `phoenix-dev`
- keep live Pi 4 image composition unchanged in this step

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-xhci-enable-slot-scope.md`

## Notes

- Risks:
  avoid widening directly into `Address Device` or endpoint-0 ring/context work
- Dependencies:
  completed `STEP-0375` xHCI `Enable Slot` scope
- User-visible control point before next step:
  the next implementation step should answer whether Phoenix now has the first
  real child-device xHCI command beyond the roothub
