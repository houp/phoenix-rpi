# 2026-04-11 Pi 4 Armstub Kernel-Entry Fallback

## Scope

- analyze the second UART-assisted Pi 4 retry
- correlate the new UART log with the matching ACT-LED decode
- replace the failing `kernel_entry32 == 0 -> halt` armstub path with the
  smallest firmware-observed fallback that matches the real board log

## New Live Evidence

From:

- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260411-234639.log`
- `/Users/witoldbolt/Downloads/IMG_0016.mov`

The real board now shows:

- `loader.disk` loaded to `0x08000000`
- `kernel8.img` loaded to `0x00200000`
- firmware relocation of `kernel8.img` to `0x00080000`
- ACT decode reaching special terminal stage `31`

Interpretation:

- the low-memory image-placement fix is working
- the custom armstub still sees `kernel_entry32 == 0`
- the current halt-on-empty-slot behavior is therefore wrong for this firmware
  path

## Implementation

Updated:

- `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S`
  - preserve firmware entry `x0` in `x28`
  - if `dtb_ptr32 == 0`, fall back to preserved entry `x0`
  - if `kernel_entry32 == 0`, fall back to `0x00080000`
  - remove the old hard halt on empty `kernel_entry32`
  - keep stage `4` as the final pre-branch point
  - add special stage `29` for DTB fallback to entry `x0`
  - add special stage `30` for kernel-entry fallback to `0x80000`

- `/Users/witoldbolt/phoenix-rpi/scripts/rpi4_actled_probe_layout.py`
  - updated current stage map for the new fallback stages
  - removed stage `31` from the active current-layout interpretation

- `/Users/witoldbolt/phoenix-rpi/scripts/rebuild-rpi4b-fast.sh`
  - now auto-runs DTB preparation when the VM-local `/tmp/rpi4b-dtb/...` file
    is missing

## Warnings Surfaced

### DTB preparation

Running:

- `/Users/witoldbolt/phoenix-rpi/scripts/prepare-rpi4b-dtb.sh`

still fails by default on upstream Raspberry Pi Linux DTS warnings such as:

- `unit_address_vs_reg`
- `simple_bus_reg`
- `avoid_unnecessary_addr_size`
- `unique_unit_address`
- `gpios_property`

Those warnings were not ignored. For this rebuild only, DTB preparation was
rerun with:

- `RPI4B_DTB_ALLOW_WARNINGS=1`

because no locally available final Pi 4 DTB blob exists yet to replace the DTS
compile path.

### Fast rebuild helper

The fast rebuild helper previously failed after `/tmp` was cleared:

- missing `/tmp/rpi4b-dtb/bcm2711-rpi-4-b.dtb`
- missing `_fs/.../root/etc/system.dtb`

This is now fixed in the helper itself.

## Validation

- `./scripts/rebuild-rpi4b-fast.sh --scope project --qemu-sanity`
  - passed after DTB preparation
- direct Pi 4 QEMU serial sanity still reaches:
  - `call: exec go!`
  - `go: enter`
  - `hal: jump exit el1`
  - `A3`
  - `KLMconsole: pl011 init done`
  - later known `Exception #37: Data Abort (EL1)`
- canonical export:
  - passed
- FAT-aware verify:
  - passed

## Exported Image

- path:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- SHA-256:
  `4d9daf70168d6990e7525d0c0accda4a8a1ffed0a5fe62432aab4dcff8e70217`

## Next Step

- flash the refreshed image
- capture UART with `tio`
- capture LED video in parallel
- strongest success condition:
  - stage `30` appears
  - then UART reaches `TR0..TR3`
