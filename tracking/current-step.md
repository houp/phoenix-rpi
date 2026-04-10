# Current Step

## Metadata

- Step ID: `STEP-0449`
- Title: Await the next Pi 4 board retry on the compact stage-code telemetry image
- Status: `in_progress`
- Date: `2026-04-10`
- Milestone / phase: `Phase 1`

## Objective

- run the next real Pi 4 retry on the refreshed compact stage-code image
- capture one high-quality LED video that can identify the highest completed
  early-boot checkpoint without another immediate probe redesign
- use that video to distinguish failure:
  - before `currentEL`
  - after `currentEL` but before EL-path selection
  - in a specific EL path
  - or later in `start_common` / core-0 startup

## Scope

In scope:

- flashing the current exported SD image
- recording a close ACT-LED video at `60 fps` or better when possible
- decoding the highest completed telemetry stage from the new compact format

Out of scope:

- new code changes before the next video is analyzed
- unrelated USB, framebuffer, DTB, or later-runtime work

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- `/Users/witoldbolt/phoenix-rpi/docs/pi4-first-hardware-trial.md`
- `/Users/witoldbolt/phoenix-rpi/docs/manual-operator-instructions.md`
- `/Users/witoldbolt/phoenix-rpi/tracking/current-step.md`

## Acceptance Criteria

- the operator flashes the refreshed image
- the next video is long enough to contain several stage-code bursts
- the next analysis can identify the highest completed stage with materially
  better confidence than the old count-based pulse protocol

## Validation Plan

- current code/image baseline already validated:
  - Pi 4 A72 rebuild: pass
  - generic shell smoke: pass
  - direct Pi 4 QEMU serial sanity: pass
  - canonical SD-image export: pass
  - FAT-aware verifier: pass

## Rollback / Baseline

- Known-good manifest or commit set:
  `/Users/witoldbolt/phoenix-rpi/manifests/2026-04-10-pi4-compact-stage-code-currentel-split.md`

## Notes

- `IMG_0009.mov` was the key trigger for the redesign:
  it most strongly fit completion through stage `6`, so the next split was
  centered on `mrs currentEL`.
- `gemini-findings.md` influenced this step in a narrow way:
  - `dsb sy` / `isb` before `mrs currentEL` was adopted
  - preserving `x0` remains plausible later but is not the tightest current
    explanation for this earliest `plo` band
- Current compact telemetry protocol:
  - one sync pulse starts each stage burst
  - then `5` fixed-width bits are emitted MSB-first
  - short on-time encodes `0`
  - long on-time encodes `1`
  - a longer off gap separates stage bursts
- Current checkpoint map:
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
  - `14` / `01110`: EL3 path complete, just before `start_common`
  - `15` / `01111`: EL2 path complete, just before `start_common`
  - `16` / `10000`: EL1 path complete, just before `start_common`
  - `17` / `10001`: `start_common`
  - `18` / `10010`: after stack initialization
  - `19` / `10011`: core-0 branch to `_startc`
  - `20` / `10100`: unexpected-EL trap path
- Current refreshed exported image:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- Current validated SHA-256:
  `cada5a0cf3c5ce41a2197cc4296e81ed43b6b671d878660e3e303e16098ab60c`
