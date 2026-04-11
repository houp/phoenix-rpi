# 2026-04-12 Pi 4 HDMI Kernel-Jump Boundary

## Summary

The latest real Raspberry Pi 4 retry moved the live failure boundary
substantially later. The board now reaches the Phoenix `plo` HDMI progress
panel on real hardware with all three stage squares lit, which means the run
gets at least as far as `video_markKernelJump()` in `hal_cpuJump()`.

## Evidence

### UART

Source:

- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260412-000322.log`

Important lines:

- `Loaded 'loader.disk' to 0x8000000 size 0x311cd0`
- `initramfs (loader.disk) loaded to 0x8000000 (size 0x311cd0)`
- `Loaded 'kernel8.img' to 0x200000 size 0xe0d8`
- `Kernel relocated to 0x80000`
- `Device tree loaded to 0x2eff5600 (size 0xa9d0)`
- `uart: Set PL011 baud rate to 103448.300000 Hz`
- `uart: Baud rate change done...`

Interpretation:

- the low-memory image fix is active on real hardware
- firmware still changes PL011 to about `103448.3` Hz
- the current host capture remains blind after that point

### HDMI

Source:

- `/var/folders/jt/_gyk57f575q5gl68ltg0_y6w0000gn/T/TemporaryItems/NSIRD_screencaptureui_nweyRG/Screenshot 2026-04-12 at 00.08.55.png`

Observed screen:

- brown background
- top-left progress panel
- all three squares lit

Source correlation:

- `/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/video.c`
  defines:
  - `video_stageFramebufferReady = 0`
  - `video_stageHalReady = 1`
  - `video_stageKernelJump = 2`
  - `video_stageCount = 3`
- `/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/hal.c`
  calls `video_markKernelJump()` inside `hal_cpuJump()` immediately before
  `hal_exitToEL1()`

Interpretation:

- real hardware now reaches the `plo` kernel-handoff boundary
- the board is no longer failing in the old armstub-only seam

### LED

Source:

- `/Users/witoldbolt/Downloads/IMG_0017.mov`

Current status:

- the current ACT decode is noisy and produced mixed codes that do not fit the
  later HDMI proof cleanly
- for this retry, HDMI is the stronger signal than LED

## Conclusions

- the custom armstub is executing
- the relocatable `kernel8` trampoline is not disproved by UART silence anymore
- low-memory `plo` placement is active
- the mailbox framebuffer path works on real hardware
- `plo` reaches the kernel-jump panel on real hardware
- the active live failure boundary has moved to the post-`plo` band:
  after `video_markKernelJump()` and around or after the EL1 handoff into the
  kernel

## Warning Handling

Warnings seen in the UART log and still surfaced:

- `[sdcard] vl805.bin not found`
- `[sdcard] pieeprom.upd not found`
- `Failed to open command line file 'cmdline.txt'`
- repeated `dterror: no symbols found`
- HDMI EDID read failures

These warnings are not ignored. They simply do not match the current blocker,
because the board now demonstrably reaches the Phoenix `plo` kernel-jump panel.

## Next Step

- stop widening armstub-only telemetry
- improve post-firmware UART observability
- diagnose the earliest kernel / EL1 path on real hardware
- add a kernel-side breadcrumb if the UART lane cannot yet stay readable across
  the firmware baud switch
