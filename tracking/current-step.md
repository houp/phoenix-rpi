# Current Step

## Metadata

- Step ID: `STEP-0464`
- Title: Restore the Pi 4 firmware handoff contract in armstub and early plo
- Status: `in_progress`
- Date: `2026-04-11`
- Milestone / phase: `Phase 1`

## Objective

- replace the current Pi 4 custom armstub hardcoded fixed-target branch with a
  firmware-contract handoff based on `kernel_entry32` and `dtb_ptr32`
- preserve the firmware DTB pointer into the earliest `plo` path instead of
  clobbering it before it can be used
- reduce the current armstub seam complexity so the next hardware retry tests a
  real boot fix, not another increasingly fragile probe ladder

## Scope

In scope:

- replacing the hardcoded `0x40080000` late armstub branch contract
- switching the late armstub handoff to firmware-populated entry/DTB slots
- preserving `x0` through the first generic `plo` instructions
- keeping enough LED telemetry to verify that the new handoff path executes on
  real hardware

Out of scope:

- network-boot lab setup
- unrelated Pi 4 driver work
- UART capture itself, until the cable is physically available

## Expected Repositories

- coordination repo
- `phoenix-rtos-project`
- `plo`

## Expected Files Or Subsystems

- `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S`
- `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/config.txt`
- `/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S`
- `/Users/witoldbolt/phoenix-rpi/scripts/rpi4_actled_probe_layout.py`
- `/Users/witoldbolt/phoenix-rpi/docs/status.md`

## Acceptance Criteria

- the Pi 4 armstub late handoff now follows the firmware-populated entry/DTB
  slots instead of a hardcoded fixed target
- the earliest `plo` path preserves the firmware DTB pointer
- a refreshed SD image is built, exported, and verified without warnings being
  ignored
- the next real-device retry can specifically confirm whether the firmware
  handoff-contract fix moved the boot farther than the current LED-only result

## Validation Plan

- [rebuild-rpi4b-fast.sh](/Users/witoldbolt/phoenix-rpi/scripts/rebuild-rpi4b-fast.sh) `--scope project`
- [export-rpi4b-sdimg.sh](/Users/witoldbolt/phoenix-rpi/scripts/export-rpi4b-sdimg.sh)
- [verify-rpi4b-sdimg.sh](/Users/witoldbolt/phoenix-rpi/scripts/verify-rpi4b-sdimg.sh)
- direct Pi 4 QEMU serial sanity after the rebuild

## Rollback / Baseline

- latest pre-fix LED-only boundary manifests around the dense seam:
  `/Users/witoldbolt/phoenix-rpi/manifests/2026-04-10-pi4-img0012-dense-signature-analysis.md`
  `/Users/witoldbolt/phoenix-rpi/manifests/2026-04-10-pi4-fixed-target-signature-check.md`

## Notes

- strongest current fix hypothesis comes from the known-working
  `rpi4-bare-metal` armstub: branch via `kernel_entry32` and forward `dtb_ptr32`
- the earlier fixed-address plus target-dereference seam is now treated as the
  likely real bug on hardware; this step replaces it with a firmware-slot
  handoff and removes `boot_load_flags=0x1` from the active Pi 4 config
- current rebuilt test image:
  - `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  - SHA-256:
    `7ba2d0773a60451691a45083d376cd6ccc3293dd800ffe14a8c741ec064db61c`
- current known residual runtime warning after the fix:
  - direct Pi 4 QEMU serial sanity still ends in the later known
    `Exception #37: Data Abort (EL1)`
