# 2026-04-17 - Pi 4 UART marker rename X3 -> 3C

## Trigger

Before the next real-board retry, the late pre-MMU marker `X3` needed to be
changed to a distinct value so the next UART log can unambiguously confirm that
the board is running the refreshed image rather than an older one.

## Applied change

In
`/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-kernel/hal/aarch64/_init.S`:

- changed the marker emitted by `uart_tag2` from:
  - `X3`
- to:
  - `3C`

No boot logic changed in this step.

## Validation

- `./scripts/rebuild-rpi4b-fast.sh --scope core --qemu-sanity`: pass
- canonical export: pass
- FAT-aware verify: pass

## Warning surfaced

The broad `--qemu-sanity` captured tail still only showed `A3 / KLM`; the
explicit Pi 4 shell smoke was not rerun in this step because the change only
renamed an early UART marker and did not alter control flow.

## Source commit

- `phoenix-rtos-kernel`: `5dc1990d`

## Exported image

- path:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- SHA-256:
  `bc08128b86c3d7b22cbd1160b81281d0ef5849c34c88f962b3cadfad29aa559d`
