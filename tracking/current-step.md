# Current Step

## Metadata

- Step ID: `STEP-0450`
- Title: Split the Pi 4 failure between armstub stage `3` and earliest `plo` stage `4`
- Status: `in_progress`
- Date: `2026-04-10`
- Milestone / phase: `Phase 1`

## Objective

- explain why the compact stage-code image reaches armstub stage `3` but does
  not emit earliest generic `plo` stage `4`
- choose the smallest next experiment that distinguishes:
  - failure before the fixed-address branch target is entered
  - failure at the first instructions of generic `plo _start`
  - reset or re-entry that hides stage `4`

## Scope

In scope:

- analysis of the compact stage-code video result
- the next bounded early-handoff experiment between stage `3` and stage `4`
- rebuilding and re-exporting the Pi 4 image after that experiment

Out of scope:

- unrelated `currentEL`, EL-path, USB, framebuffer, DTB, or later-runtime work

## Expected Repositories

- `sources/plo`
- `sources/phoenix-rtos-project`
- coordination repo

## Expected Files Or Subsystems

- `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S`
- `/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S`
- `/Users/witoldbolt/phoenix-rpi/docs/status.md`
- `/Users/witoldbolt/phoenix-rpi/tracking/current-step.md`

## Acceptance Criteria

- the current compact-stage video result is preserved in the knowledge base
- the next experiment is explicitly centered on the stage `3 -> 4` seam
- the next image should answer whether generic `plo` is entered at all

## Validation Plan

- use the decoded `IMG_7135.mov` result as the current hardware baseline
- preserve the usual rebuild, export, and FAT-aware verification flow for the
  next image

## Rollback / Baseline

- Known-good manifest or commit set:
  `/Users/witoldbolt/phoenix-rpi/manifests/2026-04-10-pi4-compact-stage-code-currentel-split.md`

## Notes

- `IMG_7135.mov` is genuinely high-framerate:
  `ffprobe` reports `60000/1001` nominal rate and about `30.21s` duration.
- The later ACT windows decode cleanly as compact stage-code bursts:
  - around `8.44s - 10.79s`: stage `1` / `00001`
  - around `11.28s - 13.61s`: stage `2` / `00010`
  - around `14.10s - 16.53s`: stage `3` / `00011`
- No later stage-`4` sync burst is visible after stage `3`.
- There is earlier green activity at about `1.95s - 7.42s`, but it does not
  fit the sync-plus-`5`-bit structure and is therefore treated as pre-telemetry
  firmware / media activity, not as a valid decoded Phoenix stage.
- The current strongest interpretation is now:
  - armstub primary-core entry is reached
  - armstub timer / GIC prep is reached
  - armstub reaches the final fixed-address pre-`plo` branch point
  - earliest generic `plo` stage `4` is not observed
  - so the active failure band moves back from `currentEL` to the stage
    `3 -> 4` handoff itself
