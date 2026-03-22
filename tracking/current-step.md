# Current Step

## Metadata

- Step ID: `STEP-0392`
- Title: Implement the smallest Pi 4 live USB-host staging step
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- implement the smallest live Pi 4 image-integration step after the new bounded
  xHCI keyboard-transfer support

## Scope

In scope:

- staging `/sbin/usb` on the Pi 4 image path
- preserving the current `pcie -> usb -> psh` bring-up order
- validating that the new staged image still keeps the Pi 4 QEMU shell lane
  alive

Out of scope:

- generic endpoint-0 transfer support
- implementing the next interrupt-endpoint path yet
- staging `/sbin/usb` or `/sbin/usbkbd` on the Pi 4 image
- SD-image export or manual hardware execution

## Expected Repositories

- `phoenix-rtos-devices`
- coordination repo

## Expected Files Or Subsystems

- `sources/phoenix-rtos-devices/usb/xhci/xhci.c`
- `sources/phoenix-rtos-usb/usb/dev.c`
- `sources/phoenix-rtos-usb/libusb/driver.c`
- `sources/phoenix-rtos-devices/tty/usbkbd/usbkbd.c`
- `sources/phoenix-rtos-usb/usb/drv.c`
- `sources/phoenix-rtos-usb/usb/usbhost.h`
- `sources/phoenix-rtos-usb/usb/usb.c`
- `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/user.plo.yaml`
- `docs/status.md`
- `tracking/current-step.md`
- `tracking/step-history.md`
- `docs/status.md`
- `docs/source-artifacts.md`
- `manifests/`

## Acceptance Criteria

- the Pi 4 image stages `/sbin/usb` in the intended place
- a fresh full `aarch64a72-generic-rpi4b` build still passes
- the Pi 4 shell smoke still passes after the staging change

## Validation Plan

- fresh Pi 4 A72 build in `phoenix-dev` using the standard copied-buildroot
  path
- Pi 4 shell smoke after rebuild

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-xhci-interrupt-transfer.md`

## Notes

- Risks:
  avoid widening the step into SD export or manual execution too early
- Dependencies:
  completed `STEP-0391` live USB-host staging scope
- User-visible control point before next step:
  after this step, the next bounded move should be either SD-image refresh or a
  clearly justified additional live-image fix discovered during validation
