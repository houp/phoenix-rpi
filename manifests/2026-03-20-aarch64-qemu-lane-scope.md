# AArch64 Generic QEMU Lane Scope

## Summary

- Date: `2026-03-20`
- Step name: `STEP-0010`
- Scope: identify the first narrow executable step toward a non-Xilinx generic AArch64 QEMU lane
- Validation lanes used:
  - local environment inspection in `phoenix-dev`
- Result: success

## Findings

- `phoenix-dev` provides `qemu-system-aarch64` version `8.2.2` with the standard `virt` machine available.
- The locally dumped `virt` DTB contains:
  - root-level `pl011@9000000`
  - root-level `intc@8000000`
  - `arm,armv8-timer`
  - PSCI with `method = "hvc"`
  - `chosen/stdout-path = "/pl011@9000000"`
- Current Phoenix AArch64 gaps relevant to this lane:
  - the kernel DTB parser only discovers serials named `serial@...`
  - the kernel DTB parser only discovers the GIC through the current `amba_apu` or `interrupt-controller@...` assumptions
  - `plo` AArch64 still lacks generic DTB and PSCI support, so a true `plo`-based `virt` lane is broader than a first follow-up step

## Selected Next Step

- First executable follow-up step:
  - extend the kernel AArch64 DTB parser to recognize the `virt`-style root-level GIC and PL011 node names while keeping the existing ZynqMP path intact
- Why this is the right next step:
  - it is kernel-only
  - it is smaller than introducing a full `virt` target
  - it improves both future generic QEMU work and the later Raspberry Pi DTB generalization

## Notes

- New constraints discovered:
  - the first non-Xilinx AArch64 QEMU step should be DTB parser work, not target-metadata-only work
- Next smallest task:
  - implement root-level `intc@...` and `pl011@...` DTB recognition in `phoenix-rtos-kernel/hal/aarch64/dtb.c`
