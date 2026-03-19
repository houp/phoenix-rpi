# Current Step

## Metadata

- Step ID: `STEP-0007`
- Title: Verify a clean baseline Phoenix Linux build in `phoenix-dev`
- Status: `in_progress`
- Date: `2026-03-19`
- Milestone / phase: `Phase 0`

## Objective

- Verify that the current workspace can complete one clean upstream Phoenix build inside `phoenix-dev` using the documented sibling-clone buildroot workflow

## Scope

In scope:

- refresh the disposable buildroot with `scripts/prepare-buildroot.sh`
- run one clean upstream baseline build inside `phoenix-dev`
- capture the exact target, command, and artifact paths that succeed
- update docs and tracking based on the result

Out of scope:

- changing upstream Phoenix source code for Raspberry Pi work
- adding a new target
- editing upstream Phoenix source code

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- baseline build artifacts in the disposable buildroot
- Linux VM build workflow
- tracking and status docs
- tracking files

## Acceptance Criteria

- one clean upstream Phoenix build command completes successfully inside `phoenix-dev`
- the build uses the documented buildroot workflow instead of nested submodule clones
- the resulting artifact paths are recorded for the next implementation step

## Validation Plan

- Build:
  run a clean baseline build in the VM and verify expected output paths exist afterward
- Emulator:
  not applicable
- Hardware:
  not applicable
- Environment:
  verify the build runs in `phoenix-dev` against the prepared buildroot

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-19-full-project-repo-set.md`

## Notes

- Risks:
  the first upstream build may reveal additional package requirements or target-selection issues not captured by the current bootstrap docs
- Dependencies:
  completed repo inventory step, completed buildroot step, and a running `phoenix-dev` VM
- User-visible control point before next step:
  present the exact successful baseline build command and the artifact locations before any Raspberry Pi-specific code changes begin
