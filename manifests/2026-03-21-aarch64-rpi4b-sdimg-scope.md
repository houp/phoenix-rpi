# Manifest: Pi 4 Full SD-Card Image Scope

- Date: `2026-03-21`
- Step: `STEP-0276`
- Focus: select the smallest next artifact step after the exported Pi 4 FAT
  image

## Decision

- the next bounded artifact should be a full Pi 4 disk image with:
  - DOS partition table
  - one bootable FAT32 partition
  - the current `rpi4b-bootfs.img` copied into that first partition slot

## Rationale

- the current exported artifact:
  - `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-bootfs.img`
  is only a raw FAT filesystem image
- that shape is awkward for the first real hardware trial because it still
  requires partition-level manual handling on the SD card
- the current early Pi 4 boot chain still fits entirely in the firmware-visible
  boot partition, so a one-partition MBR disk image is enough and does not
  widen the scope into persistent-rootfs design
- this also fits the current operator hardware constraints better:
  microSD is available, but USB-UART is not

## Next Bounded Move

- add one helper that assembles a full Pi 4 SD-card image around the current
  `rpi4b-bootfs.img` and validates the resulting partition table plus embedded
  FAT contents
