# Current Step

## Metadata

- Step ID: `STEP-0173`
- Title: Validate generic multi-core loader handoff behavior
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- determine whether the repeated Pi 4 assembly handoff markers are a Pi 4-specific effect or the generic result of releasing multiple loader CPUs into the same kernel entry path

## Scope

In scope:

- reuse the current validated generic build artifacts with the new assembly markers
- rerun generic `virt` with `-smp 4`
- compare the generic `virt` `-smp 4` output against:
  - generic `virt` `-smp 1`
  - Pi 4 `raspi4b` `-smp 4`
- record whether repeated assembly handoff markers or a missing kernel banner also appear on generic multi-core `virt`
- update manifests and docs with the result

Out of scope:

- code changes
- changing Pi 4 image layout
- changing DTB content or selection
- semantic EL-handoff changes
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- generic and Pi 4 multi-core QEMU handoff notes
- manifests and tracking updates for this validation step

## Acceptance Criteria

- the generic `virt` `-smp 4` run is recorded
- the result clearly says whether multi-core loader handoff failure is generic or Pi 4-specific
- the next step is chosen from that result rather than from speculation alone

## Validation Plan

- Review:
  compare the exact marker sequences across the three emulator runs
- Build:
  not applicable if existing validated build artifacts remain available
- Emulator:
  rerun:
  - generic `virt` with `-smp 4`
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-rpi4b-el-exit-visibility.md`

## Notes

- Risks:
  avoid changing loader semantics before proving whether the multi-core symptom is generic
- Dependencies:
  completed `STEP-0172` assembly-side Pi 4 EL-exit visibility
- User-visible control point before next step:
  after this step lands, the next bounded move should either be a controlled secondary-core containment experiment or an earliest-kernel-entry probe, depending on whether generic `virt -smp 4` reproduces the same failure
