# STEP-0368

## Title

Implement the smallest polled xHCI command-submission step

## Date

2026-03-22

## Repositories

- `phoenix-rtos-devices`
- coordination repo

## Change Summary

The Pi 4 xHCI path now contains the first bounded end-to-end command path in
code, still kept outside live USB enumeration.

The new code:

- adds helpers to enter and leave the xHCI run state
- initializes a single no-op command TRB in the command ring
- rings doorbell `0`
- polls the first event-ring TRB for a command-completion event
- validates:
  - event TRB cycle bit
  - command-completion TRB type
  - success completion code
  - returned command TRB pointer
- returns the controller to the halted state afterward

The step intentionally still leaves `xhci_init()` non-production:

- it returns `-ENOSYS` after the self-test
- it does not enable interrupts
- it does not enumerate devices
- it does not expose `/dev/usb` on the Pi 4 image yet

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

The xHCI path now contains:

- command ring
- controller-visible event ring
- one bounded polled command self-test path

That is a meaningful prerequisite for later real-device xHCI bring-up, but it
is not enough yet for interactive Pi 4 USB keyboard testing.

## Upstream Commit

- `phoenix-rtos-devices 6eb4e26`

## Next Step

- scope the smallest xHCI roothub control/request step needed before staging
  `/sbin/usb` on the Pi 4 image

