# Pi 4 Fixed-Target Signature Check

Date: `2026-04-10`

## Scope

- verify from the Pi 4 custom armstub that the expected `plo` entry bytes are
  actually present at `0x40080000` before the fixed-address branch
- preserve the current LED-analysis toolchain as the authoritative board-video
  decode path

## Implemented

- `phoenix-rtos-project`
  - `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/board_config.h`
    now defines the fixed-entry signature constants
  - `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S`
    now:
    - keeps the fixed-header slots intact by branching over them at `_start`
    - verifies the deliberate `plo` signature at `0x40080000 + 0x4`
    - emits stage `4` on signature match
    - emits stage `31` and halts on signature mismatch
  - upstream commit:
    - `8756d41`
- `plo`
  - `/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S`
    now places the deliberate entry signature near the fixed entry target and
    shifts the dedicated fixed-entry veneer to stage `5`
  - upstream commit:
    - `fa90aa4`
- coordination repo
  - probe-layout and interpreter helpers updated to match the new stage map
  - verifier default SHA updated to the refreshed exported image

## Validation

- Pi 4 A72 rebuild from refreshed copied buildroot: pass
- direct Pi 4 QEMU serial sanity:
  - `call: exec go!`
  - `go: enter`
  - `hal: jump exit el1`
  - `A3`
  - `KLM`
  - later `Exception #37`
- bootfs assembly: pass
- FAT image assembly: pass
- SD-image assembly: pass
- canonical SD-image export: pass
- FAT-aware verifier: pass

## Artifact

- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- SHA-256:
  `8ef476644f8fce5b5937096125421a218b8a67b0513b0fa4c0ab7e6592585e3e`

## Interpretation Goal For Next Board Retry

- highest completed `3`: failure still before or during signature verification
- highest completed `31`: `plo` signature missing at the fixed load target
- highest completed `4`: signature present before branch
- highest completed `5`: fixed-address `plo` entry veneer actually entered
