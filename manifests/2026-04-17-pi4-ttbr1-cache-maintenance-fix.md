# 2026-04-17: Pi 4 TTBR1 cache-maintenance fix

## Trigger

- real-board UART log:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260417-222617.log`
- raw tail still ends at:
  - `A2`
  - `KLM`
  - `X1`
  - `X2`
  - `X3`

## Interpretation

- the older `NO` state was real and disappeared only after we kept iterating on
  raw post-`ttbr1` UART probes
- `docs/status.md` already contained a negative result that post-`ttbr1` raw
  UART probes are not valid on this path and regress the boot until reverted
- the next stronger move was therefore to remove that invalid instrumentation
  and patch the real hardware-sensitive TTBR1 transition instead

## Change

- removed all raw post-`ttbr1` UART probes from:
  `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-kernel/hal/aarch64/_init.S`
- removed the matching stale early-UART debug helper from:
  `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-kernel/main.c`
- removed the now-unused board-config define from:
  `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/board_config.h`
- added explicit TTBR1 page-table visibility maintenance in:
  `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-kernel/hal/aarch64/_init.S`
  - clean D-cache over `PMAP_COMMON_KERNEL_TTL2 .. PMAP_COMMON_DEVICES_TTL3`
  - `tlbi vmalle1is`
  - then enable the `ttbr1` path

## Validation

- `./scripts/rebuild-rpi4b-fast.sh --scope core --qemu-sanity`
- canonical export via:
  `/Users/witoldbolt/phoenix-rpi/scripts/export-rpi4b-sdimg.sh`
- FAT-aware verify via:
  `/Users/witoldbolt/phoenix-rpi/scripts/verify-rpi4b-sdimg.sh`

## Resulting image

- path:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- SHA-256:
  `725744d23cbd7bf08080c52ec02230269f680cd554ffc8c3d23a27b31f30ec2c`

## Source commits

- `phoenix-rtos-kernel`: `54550776`
- `phoenix-rtos-project`: `925a834`
