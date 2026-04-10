# 2026-04-10 Pi 4 IMG_7136 OpenCV Analysis

## Scope

- analyze the new real-hardware clip `IMG_7136.mov`
- use a tighter, repeatable host-side decoder instead of ad hoc thresholding
- determine whether the current handoff-hardened image now emits stage `4`

## Inputs

- video:
  `/Users/witoldbolt/Downloads/IMG_7136.mov`
- current image:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- current image SHA-256:
  `4b9c967c9381e8935998a19eb1a976c43b440dd57da4c5fab489763f729a6835`

## Method

- verify clip cadence with `ffprobe`
- decode the ACT LED through a fixed ROI using the new script:
  `/Users/witoldbolt/phoenix-rpi/scripts/analyze-rpi4-actled-video.py`
- current ROI for the cropped LED clips:
  `92,108,117,118`

## Results

- `ffprobe` confirms:
  - `r_frame_rate=60000/1001`
  - `avg_frame_rate=15400/257`
  - `duration=25.700000`
- the OpenCV decoder classifies the ACT pulses as:
  - short: about `0.1138s`
  - long: about `0.2086s`
  - sync: about `0.3129s`
- valid decoded stage groups:
  - stage `1` / `00001`
  - stage `2` / `00010`
  - stage `3` / `00011`
- no valid stage `4` / `00100` group appears after stage `3`
- earlier green activity around `2.07s - 7.26s` does not fit the compact
  Phoenix protocol and is still treated as pre-telemetry firmware / media
  activity

## Conclusion

- the current handoff-hardened image still does not reach earliest generic
  `plo` stage `4` on real hardware
- the active failure band therefore remains the fixed-address handoff seam:
  armstub stage `3` reached, generic `plo` stage `4` not observed

## Next Step

- move the next proof out of the current generic `_start` body and into a
  dedicated Pi 4 fixed-address entry trampoline so the branch target itself can
  be distinguished from later generic `plo` startup
