# Manifest: `psh` First User-Schedule Visibility

- Date: `2026-03-21`
- Step: `STEP-0235`
- Status: `completed`

## Goal

- prove whether the spawned `psh` process ever reaches first user execution

## Implementation

Changed file:

- `sources/phoenix-rtos-kernel/proc/threads.c`

Bounded change:

- added a one-time marker in `_threads_schedule()` immediately before
  `hal_cpuRestore(context, selCtx)` when:
  - the selected thread belongs to process `psh`
  - the selected context is user mode

Marker:

- `threads: psh user scheduled`

## Validation

### Build guardrails

- generic build:
  `TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- Pi 4 build:
  `RPI4B_DTB_PATH=.../bcm2711-rpi-4-b.dtb RPI4B_QEMU_MEMORY_SIZE=80000000 TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- both succeeded in `phoenix-dev`

### Runtime verification

#### Generic `virt`

- within the 30-second QEMU window, now prints:
  - `threads: psh user scheduled`

#### Pi 4 `raspi4b`

- within the same 30-second QEMU window, now prints:
  - `threads: psh user scheduled`

## Result

- `psh` definitely reaches first user execution on both generic and Pi 4
- the later-boot blocker is therefore after first user-mode entry, not in the
  spawn path or in “process never runs” failure
- the next smallest useful split is the earliest `psh`-specific syscall result,
  beginning with the `lookup("/")` wait loop in `psh.c`

## Next Step

- scope the smallest kernel-side `psh` root-lookup success trace
