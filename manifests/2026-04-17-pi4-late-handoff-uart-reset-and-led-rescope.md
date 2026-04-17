# Pi 4 Late-Handoff UART Reset and LED Re-scope

Date: `2026-04-17`

## Summary

The previous broad post-panel LED image was too perturbing and too noisy.
After the host-side LED interpreter was corrected for the current simple
pulse-count protocol, the long clip `IMG_7161.mov` most strongly reached only
stage `6`, with no later stage `7`.

The next bounded image therefore:

- removed the early panel-path GPIO42 pulses
- kept only the late `plo` -> kernel seam
- restored readable UART at the first kernel instruction on the Pi 4 PL011 lane

## Touched Repositories

- `plo`
  - commit: `33bcb3a`
  - `hal/aarch64/generic/video.c`
  - `hal/aarch64/generic/hal.c`
- `phoenix-rtos-kernel`
  - commit: `1b99fa59`
  - `hal/aarch64/_init.S`
  - `hal/aarch64/hal.c`
  - `main.c`
- coordination repo
  - updated LED toolchain current layout to the count-based `6..11` map

## New Checkpoint Map

- `6` `video_markKernelJump()` entry
- `7` `video_markKernelJump()` draw complete
- `8` final pre-`hal_exitToEL1()` handoff
- `9` kernel `_start`
- `10` kernel `_hal_init()` entry
- `11` kernel `main()` after `_hal_init()`

## Additional Change

- earliest kernel `_start` now reprograms Pi 4 PL011 back to `115200` before
  the first kernel UART breadcrumb on the `48 MHz` PL011 lane

## Validation

- `./scripts/rebuild-rpi4b-fast.sh --scope core --qemu-sanity`: pass
- direct Pi 4 QEMU serial sanity still reaches:
  - `hal: jump exit el1`
  - `A3`
  - `KLMconsole: pl011 init done`
- canonical export: pass
- FAT-aware verify: pass
- `python3 -m py_compile` on LED-toolchain scripts: pass

## Exported Image

- path:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- SHA-256:
  `405396dbd5328393223787288d832cea98ca28c417eacc8b1cbea72d316760a9`

## Interpretation Rule

- highest `6` only:
  failure is inside or immediately after `video_markKernelJump()`
- reaches `7` but not `8`:
  failure is between final `plo` panel draw and the final EL1 handoff
- reaches `8` but not `9`:
  failure is between `hal_exitToEL1()` and first kernel instruction
- reaches `9` but not `10`:
  kernel dies before `_hal_init()`
- reaches `10` but not `11`:
  kernel dies inside `_hal_init()`
- reaches `11`:
  failure is later than early HAL init
