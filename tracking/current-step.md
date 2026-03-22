# Current Step

## Metadata

- Step ID: `STEP-0387`
- Title: Scope the smallest xHCI interrupt-IN endpoint step
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- choose the smallest real interrupt-endpoint seam needed after the new bounded
  control-transfer support

## Scope

In scope:

- deciding which first interrupt-endpoint responsibility should be implemented
  next
- keeping the next move as narrow as possible
- using the existing Phoenix USB host and `usbkbd` flow to justify the chosen
  seam

Out of scope:

- generic endpoint-0 transfer support
- implementing the next interrupt-endpoint path yet
- interrupt-IN endpoint work
- staging `/sbin/usb` or `/sbin/usbkbd` on the Pi 4 image

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
- `tracking/current-step.md`
- `tracking/step-history.md`
- `docs/status.md`
- `docs/source-artifacts.md`
- `manifests/`

## Acceptance Criteria

- the next interrupt-endpoint seam is explicitly chosen and documented
- the choice is justified against the existing Phoenix USB host and `usbkbd`
  call paths
- the chosen next step stays narrow and below live Pi 4 image staging

## Validation Plan

- code reading and bounded source-path analysis only

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-xhci-control-write.md`

## Notes

- Risks:
  avoid widening the next move into generic interrupt scheduling or full device
  enumeration too early
- Dependencies:
  completed `STEP-0386` bounded control-write/no-data support
- User-visible control point before next step:
  after this scope step, the next bounded move should be the first interrupt
  endpoint responsibility that still blocks keyboard report delivery
