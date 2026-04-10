# Pi 4 First-Read Focus Image

Date: `2026-04-10`

## Goal

- remove ambiguity in the live `24 -> 25` seam
- make the next board video separate the first and second fixed-target reads
  more clearly

## Implemented

- `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S`
  now:
  - emits duplicated focus-stage bursts with an extra long gap for the armstub
    seam of interest
  - inserts `dsb sy` / `isb` before the first signature-word read
  - emits stage `21` immediately before the first signature-word read
  - emits stage `25` immediately after the first signature-word read
  - inserts `dsb sy` / `isb` before the second signature-word read
  - emits stage `22` immediately before the second signature-word read
  - emits stage `26` immediately after the second signature-word read
- this temporarily repurposes codes `21` and `22` away from the later `plo`
  meanings for this diagnostic image

## Validation

- Pi 4 A72 rebuild from refreshed copied buildroot: pass
- direct Pi 4 QEMU serial sanity:
  - `call: exec go!`
  - `go: enter`
  - `hal: jump exit el1`
  - `A3`
  - `KLM`
  - later `Exception #37`
- bootfs assembly: pass
- FAT image assembly: pass
- SD-image assembly: pass
- canonical SD-image export: pass
- FAT-aware verifier: pass

## Artifact

- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- SHA-256:
  `6932d3a31fc0fee1494295c4e9d0587c689b7cde20a6fb1907d86164e9815883`

## Next Video Interpretation

- highest `24` only:
  fail before the first-read pre-marker completes
- highest `21`:
  fail on the first signature-word read itself
- highest `25` but no `22`:
  survive the first read, fail before the second-read pre-marker completes
- highest `22`:
  fail on the second signature-word read itself
- highest `26` and later:
  both reads survived; move diagnosis to compare or branch band
