# 2026-04-17 - Pi 4 remove temporary post-MMU UART probes

## Trigger

The real-board UART log
`/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260417-233128.log`
still ended at:

- `A2`
- `KLM`
- `X1`
- `X2`
- `X3`

even after the Linux-style pre-MMU page-table invalidation fix.

At that point, the first post-MMU operations on the active hardware path were
still temporary TTBR1-based PL011 MMIO probes used only for diagnosis.

## Decision

Remove the temporary high-half PL011 debug path entirely and let the kernel
continue toward the normal console path without that early MMIO seam.

## Applied change

In
`/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-kernel/hal/aarch64/_init.S`:

- removed the `uart_putc_virt` macro
- removed the temporary device-TTL3 entry for `PL011_TTY_EARLY_VADDR`
- removed the post-MMU breadcrumbs that emitted:
  - `N`
  - `O`
  - `P`
  - `Q`
  - `R`

In
`/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-kernel/main.c`:

- removed `main_earlyUartPutch('S')`
- removed the associated temporary `PL011_TTY_EARLY_VADDR` helper

In
`/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/board_config.h`:

- removed `PL011_TTY_EARLY_VADDR`

## Why this is stronger

- Linux's early arm64 switch path does not rely on temporary high-half MMIO
  writes in the first instructions after MMU enable
- the public-source cross-check did not justify that temporary post-MMU PL011
  path as a Pi 4 requirement
- if that probe seam was the hardware-only fault, keeping it would continue to
  block the real boot for the sake of observability

## Validation

- `./scripts/rebuild-rpi4b-fast.sh --scope core --qemu-sanity`: pass
- `./scripts/qemu-shell-smoke.sh rpi4b`: pass
- canonical export: pass
- FAT-aware verify: pass

## Warning surfaced

The broad `--qemu-sanity` tail still only showed `A3 / KLM`, while the explicit
Pi 4 shell smoke still reached `(psh)%`. Continue treating the explicit shell
smoke as the stronger QEMU runtime signal.

## Exported image

- source commits:
  - `phoenix-rtos-kernel`: `abcb82ad`
  - `phoenix-rtos-project`: `277c793`

- path:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- SHA-256:
  `358f4325dec6009e0b9441c052dad370b2fedeb81a6f93eb43db1eadd06f750a`

## Next expected hardware result

The next real-board UART retry should show whether the kernel now proceeds
beyond the old temporary `X1 / X2 / X3` seam into normal console output such as:

- `console: pl011 init done`
- `main: hal init done`
- the kernel banner
