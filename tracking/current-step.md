# Current Step

## Metadata

- Step ID: `STEP-0389`
- Title: Scope the smallest xHCI interrupt-IN transfer step
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- choose the smallest real interrupt-transfer seam needed after the new
  endpoint-configuration support

## Scope

In scope:

- deciding which first interrupt-IN transfer and completion responsibility
  should be implemented next
- keeping the next move as narrow as possible
- using the current `usbkbd` async URB flow and the no-IRQ xHCI reality to
  justify the chosen seam

Out of scope:

- generic endpoint-0 transfer support
- implementing the next interrupt-endpoint path yet
- implementing the next interrupt transfer path yet
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
- `sources/phoenix-rtos-usb/usb/usb.c`
- `tracking/current-step.md`
- `tracking/step-history.md`
- `docs/status.md`
- `docs/source-artifacts.md`
- `manifests/`

## Acceptance Criteria

- the next interrupt-transfer seam is explicitly chosen and documented
- the choice is justified against the current async URB and no-IRQ xHCI flow
- the chosen next step stays narrow and below live Pi 4 image staging

## Validation Plan

- code reading and bounded source-path analysis only

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-xhci-interrupt-endpoint.md`

## Notes

- Risks:
  avoid widening the next move into generic multi-endpoint scheduling or live
  image staging too early
- Dependencies:
  completed `STEP-0388` bounded interrupt-endpoint ownership/configuration
  support
- User-visible control point before next step:
  after this scope step, the next bounded move should be the first real
  interrupt transfer or completion seam that still blocks keyboard reports
