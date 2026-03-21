# Current Step

## Metadata

- Step ID: `STEP-0251`
- Title: Inspect the built `psh` image and `open` references
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- determine why `psh_ttyopen()` reports `open -2` while both the libphoenix and
  kernel open traces stay silent

## Scope

In scope:

- inspect the built `psh` binary and its symbol references
- avoid code changes unless the inspection exposes one tiny, obvious next move

Out of scope:

- source changes outside minimal binary or symbol inspection
- shell-policy changes
- console-device selection changes
- unrelated kernel or project changes
- Pi 5 or RP1 work
- real hardware work

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `sources/phoenix-rtos-utils/psh/psh.c`
- copied build artifacts under `phoenix-dev`
- `docs/status.md`
- `docs/source-artifacts.md`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- the inspection identifies one concrete next seam
- the selected follow-up does not widen the work beyond the shared fast lane
- the result is captured in one manifest and the next active step

## Validation Plan

- Analysis only:
  not applicable
- Emulator:
  not required unless symbol inspection leaves ambiguity that needs one
  minimal runtime split
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-console-callpath-scope.md`

## Notes

- Risks:
  avoid adding more runtime traces before checking whether the expected `open()`
  call path is actually the one used by the built `psh` image
- Dependencies:
  completed `STEP-0250` call-path scope
- Source reminder:
  both the libphoenix and kernel console-open traces stayed silent on generic
  and Pi 4
- User-visible control point before next step:
  after this step lands, the next follow-up should depend on what the built
  `psh` image actually references for `open()`
