# Pi 4 IMG_7138 Signature-Check Video Analysis

Date: `2026-04-10`

## Input

- video:
  `/Users/witoldbolt/Downloads/IMG_7138.mov`
- active image:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- active stage layout:
  `/Users/witoldbolt/phoenix-rpi/scripts/rpi4_actled_probe_layout.py`

## Toolchain

- analyzer:
  `/Users/witoldbolt/phoenix-rpi/scripts/analyze-rpi4-actled-video.py`
- interpreter:
  `/Users/witoldbolt/phoenix-rpi/scripts/interpret-rpi4-actled-analysis.py`

## Result

- `ffprobe`:
  - effective frame rate about `59.92 fps`
  - duration about `39.9s`
- analyzer ROI:
  - `x1=198`
  - `y1=93`
  - `x2=221`
  - `y2=114`
- best contiguous Phoenix run:
  - stage `3` / `00011`
  - label:
    `armstub before fixed jump`
- next expected stage not seen:
  - stage `4` / `00100`
  - label:
    `armstub target signature ok`
- also not seen:
  - stage `31` / `11111`
  - label:
    `armstub target signature bad`
- unmatched decoded groups:
  - stage `16`
  - stage `26`
  - treated as noise, not the main contiguous Phoenix run

## Inference

- the fixed-target-signature image moved the diagnostic seam correctly
- the board still reaches stage `3`
- the board still does not emit any completed signature-check outcome
- the next smallest useful code change is not later `plo` telemetry
- the next smallest useful code change is a finer armstub split between:
  - stage `3`
  - first fixed-target address / memory read
  - second signature-word read
  - existing stage `31` mismatch halt
