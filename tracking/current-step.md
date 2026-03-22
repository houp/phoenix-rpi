# Current Step

## Metadata

- Step ID: `STEP-0391`
- Title: Scope the smallest Pi 4 live USB-host staging step
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- choose the smallest live Pi 4 image-integration step after the new bounded
  xHCI keyboard-transfer support

## Scope

In scope:

- deciding the smallest live-image staging change needed to exercise the new
  Pi 4 USB path on real hardware
- confirming whether `/sbin/usb` alone is the relevant staged binary for the
  internal-host-driver path
- keeping the next move below SD export and operator execution

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
- `tracking/current-step.md`
- `tracking/step-history.md`
- `docs/status.md`
- `docs/source-artifacts.md`
- `manifests/`

## Acceptance Criteria

- the next live-image staging seam is explicitly chosen and documented
- the staging choice is justified against the current internal-driver USB host
  architecture
- the chosen next step stays narrow and below SD export or manual execution

## Validation Plan

- code reading and bounded source-path analysis only

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-xhci-interrupt-transfer.md`

## Notes

- Risks:
  avoid widening the next move into SD export or manual hardware execution too
  early
- Dependencies:
  completed `STEP-0390` bounded interrupt-transfer and no-IRQ completion
  support
- User-visible control point before next step:
  after this scope step, the next bounded move should be the first live Pi 4
  staging change needed for keyboard testing
