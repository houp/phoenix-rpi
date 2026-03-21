# STEP-0350

## Title

Implement the smallest xHCI operational memory-layout register step

## Date

2026-03-22

## Repositories

- `phoenix-rtos-devices`
- coordination repo

## Change Summary

The Pi 4 xHCI path now extracts the first operational-register state needed for
later controller memory layout:

- `CRCR`
- `DCBAAP`

The step also adds only the smallest sanity checks justified before later
programming:

- if the controller does not advertise 64-bit addressing support, the high
  halves of `CRCR` and `DCBAAP` must be zero
- low-word reserved bits outside the documented command-ring flags and pointer
  masks must be clear

The step remains intentionally pre-DCBAA programming, pre-command-ring
programming, pre-interrupt-enable, and pre-enumeration.

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

The xHCI path now records the first operational register values needed before
later memory programming:

- `crcrLo`
- `crcrHi`
- `dcbaapLo`
- `dcbaapHi`
- masked 64-bit `crcr`
- masked 64-bit `dcbaap`

## Next Step

- scope the smallest xHCI memory-allocation step for `DCBAA` and the command
  ring
