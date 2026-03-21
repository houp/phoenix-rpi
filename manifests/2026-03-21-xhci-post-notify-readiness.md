# STEP-0340: Implement the smallest post-notify xHCI readiness refinement

## Date

- 2026-03-21

## Repositories And SHAs

- `phoenix-rtos-devices`: `4e46b4b`

## Change Summary

- extended `sources/phoenix-rtos-devices/usb/xhci/xhci.c` with the next bounded
  controller-readiness checks after reset and Pi 4 firmware notify
- added:
  - `OP_PAGESIZE` readback
  - 4K page-support validation
  - basic port-count extraction from `HCSPARAMS1`
  - zero-port rejection
- kept the driver intentionally non-operational after these checks:
  `xhci_init()` still returns failure before root-hub, ring, interrupt, or
  enumeration work

## Validation

- refreshed the copied VM-local buildroot with:
  `./scripts/prepare-buildroot.sh --copy-components`
- built a fresh Pi 4 A72 image in `phoenix-dev` with:
  `TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- result:
  build passed, and the staged Pi 4 image remained unchanged apart from rebuilt
  `xhci` support code

## Result

- the Pi 4 xHCI path now proves a little more real controller shape before any
  host-operation claim:
  version, reset, page size, and non-zero port count
- the next missing slice should stay structural:
  extract the remaining xHCI capability state needed before root-hub modeling
