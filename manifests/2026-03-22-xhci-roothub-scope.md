# STEP-0369

## Title

Scope the smallest xHCI roothub control/request step

## Date

2026-03-22

## Outcome

The next bounded USB-host move is now fixed:

- add the first xHCI roothub request handler path behind
  `usb_isRoothub(pipe->dev)`
- start with the smallest subset needed before live Pi 4 staging:
  - hub descriptor
  - port status read
  - minimal port feature set/clear operations used by enumeration
- keep the step pre-general USB-device enumeration and pre-keyboard claims

## Why This Step

The project is still not ready for interactive real-device testing because:

- the live Pi 4 image still does not stage `/sbin/usb` or `/sbin/usbkbd`
- staging them now would still fail, because `usb` exits if `hcd_init()`
  cannot produce a usable HCD
- even after the new xHCI no-op command path, the roothub path is still a stub:
  `xhci_transferEnqueue()` does not special-case roothub control requests and
  `xhci_getHubStatus()` still returns `0`

So the next real seam is no longer deeper command plumbing; it is the first
roothub control/request subset.

## Next Step

- implement the smallest xHCI roothub control/request step
