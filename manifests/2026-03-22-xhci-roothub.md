# STEP-0370

## Title

Implement the smallest xHCI roothub control/request step

## Date

2026-03-22

## Repositories

- `phoenix-rtos-devices`
- coordination repo

## Change Summary

The Pi 4 xHCI path now contains the first bounded Phoenix roothub contract.

The new code in `usb/xhci/xhci.c` now adds:

- a small xHCI root-hub descriptor set:
  - device descriptor
  - configuration descriptor
  - string descriptors
  - hub descriptor
- `usb_isRoothub(pipe->dev)` special-casing in `xhci_transferEnqueue()`
- `PORTSC`-based root-port status decoding into `usb_port_status_t`
- the minimal port feature handling needed by Phoenix hub enumeration:
  - `POWER`
  - `RESET`
  - clear `C_CONNECTION`
  - clear `C_ENABLE`
  - clear `C_OVER_CURRENT`
  - clear `C_RESET`
- roothub status-bit synthesis in `xhci_getHubStatus()`

The step still intentionally stays pre-general USB-device transfers:

- non-roothub transfers still return `-ENOSYS`
- no xHCI slot/context/device-enumeration code was added
- the live Pi 4 image still does not stage `/sbin/usb` or `/sbin/usbkbd`

## Files

- `sources/phoenix-rtos-devices/usb/xhci/xhci.c`

## Validation

Validated in `phoenix-dev` with a fresh copied-buildroot Pi 4 A72 build:

```sh
./scripts/prepare-buildroot.sh --copy-components
cd ~/phoenix-buildroots/phoenix-rtos-project-copy
export PATH="$HOME/phoenix-toolchains/aarch64-phoenix/bin:$PATH"
export RPI4B_DTB_PATH=/tmp/rpi4b-dtb/bcm2711-rpi-4-b.dtb
export RPI4B_QEMU_MEMORY_SIZE=80000000
TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image
```

Result:

- build completed successfully
- the staged Pi 4 image composition remained unchanged

Important validation limit:

- there is still no faithful no-hardware runtime lane for the Pi 4
  BCM2711 PCIe -> VL805 xHCI path
- so this step is compile-validated preparation, not yet runtime-proven

## Result

The xHCI path now answers the bounded root-hub requests Phoenix needs before
`/sbin/usb` can become a realistic staging candidate.

The next remaining blocker is narrower:

- `xhci_init()` still returns `-ENOSYS` after the internal controller tests
- so `usb` would still exit during `hcd_init()` even though the roothub request
  path now exists

## Upstream Commit

- `phoenix-rtos-devices c9ee03d`

## Next Step

- scope the smallest xHCI init-success step needed before live `/sbin/usb`
  staging
