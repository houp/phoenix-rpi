# 2026-04-17: Pi 4 pre-MMU syspage copy fix

## Trigger

- real-board UART log:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260417-215745.log`
- unchanged raw tail:
  - `A2`
  - `KLM`
  - `X1`
  - `X2`
  - `X3`
  - `NO`

## Interpretation

- the earlier byte-tail syspage-copy fix did not move the live hardware
  boundary
- the board still survives MMU enable and reaches `_core_0_virtual`
- the remaining fragile step is the whole post-MMU syspage copy / syspage
  variable initialization block in
  `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-kernel/hal/aarch64/_init.S`

## Change

- moved syspage copy to the pre-MMU physical phase in:
  `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-kernel/hal/aarch64/_init.S`
- initialized `hal_syspage` and `relOffs` before the virtual jump, using the
  virtual `VADDR_SYSPAGE` value and physical addresses of the globals
- enlarged `_hal_syspageCopied` from `SIZE_PAGE` to `16 * SIZE_PAGE`

## Why this fix

- it removes the live fault candidate from the fragile post-MMU seam entirely
- it matches the simpler syspage-materialization pattern used by the older ARM
  and RISC-V ports
- it avoids relying on a one-page syspage backing buffer

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
  `5d6f4bba3786543db10132cf2febf1ebdd37d819e780d795e611bdc141bb422e`

## Source commit

- `phoenix-rtos-kernel`: `c0808a43`
