# STEP-0351

## Title

Scope the smallest xHCI memory-allocation step for `DCBAA` and the command ring

## Date

2026-03-22

## Outcome

The next bounded xHCI move is now fixed:

- allocate the first controller-owned memory objects needed for later
  controller programming:
  - `DCBAA`
  - command ring segment
- keep the step allocation-only and bookkeeping-only
- keep the result pre-register-programming, pre-doorbell, pre-interrupt-enable,
  and pre-enumeration

## Why This Step

After the capability block and first operational register layout are decoded,
the next smallest concrete step is to prepare the memory objects that later
controller programming will need. Phoenix already has uncached USB memory
helpers, so allocation can be separated cleanly from the later register writes.

## Reference

- `sources/phoenix-rtos-usb/usb/mem.c`
- `external/circle/include/circle/usb/xhci.h`

## Validation Plan

- fresh copied-buildroot Pi 4 A72 build in `phoenix-dev`

## Next Step

- implement bounded `DCBAA` and command-ring allocation bookkeeping in
  `usb/xhci/xhci.c`
