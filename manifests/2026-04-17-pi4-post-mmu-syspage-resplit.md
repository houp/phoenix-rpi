# 2026-04-17: Pi 4 post-MMU syspage seam re-split

## Trigger

- real-board UART log:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260417-220842.log`
- raw tail regressed to:
  - `A2`
  - `KLM`
  - `X1`
  - `X2`
  - `X3`

## Interpretation

- the previous semantic change that moved the syspage copy before the MMU jump
  made the live hardware boundary earlier
- the larger syspage backing buffer remains plausible, but the active fault
  still belongs to the original post-MMU `_core_0_virtual` copy seam

## Change

- restored the original post-MMU syspage copy in:
  `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-kernel/hal/aarch64/_init.S`
- kept `_hal_syspageCopied = 16 * SIZE_PAGE`
- added narrow UART breadcrumbs:
  - `U` after `relOffs` store
  - `V` after `hal_syspage` store
  - `W` after `syspage->size` load
  - `Z` before entering the first copy iteration
  - `Y` after the first 8-byte copy iteration
  - `P` after full copy completion

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
  `4d3d4860eaba47566e0d7c190b2809dc477d80ae8d63fb43b9adee923c742583`

## Source commit

- `phoenix-rtos-kernel`: `866fc206`
