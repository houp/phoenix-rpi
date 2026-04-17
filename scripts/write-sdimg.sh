#!/bin/bash

echo "WARNING: this only works on macOS and assumes that and SD card is in /dev/disk4"
echo "WARNING: if your SD card is not in /dev/disk4 - you might corrupt your system (i.e. get some important data erased)"

/Users/witoldbolt/phoenix-rpi/scripts/verify-rpi4b-sdimg.sh

diskutil unmountDisk /dev/disk4

sudo dd if=/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img of=/dev/rdisk4 bs=4m

sync

diskutil eject /dev/disk4

