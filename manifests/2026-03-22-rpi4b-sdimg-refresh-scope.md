# 2026-03-22: Pi 4 SD-image refresh scope for first HDMI plus USB-keyboard trial

## Scope

Define the smallest artifact-handoff step after the live Pi 4 image now stages:

- `pcie`
- `usb`
- `psh`

## Decision

The next bounded move should be:

- rebuild the Pi 4 A72 image on the standard copied-buildroot lane
- regenerate the boot filesystem image and full SD-card image
- export the refreshed SD image to the coordination repository artifact path
- update the operator runbook with the first HDMI plus USB-keyboard trial
  expectations

## Why This Is The Next Step

- the integrated Pi 4 image already passed the existing QEMU shell smoke after
  `/sbin/usb` staging
- QEMU still cannot validate the BCM2711 PCIe plus VL805 transport itself, so
  further pre-hardware xHCI work is not a better next investment than preparing
  the handoff artifact
- the next strongest validation lane is the real Raspberry Pi 4 board with HDMI
  and USB keyboard attached
