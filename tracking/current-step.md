# Current Step

## Metadata

- Step ID: `STEP-0317`
- Title: Scope the smallest BCM2711 link-bring-up step
- Status: `in_progress`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest BCM2711 link-bring-up slice that should follow the new
  early host-bridge preparation hook without widening into xHCI or claiming
  downstream enumeration

## Scope

In scope:

- reviewing the remaining gap between the new early BCM2711 host-bridge
  preparation hook and a meaningful link-up state
- selecting the smallest next slice that can follow without widening into xHCI
  or downstream enumeration
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

- the next link-bring-up step is explicitly bounded and justified against the
  current Phoenix and Circle references
- the next step does not widen into xHCI or claim live downstream enumeration
- the tracking docs clearly state the exact remaining gap after the new
  early host-bridge preparation hook

## Validation Plan

- source inspection against the current Phoenix PCIe stack plus the existing
  Circle reference for BCM2711 link bring-up

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-bcm2711-host-init-prepare.md`

## Notes

- Risks:
  do not jump straight into xHCI or overclaim that config-space access alone
  or early host-bridge prep implies a working link
- Dependencies:
  completed `STEP-0316` BCM2711 host-bridge preparation hook
- User-visible control point before next step:
  after this step lands, the repo should clearly show the exact smallest
  link-bring-up move and why it comes before xHCI

Current scope finding:

- the BCM2711 backend now supplies config-space access plus early reset and
  `MISC_CTRL` preparation
- the remaining gap is now narrower:
  outbound-window setup, PERST release, link-up checks, and RC-mode
  verification before downstream enumeration can be treated as meaningful
- Pi 4 `raspi4b` QEMU is still not expected to validate that real PCIe step
