# STEP-0346

## Title

Implement the smallest pre-interrupt xHCI capability refinement

## Date

2026-03-22

## Repositories

- `phoenix-rtos-devices`
- coordination repo

## Change Summary

The Pi 4 xHCI path now extracts the remaining pre-interrupt capability fields
needed before any later event-ring or interrupter step:

- max interrupters
- interrupt moderation interval scale
- maximum event-ring segment table size

The code adds only one new hard rejection:

- zero reported interrupters

The step remains intentionally pre-interrupt-enable, pre-ring-allocation, and
pre-enumeration.

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

The xHCI path now records the remaining pre-interrupt sizing facts needed
before later event-ring work:

- `nintrs`
- `ist`
- `erstMax`

## Next Step

- scope the smallest xHCI addressability and scratchpad-capability refinement
