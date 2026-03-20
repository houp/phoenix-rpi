# Manifest: Post-`ttbr1` Raw UART Visibility Negative Result

- Date: `2026-03-20`
- Step: `STEP-0188`
- Status: `completed`

## Goal

- split the remaining `A3KLM` boundary using raw UART markers at `_core_0_virtual` and immediately before branching to `main()`

## Upstream Repository

No upstream changes were kept from this experiment.

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0188-qemu`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0188-rpi4b`

Runtime validation with the temporary markers showed:

- generic `virt` regressed from its previous boot band and now stopped at `A3KLM`
- Pi 4 A72 also still stopped at `A3KLM`
- neither lane printed the new post-`ttbr1` markers

The experiment was reverted immediately and the previous generic fast-lane behavior was restored.

## Conclusion

- raw UART markers that rely on the early `PL011_TTY_BASE` physical address are not valid after the `ttbr1_el1` switch in this path
- the next safe split must use a console mechanism that becomes valid after `_hal_consoleInit()`, or instrument the console-init path itself

## Selected Next Step

- move the visibility split to the first C-managed console points in `_hal_init()` and `main()`
