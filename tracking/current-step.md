# Current Step

## Metadata

- Step ID: `STEP-0452`
- Title: Implement a dedicated Pi 4 entry trampoline at the stage-`3 -> 4` seam
- Status: `in_progress`
- Date: `2026-04-10`
- Milestone / phase: `Phase 1`

## Objective

- respond to the second clean OpenCV decode that still shows only stages `1`,
  `2`, and `3` on the handoff-hardened image
- split the current failure more aggressively than “armstub branch vs generic
  `_start`” by introducing a dedicated fixed-address Pi 4 entry trampoline
- distinguish:
  - failing before the fixed-address branch target is fetched
  - reaching the fixed-address target veneer but not the existing generic
    `plo _start`
  - or reaching generic `plo` after a minimal trampoline succeeds

## Scope

In scope:

- introducing a dedicated first fetched instruction band at the Pi 4 branch
  target
- keeping GPIO42 telemetry but moving the first post-branch proof out of the
  current generic `_start` body
- rebuilding, exporting, and re-verifying the next SD image

Out of scope:

- broader EL-path, USB, framebuffer, or DTB work before the next handoff split

## Expected Repositories

- `phoenix-rtos-project`
- `plo`
- coordination repo

## Expected Files Or Subsystems

- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- `/Users/witoldbolt/phoenix-rpi/docs/pi4-first-hardware-trial.md`
- `/Users/witoldbolt/phoenix-rpi/docs/manual-operator-instructions.md`
- `/Users/witoldbolt/phoenix-rpi/scripts/analyze-rpi4-actled-video.py`
- `/Users/witoldbolt/phoenix-rpi/tracking/current-step.md`

## Acceptance Criteria

- the next image separates the raw branch target from the existing generic
  `_start` body
- the next video can answer whether the board reaches a dedicated fixed-address
  Pi 4 entry veneer
- the resulting analysis narrows the seam beyond the current “stage `3` yes,
  stage `4` no” result

## Validation Plan

- current analysis baseline:
  - `ffprobe` on `IMG_7136.mov`: `59.92 fps`
  - OpenCV decoder:
    [scripts/analyze-rpi4-actled-video.py](/Users/witoldbolt/phoenix-rpi/scripts/analyze-rpi4-actled-video.py)
  - clean decode still yields only:
    - stage `1`
    - stage `2`
    - stage `3`
- current code/image baseline already validated:
  - Pi 4 A72 rebuild: pass
  - generic AArch64 rebuild: pass
  - generic QEMU shell path still reaches runtime and `help`
  - direct Pi 4 QEMU serial sanity: pass
  - canonical export: pass
  - FAT-aware verifier: pass

## Rollback / Baseline

- Known-good manifest or commit set:
  `/Users/witoldbolt/phoenix-rpi/manifests/2026-04-10-pi4-stage34-handoff-hardening.md`

## Notes

- Both `IMG_7135.mov` and `IMG_7136.mov` now decode only stages `1`, `2`, and
  `3`.
- `IMG_7136.mov` was decoded with the new OpenCV helper and confirms the same
  boundary more robustly than the earlier manual pulse mapping.
- The current handoff-hardened image is still the rollback baseline:
  - preserve the primary armstub path argument registers
  - insert `dsb sy; ic iallu; dsb sy; isb` immediately before `br 0x40080000`
  - inline stage `4` at the first `_start` instruction in generic `plo`
- Current refreshed exported image:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- Current validated SHA-256:
  `4b9c967c9381e8935998a19eb1a976c43b440dd57da4c5fab489763f729a6835`
