# 2026-04-08 Pi 4 SD-Image Export Fix

## Scope

Close the host-side Pi 4 SD-image corruption report by proving whether the
problem is in the VM-local image build or in the macOS-visible export path,
then make the exported image safe to flash again.

## Findings

- the VM-local Pi 4 SD image inside `phoenix-dev` remained valid
- the previous host-visible copy in
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img` was corrupt
- the corruption was not in the MBR itself:
  - the first partition still started at LBA `2048`
  - the embedded FAT partition offset was still `1048576`
- the corruption was specifically in the exported FAT partition:
  - the host copy had an invalid zeroed boot sector at that offset
  - the VM-local copy had a valid FAT boot sector with signature `0x55aa`
  - `mdir` succeeded on the VM-local copy and failed on the corrupted host copy

## Root Cause

The current Pi 4 image assembly inside `phoenix-dev` was not the problem. The
problem was the host-side export path used to bring the VM-local `rpi4b-sd.img`
into the macOS workspace.

## Changes

### `scripts/export-rpi4b-sdimg.sh`

- switched the export path to:
  - `limactl copy --backend=rsync`
- kept the helper narrow:
  - verify the remote file exists
  - export into a temporary host file
  - atomically rename into the final artifact path

### `scripts/verify-rpi4b-sdimg.sh`

- kept the existing image path, size, and SHA-256 checks
- added FAT-aware validation:
  - parse the first MBR partition entry
  - compute the embedded partition offset
  - validate boot-sector signature `0x55aa`
  - validate a non-zero FAT bytes-per-sector field
  - run `mdir` at the computed partition offset

## Validation

### Broken host copy before the fix

- host image path:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- first partition offset:
  `1048576`
- boot sector at that offset:
  - invalid
  - zeroed
- `mdir` result:
  - failed with `non DOS media`

### VM-local source image

- VM-local image path:
  `/home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy/_boot/aarch64a72-generic-rpi4b/rpi4b-sd.img`
- first partition offset:
  `1048576`
- boot sector at that offset:
  - valid
  - signature `0x55aa`
- `mdir` result:
  - succeeded
- VM-local SHA-256:
  `44085197192f5578759269813c3aa38a8adcf04b18bc0092ec509b8fa5543920`

### Corrected host export

- reran the export helper after switching to `rsync`
- reran the strengthened verifier on the exported host image
- result:
  - size `69206016`
  - SHA-256
    `44085197192f5578759269813c3aa38a8adcf04b18bc0092ec509b8fa5543920`
  - FAT offset `1048576`
  - verification `OK`

## Current Correct Artifact

- image:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- SHA-256:
  `44085197192f5578759269813c3aa38a8adcf04b18bc0092ec509b8fa5543920`

## Next Step

Reflash the real Pi 4 board's SD card from the corrected exported image and
classify the next board result before changing the runtime code again.
