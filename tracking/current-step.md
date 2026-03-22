# Current Step

## Metadata

- Step ID: `STEP-0359`
- Title: Scope the smallest xHCI pre-run operational step beyond command-space register programming
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- define the next bounded xHCI move after `DCBAAP`, `CRCR`, and `CONFIG`
  programming, keeping the controller still pre-interrupt, pre-doorbell, and
  pre-enumeration

## Scope

In scope:

- deciding the smallest safe post-programming xHCI seam
- checking the current `xhci.c` controller state against the xHCI bring-up
  sequence
- choosing one bounded operational step that is still meaningful without real
  hardware feedback
- documenting the acceptance criteria for that step

Out of scope:

- implementing xHCI controller changes in this step
- event-ring, interrupter, root-hub, or enumeration logic
- SD-image export or checksum refresh
- manual hardware execution
- unrelated shell, console, or PCIe changes

## Expected Repositories

- coordination repo
- `phoenix-rtos-kernel`

## Expected Files Or Subsystems

- `sources/phoenix-rtos-devices/usb/xhci/xhci.c`
- `docs/status.md`
- `docs/source-artifacts.md`
- `tracking/current-step.md`
- `tracking/step-history.md`
- `manifests/`

## Acceptance Criteria

- the next xHCI step is narrowed to one specific operational-register or
  run-state slice
- the scoped step stays pre-interrupt, pre-doorbell, and pre-enumeration
- the scope is grounded in the current `xhci.c` state rather than in generic
  USB design prose

## Validation Plan

- inspect the current `xhci.c` implementation and the existing project notes
- cross-check the next seam against the already extracted xHCI controller state

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-pi4-shell-smoke-probe-cleanup.md`

## Notes

- Risks:
  avoid widening straight into event rings or enumeration before a smaller
  controller-run-state step is isolated
- Dependencies:
  completed `STEP-0358` shell-smoke probe cleanup
- User-visible control point before next step:
  the next implementation step should touch only one narrow xHCI operational
  seam and keep the current QEMU boot baselines intact
