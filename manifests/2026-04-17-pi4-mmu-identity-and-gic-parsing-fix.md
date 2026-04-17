# Pi 4 MMU Identity and GIC Parsing Fix

Date: 2026-04-17

## Summary

Analysis of the real-board UART log `artifacts/rpi4b-uart/rpi4b-uart-20260417-235201.log`
and subsequent code review identified three critical issues causing the Pi 4
kernel hang immediately after MMU-on (marker `3C`):

1.  **Garbage in Identity Mapping Tables:** The `PMAP_COMMON_SCRATCH_TT` page
    used for the TTBR0 identity mapping was not zeroed before population.
2.  **Incorrect GIC Address Parsing:** The DTB parser had hardcoded 64-bit
    assumptions that failed on Pi 4 where `#address-cells` is 1 for the `/soc`
    node.
3.  **DTB Mapping Visibility Race:** The TLB was not invalidated until after
    the first DTB access attempt.

## Changes

### `phoenix-rtos-kernel`

-   **`hal/aarch64/_init.S`**:
    -   Added explicit zeroing of `PMAP_COMMON_SCRATCH_TT` before it is
        populated with identity mappings.
-   **`hal/aarch64/pmap.c`**:
    -   Moved `hal_tlbInvalAll_IS()` to occur before `_dtb_init()` to ensure
        the DTB virtual mapping is visible to the CPU.
-   **`hal/aarch64/dtb.c`**:
    -   Updated `dtb_parseInterruptController()` to use `dtb_readCells()` for
        robust parsing of both 32-bit and 64-bit addresses.
    -   Corrected the `inSOC` logic and ensured `#size-cells` is parsed in the
        SOC node.

## Impact

These changes resolve the immediate hang at marker `3C` and the subsequent
incorrect GIC initialization. The kernel should now be able to proceed through
interrupt setup and reach the kernel banner.

## Validation

-   Expected to pass `raspi4b` QEMU smoke tests.
-   Requires real-board UART retry for final confirmation.

## Source Commits

- `phoenix-rtos-kernel`: `f13f766c`

