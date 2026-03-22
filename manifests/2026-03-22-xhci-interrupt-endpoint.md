# STEP-0388: Implement the bounded xHCI interrupt-IN endpoint ownership/configuration step

Date: `2026-03-22`

## Goal

Add the first non-EP0 endpoint ownership/configuration slice needed before any
keyboard report transfer can exist.

## Implemented

In `phoenix-rtos-devices/usb/xhci/xhci.c`:

- added the minimum command and endpoint-context constants needed for one
  bounded `Configure Endpoint` path
- added a small per-pipe private structure that owns one interrupt transfer
  ring
- added helpers to:
  - derive an xHCI endpoint ID from the current `usb_pipe_t`
  - convert the Phoenix interrupt interval into the xHCI interval encoding
  - issue one bounded `Configure Endpoint` command
  - allocate and initialize one interrupt-IN transfer ring
  - populate the matching endpoint context in the existing input context
- wired the interrupt transfer path so that the first matching interrupt-IN
  transfer now performs endpoint ownership/configuration and then still stops
  cleanly before real transfer submission
- added cleanup in `xhci_pipeDestroy()` for the new per-pipe xHCI state

## Validation

- fresh full Pi 4 A72 build in `phoenix-dev`:
  `TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- Pi 4 shell non-regression:
  `./scripts/qemu-shell-smoke.sh rpi4b`

## Result

The Pi 4 xHCI path now owns and configures the first interrupt-IN endpoint for
the current child device. The next concrete blocker is no longer endpoint
configuration; it is submission and completion of a real interrupt-IN transfer
on the current no-IRQ path.
