# Pi 4 Firmware Handoff Contract Fix

- Date: `2026-04-11`
- Scope: Pi 4 real-hardware boot path
- Step: `STEP-0464`

## Summary

The active Pi 4 image no longer uses the fragile late-armstub fixed-target
signature ladder. The custom armstub now follows the same high-level handoff
contract as the known-working `rpi4-bare-metal` reference:

- load `dtb_ptr32`
- load `kernel_entry32`
- halt only if `kernel_entry32 == 0`
- restore `x0`
- execute `dsb sy; ic iallu; dsb sy; isb`
- branch through the firmware-provided entry slot

The earliest generic AArch64 `plo` path now preserves the firmware DTB pointer
and stores it in `hal_firmwareDtb` at `start_common`.

## Touched Repositories

- `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project`
- `/Users/witoldbolt/phoenix-rpi/sources/plo`
- `/Users/witoldbolt/phoenix-rpi`

## Touched Files

- `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S`
- `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/config.txt`
- `/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S`
- `/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/hal.c`
- `/Users/witoldbolt/phoenix-rpi/sources/plo/Makefile`
- `/Users/witoldbolt/phoenix-rpi/scripts/rpi4_actled_probe_layout.py`
- `/Users/witoldbolt/phoenix-rpi/docs/status.md`
- `/Users/witoldbolt/phoenix-rpi/docs/manual-operator-instructions.md`
- `/Users/witoldbolt/phoenix-rpi/docs/pi4-first-hardware-trial.md`
- `/Users/witoldbolt/phoenix-rpi/docs/testing-automation.md`
- `/Users/witoldbolt/phoenix-rpi/docs/source-artifacts.md`
- `/Users/witoldbolt/phoenix-rpi/tracking/current-step.md`
- `/Users/witoldbolt/phoenix-rpi/tracking/step-history.md`

## Why This Is The Current Best Fix

The strongest current external reference is:

- `/Users/witoldbolt/phoenix-rpi/external/rpi4-bare-metal/armstub/src/armstub8.S`

Its primary CPU path uses:

- `ldr w4, kernel_entry32`
- `ldr w0, dtb_ptr32`
- `br x4`

The previous Phoenix experiment diverged at exactly that seam:

- hardcoded `0x40080000`
- dereferenced the target image before branching
- kept `boot_load_flags=0x1` in `config.txt`

The live LED evidence had already narrowed the failure to the late custom
armstub seam. So the most promising next move was to restore the firmware
contract rather than widen the probe ladder further.

## Build Warning Fixed In This Step

The first rebuild surfaced:

- `fatal: not a git repository (or any of the parent directories): .git`

Root cause:

- `/Users/witoldbolt/phoenix-rpi/sources/plo/Makefile`
  computed `VERSION` using unconditional `git rev-parse`

Fix:

- guard the git lookup and fall back to `unknown` when the copied buildroot has
  no `.git`

Result:

- the second rebuild completed without that fake-fatal

## Validation

- `./scripts/rebuild-rpi4b-fast.sh --scope project --qemu-sanity`
  - result: pass
- direct Pi 4 QEMU serial sanity:
  - still reaches:
    - `call: exec go!`
    - `go: enter`
    - `hal: jump exit el1`
    - `A3`
    - `KLMconsole: pl011 init done`
  - still ends in the known later:
    - `Exception #37: Data Abort (EL1)`
- canonical SD-image export:
  - result: pass
- FAT-aware SD-image verification:
  - result: pass

## Refreshed Test Artifact

- path:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- SHA-256:
  `7ba2d0773a60451691a45083d376cd6ccc3293dd800ffe14a8c741ec064db61c`

## Expected Next Hardware Signal

The next LED video should answer a better question than the old dense
signature-read image:

- whether the armstub now gets through:
  - `23`: seam entered
  - `24`: `dtb_ptr32` loaded
  - `25`: `kernel_entry32` loaded
  - `26`: `kernel_entry32` nonzero
  - `4`: branch imminent
- and whether generic `plo` now appears at:
  - `5`: earliest entry veneer
  - `6`: old generic `_start` body
