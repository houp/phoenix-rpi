# Current Step

## Metadata

- Step ID: `STEP-0323`
- Title: Scope the smallest first endpoint-visibility and enumeration step
- Status: `in_progress`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest BCM2711 endpoint-visibility and first-enumeration slice
  that should follow the new bridge exposure step without widening into xHCI

## Scope

In scope:

- reviewing the remaining gap between the new bridge-exposure step and
  meaningful downstream endpoint visibility
- selecting the smallest next slice that can follow without widening into xHCI
  or endpoint-driver work
- preserving the current HDMI text-console baseline and the deferred SD export

Out of scope:

- SD-image export or checksum refresh
- manual hardware execution
- broad framebuffer-console redesign
- changes to the existing `usbkbd` or `pl011-tty` logic
- full BCM2711 PCIe register bring-up in the same step
- xHCI, USB enumeration, or keyboard end-to-end validation

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `docs/status.md`
- `docs/source-artifacts.md`
- `tracking/current-step.md`
- `tracking/step-history.md`
- `manifests/`

## Acceptance Criteria

- the next endpoint-visibility / enumeration step is explicitly bounded and
  justified against the current Phoenix and Circle references
- the next step does not widen into xHCI or endpoint-driver work
- the tracking docs clearly state the exact remaining gap after the new
  bridge-exposure step

## Validation Plan

- source inspection against the current Phoenix PCIe stack plus the existing
  Circle reference for BCM2711 bridge enablement and first downstream endpoint
  visibility

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-bcm2711-bridge-exposure.md`

## Notes

- Risks:
  do not jump straight into xHCI or overclaim that bridge-side exposure alone
  implies meaningful downstream endpoint visibility
- Dependencies:
  completed `STEP-0322` BCM2711 bridge exposure
- User-visible control point before next step:
  after this step lands, the repo should clearly show the exact smallest
  endpoint-visibility move and why it comes before xHCI

Current scope finding:

- the BCM2711 backend now supplies config-space access, early host-bridge prep,
  link-state gating, outbound-window setup, root-bridge shaping, and
  bridge-side memory-window / bus exposure
- the remaining gap is now narrower:
  first downstream endpoint visibility before any xHCI-specific work can be
  treated as meaningful
- Pi 4 `raspi4b` QEMU is still not expected to validate that real PCIe step
