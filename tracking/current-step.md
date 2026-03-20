# Current Step

## Metadata

- Step ID: `STEP-0048`
- Title: Define first non-Xilinx generic AArch64 QEMU `virt` milestone
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- define the first concrete milestone and first code step for a generic AArch64 QEMU `virt` path that can later be reused for Raspberry Pi 4 bring-up

## Scope

In scope:

- inspect the current build, project, and test scaffolding around AArch64 QEMU targets
- decide which repository should move first for the generic `virt` path
- select the first concrete code step under the boot-first policy

Out of scope:

- implementation code in upstream Phoenix repositories
- introducing the new target in this planning step
- adding Raspberry Pi-specific code in this planning step

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `docs/status.md`
- tracking files and manifest updates for this step

## Acceptance Criteria

- the chosen milestone explicitly identifies the first non-Xilinx AArch64 QEMU target shape
- the chosen first code step is small, repo-scoped, and has a realistic validation lane
- the result explains why that first code step was selected over nearby alternatives

## Validation Plan

- Review:
  inspect the current build/project/test files and record the selected milestone and first code step
- Build:
  not applicable
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-boot-first-fast-lane-policy.md`

## Notes

- Risks:
  the result must stay as one milestone-selection step and must not silently turn into a multi-repo implementation patch
- Dependencies:
  completed planning step `STEP-0047`
- User-visible control point before next step:
  after this planning step lands, the next slice should be the selected first code step for the generic `virt` path
