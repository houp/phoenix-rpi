# Manifest: Pi 4 QEMU `plo.elf` Handoff

- Date: `2026-03-20`
- Step: `STEP-0116`
- Status: `completed`

## Goal

- rerun the `raspi4b` QEMU smoke with `plo.elf` as `-kernel` and replace the earlier silent timeout with a more specific early-boot failure signal

## Validation

- environment:
  - `phoenix-dev`
  - VM-local QEMU `10.2.2` at `/home/witoldbolt.guest/tools/qemu-10.2.2/bin/qemu-system-aarch64`
- staged artifacts:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0113-firmware/_boot/aarch64a53-generic-rpi4b/plo.elf`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0113-firmware/_boot/aarch64a53-generic-rpi4b/rpi4b/loader.disk`
- command shape:
  - `-machine raspi4b -cpu cortex-a72 -smp 4 -m 2G -nographic -monitor none`
  - `-kernel .../plo.elf`
  - `-device loader,file=.../loader.disk,addr=0x48000000,force-raw=on`

## Result

- the new handoff is materially more informative than the earlier no-output timeout
- `plo` now reaches visible early output on the Pi 4 QEMU lane:
  - `Phoenix-RTOS loader v. 1.21 ...`
  - `cmd: Executing pre-init script`
- the run then collapses into repeated early exceptions, including:
  - `Exception #00: Unknown reason`
  - `Exception #34: PC alignment fault`
- the serial stream is heavily interleaved and corrupted, consistent with multiple CPUs printing or faulting concurrently

## Additional Evidence

From local QEMU `10.2.2` source:

- `hw/arm/boot.c`
  - explicitly assumes raw images are Linux and ELF images are not:
    - `/* Assume that raw images are linux kernels, and ELF images are not. */`
  - secondary CPU boot stubs are only installed when:
    - `info->is_linux && nb_cpus > 1`
- `hw/arm/raspi.c`
  - `raspi4b` does provide an AArch64 secondary spin-table stub through `write_smpboot64()`
  - that path depends on the Linux-classified boot flow above

From current Phoenix sources:

- `sources/plo/hal/aarch64/generic/_init.S`
  - has no generic AArch64 secondary-core trap or parking path
- `sources/plo/hal/aarch64/zynqmp/_init.S`
  - already traps non-zero MPIDR cores in a `wfe` loop until CPU 0 releases them

## Conclusion

- the `plo.elf` handoff is a valid QEMU-only visibility lane for Pi 4 bring-up
- the first bounded blocker after that handoff is no longer raw load placement
- the next smallest blocker is secondary-core containment:
  - QEMU `raspi4b` does not engage its Linux-only secondary boot stub for `plo.elf`
  - generic AArch64 `plo` currently lets any arriving secondary core execute the same early loader path

## Selected Next Step

- scope the smallest generic AArch64 `plo` containment patch:
  - trap non-primary cores in generic `_init.S`
  - release them only through the existing jump-to-kernel handoff path
  - keep the current real-device Pi 4 artifact layout unchanged
