# Current Step

## Metadata

- Step ID: `STEP-0327`
- Title: Scope the smallest first direct downstream endpoint-readback step
- Status: `in_progress`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest BCM2711 direct downstream endpoint-readback slice that
  should follow the new Pi 4 PCIe project-integration step without widening
  into full enumeration or xHCI

## Scope

In scope:

- reviewing the remaining gap between the new Pi 4 image integration state and
  a meaningful first downstream config-space readback
- selecting the smallest next slice that can follow without widening into full
  bus management, xHCI, or endpoint-driver work
- preserving the current HDMI text-console baseline and the deferred SD export

Out of scope:

- SD-image export or checksum refresh
- manual hardware execution
- broad framebuffer-console redesign
- changes to the existing `usbkbd` or `pl011-tty` logic
- broad BCM2711 PCIe host-bridge redesign
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

- the next downstream-readback step is explicitly bounded and justified against
  the current Phoenix and Circle references
- the next step does not widen into full enumeration, xHCI, or endpoint-driver
  work
- the tracking docs clearly state the exact remaining gap after the new
  bridge-capability step

## Validation Plan

- source inspection against the current Phoenix PCIe stack plus the existing
  Circle reference for BCM2711 bridge enablement and first downstream endpoint
  discovery

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-rpi4b-pcie-project-integration.md`

## Notes

- Risks:
  do not jump straight into xHCI or overclaim that one downstream readback
  implies robust enumeration
- Dependencies:
  completed `STEP-0326` Pi 4 PCIe project integration
- User-visible control point before next step:
  after this step lands, the repo should clearly show the exact smallest
  downstream-readback move and why it comes before xHCI

Current scope finding:

- the BCM2711 backend now supplies config-space access, early host-bridge prep,
  link-state gating, outbound-window setup, root-bridge shaping, and
  bridge-side memory-window / bus exposure
- the BCM2711 backend now also mirrors the remaining Circle `enable_bridge()`
  capability-state slice:
  root-bridge parity and PCIe root-control CRS software visibility
- the Pi 4 image path now also stages the `pcie` server itself, so the next
  remaining gap can be defined against a real runtime component instead of an
  unreferenced server binary
- the remaining gap is now narrower:
  the first bounded downstream config-space readback before any xHCI-specific
  work can be treated as meaningful
- Pi 4 `raspi4b` QEMU is still not expected to validate that real PCIe step
