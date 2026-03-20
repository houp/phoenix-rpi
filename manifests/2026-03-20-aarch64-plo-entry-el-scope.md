# Manifest: Scope Generic `plo` Entry-EL Visibility on the Pi 4 Lane

- Date: `2026-03-20`
- Step: `STEP-0161`
- Status: `completed`

## Goal

- define the smallest next experiment that can show which exception level the generic loader actually runs at on the current Pi 4 QEMU lane

## Decision

The next implementation step is bounded to:

- `sources/plo/hal/aarch64/generic/hal.c`
- add a small console-visible current-EL trace after generic loader console initialization
- preserve the existing generic loader boot flow and interrupt setup
- use the same trace on both the generic `virt` and Pi 4 `raspi4b` lanes

## Why This Step

- `STEP-0160` proved the generic EL3 GIC path unblocks the timer and tty registration on `virt`
- the same change does not move the Pi 4 `raspi4b` lane at all
- the smallest remaining high-signal difference to verify is whether `raspi4b` enters generic `plo` at EL3, EL2, or EL1

## Explicitly Deferred

- broader `plo` refactoring
- Pi 4-specific GIC policy changes
- kernel-side timer or scheduler changes
- any real-hardware-only conclusions

## Acceptance Criteria

- the next implementation patch stays local to generic `plo` startup visibility
- the next generic and Pi 4 QEMU runs both expose the loader entry EL
- the result allows the following step to decide whether the Pi 4 lane needs EL2-specific setup, a different handoff path, or a separate blocker investigation

## Selected Next Step

- implement generic `plo` entry-EL visibility and rerun both QEMU lanes
