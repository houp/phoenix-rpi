# Current Step

## Metadata

- Step ID: `STEP-0348`
- Title: Implement the smallest xHCI addressability and scratchpad-capability refinement
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- implement the next bounded xHCI addressability and scratchpad-capability step
  after the new pre-interrupt data without widening into operational
  controller logic

## Scope

In scope:

- extracting:
  - 64-bit addressing support
  - scratchpad-restore support
  - maximum primary stream array size
- validating only the smallest capability constraints needed before later
  DCBAA or scratchpad-allocation work
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
- command/event ring setup
- interrupter setup
- root-hub modeling or enumeration

## Expected Repositories

- coordination repo
- `phoenix-rtos-devices`

## Expected Files Or Subsystems

- `sources/phoenix-rtos-devices/usb/xhci/`
- `external/circle/include/circle/usb/xhci.h`
- `docs/status.md`
- `docs/source-artifacts.md`
- `tracking/current-step.md`
- `tracking/step-history.md`
- `manifests/`

## Acceptance Criteria

- `xhci` extracts and records the next addressability and scratchpad capability
  fields
- the new checks stay pre-DCBAA, pre-scratchpad-allocation, pre-ring, and
  pre-enumeration
- the full `aarch64a72-generic-rpi4b` build remains clean with the live staged
  Pi 4 image unchanged

## Validation Plan

- fresh `aarch64a72-generic-rpi4b` build from the copied VM-local buildroot in
  `phoenix-dev`

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-xhci-interrupter-shape.md`

## Notes

- Risks:
  do not widen the step into actual DCBAA setup, scratchpad allocation,
  stream-context work, or root-port logic just because the related capability
  bits are nearby
- Dependencies:
  completed `STEP-0347` xHCI addressability and scratchpad-capability scope
- User-visible control point before next step:
  after this step lands, `xhci` should know the remaining addressability and
  memory-layout facts needed for later scratchpad and DCBAA work, but should
  still make no host-operation claim
