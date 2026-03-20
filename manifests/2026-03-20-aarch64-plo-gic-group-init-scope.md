# Manifest: Scope First Generic `plo` EL3 GIC Group-Initialization Experiment

- Date: `2026-03-20`
- Step: `STEP-0159`
- Status: `completed`

## Goal

- define the smallest next experiment that can establish Group 1 interrupt state before the generic loader exits EL3 for the kernel

## Decision

The next implementation step is bounded to:

- `sources/plo/hal/aarch64/generic/interrupts.c`
- detect EL3 in generic `plo` GIC initialization
- when running in EL3, initialize GICv2 secure state so interrupts are managed as Group 1 for the later non-secure handoff
- preserve the current non-EL3 generic path
- validate by rerunning the existing kernel-side timer IRQ traces on the generic and Pi 4 QEMU lanes

## Why This Step

- `STEP-0156` proved the timer IRQ reads back as `grp 0 en 0`
- `STEP-0158` proved the kernel-side attempt to move the timer IRQ to Group 1 does not take effect
- `sources/plo/hal/aarch64/generic/_init.S` exits EL3 to EL1 non-secure
- `sources/plo/hal/aarch64/zynqmp/_init.S` already performs secure GIC Group 1 setup before non-secure handoff
- the smallest remaining high-signal experiment is therefore generic `plo` GIC initialization at EL3

## Explicitly Deferred

- Pi 4-specific loader hacks
- broad `plo` interrupt-controller refactoring
- kernel-side timer or scheduler changes
- real-hardware-only conclusions

## Acceptance Criteria

- the next implementation patch stays local to generic `plo` GIC initialization
- the next generic QEMU run shows whether the later kernel trace moves away from `grp 0 en 0`
- the result allows the following step to choose between keeping the new generic `plo` group-init path and refining the exact EL3 GIC control values

## Selected Next Step

- implement the generic `plo` EL3 GIC group-initialization experiment and rerun both QEMU lanes
