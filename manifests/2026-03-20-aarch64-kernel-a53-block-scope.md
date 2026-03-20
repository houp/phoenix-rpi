# Manifest: Generic AArch64 Post-Entry A53-Block Split Scope

- Date: `2026-03-20`
- Step: `STEP-0178`
- Status: `completed`

## Goal

- select the smallest next early-init visibility step after proving that Pi 4 reaches generic kernel `_start`

## Changes

No code changes.

## Review Basis

Reviewed:

- `sources/phoenix-rtos-kernel/hal/aarch64/_init.S`
- `manifests/2026-03-20-aarch64-kernel-entry-visibility.md`

## Findings

- the new `K` marker proves Pi 4 reaches generic kernel `_start`
- the next high-signal candidate region in `_init.S` is the `#ifdef __TARGET_AARCH64A53` block that accesses:
  - `CPUECTLR_EL1`
  - `CPUACTLR_EL1`
- the current Pi 4 lane is built as `aarch64a53-generic-rpi4b`, but `raspi4b` QEMU uses `-cpu cortex-a72`
- there is no existing `aarch64a72` target family in the local Phoenix tree, so the A53-specific block is the first bounded architecture mismatch worth testing before broader target-family work

## Conclusion

- the next bounded implementation step should add tiny raw markers immediately before and after the A53-specific tuning block in `hal/aarch64/_init.S`
- this will distinguish:
  - failure before that block
  - failure on the first A53-specific system-register access
  - or failure later in early init

## Selected Next Step

- implement raw post-entry markers around the A53-specific kernel setup block
