# Current Step

## Metadata

- Step ID: `STEP-0239`
- Title: Implement bounded first-result visibility for `psh` `lookup("/")`
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- distinguish “`psh` failed `lookup("/")` and is looping” from “`psh` never
  reached that syscall at all” using one bounded first-result trace

## Scope

In scope:

- review the current `psh` startup path in `psh.c` and `pshapp.c`
- update the current `psh` root-lookup trace to print on the first result,
  including failure
- rebuild the generic and Pi 4 QEMU lanes
- record the first observed result code

Out of scope:

- changing behavior
- broad syscall tracing
- real hardware work
- Pi 5 or RP1 work

## Expected Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Expected Files Or Subsystems

- `sources/phoenix-rtos-kernel/syscalls.c`
- `docs/status.md`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- generic QEMU exposes the first `psh` root-lookup result
- Pi 4 QEMU is rerun too if the result remains on the shared path
- the result narrows the next move to one concrete follow-up

## Validation Plan

- Emulator:
  - rebuild generic `virt`
  - rerun generic QEMU
  - rerun Pi 4 QEMU if the result remains shared
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-psh-root-lookup-result-scope.md`

## Notes

- Risks:
  keep the next trace one-time and `/`-specific so the syscall path stays quiet
- Dependencies:
  completed `STEP-0238` first-attempt `psh` root-lookup trace scope
- Source reminder:
  both lanes prove `psh` reaches user mode but still do not prove any observed
  root-lookup result
- User-visible control point before next step:
  after this step lands, the next follow-up should depend on the first observed
  `psh` root-lookup result code
