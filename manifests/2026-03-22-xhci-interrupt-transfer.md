# STEP-0390: Implement the bounded xHCI interrupt-IN transfer and no-IRQ completion step

Date: `2026-03-22`

## Goal

Add the first real keyboard-report transfer path on the current no-IRQ Pi 4
xHCI lane.

## Implemented

In `phoenix-rtos-devices/usb/xhci/xhci.c`:

- added the minimum state for one outstanding interrupt-IN transfer:
  - pending transfer pointer
  - expected transfer-event TRB address
  - one active interrupt endpoint slot on the host
- extended the existing xHCI status thread so it now:
  - still handles root-hub change delivery
  - also polls the current event ring for one matching interrupt transfer event
  - validates endpoint ID, slot ID, and transfer-event pointer
  - completes the pending transfer through `usb_transferFinished()`
  - rearms `ERDP` and halts the controller again after completion
- added `xhci_submitInterruptIn()` that:
  - emits one normal TRB on the already-configured interrupt ring
  - rearms the event ring
  - enters run state
  - rings the current slot doorbell for the configured interrupt endpoint
  - returns immediately so the async URB path stays asynchronous
- updated the bounded interrupt transfer path so it now:
  - configures the endpoint if needed
  - submits the first interrupt-IN transfer instead of stopping at `-ENOSYS`

## Validation

- fresh full Pi 4 A72 build in `phoenix-dev`:
  `TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- Pi 4 shell non-regression, rerun sequentially after the rebuild:
  `./scripts/qemu-shell-smoke.sh rpi4b`

## Result

The Pi 4 xHCI path now has a bounded no-IRQ interrupt-IN transfer path for one
configured endpoint. The next blocker is no longer keyboard-transfer mechanics
inside xHCI itself; it is staging the USB host on the live Pi 4 image and then
testing the end-to-end real-device path.
