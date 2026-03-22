# 2026-03-22: Pi 4 live USB-host staging

## Scope

Close `STEP-0392` by staging `/sbin/usb` on the Pi 4 image path and proving the
board image still survives the existing Pi 4 QEMU shell smoke.

## Changes

- updated
  `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/user.plo.yaml`
- staged `usb` between `pcie` and `psh`:
  - `pcie`
  - `usb`
  - `psh`

## Validation

Fresh Pi 4 A72 build in `phoenix-dev`:

```sh
export PATH="$HOME/phoenix-toolchains/aarch64-phoenix/bin:$PATH"
cd ~/phoenix-buildroots/phoenix-rtos-project-copy
/Users/witoldbolt/phoenix-rpi/scripts/prepare-buildroot.sh --copy-components
export RPI4B_DTB_PATH=/tmp/rpi4b-dtb/bcm2711-rpi-4-b.dtb
export RPI4B_QEMU_MEMORY_SIZE=80000000
TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image
```

Sequential QEMU validation:

```sh
./scripts/qemu-shell-smoke.sh rpi4b
```

Observed result:

- the Pi 4 shell smoke still reaches `(psh)% help`
- no new boot regression was introduced by staging `/sbin/usb`

## Outcome

- the Pi 4 image now stages the USB host binary on the live board path
- the linked internal-driver path remains the intended integration model:
  `/sbin/usb` is the staged binary and a separate staged `/sbin/usbkbd` process
  is not required
- the next practical step is no longer live-image integration; it is exporting
  a refreshed SD-card image for the first real-device HDMI plus USB-keyboard
  trial
