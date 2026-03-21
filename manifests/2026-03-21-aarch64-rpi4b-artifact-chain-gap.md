# 2026-03-21: Pi 4 artifact-chain gap after SD-image assembly attempt

## Scope

- Step: `STEP-0291`
- Goal: assemble and export the refreshed Pi 4 SD image

## Result

- negative but bounded result

Running `scripts/assemble-rpi4b-sdimg.sh` failed with:

```text
missing FAT image: /home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy/_boot/aarch64a72-generic-rpi4b/rpi4b-bootfs.img
```

## What This Proves

- after the latest `build.sh project image` refresh, the higher-level artifact
  wrappers are not rebuilt automatically
- the SD-image helper depends on the FAT bootfs image
- therefore the refreshed host artifact chain must currently be rerun in full:
  1. `scripts/assemble-rpi4b-bootfs.sh`
  2. `scripts/assemble-rpi4b-bootfs-img.sh`
  3. `scripts/assemble-rpi4b-sdimg.sh`
  4. `scripts/export-rpi4b-sdimg.sh`

## Next Step

- rerun the full Pi 4 artifact helper chain and record the refreshed host
  checksum
