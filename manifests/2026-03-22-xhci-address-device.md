# STEP-0382: Implement the bounded xHCI `REQ_SET_ADDRESS` path under the temporary slot-ID contract

Date: `2026-03-22`

## Goal

Add the first real xHCI `Address Device` command path while keeping the address
policy explicit and narrow.

## Implemented

In `phoenix-rtos-devices/usb/xhci/xhci.c`:

- added the first bounded internal `Address Device` command helper
- added the command TRB constants needed for `Address Device`
- wired the non-roothub control path so that, when:
  - the device is still at address `0`
  - the request is `REQ_SET_ADDRESS`
  - the requested USB address matches the already-enabled xHCI slot ID
  Phoenix will:
  - initialize the EP0 ring
  - prepare the minimal input context
  - issue `Address Device`
  - complete the transfer successfully
- kept the contract explicit and temporary:
  - if the requested USB address does not match the slot ID, the path still
    fails cleanly instead of silently rewriting addresses

## Validation

- fresh full Pi 4 A72 build in `phoenix-dev`:
  `TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- no new QEMU runtime claim for this step, because `/sbin/usb` is still not
  staged on the live Pi 4 image and the new path is not exercised by the
  current boot-smoke lane

## Result

The Pi 4 xHCI path now has the first real non-roothub command-backed child
device action. The next concrete blocker is no longer `SET_ADDRESS`; it is the
first non-`SET_ADDRESS` endpoint-0 control-transfer path needed for descriptor
reads.
