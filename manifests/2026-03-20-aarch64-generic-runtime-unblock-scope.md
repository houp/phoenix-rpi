# Manifest: Generic QEMU Runtime-Unblock Discovery

- Date: `2026-03-20`
- Step: `STEP-0077`
- Result: `completed`

## Scope

- rerun the generic QEMU smoke lane from the refreshed current artifacts
- inspect the runtime output after the existing loader and kernel milestones
- identify the smallest next runtime-oriented step

## Upstream Repositories

- none

## Validation

- reran the generic QEMU smoke lane with:
  `timeout 10s ./scripts/aarch64a53-generic-qemu.sh`

## Validation Evidence

- the generic lane still reaches the existing milestones cleanly:
  - Phoenix loader banner
  - generic loader pre-init output
  - first Phoenix kernel banner line
- the run still times out after the first kernel banner line, with no new userspace-visible output

## Findings

- the current generic `user.plo` is intentionally kernel-only:
  - `kernel {{ env.BOOT_DEVICE }}`
  - `blob {{ env.BOOT_DEVICE }} /etc/system.dtb ddr`
  - `go!`
- unlike the existing ZynqMP QEMU user script, the generic script does not yet load:
  - a tty driver
  - `dummyfs`
  - `psh`
- `phoenix-rtos-devices` does not yet contain a PL011 tty driver, and it also lacks a generic AArch64 target file

## Notes

- this is a design-boundary result, not evidence of a new mysterious kernel failure
- the next fastest boot-first progress is to begin the minimal userspace-console path needed by both generic QEMU `virt` and Raspberry Pi 4 bring-up

## Selected Next Step

- define the first generic AArch64 devices-target step for the PL011 console path
