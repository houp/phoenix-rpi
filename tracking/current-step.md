# Current Step

## Metadata

- Step ID: `STEP-0494`
- Title: Await the next Pi 4 retry on the late-seam UART-reset image
- Status: `ready`
- Date: `2026-04-17`
- Milestone / phase: `Phase 1`

## Objective

- run one new real-device Pi 4 retry on the narrower late-seam image
- classify the late `plo` -> kernel handoff from the reduced `6..11` GPIO42 map
- verify whether the earliest kernel UART reset restores readable post-handoff
  serial output

## Scope

In scope:
- one new real-device retry on the refreshed image
- HDMI observation
- GPIO42 / ACT LED pulse observation or recording
- UART capture if any output remains visible after the kernel-side `115200`
  reset

Out of scope:
- deeper userspace rollback before seeing the new boundary
- unrelated pre-`plo` boot-model changes
- another whole-boot LED protocol

## Acceptance Criteria

- the refreshed image is tried on real hardware
- the retry identifies the highest completed checkpoint among the documented
  late-seam pulse groups
- the retry also shows whether readable UART resumes at the new earliest kernel
  reset point
- the next engineering step can target one narrow seam instead of the whole
  post-`plo` band

## Validation Plan

- flash `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- observe:
  - HDMI output
  - GPIO42 / ACT LED pulse groups
  - any UART output after the kernel-side `115200` reset
- if possible, record a close LED video for later analysis

## Rollback / Baseline

- current bounded diagnostic image:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  (SHA-256: `405396dbd5328393223787288d832cea98ca28c417eacc8b1cbea72d316760a9`)
- prior restored-clock baseline:
  `60e0aac62028e25c6f409839103e9cc500231855b8542eb579ea29db4f7e2fd7`
- this step recorded in:
  `manifests/2026-04-17-pi4-late-handoff-uart-reset-and-led-rescope.md`

## Notes

- prior image analysis with the corrected count-based interpreter suggests the
  broad `1..10` post-panel map was too perturbing and too noisy to decode
  reliably
- the current image keeps only the late seam:
  - `6` `video_markKernelJump()` entry
  - `7` `video_markKernelJump()` draw complete
  - `8` just before `hal_exitToEL1()`
  - `9` kernel `_start` after earliest UART reinit
  - `10` kernel `_hal_init()` entry
  - `11` kernel `main()` after `_hal_init()`
- earliest kernel `_start` now reprograms PL011 to `115200` before the first
  kernel UART breadcrumb on the Pi 4 `48 MHz` lane
- QEMU remains non-regression green on this image:
  - Pi 4 shell smoke: pass
  - Pi 4 HDMI smoke: pass
