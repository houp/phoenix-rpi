# STEP-0375

## Title

Scope the smallest xHCI non-roothub device-enumeration step

## Date

2026-03-22

## Outcome

The next bounded xHCI move is now fixed:

- implement the first internal `Enable Slot` command path
- keep it pre-address-device and pre-control-transfer
- use it as the first child-device enumeration seam after the roothub

## Why This Step

After the roothub, Phoenix child-device enumeration immediately needs a real
USB device behind xHCI. That cannot happen until the controller has a slot
allocated for the new port/device path.

So the smallest meaningful post-roothub seam is not “generic transfer support”
yet. It is:

- submit `Enable Slot`
- parse command completion
- record the returned slot ID

Only after that is it sensible to move toward `Address Device`, endpoint 0, and
real control transfers.

## Next Step

- implement the smallest xHCI `Enable Slot` step
