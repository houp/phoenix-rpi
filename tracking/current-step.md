# Current Step

## Metadata

- Step ID: `STEP-0005`
- Title: Complete the sibling Phoenix repo set required for local builds
- Status: `in_progress`
- Date: `2026-03-19`
- Milestone / phase: `Phase 0`

## Objective

- Clone the remaining Phoenix repositories needed for a clean local `phoenix-rtos-project` build into `sources/`

## Scope

In scope:

- inspect `phoenix-rtos-project/.gitmodules` and the current local repo set
- clone the missing build-relevant Phoenix repositories into `sources/`
- record the expanded repo set in manifests and coordination docs

Out of scope:

- wiring the local buildroot for the sibling-clone workflow
- verifying the first Phoenix Linux build
- editing upstream Phoenix source code

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `sources/`
- `manifests/`
- repository-preparation docs
- tracking files

## Acceptance Criteria

- all currently required Phoenix project submodule repositories needed for local builds exist as sibling clones under `sources/`
- their exact SHAs are recorded in a manifest or manifest update
- the coordination docs no longer describe an incomplete local repo set

## Validation Plan

- Build:
  not applicable
- Emulator:
  not applicable
- Hardware:
  not applicable
- Environment:
  verify the required sibling repositories exist locally and resolve cleanly

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-19-baseline-clones.md`

## Notes

- Risks:
  the initial repo list may be incomplete because `phoenix-rtos-project` expects more submodule repositories than the first baseline clone step captured
- Dependencies:
  completed VM/bootstrap steps and network access for any missing clones
- User-visible control point before next step:
  present the completed sibling repo inventory before creating the local buildroot wiring
