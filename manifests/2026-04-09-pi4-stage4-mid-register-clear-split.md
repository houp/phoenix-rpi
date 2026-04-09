# 2026-04-09: Pi 4 mid-register-clear split

## Summary

`IMG_0005.mov` still most strongly indicated that the real Pi 4 reaches
checkpoint `4` (`plo _start`) but dies before the old post-register-clear
checkpoint `5`. The bounded response in this step was to split the current
general-purpose register-clearing block itself.

## Implemented Change

Repository:

- `plo`

File:

- `sources/plo/hal/aarch64/generic/_init.S`

Updated checkpoint map:

- `1`: armstub primary-core entry
- `2`: armstub after early timer / GIC preparation
- `3`: armstub just before the fixed-address jump to `plo`
- `4`: earliest generic AArch64 `plo` `_start`
- `5`: midpoint of general-purpose register clearing
- `6`: end of general-purpose register clearing
- `7`: after `currentEL` sampling, before EL dispatch
- `8`: `start_el3`
- `9`: `start_el2`
- `10`: `start_el1`
- `11`: `start_common`
- `12`: core-0 branch to `_startc`
- `13`: unexpected-EL trap path

The actual code change places the new midpoint marker immediately after clearing
`x0..x15`, then keeps the earlier end-of-clear marker after clearing `x16..x30`.

## Validation

Build:

- rebuilt Pi 4 A72 image in `phoenix-dev`: pass

Emulator:

- generic QEMU shell smoke: pass
- direct Pi 4 QEMU serial sanity on the real-device build still reaches:
  - `call: exec go!`
  - `go: enter`
  - `hal: jump exit el1`
  - `A3`
  - `KLM`
  - later `Exception #37`

Artifact:

- assembled bootfs: pass
- assembled FAT image: pass
- assembled SD image: pass
- exported SD image through canonical helper: pass
- FAT-aware host verifier: pass

## Current Artifact

- path:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- SHA-256:
  `03a0729254dc0bc81f542fe8db276f7a2b70d3fb76de9fc7303ea470aca83137`

## Next Step

Run the next real Pi 4 board retry on this refreshed image and use the new
`1..13` map to determine whether the failure is in the first half or second
half of the earliest `_start` register-clearing block.
