# Current Step

## Metadata

- Step ID: `STEP-0453`
- Title: Await next Pi 4 board retry on dedicated fixed-entry-trampoline image
- Status: `in_progress`
- Date: `2026-04-10`
- Milestone / phase: `Phase 1`

## Objective

- run next real Pi 4 retry on dedicated fixed-entry-trampoline image
- determine whether new stage `4` and stage `5` split appears on real hardware
- distinguish:
  - failing before fixed-address branch target is fetched
  - reaching fixed-address target veneer but not old generic `_start` body
  - or reaching both veneer and generic body before dying later

## Scope

In scope:

- flashing refreshed image
- recording one new close ACT-LED video
- decoding whether stage `4` only, or stages `4` and `5`, now appear

Out of scope:

- broader EL-path, USB, framebuffer, or DTB work before next video

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- `/Users/witoldbolt/phoenix-rpi/docs/pi4-first-hardware-trial.md`
- `/Users/witoldbolt/phoenix-rpi/docs/manual-operator-instructions.md`
- `/Users/witoldbolt/phoenix-rpi/tracking/current-step.md`

## Acceptance Criteria

- operator flashes refreshed image
- next video is sufficient to answer whether stage `4` and stage `5` appear
- resulting analysis narrows seam beyond current “stage `3` yes, stage `4` no”
  result

## Validation Plan

- current code/image baseline validated:
  - Pi 4 A72 rebuild: pass
  - generic QEMU shell path still reaches runtime and `help`
  - direct Pi 4 QEMU serial sanity: pass
  - canonical export: pass
  - FAT-aware verifier: pass

## Rollback / Baseline

- Known-good manifest or commit set:
  `/Users/witoldbolt/phoenix-rpi/manifests/2026-04-10-pi4-fixed-entry-trampoline.md`

## Notes

- `IMG_7135.mov` and `IMG_7136.mov` both decoded only stages `1`, `2`, `3`.
- current image response is:
  - dedicated stage `4` inline at fixed-address branch target
  - dedicated stage `5` inline at first instruction of old generic `_start`
    body
  - later stages shifted by `+1`
- Current refreshed exported image:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- Current validated SHA-256:
  `d76a6c2bb0d15173f4a6a90aa5c82211b0ea286b5bb236960e51fdd3388c2320`
