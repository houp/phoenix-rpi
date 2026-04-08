# 2026-04-08 Pi 4 Armstub EL3 GIC Preparation

## Scope

Close the next smallest real-hardware early-boot hypothesis after the corrected
Pi 4 GIC-address image still left the board black and silent:

- compare the current custom Pi 4 armstub against Circle and the other local
  Pi 4 bare-metal references
- extend the custom armstub with the missing bounded EL3 timer and GIC setup
  that can plausibly affect real hardware before any Phoenix-visible HDMI
  output
- rebuild and revalidate the strongest relevant non-hardware lanes
- export a refreshed Pi 4 SD-card image for the next board retry

## Trigger

Real Pi 4 board result on the corrected-GIC-address image remained unchanged:

- black HDMI output
- only brief initial ACT blinks, then silence
- no keyboard-visible reaction

That moved the next justified step earlier than `plo` runtime MMIO touches and
back into the firmware-handoff path itself.

## Root Cause Hypothesis

Deep comparison against the local Pi 4 bare-metal references showed that the
current custom `phoenix-armstub8-rpi4.S` still stopped at the small `0x100`
firmware handoff header, while Circle's working Pi 4 `armstub8-rpi4` also
performs real EL3 hardware preparation before entering the runtime image:

- local timer control and prescaler setup
- `CNTFRQ_EL0 = 54000000`
- GIC group-1 preparation and distributor / CPU-interface enablement using the
  ARM-visible GIC aliases

Because Phoenix `plo` still performs generic interrupt and timer setup before
its HDMI path becomes visible, missing EL3 GIC preparation is the kind of gap
that can still kill the board before any Phoenix-visible output appears.

## References Consulted

### Circle

- `/Users/witoldbolt/phoenix-rpi/external/circle/boot/armstub/armstub8.S`
- `/Users/witoldbolt/phoenix-rpi/external/circle/boot/config64.txt`
- `/Users/witoldbolt/phoenix-rpi/external/circle/boot/README`
- `/Users/witoldbolt/phoenix-rpi/external/circle/lib/startup64.S`

Relevant confirmed details:

- `LOCAL_CONTROL = 0xff800000`
- `LOCAL_PRESCALER = 0xff800008`
- `GIC_DISTB = 0xff841000`
- `GIC_CPUB = 0xff842000`
- `CNTFRQ_EL0 = 54000000`
- `setup_gic` runs in EL3 before entering the runtime image

### Other Local Pi 4 Bare-Metal References

- `/Users/witoldbolt/phoenix-rpi/external/rpi4-osdev/part14-spi-ethernet/boot/boot.S`
- `/Users/witoldbolt/phoenix-rpi/external/rpi-os/src/boot.S`

These confirm the same Pi 4 high-peripheral local-timer addresses and the same
54 MHz architectural-timer frequency expectation.

### Official Raspberry Pi Documentation

- [Legacy config.txt options](https://www.raspberrypi.com/documentation/computers/legacy_config_txt.html)

Relevant current notes:

- `arm_peri_high=1` enables high peripheral mode on Pi 4 and is set
  automatically if a suitable DTB is loaded
- high peripheral mode without a compatible DTB will fail to boot, and ARM
  stub support is also required
- `kernel_address` is legacy, and default 64-bit kernel loading is at
  `0x200000`
- `enable_gic` defaults to `1` on Pi 4

## Code Change

### `phoenix-rtos-project`

File:

- `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S`

Commit:

- `2f618c1`

Change:

- add Pi 4 local timer control and prescaler setup
- set `CNTFRQ_EL0 = 54000000`
- keep the existing `cntvoff_el2` and `cnthctl_el2` setup
- add a bounded Circle-style `setup_gic` routine in EL3:
  - enable the distributor
  - enable the CPU interface
  - move all interrupt groups to Group 1
- add the `FIQS` marker at offset `0xd4`

The rest of the Phoenix loader path remains unchanged in this step.

## Validation

### Build

Passed in `phoenix-dev`:

- real-hardware artifact build:
  - `TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
    with `RPI4B_DTB_PATH=/tmp/rpi4b-dtb/bcm2711-rpi-4-b.dtb`
- QEMU validation build:
  - `TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
    with:
    - `RPI4B_DTB_PATH=/tmp/rpi4b-dtb/bcm2711-rpi-4-b.dtb`
    - `RPI4B_QEMU_MEMORY_SIZE=80000000`

### Pi 4 QEMU

Passed on the DTB-prepared validation lane:

- `./scripts/qemu-shell-smoke.sh rpi4b`
- `/bin/bash /Users/witoldbolt/phoenix-rpi/scripts/qemu-rpi4b-hdmi-smoke.sh`

Important note:

- the direct Pi 4 QEMU lane still needs the dedicated DTB-prepared build,
  because the staged downstream DTB inherits Raspberry Pi Linux's bootloader-
  filled `memory@0` placeholder and QEMU does not run Raspberry Pi firmware to
  patch it

### Artifact Validation

Passed:

- `./scripts/assemble-rpi4b-bootfs.sh`
- `./scripts/assemble-rpi4b-bootfs-img.sh`
- `./scripts/assemble-rpi4b-sdimg.sh`
- `./scripts/export-rpi4b-sdimg.sh`
- `./scripts/verify-rpi4b-sdimg.sh`

## Refreshed Artifact

Exported host-visible image:

- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`

Size:

- `69206016`

SHA-256:

- `16c4f7f5e313266bdb197a9ddc4d3dc81a080fffb6bea631ab7016dbbb741590`

## Additional Preserved Clue

The staged downstream `system.dtb` still contains the Raspberry Pi Linux source
placeholder:

- `memory@0 { reg = <0 0 0>; }`

That is expected at build time because Raspberry Pi firmware patches the live
DTB before kernel handoff. It is not the current pre-visible boot blocker, but
future real-hardware cleanup should stop treating the staged build-time DTB
blob as equivalent to the firmware-patched live DTB.

## Result

The next real-device retry should now use the refreshed image with the expanded
Pi 4 custom armstub EL3 timer and GIC preparation. If the board still remains
black and silent after this image, the next justified move should become a
stricter earliest-entry diagnostic or a Pi-4-specific delay/bypass of early
generic `plo` interrupt setup rather than another loader-address cleanup.
