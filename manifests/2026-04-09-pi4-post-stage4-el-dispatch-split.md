# 2026-04-09: Pi 4 post-stage-4 EL-dispatch split

## Summary

The latest `IMG_0004.mov` `60 fps` hardware video was mapped onto the slower
GPIO42 telemetry image and most strongly suggested that the real Pi 4 completes
checkpoint `4` (`plo _start` entry) and then dies before the first EL-path
marker from the older `1..9` map. The bounded response in this step was to keep
the existing armstub and earliest-`plo` checkpoints intact and split only the
post-stage-`4` band inside generic AArch64 `plo _start`.

## Evidence From `IMG_0004.mov`

Highest-confidence ACT-LED green-on windows from the frame-by-frame review:

- `0.78s - 0.82s`
- `0.93s - 5.72s`
- `6.70s - 7.12s`
- `7.92s - 8.35s`
- `9.95s - 10.37s`
- `14.03s - 14.43s`
- `15.23s - 15.65s`
- `16.45s - 16.85s`

Interpretation used for this step:

- the older slower protocol most strongly fit completion through checkpoint `4`
- visible activity ending at about `16.85s` fit cumulative timing through stage
  `4` much better than through stage `6`
- therefore the next bounded split belonged inside `plo _start`, not in the
  armstub and not in later runtime code

## Implemented Change

Repository:

- `plo`

File:

- `sources/plo/hal/aarch64/generic/_init.S`

New checkpoint map:

- `1`: armstub primary-core entry
- `2`: armstub after early timer / GIC preparation
- `3`: armstub just before the fixed-address jump to `plo`
- `4`: earliest generic AArch64 `plo` `_start`
- `5`: after general-purpose register clearing
- `6`: after `currentEL` sampling, before EL dispatch
- `7`: `start_el3`
- `8`: `start_el2`
- `9`: `start_el1`
- `10`: `start_common`
- `11`: core-0 branch to `_startc`
- `12`: unexpected-EL trap path

The new code keeps the same slower video-decodable timing target:

- about `0.4s` LED on per pulse
- about `0.4s` LED off between pulses inside one group
- about `2.0s` LED off between groups

## Validation

Build:

- rebuilt Pi 4 A72 image in `phoenix-dev`: pass

Emulator:

- generic QEMU shell smoke: pass on rerun
- direct Pi 4 QEMU serial sanity on the real-device build still reaches:
  - `call: exec go!`
  - `go: enter`
  - `hal: jump exit el1`
  - `A3`
  - `KLM`
  - later `Exception #37`

Artifact:

- assembled bootfs image: pass
- assembled SD image: pass
- exported SD image through canonical helper: pass
- FAT-aware host verifier: pass

## Current Artifact

- path:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- SHA-256:
  `d1e0fd5b2e3817d4e0d2ad339b63be34fb96d17f2d8a05d4e318d52a02952c20`

## Next Step

The next bounded step is a real Pi 4 board retry on this refreshed image with a
high-framerate LED video long enough to decode the new `1..12` map and decide
whether the failure is before `currentEL` sampling, before EL dispatch, inside
the selected EL path, or later in `start_common`.
