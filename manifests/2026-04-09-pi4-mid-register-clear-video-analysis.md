# Pi 4 Mid-Register-Clear Video Analysis

Date: 2026-04-09

## Summary

The new real-device hardware clip `IMG_0009.mov` narrows the current earliest
generic AArch64 `plo` failure band further than the previous `IMG_0005.mov`
result.

`ffprobe` confirms the clip is actually `59.93 fps`, so it is suitable for
frame-level timing.

The ACT LED signal was extracted from a tight left-side crop of the LED-only
video. The highest-confidence green-on windows are:

- `1.985s - 2.002s`
- `2.203s - 7.392s`
- `8.360s - 8.760s`
- `9.562s - 9.978s`
- `11.597s - 11.997s`
- `12.798s - 12.848s` weak / near-threshold
- `15.668s - 16.068s`
- `16.870s - 17.287s`
- `18.088s - 18.488s`

No later ACT activity was detected after about `18.49s`.

## Interpretation

The earliest armstub activity is still partially merged by camera exposure and
LED persistence, so the clip does not provide a literal one-pulse-per-checkpoint
decode from stage `1`.

However, the later pulse envelopes fit the current `1..13` checkpoint map more
strongly than the previous board retry:

- stage `4` is still clearly reached
- the later grouped activity now fits completion through stage `6` better than
  a halt before stage `5`
- no convincing later group appears where stage `7` should start

The strongest current conclusion is therefore:

- the board most likely completes stage `6`
  (`end of general-purpose register clearing`)
- the failure is now most likely between:
  - the end of stage `6`
  - and the existing stage `7` marker
    (`after currentEL sampling, before EL dispatch`)

This makes the next bounded step clear:

- split the `stage 6 -> stage 7` boundary directly around `mrs currentEL`
- do not widen into deeper EL-path changes yet

## Evidence Handling Notes

- The clip is more reliable than `IMG_0005.mov` because it is truly `59.93 fps`.
- The interpretation above is still an inference from grouped LED envelopes, not
  a literal human-readable pulse count for every early stage.
- That remaining ambiguity is why the next move should be another narrow split
  around `currentEL`, not a broader rewrite.
