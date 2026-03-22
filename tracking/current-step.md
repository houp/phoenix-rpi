# Current Step

## Metadata

- Step ID: `STEP-0383`
- Title: Scope the smallest non-`SET_ADDRESS` xHCI endpoint-0 control-transfer step
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest non-`SET_ADDRESS` endpoint-0 control-transfer seam after
  the new bounded `Address Device` path

## Scope

In scope:

- deciding the smallest control-transfer subset needed after `SET_ADDRESS`
- deciding whether the next seam should start with:
  - `GET_DESCRIPTOR`
  - setup-stage TRBs only
  - a bounded single-control-read path for device descriptor bytes
- keeping the scope below broad generic control-transfer support

Out of scope:

- broad generic control-transfer support
- broad xHCI enumeration or generic transfer support
- staging `/sbin/usb` or `/sbin/usbkbd` on the Pi 4 image
- SD-image export or manual hardware execution

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `tracking/current-step.md`
- `tracking/step-history.md`
- `docs/status.md`
- `docs/source-artifacts.md`
- `manifests/`

## Acceptance Criteria

- the next bounded endpoint-0 transfer move is explicitly selected
- the selected seam stays below broad generic transfer support
- the step explains which first control-transfer subset should land next

## Validation Plan

- source review of the current Phoenix USB enumeration flow and current xHCI
  child-device state
- no code changes required for the planning step itself

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-xhci-address-device.md`

## Notes

- Risks:
  avoid jumping directly into a broad generic control-transfer engine when only
  a bounded descriptor-read seam is needed next
- Dependencies:
  completed `STEP-0382` bounded `Address Device` support for `REQ_SET_ADDRESS`
- User-visible control point before next step:
  the next implementation step should name the smallest concrete endpoint-0
  control subset Phoenix needs to get beyond `SET_ADDRESS`
