# Manifest: Pi 4 `_map_init` Zero-Free Visibility

- Date: `2026-03-20`
- Step: `STEP-0193`
- Status: `completed`

## Goal

- narrow the first post-banner Pi 4 kernel abort inside `_vm_init` / `_map_init`

## Upstream Repository

### `phoenix-rtos-kernel`

- Commit: `71fbea74`

## Changes

Updated:

- `sources/phoenix-rtos-kernel/vm/vm.c`
- `sources/phoenix-rtos-kernel/vm/map.c`

Added tightly bounded visibility:

- fixed console markers around `_vm_init`, `_page_init`, and `_map_init`
- one pool-link marker in `_map_init`
- one explicit `map: zero free` marker when `map_common.nfree == 0U`
- one `lib_printf()` pool-state line before the link loop for the working generic lane

The step intentionally does not change VM behavior yet.

## Validation

Environment:

- `phoenix-dev`
- copied buildroot:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy`
- `PATH="$HOME/phoenix-toolchains/aarch64-phoenix/bin:$PATH"` exported explicitly for non-interactive Lima shells

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt` lane

   - reaches:
     - `vm: enter`
     - `vm: page init done`
     - `vm: map init`
     - `map: enter`
     - `map: pool link`
     - `vm: map init done`
   - later buffered log line confirms:
     - `map: pool bss=... free=85743 total=85743 pool=9603216`
   - still continues into the established boot band

2. Pi 4 A72 `raspi4b` lane

   - reaches:
     - `vm: enter`
     - `vm: page init done`
     - `vm: map init`
     - `map: enter`
     - `map: pool link`
     - `map: zero free`
   - then aborts with:
     - `Exception #37: Data Abort (EL1)`
     - `pc=ffffffffc000b21c`
     - `lr=ffffffffc000b360`

Symbolization:

- `pc=ffffffffc000b21c` -> `_map_init` at `phoenix-rtos-kernel/vm/map.c:1645`
- `lr=ffffffffc000b360` -> `_map_init` at `phoenix-rtos-kernel/vm/map.c:1644`

## Additional Evidence

Relevant current code facts:

- `vm/page.c` sets `pages_info.freesz = 0` initially and only increases it when `pmap_getPage()` reports `PAGE_FREE`
- `hal/aarch64/pmap.c:_pmap_preinit()` seeds physical memory ranges from `dtb_getMemory()`
- `hal/aarch64/dtb.c:dtb_parseMemory()` still assumes fixed 16-byte `<addr,size>` tuples

Pi 4 official DTS facts:

- `bcm2711-rpi.dtsi` keeps `memory@0` as bootloader-filled
- the effective root memory property shape is a 3-cell form (`#address-cells = 2`, `#size-cells = 1`)

## Conclusion

- the live Pi 4 failure is no longer speculative: `_map_init` is entering the link loop with `map_common.nfree == 0U`
- the most likely root cause is earlier DTB-backed memory-bank parsing, not generic console or late VM logic
- the previous one-off QEMU DTB memory-size patch was negative because the current kernel DTB memory parser still ignores the Pi 4 3-cell root memory `reg` layout

## Selected Next Step

- generalize root memory `reg` parsing in `hal/aarch64/dtb.c` to use root `#address-cells` and `#size-cells`, then revalidate Pi 4 with the patched-memory QEMU DTB lane
