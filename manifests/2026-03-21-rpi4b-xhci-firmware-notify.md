# STEP-0338: Implement the Pi 4 firmware `notify xHCI reset` step

## Date

- 2026-03-21

## Repositories And SHAs

- `phoenix-rtos-devices`: `88cf140`
- `phoenix-rtos-project`: `0ff6c3e`

## Change Summary

- added a bounded Pi 4 mailbox/property helper to
  `sources/phoenix-rtos-devices/pcie/server/pcie.c`
- the helper:
  - maps the Raspberry Pi mailbox registers
  - allocates an uncached contiguous message buffer
  - fills a `PROPTAG_NOTIFY_XHCI_RESET` request
  - passes the physical message address with `va2pa()`
  - waits for the mailbox property response
- wired the helper into the existing Pi 4 PCIe scan path so it runs only for
  the fixed VL805 endpoint before memory-space and bus-master enable
- added `RPI_MAILBOX_BASE_ADDRESS` to the Pi 4 board config in
  `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/board_config.h`

## Validation

- refreshed the copied VM-local buildroot with:
  `./scripts/prepare-buildroot.sh --copy-components`
- built a fresh Pi 4 A72 image in `phoenix-dev` with:
  `TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- result:
  build passed, and the staged Pi 4 image remains unchanged apart from the
  rebuilt `pcie` binary

## Result

- the Pi 4 image path now contains the first real-device-oriented VL805
  firmware-load hook
- the next missing xHCI slice is no longer firmware notify:
  it is controller-readiness refinement inside `xhci`, starting with page-size
  and basic port-count validation before any root-hub or enumeration work
