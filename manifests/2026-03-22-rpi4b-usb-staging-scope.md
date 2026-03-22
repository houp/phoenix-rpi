# STEP-0391

## Title

Scope the smallest Pi 4 live USB-host staging step

## Date

2026-03-22

## Outcome

The next bounded Pi 4 move is now fixed:

- stage `/sbin/usb` on the Pi 4 image after `pcie` and before `psh`
- do not stage a separate `/sbin/usbkbd` process for this path
- keep the step below SD export and below manual operator execution

## Why This Step

The current USB host architecture on this project path is now explicit:

- `phoenix-rtos-usb/usb/usb.c` initializes registered linked drivers as
  internal host-side drivers
- `libusbdrv-usbkbd` is already linked into `/sbin/usb` on the A72 lane
- `pl011-tty` already contains the Pi 4 `/dev/kbd0` bridge

So the smallest live-image integration step is not a new service architecture;
it is simply staging `/sbin/usb` on the Pi 4 image path.

## Next Step

- implement the smallest Pi 4 live USB-host staging step
