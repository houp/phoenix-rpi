# Pi 4 Post-Panel LED Diagnostics

Date: `2026-04-17`

## Summary

After the restored-clock image still produced the same brown three-square HDMI
panel and no useful later UART, the next bounded step moved observability to a
very small Pi 4-only GPIO42 pulse map around the already proven late visible
boundary:

- `plo` video panel path
- kernel `_start`
- kernel `_hal_init()`
- kernel `main()` immediately after `_hal_init()`

The purpose of this image is not to keep broad legacy telemetry. It is to
classify the current regression into one narrow band.

## Touched Repositories

- `phoenix-rtos-project`
  - commit: `1e5b2bf`
  - `_projects/aarch64a72-generic-rpi4b/board_config.h`
- `plo`
  - commit: `b96c51a`
  - `hal/aarch64/generic/video.c`
- `phoenix-rtos-kernel`
  - commit: `bece576e`
  - `hal/aarch64/_init.S`
  - `hal/aarch64/hal.c`
  - `main.c`

## Checkpoint Map

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

## Validation

- `./scripts/rebuild-rpi4b-fast.sh --scope core --qemu-sanity`: pass
- Pi 4 shell smoke: pass
- Pi 4 HDMI smoke: pass
- canonical export: pass
- FAT-aware verify: pass

## Exported Image

- path:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- SHA-256:
  `06c3756584acd2a06f9143caece9fc29b93a61b6fcab84a439e19b0fc3e16868`

## Interpretation Rule

- highest `1..3` only:
  failure is still inside the early HDMI panel path
- reaches `7` but not `8`:
  failure is between `video_markKernelJump()` and kernel `_start`
- reaches `8` but not `9`:
  kernel dies before `_hal_init()`
- reaches `9` but not `10`:
  kernel dies inside `_hal_init()`
- reaches `10`:
  failure is later than early HAL init
