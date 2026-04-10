# 2026-04-10: Pi 4 compact stage-code `currentEL` split

## Summary

`IMG_0009.mov` most strongly suggested that the real Pi 4 completes the first
two register-clear checkpoints and then dies before the old post-`currentEL`
marker. The bounded response in this step was to both densify the ACT-LED
protocol and split the `currentEL` seam more aggressively, so the next single
video can localize the failure without another immediate probe redesign.

## Implemented Change

Repositories:

- `phoenix-rtos-project`
- `plo`

Files:

- `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/board_config.h`
- `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S`
- `sources/plo/hal/aarch64/generic/_init.S`

The old count-based GPIO42 protocol was replaced with a compact stage-code
format:

- one sync pulse
- then `5` fixed-width bits, MSB first
- short on-time = `0`
- long on-time = `1`
- one longer off gap between stage bursts

Updated checkpoint map:

- `1` / `00001`: armstub primary-core entry
- `2` / `00010`: armstub after early timer / GIC preparation
- `3` / `00011`: armstub just before the fixed-address jump to `plo`
- `4` / `00100`: earliest generic AArch64 `plo` `_start`
- `5` / `00101`: after clearing `x0..x7`
- `6` / `00110`: after clearing `x8..x15`
- `7` / `00111`: after clearing `x16..x23`
- `8` / `01000`: after clearing `x24..x30`
- `9` / `01001`: after `dsb sy` / `isb`
- `10` / `01010`: after `mrs currentEL`
- `11` / `01011`: `start_el3`
- `12` / `01100`: `start_el2`
- `13` / `01101`: `start_el1`
- `14` / `01110`: EL3 path complete, before `start_common`
- `15` / `01111`: EL2 path complete, before `start_common`
- `16` / `10000`: EL1 path complete, before `start_common`
- `17` / `10001`: `start_common`
- `18` / `10010`: after stack setup
- `19` / `10011`: core-0 branch to `_startc`
- `20` / `10100`: unexpected-EL trap path

The earliest generic AArch64 `plo` path also now inserts:

- `dsb sy`
- `isb`

immediately before:

- `mrs currentEL`

This directly tests the narrowest plausible fix from `gemini-findings.md`
without broadening the experiment beyond the current failure band.

## Validation

Build:

- rebuilt Pi 4 A72 image in `phoenix-dev`: pass
- rebuilt generic AArch64 QEMU image in `phoenix-dev`: pass

Emulator:

- generic QEMU shell smoke: pass
- direct Pi 4 QEMU serial sanity on the real-device build still reaches:
  - `call: exec go!`
  - `go: enter`
  - `hal: jump exit el1`
  - `A3`
  - `KLM`
  - later `Exception #37`

Artifact:

- assembled bootfs: pass
- assembled FAT image: pass
- assembled SD image: pass
- exported SD image through canonical helper: pass
- FAT-aware host verifier: pass after refreshing the pinned expected SHA

## Current Artifact

- path:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- SHA-256:
  `cada5a0cf3c5ce41a2197cc4296e81ed43b6b671d878660e3e303e16098ab60c`

## Commits

- `phoenix-rtos-project`: `06179be`
- `plo`: `5d0daae`

## Next Step

Run the next real Pi 4 board retry on this refreshed image and decode the
highest completed stage burst from one high-framerate LED video. The intended
next decision is whether the failure is still pre-`currentEL`, directly on the
`currentEL` seam, in a specific EL-path body, or later in `start_common`.
