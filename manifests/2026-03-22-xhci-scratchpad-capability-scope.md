# STEP-0347

## Title

Scope the smallest xHCI addressability and scratchpad-capability refinement

## Date

2026-03-22

## Outcome

The next bounded xHCI move is now fixed:

- extract the remaining controller capability bits that shape later memory
  layout but still do not require operational controller logic:
  - 64-bit addressing support
  - scratchpad-restore support
  - maximum primary stream array size
- keep the step read-only and structural
- keep the result pre-DCBAA setup, pre-scratchpad allocation, pre-ring, and
  pre-enumeration

## Why This Step

After slot, scratchpad-count, doorbell/runtime offsets, and pre-interrupt
limits are known, the remaining obvious structural dependency is the
controller's addressing and memory-layout capability. Those values shape later
DCBAA and scratchpad allocation work, but they can still be collected without
claiming runtime USB-host behavior.

## Reference

- `external/circle/include/circle/usb/xhci.h`

## Validation Plan

- fresh copied-buildroot Pi 4 A72 build in `phoenix-dev`

## Next Step

- implement the bounded addressability and scratchpad-capability extraction in
  `usb/xhci/xhci.c`
