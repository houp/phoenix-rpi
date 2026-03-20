# Manifest: Pi 4 QEMU Raw-Image Load Mismatch Scope

- Date: `2026-03-20`
- Step: `STEP-0115`
- Status: `completed`

## Goal

- identify the smallest credible blocker behind the no-output `raspi4b` smoke and select one bounded next step

## Source Findings

From the current Phoenix Pi 4 artifact set:

- `sources/phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/config.txt`
  - sets `kernel_address=0x40080000`
  - sets `initramfs loader.disk 0x48000000`
- `sources/plo/ld/aarch64a53-generic.ldt`
  - links `plo` at `ADDR_PLO 0x40080000`
- the built `plo` ELF in `phoenix-dev` reports:
  - entry point address `0x40080000`
- the staged `kernel8.img` is a raw binary image rather than an ELF image

From QEMU `10.2.2` source in `phoenix-dev`:

- `hw/arm/raspi4b.c`
  - reuses the common Raspberry Pi machine path through `raspi_base_machine_init()`
- `hw/arm/raspi.c`
  - unless `machine->firmware` is used, the board goes through `arm_load_kernel()`
- `hw/arm/boot.c`
  - defines:
    - `KERNEL64_LOAD_ADDR 0x00080000`
  - in `load_aarch64_image()`, raw AArch64 images are loaded at:
    - `mem_base + KERNEL64_LOAD_ADDR`
  - in `arm_setup_direct_kernel_boot()`, ELF images are attempted before the raw AArch64 image path

## Conclusion

- the first no-output `raspi4b` smoke is most likely blocked by a load-address mismatch, not by missing Pi 4 board support
- inference:
  - real Pi firmware is expected to place `kernel8.img` at `0x40080000`
  - direct QEMU `-kernel kernel8.img` on `raspi4b` does not emulate that firmware `kernel_address=` behavior for raw binaries
  - the direct QEMU path is therefore likely starting the raw image at `0x00080000` relative to RAM instead of `0x40080000`

## Selected Next Step

- keep the real-device artifact shape unchanged
- for QEMU `raspi4b` only, validate the smallest alternative handoff:
  - use the already-built `plo.elf` as the QEMU `-kernel`
  - keep `loader.disk` preloaded at `0x48000000`
- success signal for the next step:
  - any new visible loader or kernel output
  - or a more specific failure mode than the current silent timeout
