# 2026-04-09 Pi 4 Fixed Armstub Entry Experiment

## Scope

Implement the smallest bounded response to the latest real Pi 4 LED sequence:
keep the current armstub GPIO42 split, but stop relying on the
firmware-patched `kernel_entry32` slot and jump directly to the configured Pi 4
`kernel_address`.

## Starting Point

The latest real Pi 4 board result on the previous image was:

- red LED on
- green LED on, then briefly off, then on forever
- blank screen
- no keyboard-visible reaction

That result sharpened the earlier clue:

- GPIO42 high proved armstub entry
- the brief low pulse proved the armstub reaches its final pre-branch split
- GPIO42 returning high strongly suggested the board resets and re-enters the
  armstub immediately after the current branch to `kernel8.img`

## Root Hypothesis

The current `kernel8.img` is a raw `plo` binary, not a Linux `Image`-format
payload. It begins directly with AArch64 instructions.

The real Raspberry Pi firmware path populates armstub `kernel_entry32` by
parsing the 64-bit kernel header, so the current raw `kernel8.img` versus
firmware-entry mismatch became the highest-probability cause of the observed
reset loop.

## Final Change

### `phoenix-rtos-project`

`_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S`

- added:
  - `PLO_LOAD_ADDR 0x40080000`
- kept:
  - the current GPIO42-high earliest-entry proof
  - the current GPIO42-low final handoff split
  - the current EL3 timer and GIC preparation
- changed the primary-core handoff:
  - before:
    - `ldr w4, kernel_entry32`
    - `ldr w0, dtb_ptr32`
    - `br x4`
  - after:
    - `ldr x4, =PLO_LOAD_ADDR`
    - `mov x0, #0`
    - `br x4`

This is deliberate:

- the current `plo` path does not use the initial firmware `x0` DTB pointer
  before zeroing registers
- the live question is the branch target, not firmware-patched register state

- upstream commit:
  - `abb8b0b`

## Validation

### Build

- rebuilt `aarch64a72-generic-rpi4b` in `phoenix-dev`

### No-hardware sanity

- the direct Pi 4 QEMU serial-log sanity lane still reaches:
  - `go!`
  - `hal: jump exit el1`
  - kernel markers `A3` and `KLM`

### Artifact chain

- reran:
  - `scripts/assemble-rpi4b-bootfs.sh`
  - `scripts/assemble-rpi4b-bootfs-img.sh`
  - `scripts/assemble-rpi4b-sdimg.sh`
- the host export helper was tightened again during this step:
  - earlier `rsync` export produced a zeroed FAT boot sector
  - a raw streamed `dd` export still produced a zeroed FAT boot sector
  - the current helper now uses:
    - `limactl shell ... base64 | base64 -d`
- the host-visible image was re-verified at the embedded FAT partition level

## Current Artifact

- image:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- SHA-256:
  `d4e02f329c35f8187969f3c02e8f0d78189fac07b8884ddb774898598a1ddc36`

## Next Step

Flash the refreshed image and retry the real Pi 4 board boot. The most useful
new observations will be:

- whether the earlier LED low-then-reset pattern disappears
- whether any HDMI output appears
- whether the board now reaches a later visible phase or still resets silently
