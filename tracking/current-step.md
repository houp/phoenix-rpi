# Current Step

## Metadata

- Step ID: `STEP-0162`
- Title: Implement generic `plo` entry-EL visibility
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- determine which exception level the generic loader actually runs at on the Pi 4 `raspi4b` QEMU lane, relative to the now-working generic `virt` lane

## Scope

In scope:

- `sources/plo/hal/aarch64/generic/hal.c`
- add a small console-visible current-EL trace after generic loader console initialization
- preserve the existing generic loader boot flow and interrupt initialization
- use the same trace on both the generic `virt` and Pi 4 `raspi4b` lanes
- validate on the generic `virt` lane first, then on the Pi 4 DTB-backed `raspi4b` lane

Out of scope:

- broader `plo` refactoring
- any new interrupt-controller policy changes
- kernel-side timer changes
- Pi 4-specific `plo` hacks
- changing `pl011-tty` retry semantics
- changing scheduler policy
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- `sources/plo`
- coordination repo

## Expected Files Or Subsystems

- `sources/plo/hal/aarch64/generic/hal.c`
- relevant generic and Pi 4 QEMU smoke notes
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the generic lane still reaches the current working timer and tty boundary
- both QEMU lanes expose the generic loader entry EL
- the experiment remains local to generic `plo` startup visibility
- neither QEMU lane regresses from current known-good boot output

## Validation Plan

- Review:
  confirm the patch stays localized to generic `plo` startup visibility and only adds current-EL reporting
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
  `manifests/2026-03-20-aarch64-plo-gic-group-init-experiment.md`

## Notes

- Risks:
  avoid widening a bounded loader visibility step into another behavioral change before the Pi 4 entry mode is known
- Dependencies:
  completed `STEP-0161` scope decision
- User-visible control point before next step:
  after this step lands, the next bounded move should come from whether the Pi 4 lane reaches generic `plo` in EL3, EL2, or EL1
