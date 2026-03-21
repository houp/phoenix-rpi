# Current Step

## Metadata

- Step ID: `STEP-0329`
- Title: Scope the first compile-only Pi 4 xHCI HCD skeleton and discovery stub
- Status: `in_progress`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- define the first compile-only Pi 4 xHCI HCD skeleton slice and discovery
  stub that should follow the new VL805 fast-path constants without widening
  into staged runtime USB support

## Scope

In scope:

- reviewing the remaining gap between the new Pi 4 VL805 fast-path constants
  and the first compile-valid xHCI code
- selecting the smallest xHCI slice that can compile without forcing the live
  Pi 4 image to stage an unready USB host binary
- preserving the current HDMI text-console baseline and the deferred SD export

Out of scope:

- SD-image export or checksum refresh
- manual hardware execution
- broad framebuffer-console redesign
- changes to the existing `usbkbd` or `pl011-tty` logic
- broad BCM2711 PCIe host-bridge redesign
- staged runtime USB host support
- USB enumeration or keyboard end-to-end validation

## Expected Repositories

- coordination repo
- `phoenix-rtos-devices`

## Expected Files Or Subsystems

- `sources/phoenix-rtos-devices/usb/`
- `docs/status.md`
- `docs/source-artifacts.md`
- `tracking/current-step.md`
- `tracking/step-history.md`
- `manifests/`

## Acceptance Criteria

- the first xHCI skeleton step is explicitly bounded and justified against the
  current Phoenix USB stack plus the current Circle Pi 4 xHCI reference
- the next step does not stage an unready USB runtime path on the live Pi 4
  image
- the tracking docs clearly state the exact remaining gap after the new VL805
  fast-path constants

## Validation Plan

- source inspection against the current Phoenix USB HCD model plus the existing
  Circle Pi 4 xHCI reference

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-rpi4b-vl805-fastpath-constants.md`

## Notes

- Risks:
  do not stage a USB host binary that would fail the current Pi 4 boot path
  just because the first xHCI code compiles
- Dependencies:
  completed `STEP-0328` Pi 4 VL805 fast-path constants
- User-visible control point before next step:
  after this step lands, the repo should clearly show the exact smallest
  xHCI skeleton move and why it comes before staged runtime USB support

Current scope finding:

- the Pi 4 board config now carries the first VL805 fast-path assumptions taken
  from Circle:
  `bus 1 / slot 0 / func 0`, class code `0x0c0330`, and MMIO through the
  outbound PCIe window
- the current Phoenix USB stack already provides the generic HCD, hub,
  enumeration, and keyboard-driver layers
- the remaining gap is now narrower:
  the first compile-valid xHCI HCD skeleton and discovery stub before any
  staged runtime USB host path can be treated as meaningful
- Pi 4 `raspi4b` QEMU is still not expected to validate that real xHCI path
