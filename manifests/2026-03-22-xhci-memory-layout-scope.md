# STEP-0349

## Title

Scope the smallest xHCI operational memory-layout register step

## Date

2026-03-22

## Outcome

The next bounded xHCI move is now fixed:

- extract the first operational-register fields that will later anchor memory
  layout, but still do not require active controller programming:
  - `DCBAAP`
  - `CRCR`
- keep the step read-only and structural
- keep the result pre-DCBAA programming, pre-command-ring allocation,
  pre-interrupt-enable, and pre-enumeration

## Why This Step

After the capability block is now largely decoded, the next clean seam is the
subset of operational-register layout that will eventually matter for command
ring and device-context setup. Those register locations can be captured before
the code starts programming the controller.

## Validation Plan

- fresh copied-buildroot Pi 4 A72 build in `phoenix-dev`

## Next Step

- implement the bounded xHCI operational memory-layout register extraction in
  `usb/xhci/xhci.c`
