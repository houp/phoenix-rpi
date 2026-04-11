# Pi 4 Relocatable `kernel8.img` Trampoline

- Date: `2026-04-11`
- Scope: Pi 4 real-hardware boot image format
- Step: `STEP-0465`

## Summary

The first real Pi 4 UART log proved the remaining boot blocker was no longer
the late armstub contract alone. Real firmware reported:

- `Loaded 'kernel8.img' to 0x40080000 size 0x0`
- `Kernel relocated to 0x80000`

That means the previous active image was still packaging `kernel8.img` as a raw
direct copy of the high-linked `plo` image, even though real firmware was
executing it from a relocated low address.

The active Pi 4 image now fixes that mismatch by building `kernel8.img` as:

- a relocatable AArch64 trampoline
- plus an embedded high-linked `plo` payload

The trampoline:

- preserves the firmware DTB pointer in `x0`
- emits direct PL011 breadcrumbs:
  - `TR0`
  - `TR1`
  - `TR2`
  - `TR3`
- copies the embedded `plo` payload to `0x40080000`
- cleans the copied region with `dc cvau`
- executes `ic iallu` and barriers
- branches to the real high-linked `plo`

## Touched Repositories

- `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project`
- `/Users/witoldbolt/phoenix-rpi`

## Touched Files

- `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-kernel8-reloc.S`
- `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-kernel8-reloc.lds`
- `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/build.project`
- `/Users/witoldbolt/phoenix-rpi/scripts/summarize-rpi4b-uart-log.py`
- `/Users/witoldbolt/phoenix-rpi/docs/status.md`
- `/Users/witoldbolt/phoenix-rpi/docs/testing-automation.md`
- `/Users/witoldbolt/phoenix-rpi/docs/manual-operator-instructions.md`
- `/Users/witoldbolt/phoenix-rpi/docs/pi4-first-hardware-trial.md`
- `/Users/witoldbolt/phoenix-rpi/docs/source-artifacts.md`
- `/Users/witoldbolt/phoenix-rpi/tracking/current-step.md`
- `/Users/witoldbolt/phoenix-rpi/tracking/step-history.md`

## UART Evidence That Triggered This Step

The first real Pi 4 UART log also surfaced warnings:

- `[sdcard] vl805.bin not found`
- `[sdcard] pieeprom.upd not found`
- `[sdcard] recover4.elf not found`
- `Failed to open command line file 'cmdline.txt'`
- repeated `dterror: no symbols found`

Current classification:

- the missing recovery and update files are expected for the current Phoenix SD
  image and are not the blocker
- missing `cmdline.txt` is expected for the current Phoenix image and is not
  the blocker
- `dterror: no symbols found` is now explicitly surfaced as a stripped-DTB
  property, but it does not match the current pre-Phoenix failure

## Validation

- `./scripts/rebuild-rpi4b-fast.sh --scope project --qemu-sanity`
  - result: pass
- surfaced warnings/errors during this step:
  - none from the rebuild itself after the code change
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
  `610dbbfd0192760f061395f7e85573261b85b18857bea426e6adab4930468698`

## Expected Next Hardware Signal

The next real Pi 4 UART capture should answer the relocation question directly:

- if `TR0` appears:
  firmware reached the trampoline entry
- if `TR1` appears:
  payload copy started
- if `TR2` appears:
  payload copy plus cache maintenance completed
- if `TR3` appears:
  the branch to the real high-linked `plo` was attempted
- if later Phoenix `plo` or kernel markers appear:
  the earlier relocation mismatch was the real remaining blocker
