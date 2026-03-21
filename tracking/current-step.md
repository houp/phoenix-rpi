# Current Step

## Metadata

- Step ID: `STEP-0315`
- Title: Scope the smallest BCM2711 host-bridge initialization step
- Status: `in_progress`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest BCM2711 PCIe host-bridge initialization slice that
  should follow the new indexed config-space backend without widening into xHCI
  or claiming downstream enumeration

## Scope

In scope:

- reviewing the remaining gap between the new indexed config-space backend and
  real BCM2711 host-bridge operation
- selecting the smallest initialization slice that can follow without widening
  into xHCI or full PCIe bring-up
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

- the next host-bridge step is explicitly bounded and justified against the
  current Phoenix and Circle references
- the next step does not widen into xHCI or claim live downstream enumeration
- the tracking docs clearly state the exact remaining gap after the new
  indexed-config backend

## Validation Plan

- source inspection against the current Phoenix PCIe stack plus the existing
  Circle reference for BCM2711 host-bridge initialization

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-bcm2711-indexed-cfg-backend.md`

## Notes

- Risks:
  do not jump straight into xHCI or overclaim that config-space access alone
  implies a working link
- Dependencies:
  completed `STEP-0314` compile-only BCM2711 indexed config-space backend
- User-visible control point before next step:
  after this step lands, the repo should clearly show the exact remaining
  smallest host-bridge move and why it comes before xHCI

Current scope finding:

- the new backend now supplies only config-space access mechanics
- the remaining gap is BCM2711 host-bridge initialization and link bring-up
  before downstream enumeration can be treated as meaningful
- Pi 4 `raspi4b` QEMU is still not expected to validate that real PCIe step
