# Current Step

## Metadata

- Step ID: `STEP-0371`
- Title: Scope the smallest xHCI init-success step before live `/sbin/usb` staging
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest safe step that could let the Pi 4 xHCI path survive
  `hcd_init()` as a roothub-only host, without yet claiming general USB-device
  enumeration or keyboard support

## Scope

In scope:

- deciding whether the current xHCI state is sufficient for `xhci_init()` to
  return success instead of `-ENOSYS`
- keeping the step bounded to controller lifetime and roothub-only readiness
- keeping `/sbin/usb` image staging out of this planning step

Out of scope:

- non-roothub transfer support
- slot enable / address-device / endpoint-context work
- staging `/sbin/usb` or `/sbin/usbkbd` on the Pi 4 image
- SD-image export or checksum refresh
- manual hardware execution
- unrelated shell, HDMI, or PCIe changes

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `tracking/current-step.md`
- `tracking/step-history.md`
- `docs/status.md`
- `docs/source-artifacts.md`
- `manifests/`

## Acceptance Criteria

- the next bounded USB-host move is explicitly selected
- the scope stays pre-non-roothub enumeration and pre-keyboard claims
- the next implementation step answers whether `/sbin/usb` is any closer to
  being stageable on the Pi 4 image

## Validation Plan

- source-level review of the current xHCI init, roothub, and Phoenix `usb`
  startup contract
- no code changes required for the planning step itself

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-xhci-roothub.md`

## Notes

- Risks:
  do not widen directly into child-device enumeration or live image staging
  before the controller can survive `hcd_init()`
- Dependencies:
  completed `STEP-0370` xHCI roothub request implementation
- User-visible control point before next step:
  the next implementation step should say whether the current Pi 4 xHCI path
  can now remain alive as a roothub-only host
