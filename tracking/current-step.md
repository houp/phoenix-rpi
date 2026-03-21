# Current Step

## Metadata

- Step ID: `STEP-0350`
- Title: Implement the smallest xHCI operational memory-layout register step
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- implement the next bounded xHCI operational memory-layout register step after
  the new capability-decoding baseline without widening into active controller
  programming

## Scope

In scope:

- extracting:
  - `DCBAAP`
  - `CRCR`
- validating only the smallest register-layout constraints needed before later
  DCBAA or command-ring programming
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

- `xhci` extracts and records the first operational memory-layout register
  fields
- the new checks stay pre-DCBAA programming, pre-command-ring programming,
  pre-interrupt-enable, and pre-enumeration
- the full `aarch64a72-generic-rpi4b` build remains clean with the live staged
  Pi 4 image unchanged

## Validation Plan

- fresh `aarch64a72-generic-rpi4b` build from the copied VM-local buildroot in
  `phoenix-dev`

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-xhci-scratchpad-capability.md`

## Notes

- Risks:
  do not widen the step into actual DCBAA setup, command-ring programming,
  interrupter work, or root-port logic just because the register locations are
  nearby
- Dependencies:
  completed `STEP-0349` xHCI operational memory-layout register scope
- User-visible control point before next step:
  after this step lands, `xhci` should know the first operational register
  locations needed for later DCBAA and command-ring work, but should still make
  no host-operation claim
