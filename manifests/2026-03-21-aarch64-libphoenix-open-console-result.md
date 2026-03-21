# Manifest: libphoenix `open("/dev/console")` Visibility

- Date: `2026-03-21`
- Step: `STEP-0249`
- Status: `completed`

## Goal

- distinguish whether the shared `open("/dev/console") -> -ENOENT` failure sits
  in libphoenix `stat()`, `resolve_path()`, or `sys_open()`

## Implementation

Changed repository:

- `sources/libphoenix`

Changed file:

- `sources/libphoenix/unistd/file.c`

Upstream commit:

- `libphoenix 81ffc84`

Bounded change:

- added a one-shot console-open trace in libphoenix `open()` that would print:
  - `open: console stat <err>`
  - `open: console resolve <err>`
  - `open: console sys_open <err>`

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
- prints **no** `open: console ...` marker

#### Pi 4 `raspi4b`

- matches the same negative result:
  - `psh: tty open`
  - `psh: tty open fail open -2`
- prints **no** `open: console ...` marker

## Result

- the shared failure is now bounded even more tightly:
  - `psh_ttyopen()` reports an `open()` failure
  - the libphoenix `open()` wrapper trace stays silent
  - the kernel `posix_open()` trace also stays silent
- the next strongest hypothesis is now call-path mismatch or symbol/linkage
  mismatch rather than simple pathname handling

## Next Step

- scope the smallest binary or symbol inspection step for `psh_ttyopen()`
