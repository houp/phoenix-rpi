# Manifest: Post-FAT-Image Artifact Scope

- Date: `2026-03-21`
- Step: `STEP-0273`
- Focus: choose the smallest next device-facing artifact after the new Pi 4 FAT
  image

## Decision

- for the first real-device Pi 4 attempt, use the current FAT image directly as
  the primary device-facing artifact:
  - `rpi4b-bootfs.img`

## Rationale

- the current early boot chain is still firmware-partition-centric:
  - `kernel8.img`
  - `config.txt`
  - `loader.disk`
  - DTB
  - Raspberry Pi firmware files
- the current path does not yet require a larger multi-partition media image to
  prove first hardware boot
- a full SD-card image can stay deferred until persistence or richer storage
  layout becomes necessary

## Next Bounded Move

- improve operator-facing handling of the FAT image artifact rather than
  widening immediately into full-media image generation
