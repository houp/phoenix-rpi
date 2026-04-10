# Pi 4 Fast Incremental Rebuild Helper

Date: `2026-04-10`

## Goal

- stop defaulting to `build.sh clean host core project image` for every small
  Pi 4 bring-up edit
- make repeated armstub and `plo` debug loops cheap and predictable

## Implemented

- new helper:
  `/Users/witoldbolt/phoenix-rpi/scripts/rebuild-rpi4b-fast.sh`
- behavior:
  - incrementally refreshes the copied VM-local buildroot
  - auto-selects the narrowest safe build phase from dirty sibling repos
  - can rebuild only:
    - `project image`
    - `core project image`
    - or `clean host core project image`
  - reuses the canonical bootfs, SD-image export, and FAT-aware verify helpers
  - supports `--build-only` for very fast VM-local iteration

## Verified

- forced `--scope project --build-only`: pass
- default `--build-only` auto scope: pass
  - selected:
    - `project image`
  - reason:
    - no source repo dirt detected

## Current Policy

- use this helper by default for iterative Pi 4 bring-up rebuilds
- keep full clean rebuilds for:
  - build-infra changes
  - upstream-sync churn
  - toolchain/sysroot changes
  - suspected stale-build state
