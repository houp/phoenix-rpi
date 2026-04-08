# 2026-04-08 Pi 4 Low-Level Reference Survey EDK2 Addendum

## Scope

Review the EDK2 Raspberry Pi 4 platform and fold any durable Pi 4
firmware-facing facts into the existing low-level survey.

## Source reviewed

- EDK2 Raspberry Pi 4 platform:
  `external/edk2-platforms/Platform/RaspberryPi/RPi4`
  Local clone commit: `bc4e91c`

## Durable findings folded into the knowledge base

- EDK2 independently confirms the Pi 4 ARM-visible GIC aliases:
  - `0xFF841000`
  - `0xFF842000`
- EDK2 independently confirms the Pi 4 PCIe and SoC constants:
  - PCIe register base `0xfd500000`
  - outbound window `0xf8000000`
  - GENET base `0xfd580000`
  - BCM283x peripheral base `0xfe000000`
- EDK2 uses:
  - PL011 clock input `48000000`
  - mini-UART clock `500000000`
  which reinforces the current Phoenix preference for PL011 as the safer early
  serial path
- EDK2 explicitly reserves the DTB in the low-memory window
  `0x003e0000..0x00400000`
- EDK2 warns that Pi 4 xHCI may be sensitive to DMA constraints above 3 GB of
  RAM

## Practical outcome

The EDK2 addendum does not overturn the current Pi 4 boot model, but it
strengthens two future choices:

1. the next earliest-entry experiment should avoid trampling low memory around
   the DTB staging window
2. future real-hardware xHCI debugging should keep >3 GB DMA constraints in
   mind

## Files updated

- `docs/raspberry-pi-4-low-level-reference-survey.md`
- `docs/source-artifacts.md`
- `docs/platforms/raspberry-pi-4.md`
- `docs/status.md`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Validation

- documentation-only step
- local source inspection completed for the cloned EDK2 Pi 4 platform tree
