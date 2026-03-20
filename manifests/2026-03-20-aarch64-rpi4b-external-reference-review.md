# Manifest: Pi 4 External Bare-Metal Reference Review

- Date: `2026-03-20`
- Step: `STEP-0202`
- Status: `completed`

## Goal

- review external Raspberry Pi bare-metal references and extract only the Pi 4
  bring-up details that can sharpen current Phoenix work or future subsystem
  phases

## Reviewed Sources

- `sypstraw/rpi4-osdev`
  - repo: <https://github.com/sypstraw/rpi4-osdev>
  - commit: `c9a2a1429ae7cf0f6d1632c3f4a92fb8457e6c07`
- `rsta2/circle`
  - repo: <https://github.com/rsta2/circle>
  - commit: `5d819ab24f9a6c53ebab3525558051826b39d757`
- `markCwatson/rpi-os`
  - repo: <https://github.com/markCwatson/rpi-os>
  - commit: `ecc9418d88adb3fc4c6a30e3182b2e029c2bd90f`
- OSDev article:
  - <https://wiki.osdev.org/Raspberry_Pi_Bare_Bones>

## Main Findings

- all reviewed sources agree on the Pi 4 low-peripheral-mode MMIO base
  `0xFE000000`
- all reviewed sources agree that normal AArch64 firmware boot on Pi 4 centers
  on the `0x80000` load convention, even though Phoenix currently stages `plo`
  at a different address for its own boot chain
- the light-weight references (`rpi4-osdev`, `rpi-os`, OSDev) are useful for:
  - single-core gating via `mpidr_el1` plus `wfe`
  - BSS clearing and early C entry
  - mini-UART and PL011 setup patterns
  - mailbox and framebuffer concepts
- those light-weight references are not good models for the current Phoenix
  timer bug because their interrupt demos rely on the BCM2711 legacy system
  timer or legacy IRQ controller
- Circle is the decisive external reference for the current Phoenix seam:
  - Pi 4 AArch64 startup from EL2 in `lib/startup64.S`
  - explicit `CNTHCTL_EL2` timer access enable
  - `CNTVOFF_EL2 = 0`
  - required physical-counter use on Pi 4 in `lib/timer.cpp`
  - non-secure physical timer IRQ identity `ARM_IRQLOCAL0_CNTPNS = GIC_PPI(14) = 30`
    in `include/circle/bcm2711int.h`
- the current Phoenix Pi 4 A72 debug lane already matches Circle on the most
  important points:
  - Cortex-A72 focus
  - non-secure physical timer choice
  - IRQ `30`
- the highest-value next step is therefore still a bounded GIC PPI-state
  follow-up, not another timer-source experiment

## Knowledge-Base Updates

- added
  [`docs/raspberry-pi-bare-metal-reference-notes.md`](/Users/witoldbolt/phoenix-rpi/docs/raspberry-pi-bare-metal-reference-notes.md)
- updated
  [`docs/source-artifacts.md`](/Users/witoldbolt/phoenix-rpi/docs/source-artifacts.md)
- updated
  [`docs/status.md`](/Users/witoldbolt/phoenix-rpi/docs/status.md)

## Next Step

- scope one bounded Pi 4 GIC PPI-state experiment, using Circle as the primary
  external reference for the timer-to-GIC handoff details
