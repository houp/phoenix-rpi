# Current Step

## Metadata

- Step ID: `STEP-0252`
- Title: Scope the smallest `resolve_path("/dev/console")` failure split
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- determine the smallest next seam inside libphoenix path canonicalization now
  that GDB has proved `open()` is reached and `resolve_path("/dev/console")`
  returns `NULL`

## Scope

In scope:

- inspect `resolve_path()` / `_resolve_abspath()` source and contract
- allow one tiny runtime split only if source review leaves one ambiguity that
  cannot be settled from the code

Out of scope:

- source changes outside minimal path-resolution inspection
- shell-policy changes
- console-device selection changes
- unrelated kernel or project changes
- Pi 5 or RP1 work
- real hardware work

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `sources/libphoenix/unistd/dir.c`
- `sources/libphoenix/unistd/file.c`
- `docs/status.md`
- `docs/testing-automation.md`
- `docs/source-artifacts.md`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- the inspection identifies one concrete next seam inside `resolve_path()` or
  `_resolve_abspath()`
- the selected follow-up does not widen the work beyond the shared fast lane
- the result is captured in one manifest and the next active step

## Validation Plan

- Analysis:
  source review of `libphoenix` path-resolution code
- Emulator:
  optional only if source review still leaves one unresolved boundary
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-gdb-console-open-path.md`

## Notes

- Risks:
  avoid widening back into kernel or `psh` tracing now that the failing seam is
  known to be inside libphoenix canonicalization
- Dependencies:
  completed `STEP-0251` GDB-backed call-path inspection
- Source reminder:
  current live result is `stat() -> -1`, `resolve_path() -> NULL`, no
  `sys_open()`
- User-visible control point before next step:
  after this step lands, the next follow-up should depend on the smallest
  failing branch inside `resolve_path()` / `_resolve_abspath()`
