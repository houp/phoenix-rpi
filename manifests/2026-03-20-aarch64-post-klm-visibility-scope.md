# Manifest: Post-`KLM` Early-Kernel Visibility Scope

- Date: `2026-03-20`
- Step: `STEP-0186`
- Status: `completed`

## Goal

- define the smallest next visibility step after the current Pi 4 `A3KLM` boundary

## Review Basis

Reviewed:

- `manifests/2026-03-20-aarch64-rpi4b-a72-runtime-validation.md`
- `sources/phoenix-rtos-kernel/hal/aarch64/_init.S`

## Findings

- the remaining assembly path after `M` is short and high-signal:
  - trap secondaries
  - fill translation tables
  - activate `ttbr1_el1`
  - enter `_core_0_virtual`
  - copy syspage
  - call `_set_up_vbar_and_stacks`
  - branch to `main()`
- the cleanest next split is still inside `_init.S`; moving into C instrumentation now would widen the patch before we know whether `main()` is reached
- the smallest useful markers are:
  - one marker at `_core_0_virtual`
  - one marker immediately before the branch to `main()`

## Conclusion

- the next step should add exactly those two raw UART markers in common AArch64 `_init.S`, then rerun:
  - generic `aarch64a53-generic-qemu`
  - Pi 4 `aarch64a72-generic-rpi4b`

## Selected Next Step

- implement the first post-`KLM` early-kernel visibility split in `hal/aarch64/_init.S`
