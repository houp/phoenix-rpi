# Current Step

## Metadata

- Step ID: `STEP-0160`
- Title: Implement generic `plo` EL3 GIC group-initialization experiment
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- determine whether generic `plo` EL3 GIC Group 1 initialization is enough to make the later kernel-side timer IRQ state move away from `grp 0 en 0`

## Scope

In scope:

- `sources/plo/hal/aarch64/generic/interrupts.c`
- detect EL3 in generic `plo` GIC initialization
- when running in EL3, initialize secure GIC state so interrupts are placed in Group 1 for the later non-secure handoff
- preserve the current non-EL3 generic path
- keep kernel-side timer, scheduler, and DTB logic unchanged
- validate on the generic `virt` lane first, then on the Pi 4 DTB-backed `raspi4b` lane

Out of scope:

- broader `plo` interrupt refactoring
- kernel-side timer changes
- Pi 4-specific `plo` hacks
- secure-physical timer policy in the kernel
- changing `pl011-tty` retry semantics
- changing scheduler policy
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- `sources/plo`
- coordination repo

## Expected Files Or Subsystems

- `sources/plo/hal/aarch64/generic/interrupts.c`
- relevant generic and Pi 4 QEMU smoke notes
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the generic lane still reaches later kernel-side timer registration and arming
- the generic lane exposes whether the later kernel trace moves away from `grp 0 en 0`
- the experiment remains local to generic `plo` GIC initialization
- neither QEMU lane regresses from current known-good boot output

## Validation Plan

- Review:
  confirm the patch stays localized to generic `plo` GIC initialization and only changes EL3 group setup
- Build:
  rebuild the affected generic and Pi 4 project lanes in `phoenix-dev`
- Emulator:
  rerun:
  - generic `virt`
  - Pi 4 DTB-backed `raspi4b`
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-timer-group1-experiment.md`

## Notes

- Risks:
  avoid widening a bounded `plo` EL3 group-init experiment into a broad loader interrupt rewrite
- Dependencies:
  completed `STEP-0159` scope decision
- User-visible control point before next step:
  after this step lands, the next bounded move should come from whether the later kernel-side timer trace changes from `grp 0 en 0`, and whether dispatch finally starts appearing
