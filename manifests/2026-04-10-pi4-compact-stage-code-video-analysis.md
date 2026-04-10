# 2026-04-10: Pi 4 compact stage-code video analysis

## Summary

The first real-board retry on the compact stage-code image produced a much more
decodable LED trace than the older pulse-group protocol. The later ACT bursts
cleanly decode as stages `1`, `2`, and `3`, while no later stage-`4` burst is
visible. That moves the active failure band back from the `currentEL` seam to
the armstub-to-`plo` handoff itself.

## Input

Video:

- `/Users/witoldbolt/Downloads/IMG_7135.mov`

`ffprobe` result:

- width `294`
- height `156`
- `r_frame_rate=60000/1001`
- duration about `30.21s`

## Decoded Result

High-confidence later stage-code bursts:

- stage `1` / `00001`:
  - sync around `8.44s - 8.79s`
  - data bits around `9.36s - 10.79s`
- stage `2` / `00010`:
  - sync around `11.28s - 11.61s`
  - data bits around `12.18s - 13.61s`
- stage `3` / `00011`:
  - sync around `14.10s - 14.43s`
  - data bits around `15.00s - 16.53s`

No later stage-`4` burst was detected after stage `3`.

There is earlier green activity at about `1.95s - 7.42s`, but it does not fit
the current sync-plus-`5`-bit protocol and is therefore treated as pre-telemetry
firmware / SD-media activity, not as valid decoded Phoenix stage output.

## Interpretation

Current strongest inference:

- armstub primary-core entry is reached
- armstub timer / GIC preparation is reached
- armstub reaches the final fixed-address pre-`plo` branch point
- earliest generic AArch64 `plo` stage `4` is not observed

So the active failure band is now:

- after armstub stage `3`
- before earliest generic `plo` stage `4`

## Next Step

The next bounded experiment should no longer focus on `currentEL`.
It should split the stage `3 -> 4` handoff itself and answer whether:

- the fixed-address branch target is never entered
- generic `plo _start` is entered but fails before its first emitter runs
- or a reset / re-entry hides stage `4`
