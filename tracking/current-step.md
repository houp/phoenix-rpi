# Current Step

## Metadata

- Step ID: `STEP-0352`
- Title: Implement the smallest xHCI memory-allocation step for `DCBAA` and the command ring
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- implement the next bounded xHCI memory-allocation step after the new
  operational-layout baseline without widening into active controller
  programming

## Scope

In scope:

- extracting:
  - `DCBAA` allocation
  - command ring allocation
- validating only the smallest memory-layout constraints needed before later
  controller programming
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
- actual DCBAA programming
- actual command-ring programming
- doorbell writes
- interrupter programming

## Expected Repositories

- coordination repo
- `phoenix-rtos-devices`

## Expected Files Or Subsystems

- `sources/phoenix-rtos-devices/usb/xhci/`
- `sources/phoenix-rtos-usb/usb/mem.c`
- `external/circle/include/circle/usb/xhci.h`
- `docs/status.md`
- `docs/source-artifacts.md`
- `tracking/current-step.md`
- `tracking/step-history.md`
- `manifests/`

## Acceptance Criteria

- `xhci` allocates and records the first controller-owned memory objects
- the new checks stay pre-register-programming, pre-doorbell,
  pre-interrupt-enable, and pre-enumeration
- the full `aarch64a72-generic-rpi4b` build remains clean with the live staged
  Pi 4 image unchanged

## Validation Plan

- fresh `aarch64a72-generic-rpi4b` build from the copied VM-local buildroot in
  `phoenix-dev`

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-xhci-memory-layout.md`

## Notes

- Risks:
  do not widen the step into actual register programming, doorbell writes,
  interrupter work, or root-port logic just because the memory objects now
  exist
- Dependencies:
  completed `STEP-0351` xHCI memory-allocation scope
- User-visible control point before next step:
  after this step lands, `xhci` should own the first controller memory objects
  needed for later programming, but should still make no host-operation claim
