# Current Step

## Metadata

- Step ID: `STEP-0492`
- Title: Await the next Pi 4 retry on the bounded post-panel LED diagnostic image
- Status: `ready`
- Date: `2026-04-17`
- Milestone / phase: `Phase 1`

## Objective

- run one new real-device retry on the bounded post-panel diagnostic image
- classify the current regression boundary from the new Pi 4-only GPIO42 pulse
  map around the HDMI panel and earliest kernel path
- use that result to choose the first real fix instead of widening back into
  blind userspace tracing

## Scope

In scope:
- one new real-device Pi 4 retry on the refreshed image
- HDMI observation
- LED pulse observation or recording
- UART capture if any output remains visible

Out of scope:
- broad userspace HDMI mirroring rollback before seeing the new boundary
- unrelated pre-`plo` boot-model changes

## Acceptance Criteria

- the refreshed image is tried on real hardware
- the retry identifies the highest completed checkpoint among the documented
  pulse groups
- the next engineering step can target one narrow seam instead of the whole
  post-`plo` band

## Validation Plan

- flash `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- observe:
  - HDMI output
  - GPIO42 / ACT LED pulse groups
  - any UART output
- if possible, record a close LED video for later analysis

## Rollback / Baseline

- bounded diagnostic image:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  (SHA-256: `06c3756584acd2a06f9143caece9fc29b93a61b6fcab84a439e19b0fc3e16868`)
- prior restored-clock baseline:
  `60e0aac62028e25c6f409839103e9cc500231855b8542eb579ea29db4f7e2fd7`
- this step recorded in:
  `manifests/2026-04-17-pi4-post-panel-led-diagnostics.md`

## Notes

- current checkpoint map:
  - `1` `video_init()` entry
  - `2` framebuffer allocation complete
  - `3` initial brown-panel draw complete
  - `4` `video_markHalReady()` entry
  - `5` `video_markHalReady()` draw complete
  - `6` `video_markKernelJump()` entry
  - `7` `video_markKernelJump()` draw complete
  - `8` kernel `_start`
  - `9` kernel `_hal_init()` entry
  - `10` kernel `main()` after `_hal_init()`
- QEMU remains non-regression green on this image:
  - Pi 4 shell smoke: pass
  - Pi 4 HDMI smoke: pass
