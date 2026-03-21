# 2026-03-21: scope the corrected full Pi 4 artifact-refresh chain

## Scope

- Step: `STEP-0292`
- Goal: define the smallest complete helper chain needed to refresh the
  host-visible Pi 4 SD image after a `build.sh project image` update

## Decision

The full current helper chain is:

1. `scripts/assemble-rpi4b-bootfs.sh`
2. `scripts/assemble-rpi4b-bootfs-img.sh`
3. `scripts/assemble-rpi4b-sdimg.sh`
4. `scripts/export-rpi4b-sdimg.sh`

## Why This Is Required

- the boot tree helper rebuilds the firmware-visible file set
- the bootfs image helper wraps that tree into `rpi4b-bootfs.img`
- the SD-image helper wraps the FAT image into `rpi4b-sd.img`
- the export helper copies that VM-local full disk image to the host artifact
  path

## Next Step

- implement `STEP-0293`: rerun the full helper chain and record the refreshed
  host-visible checksum
