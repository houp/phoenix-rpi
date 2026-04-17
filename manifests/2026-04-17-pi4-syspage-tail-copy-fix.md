# Pi 4 Syspage Tail Copy Fix

Date: 2026-04-17

## Summary

The real-board UART log
`artifacts/rpi4b-uart/rpi4b-uart-20260417-213826.log` finally narrowed the
active boundary enough to target a plausible code defect directly.

The raw tail reached:

- `A2`
- `KLM`
- `X1`
- `X2`
- `X3`
- `NO`

Meaning:

- the board survives `plo`
- the board survives EL2 -> EL1 handoff
- kernel `_start` reaches `KLM`
- the board survives MMU setup through `X1/X2/X3`
- the board reaches `_core_0_virtual` (`O`)
- but does not reach post-copy marker `P`

So the active fault sits inside the syspage copy block in
`phoenix-rtos-kernel/hal/aarch64/_init.S`.

## Root Cause Hypothesis

The previous syspage copy loop always read in 8-byte chunks:

- load `size`
- compute end
- `ldr x3, [x9], #8`
- `str x3, [x1], #8`
- loop while source pointer is still below end

If `syspage->size` is not 8-byte aligned, the final 8-byte load can read past
the end of the blob. That can survive under QEMU but fault on real hardware,
which matches the observed `... O` and no `P` boundary.

## Fix Applied

Replaced the syspage copy loop with:

- an 8-byte loop while remaining size is `>= 8`
- a 1-byte tail loop for the final `0..7` bytes

Touched repo:

- `phoenix-rtos-kernel`
  - `hal/aarch64/_init.S`

## Validation

- `./scripts/rebuild-rpi4b-fast.sh --scope core --qemu-sanity`: pass
- canonical export: pass
- FAT-aware verify: pass

## Exported Image

- Path:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- SHA-256:
  `77164588645c65f09773165afd19eef3b7709c00fd1fc804b5dd0571003baf29`

## Expected Next Real-Board UART Outcomes

- `... NO` still with no `P`:
  the syspage copy still fails before completion and the hypothesis is false or
  incomplete
- `... NOP` and later:
  the tail overread was real, and the active boundary moves beyond the copy
  loop
