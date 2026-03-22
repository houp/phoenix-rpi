# STEP-0386: Implement the bounded xHCI EP0 control-write/no-data path

Date: `2026-03-22`

## Goal

Add the first bounded post-enumeration endpoint-0 control-write path needed by
the existing Phoenix USB host and `usbkbd` flow.

## Implemented

In `phoenix-rtos-devices/usb/xhci/xhci.c`:

- added the smallest new TRB constant needed for a no-data setup stage:
  `TRT_NONE`
- added `xhci_ep0ControlWriteNoData()` that:
  - emits setup and status TRBs only
  - uses no data stage
  - rings the slot doorbell for endpoint `0`
  - polls the current event ring for a transfer completion event
  - requires a clean `SUCCESS` completion
- wired the non-roothub child path so that, for the first direct-root-port
  child under the current temporary slot-ID-equals-address contract, Phoenix
  now accepts only these zero-length OUT requests:
  - `REQ_SET_CONFIGURATION`
  - `CLASS_REQ_SET_PROTOCOL`
  - `CLASS_REQ_SET_IDLE`
- kept the step intentionally narrow:
  - no control transfers with a data stage
  - no generic request coverage
  - no interrupt endpoint setup yet
  - no live Pi 4 image staging yet

## Validation

- fresh full Pi 4 A72 build in `phoenix-dev`:
  `TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- Pi 4 shell non-regression:
  `./scripts/qemu-shell-smoke.sh rpi4b`

## Result

The Pi 4 xHCI path now covers the bounded post-enumeration control writes used
by `usbkbd`. The next concrete blocker is no longer control writes; it is the
first interrupt-IN endpoint path needed to open the keyboard endpoint and start
receiving reports.
