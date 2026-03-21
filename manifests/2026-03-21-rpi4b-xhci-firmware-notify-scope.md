# STEP-0337: Scope the smallest Pi 4 firmware `notify xHCI reset` step

## Date

- 2026-03-21

## Goal

- define the smallest clean Pi 4-specific firmware-notify move that should
  follow the new xHCI controller-reset baseline

## Inputs Reviewed

- `sources/phoenix-rtos-devices/pcie/server/pcie.c`
- `sources/phoenix-rtos-devices/usb/xhci/xhci.c`
- `sources/libphoenix/include/sys/mman.h`
- `external/circle/lib/usb/xhcidevice.cpp`
- `external/circle/include/circle/bcmpropertytags.h`

## Scope Decision

- the cleanest place for `PROPTAG_NOTIFY_XHCI_RESET` is the Pi 4 `pcie`
  server, not `xhci` and not `plo`
- rationale:
  - `pcie` already owns the BCM2711 host-bridge reset and device-enable path
  - Circle issues the tag after PCIe reset and before enabling the VL805 device
  - Phoenix user space already provides:
    - `va2pa()`
    - uncached contiguous buffers via `mmap()`
  - that makes a firmware-visible mailbox/property buffer feasible in user
    space without forcing a new `plo` dependency

## Selected Bounded Implementation

- add a tiny Pi 4 mailbox/property helper inside `pcie.c`
- issue `PROPTAG_NOTIFY_XHCI_RESET` only when the fixed Pi 4 VL805 endpoint is
  discovered:
  - bus `1`
  - slot `0`
  - function `0`
  - class code `0x0c0330`
- keep the rest of the runtime USB path unchanged

## Deferred To Later Steps

- page-size validation in `xhci`
- xHCI ring and interrupt setup
- root-hub modeling and enumeration
- staging `/sbin/usb` into the live Pi 4 image
