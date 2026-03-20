# Manifest: Scope First Assembly-Side Pi 4 EL-Exit Visibility Step

- Date: `2026-03-20`
- Step: `STEP-0171`
- Status: `completed`

## Goal

- define the smallest next visibility patch that will split the current Pi 4 silence after `hal: jump exit el1`

## Reviewed Paths

- `sources/plo/hal/aarch64/generic/_init.S`
- `sources/plo/hal/aarch64/generic/config.h`
- `sources/phoenix-rtos-kernel/hal/aarch64/_init.S`

## Findings

- both lanes already prove they reach the C-side call into `hal_exitToEL1()`
- the loader assembly already includes the generic config header, so UART base constants are available in `_init.S`
- the next unresolved split is:
  - failure before the EL-specific `eret` / `br x0`
  - successful EL transfer followed by failure before visible kernel output
- the earliest kernel path currently has no visible marker before substantial MMU setup, so a loader-side pre-`eret` marker is the cheaper next split

## Decision

The next implementation step should change only `sources/plo/hal/aarch64/generic/_init.S` and add a tiny raw UART marker in the EL-exit path:

- one marker when `hal_exitToEL1()` is entered
- one marker immediately before the EL-specific transfer:
  - before `eret` in `exit_el3`
  - before `eret` in `exit_el2`
  - before `br x0` in `exit_el1`

Single-character or very short markers are acceptable here because the goal is boundary location, not polished logging.

## Why This Step

- it is the smallest next split after exhausting the C-side jump path
- it avoids widening into kernel assembly before the loader assembly boundary is explicit
- it keeps the current generic fast lane as a direct regression reference

## Explicitly Deferred

- earliest-kernel-entry instrumentation
- board-specific Pi 4 handoff changes
- semantic changes to the EL transition

## Acceptance Criteria

- the next patch touches only `plo/hal/aarch64/generic/_init.S`
- the generic lane still reaches the kernel banner after the new assembly marker
- the Pi 4 lane shows whether the loader reaches the EL-specific transfer point before going silent

## Selected Next Step

- implement assembly-side Pi 4 EL-exit visibility in generic `plo`
