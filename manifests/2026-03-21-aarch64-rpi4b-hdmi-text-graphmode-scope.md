# AArch64 Pi 4 HDMI Text: Graphmode Exposure Scope

Date: `2026-03-21`

## Summary

The user explicitly deferred further SD-image refresh and manual Pi 4 board
testing until HDMI text output exists. The shortest credible path is not a new
font subsystem and not direct reuse of the full `pc-tty` app.

Phoenix already contains:

- a framebuffer text renderer in
  `phoenix-rtos-devices/tty/pc-tty/ttypc_fbcon.c`
- a bundled 8x16 bitmap font in
  `phoenix-rtos-devices/tty/pc-tty/ttypc_fbfont.h`
- Pi 4 framebuffer geometry already written by `plo` into the generic AArch64
  syspage via `syspage_graphmodeSet()`

The current missing seam is generic AArch64 exposure of that framebuffer
geometry through `platformctl`, analogous to the existing IA32
`pctl_graphmode` path.

## Selected Next Step

Implement the smallest reusable generic AArch64 graphmode exposure step:

- extend `include/arch/aarch64/generic/generic.h` with `pctl_graphmode`
- teach `hal/aarch64/generic/generic.c` to return framebuffer geometry from the
  generic AArch64 syspage when graphics are present

## Why This Is The Smallest Correct Step

- it reuses metadata already produced by the current Pi 4 `plo` path
- it is generic AArch64 work, not Pi 4-only glue
- it enables later reuse of existing Phoenix framebuffer text code
- it avoids widening into keyboard, USB, or SD-image work

## Explicitly Rejected For This Step

- importing NetBSD `wsfont`
- porting the whole `pc-tty` app to Pi 4
- starting USB keyboard work
- refreshing SD artifacts again

## Validation Expectation For The Following Code Step

- clean build of the touched AArch64 target lane in `phoenix-dev`
- no runtime claim yet; runtime validation belongs to the subsequent framebuffer
  text-renderer integration step
