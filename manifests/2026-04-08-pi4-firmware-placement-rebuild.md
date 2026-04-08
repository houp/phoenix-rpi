# 2026-04-08 Pi 4 Firmware Placement Rebuild

## Scope

Close the smallest real-hardware boot response justified by the first Pi 4
board evidence:

- align the Pi 4 A72 `plo` image with the Raspberry Pi firmware default
  64-bit kernel load address model
- stop forcing the older `kernel_address=0x40080000` placement in the Pi 4
  firmware config
- rebuild and revalidate the generic and Pi 4 QEMU lanes
- export a fresh Pi 4 SD-card image for the next board retry

## Real Hardware Evidence That Triggered This Step

First Pi 4 board trial on the previous image:

- firmware could read the SD card and boot partition correctly
- the board then stayed on the firmware rainbow splash forever with no
  Phoenix-visible output
- after removing `kernel_address=0x40080000` and `boot_load_flags=0x1`
  directly on-card, the board instead hung on a black screen with no
  Phoenix-visible output

That evidence ruled out the earlier FAT/media problem and justified one
bounded early-handoff experiment on the code side.

## Code Changes

### `plo`

File:

- `/Users/witoldbolt/phoenix-rpi/sources/plo/ld/aarch64a72-generic.ldt`

Commit:

- `c89c84b`

Change:

- stop inheriting the generic A53 layout
- define an A72-specific placement for the Pi 4 boot path
- link `plo` at `0x00200000`, which matches the Raspberry Pi firmware default
  64-bit kernel load range more closely than the old generic `0x40080000`

### `phoenix-rtos-project`

File:

- `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/config.txt`

Commit:

- `a4d253c`

Changes:

- remove `kernel_address=0x40080000`
- remove `boot_load_flags=0x1`
- keep `disable_splash=1` so the firmware rainbow does not hide the next
  visible post-handoff state

## Validation

### QEMU

Passed:

- `./scripts/qemu-shell-smoke.sh generic`
- `./scripts/qemu-shell-smoke.sh rpi4b`
- `/bin/bash /Users/witoldbolt/phoenix-rpi/scripts/qemu-rpi4b-hdmi-smoke.sh`

### Rebuild

Passed in `phoenix-dev` after regenerating the Pi 4 DTB:

- `TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

## Refreshed Artifact

Exported host-visible image:

- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`

Size:

- `69206016`

SHA-256:

- `1c3bc4f6c474baad547059801ba49ea4c2de31c088aea3b1ef68fc7b8eb2924f`

## Result

The rebuilt Pi 4 image is ready for the next real-device retry. The correct
next step is now a fresh board run with the rebuilt SD image, not another
speculative early-handoff code change.
