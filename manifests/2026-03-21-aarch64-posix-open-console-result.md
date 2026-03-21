# Manifest: `posix_open()` Console Visibility

- Date: `2026-03-21`
- Step: `STEP-0247`
- Status: `completed`

## Goal

- expose the first shared `proc_lookup(filename, ...)` result reached by
  `posix_open()` for `psh` opening `"/dev/console"`

## Implementation

Changed repository:

- `sources/phoenix-rtos-kernel`

Changed file:

- `sources/phoenix-rtos-kernel/posix/posix.c`

Upstream commit:

- `phoenix-rtos-kernel 28c2908b`

Bounded change:

- added a one-shot `posix_open()` trace for process `psh` and filename
  `"/dev/console"`

## Validation

### Build guardrails

- refreshed the copied buildroot with:
  `./scripts/prepare-buildroot.sh --copy-components`
- generic build:
  `TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- Pi 4 build:
  `RPI4B_DTB_PATH=.../bcm2711-rpi-4-b.dtb RPI4B_QEMU_MEMORY_SIZE=80000000 TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- both succeeded in `phoenix-dev`

### Runtime verification

#### Generic `virt`

- still prints:
  - `psh: tty open`
  - `psh: tty open fail open -2`
- prints **no** `posix: psh console open ...` marker

#### Pi 4 `raspi4b`

- matches the same negative result:
  - `psh: tty open`
  - `psh: tty open fail open -2`
- prints **no** `posix: psh console open ...` marker

## Result

- the failing `open("/dev/console")` path is still not reaching `posix_open()`
  on either fast lane
- the next shared seam is therefore above the kernel, in libphoenix `open()`,
  `stat()`, or `resolve_path()`

## Next Step

- scope the smallest libphoenix-side `open("/dev/console")` split
