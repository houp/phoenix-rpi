# Manifest: `psh` Startup Visibility

- Date: `2026-03-21`
- Step: `STEP-0233`
- Status: `completed`

## Goal

- expose the highest visible `psh` startup boundary after the negative
  interactive-console probe

## Implementation

Changed files:

- `sources/phoenix-rtos-utils/psh/psh.c`
- `sources/phoenix-rtos-utils/psh/pshapp/pshapp.c`

Bounded markers added:

- `psh.c`:
  - `psh: root ready`
  - `psh: app run`
  - `psh: app done`
- `pshapp.c`:
  - `psh: run enter`
  - `psh: tty open`
  - `psh: tty ready`
  - `psh: tcsetpgrp`
  - `psh: readcmd`

The patch is marker-only and does not change shell control flow.

## Validation

### Build guardrails

- generic build:
  `TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- Pi 4 build:
  `RPI4B_DTB_PATH=.../bcm2711-rpi-4-b.dtb RPI4B_QEMU_MEMORY_SIZE=80000000 TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- both succeeded in `phoenix-dev`

### Runtime verification

#### Generic `virt`

- rerun with a 30-second timeout
- still reaches:
  - `main: spawned psh (4)`
  - `dummyfs: initialized`
- does not print any new `psh:` marker within that window

#### Pi 4 `raspi4b`

- rerun with a 30-second timeout
- still reaches:
  - `main: spawned psh (4)`
  - `dummyfs: initialized`
- does not print any new `psh:` marker within that window

## Result

- the negative result is shared by generic and Pi 4
- the current later-boot blocker is therefore still in the shared path
- absence of every `psh:` marker means either:
  - the spawned process does not reach even the earliest shell marker, or
  - shell-visible stdio is not a trustworthy signal path before `psh_ttyopen()`

## Next Step

- scope the smallest below-stdio visibility step, most likely one kernel-side
  first-user-execution hook for the spawned `psh` process
