# Manifest: Generic AArch64 QEMU First Smoke Run

- Date: `2026-03-20`
- Step: `STEP-0059`
- Result: `completed`

## Scope

- execute the first bounded end-to-end smoke command for `aarch64a53-generic-qemu`
- capture the earliest observable result from the current launcher and artifacts
- stop after recording the first runtime-lane blocker without fixing it in the same step

## Upstream Repositories

- none

## Validation

- reused the existing copied buildroot in `phoenix-dev` at:
  `/home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy`
- executed the selected smoke command there:
  `timeout 10s ./scripts/aarch64a53-generic-qemu.sh`

## Validation Evidence

- the expected generic QEMU artifacts were present under `_boot/aarch64a53-generic-qemu/`
- the smoke command failed before QEMU launch with:
  `timeout: failed to run command ‘./scripts/aarch64a53-generic-qemu.sh’: Permission denied`
- no serial output was produced because the launcher script itself was not executable

## Notes

- this result narrows the next step to one launcher-level fix before any QEMU or boot-path debugging
- the existing `phoenix-rtos-project` QEMU launcher scripts are executable, while the new generic launcher is currently tracked as mode `100644`

## Selected Next Step

- define the smallest launcher-fix step so the generic QEMU smoke lane can actually start QEMU
