# Current Step

## Metadata

- Step ID: `STEP-0373`
- Title: Scope the smallest xHCI roothub status-delivery step
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest safe step that can deliver Pi 4 xHCI roothub port-change
  status back into Phoenix on the current no-IRQ path

## Scope

In scope:

- deciding how the current Pi 4 xHCI path should surface roothub status changes
  while the discovery stub still reports `irq = 0`
- keeping the step bounded to roothub status delivery only

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

- the next bounded xHCI roothub-status move is explicitly selected
- the scope stays pre-non-roothub enumeration and pre-keyboard claims
- the next implementation step answers whether a temporary polling path is the
  correct bridge before a later real interrupt path

## Validation Plan

- source-level review of the current xHCI discovery contract, roothub request
  flow, and Phoenix hub-status completion model
- no code changes required for the planning step itself

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-xhci-init-success.md`

## Notes

- Risks:
  avoid widening directly into child-device enumeration or live image staging
  before roothub status changes can be delivered cleanly
- Dependencies:
  completed `STEP-0372` xHCI init-success implementation
- User-visible control point before next step:
  the next implementation step should state how plug events are expected to
  reach the pending root-hub interrupt transfer
