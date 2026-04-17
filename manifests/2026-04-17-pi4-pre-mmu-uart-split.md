# Pi 4 Pre-MMU UART Split

Date: 2026-04-17

## Summary

The new real-board UART log
`artifacts/rpi4b-uart/rpi4b-uart-20260417-213033.log` did not move beyond the
previous boundary. The raw tail is still exactly:

- `A2`
- `KLM`

and contains no post-MMU breadcrumbs.

That means the previously added post-MMU markers `N..S` were too late for the
active fault band. The live failure is therefore still between kernel breadcrumb
`M` and the first point where post-MMU UART becomes usable.

## Fix Applied

Added three earlier physical-UART breadcrumbs in
`phoenix-rtos-kernel/hal/aarch64/_init.S`:

- `X1`: immediately before MMU setup begins
- `X2`: after `ttbr0_el1` is programmed
- `X3`: immediately before `msr sctlr_el1, x0`

The previously added post-MMU breadcrumbs remain in place:

- `N`: first safe post-MMU UART point
- `O`: `_core_0_virtual`
- `P`: after syspage copy
- `Q`: after `_set_up_vbar_and_stacks`
- `R`: immediately before `b main`
- `S`: first instruction of `main()`

## Validation

- `./scripts/rebuild-rpi4b-fast.sh --scope core --qemu-sanity`: pass
- canonical export: pass
- FAT-aware verify: pass

## Exported Image

- Path:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- SHA-256:
  `fe9d163ab5d23aa88bbaea35c5df790f48b76dea58d1cd843b8cd56990c74273`

## Expected Next Real-Board UART Outcomes

- `... KLM` only:
  dies before the first pre-MMU split point
- `... KLM\nX1\n` only:
  dies during early MMU/page-table setup before `ttbr0_el1`
- `... KLM\nX1\nX2\n` only:
  dies after `ttbr0_el1` programming and before actual MMU enable
- `... KLM\nX1\nX2\nX3\n` only:
  dies on or immediately after `msr sctlr_el1, x0`
- `... X3` plus no `N`:
  MMU enable survives but post-MMU UART still does not
- `... N/O/P/Q/R/S`:
  boundary moves into the already-split post-MMU path
