# STEP-0348

## Title

Implement the smallest xHCI addressability and scratchpad-capability refinement

## Date

2026-03-22

## Repositories

- `phoenix-rtos-devices`
- coordination repo

## Change Summary

The Pi 4 xHCI path now extracts the remaining structural controller capability
bits that shape later memory layout:

- 64-bit addressing support
- scratchpad-restore support
- maximum primary stream array size

The step stays intentionally pre-DCBAA, pre-scratchpad-allocation, pre-ring,
and pre-enumeration.

## Files

- `sources/phoenix-rtos-devices/usb/xhci/xhci.c`

## Validation

Validated in `phoenix-dev` with a fresh copied-buildroot Pi 4 A72 build:

```sh
./scripts/prepare-buildroot.sh --copy-components
cd ~/phoenix-buildroots/phoenix-rtos-project-copy
export PATH="$HOME/phoenix-toolchains/aarch64-phoenix/bin:$PATH"
TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image
```

Result:

- build completed successfully
- live staged Pi 4 image composition remained unchanged

## Result

The xHCI path now records the remaining addressability and scratchpad-memory
facts needed before later DCBAA and scratchpad-allocation work:

- `ac64`
- `spr`
- `maxPsaSize`

## Next Step

- scope the smallest xHCI operational memory-layout register step
