# 2026-04-10: Pi 4 fixed-entry trampoline at stage `3 -> 5` seam

## Summary

`IMG_7135.mov` and `IMG_7136.mov` both decoded only stages `1`, `2`, and `3`,
which kept the active failure band at the fixed-address armstub handoff into
generic `plo`. The bounded response in this step was to stop treating the raw
branch target and the old generic `_start` body as one checkpoint.

## Implemented Change

Repositories:

- `plo`

Files:

- `sources/plo/hal/aarch64/generic/_init.S`

The earliest generic AArch64 `plo` path now uses:

- stage `4` inline in a dedicated fixed-address veneer at `_start`
- a plain branch from `_start` to `_start_real`
- stage `5` inline at `_start_real`, before the old generic body

Later stages were shifted by `+1`:

- `6`: after clearing `x0..x7`
- `7`: after clearing `x8..x15`
- `8`: after clearing `x16..x23`
- `9`: after clearing `x24..x30`
- `10`: after `dsb sy` / `isb`
- `11`: after `mrs currentEL`
- `12`: `start_el3`
- `13`: `start_el2`
- `14`: `start_el1`
- `15`: EL3 path complete
- `16`: EL2 path complete
- `17`: EL1 path complete
- `18`: `start_common`
- `19`: after stack setup
- `20`: core-0 branch to `_startc`
- `21`: unexpected-EL trap path

## Validation

Build:

- refreshed copied buildroot in `phoenix-dev`: pass
- rebuilt Pi 4 A72 image in `phoenix-dev`: pass

Emulator:

- generic QEMU shell smoke: pass
- direct Pi 4 QEMU serial sanity on real-device build still reaches:
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
  `d76a6c2bb0d15173f4a6a90aa5c82211b0ea286b5bb236960e51fdd3388c2320`

## Commits

- `plo`: `4d8203a`

## Next Step

Run next Pi 4 board retry on this refreshed image and decode whether:

- stage `4` still does not appear
- stage `4` appears but stage `5` does not
- or both stages `4` and `5` appear
