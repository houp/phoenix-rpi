# Current Step

## Metadata

- Step ID: `STEP-0231`
- Title: Implement the bounded interactive console probe
- Status: `planned`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- determine whether `psh` is already live and responsive on the console after
  the successful syspage spawns

## Scope

In scope:

- run the generic QEMU lane in an interactive PTY session
- send bounded console input after boot output quiets
- if generic responds, repeat the same probe on Pi 4
- document the result and the next bounded follow-up

Out of scope:

- new kernel or loader behavior changes
- real hardware work
- broad test-framework redesign
- Pi 5 or RP1 work

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `docs/status.md`
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- generic clearly shows whether console input reaches a live shell
- Pi 4 is probed too if the generic lane proves interactive
- the result narrows the next later-boot move to one concrete follow-up

## Validation Plan

- Emulator:
  - run generic `virt` in an interactive PTY-backed session
  - send a minimal command such as newline or `help`
  - repeat on Pi 4 if generic responds
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-later-boot-interactive-probe-scope.md`

## Notes

- Risks:
  do not re-open the solved early GIC path unless new evidence forces it
- Dependencies:
  completed `STEP-0230` later-boot interactive-probe scope
- Source reminder:
  generic and Pi 4 both already spawn `psh`, so the next question is whether it
  is alive on the console
- User-visible control point before next step:
  after this step lands, the next implementation move should depend on whether
  the console is already interactive or still needs later-boot visibility
