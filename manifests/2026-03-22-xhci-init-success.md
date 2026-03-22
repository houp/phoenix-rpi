# STEP-0372

## Title

Implement the smallest xHCI init-success step

## Date

2026-03-22

## Repositories

- `phoenix-rtos-devices`
- coordination repo

## Change Summary

The Pi 4 xHCI path no longer intentionally self-aborts after the current
roothub-ready initialization sequence.

The change is intentionally small:

- after:
  - controller reset
  - capability/runtime validation
  - command-space setup
  - run/halt self-test
  - event-ring setup
  - polled no-op command self-test
- `xhci_init()` now returns success instead of forcing `-ENOSYS`

The step still intentionally stays pre-general USB-device support:

- non-roothub transfers still return `-ENOSYS`
- there is still no slot-enable or address-device path
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

The xHCI path is now closer to a stageable `/sbin/usb` daemon:

- `hcd_init()` no longer has to fail immediately because of the earlier
  deliberate `-ENOSYS` tail

The next blocker is narrower and explicit:

- there is still no roothub status-delivery path on Pi 4 because the current
  xHCI discovery stub reports `irq = 0`
- even with a live roothub, port-change notification and child-device
  enumeration are still missing

## Upstream Commit

- `phoenix-rtos-devices 5593054`

## Next Step

- scope the smallest xHCI roothub status-delivery step for the current
  no-IRQ Pi 4 path
