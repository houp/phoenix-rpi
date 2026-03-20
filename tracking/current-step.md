# Current Step

## Metadata

- Step ID: `STEP-0228`
- Title: Scope the smallest shared later-boot visibility step
- Status: `planned`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- choose the smallest next visibility step that identifies the first shared
  later-boot blocker after `dummyfs: initialized`

## Scope

In scope:

- review the current generic and Pi 4 runtime parity after
  `dummyfs: initialized`
- choose one bounded shared later-boot visibility step
- focus on syspage app launch progress and `psh` readiness
- keep the next move outside the solved early GIC / timer path

Out of scope:

- new kernel or loader behavior changes
- real hardware work
- broad test-framework redesign
- Pi 5 or RP1 work

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `docs/status.md`
- `docs/testing-automation.md`
- manifests and tracking updates for this scope step

## Acceptance Criteria

- the next later-boot step is narrowed to one concrete visibility question
- the selected question is explicitly outside the solved early GIC path
- the step remains aligned with getting to a clearly interactive Pi 4 boot

## Validation Plan

- Review:
  inspect the latest generic and Pi 4 QEMU traces after the DTB fix
- Evidence:
  - use `manifests/2026-03-20-aarch64-pi4-gic-dtb-fix.md`
  - use `manifests/2026-03-20-aarch64-pi4-later-boot-parity-scope.md`
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-pi4-later-boot-parity-scope.md`

## Notes

- Risks:
  do not re-open the solved early GIC path unless new evidence forces it
- Dependencies:
  completed `STEP-0227` later-boot Pi 4 parity scope
- Source reminder:
  generic and Pi 4 now both reach the same visible later-boot band through
  `dummyfs: initialized` within the current 15-second QEMU window
- User-visible control point before next step:
  after this step lands, the next implementation move should target the first
  shared later-boot silence point rather than a Pi 4-specific early-boot guess
