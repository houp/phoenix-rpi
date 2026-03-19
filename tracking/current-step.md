# Current Step

## Metadata

- Step ID: `STEP-0008`
- Title: Provision the Phoenix AArch64 toolchain in `phoenix-dev`
- Status: `in_progress`
- Date: `2026-03-19`
- Milestone / phase: `Phase 0`

## Objective

- Provision the `aarch64-phoenix` toolchain in `phoenix-dev` so the first real AArch64 kernel and `plo` refactors can be compiled and validated locally

## Scope

In scope:

- identify and install any still-missing packages needed for Phoenix toolchain builds
- build and install the upstream `aarch64-phoenix` toolchain in VM-local storage
- make the resulting toolchain available for later AArch64 validation steps
- record the exact install path and invocation used

Out of scope:

- changing upstream Phoenix kernel, `plo`, or build logic yet
- validating a new Raspberry Pi or generic AArch64 code patch

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- Phoenix toolchain build workspace and install path inside `phoenix-dev`
- Linux VM package state
- tracking, status, and manual setup docs
- tracking files

## Acceptance Criteria

- `aarch64-phoenix-gcc` and the companion binutils resolve inside `phoenix-dev`
- the toolchain install path is documented for future sessions
- the step does not modify upstream Phoenix source repositories yet

## Validation Plan

- Build:
  build the upstream AArch64 Phoenix toolchain and verify the resulting commands resolve in the VM
- Emulator:
  not applicable
- Hardware:
  not applicable
- Environment:
  verify the toolchain can be resolved from the VM shell afterward

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-19-host-generic-pc-baseline-build.md`

## Notes

- Risks:
  the toolchain build may reveal additional host packages or a longer-running download/compile path than the initial VM bootstrap captured
- Dependencies:
  completed baseline build step and a running `phoenix-dev` VM
- User-visible control point before next step:
  present the exact toolchain install path and command availability before the first AArch64 source patch begins
