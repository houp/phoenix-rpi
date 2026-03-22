# STEP-0371

## Title

Scope the smallest xHCI init-success step before live `/sbin/usb` staging

## Date

2026-03-22

## Outcome

The next bounded USB-host move is now fixed:

- allow `xhci_init()` to return success after the current internal controller
  setup sequence
- keep the step strictly roothub-only:
  - no non-roothub transfer support
  - no slot enable or address-device work
  - no live Pi 4 image staging yet

## Why This Step

The current code is now past the earlier roothub gate:

- the roothub control path exists
- roothub enumeration uses only synchronous control requests
- the hub interrupt transfer can remain pending after `hub_conf()` without
  blocking startup

So the next concrete blocker before `/sbin/usb` can become stageable is no
longer “missing roothub requests”. It is the deliberate `-ENOSYS` tail in
`xhci_init()`.

## Next Step

- implement the smallest xHCI init-success step
