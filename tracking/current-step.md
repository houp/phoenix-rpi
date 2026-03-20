# Current Step

## Metadata

- Step ID: `STEP-0116`
- Title: Validate `plo.elf` as the first `raspi4b` QEMU kernel handoff
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- test the smallest QEMU-side handoff change that can bypass the current raw-image load mismatch and produce observable early Pi 4 boot progress

## Scope

In scope:

- keep the real-device Pi 4 artifact shape unchanged
- for QEMU `raspi4b` only, replace `kernel8.img` with the already-built `plo.elf` in the smoke command
- keep `loader.disk` preloaded at `0x48000000`
- document whether this produces:
  - visible loader output
  - visible kernel output
  - or a more specific failure than the current silent timeout

Out of scope:

- broad Pi 4 peripheral-debug work
- source changes to `plo`, kernel, or project files unless the validation proves a tiny follow-up patch is strictly required
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration
- changing the existing generic `virt` lane

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- current Pi 4 staged artifacts under `_boot/aarch64a53-generic-rpi4b/`
- relevant QEMU invocation notes
- manifests and tracking updates for this validation step

## Acceptance Criteria

- the `raspi4b` QEMU lane is rerun with `plo.elf` as `-kernel`
- the result is more informative than the previous silent raw-image timeout
- the next step can be selected from concrete runtime evidence rather than the current load-address inference alone

## Validation Plan

- Review:
  confirm that the chosen QEMU-only handoff change does not alter the real-device artifact layout
- Build:
  not required if the existing Pi 4 build artifacts are still present
- Emulator:
  run `raspi4b` QEMU with:
  - `-kernel .../plo.elf`
  - `-device loader,file=.../loader.disk,addr=0x48000000,force-raw=on`
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-rpi4b-qemu-raw-image-load-mismatch-scope.md`

## Notes

- Risks:
  keep this as a QEMU-only handoff validation; do not let it widen into a broad mixed boot-media redesign
- Dependencies:
  completed `STEP-0115`
- User-visible control point before next step:
  after this step lands, the next bounded move should come from the `plo.elf` smoke result: either keep tightening the QEMU handoff or move on to the next earliest visible boot blocker
