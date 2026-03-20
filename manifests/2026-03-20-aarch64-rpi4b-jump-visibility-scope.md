# Manifest: Scope First Pi 4 `hal_cpuJump()` / EL-Exit Visibility Step

- Date: `2026-03-20`
- Step: `STEP-0169`
- Status: `completed`

## Goal

- define the smallest next visibility patch that will split the current Pi 4 silence after `go: jump`

## Reviewed Paths

- `sources/plo/hal/aarch64/generic/hal.c`
- `sources/plo/hal/aarch64/generic/_init.S`
- `sources/plo/hal/aarch64/generic/console.c`

## Findings

- `cmd_go()` now proves both lanes reach `go: jump`
- `hal_cpuJump()` is still very small:
  - reject missing entry
  - `hal_interruptsDisableAll()`
  - set `hal_coreJumpFlag = 1`
  - call `hal_exitToEL1()`
- `hal_exitToEL1()` then performs the EL-specific handoff in assembly and ends in `eret` or `br x0`
- `hal_consolePrint()` is polling UART MMIO and does not depend on the earlier `log_info()` echo path

## Decision

The next implementation step should change only `sources/plo/hal/aarch64/generic/hal.c` and add raw console-visible markers for:

- missing-entry failure
- entry to `hal_cpuJump()`
- after `hal_interruptsDisableAll()`
- immediately before `hal_exitToEL1()`
- unexpected return from `hal_exitToEL1()`

## Why This Step

- it is the smallest patch that can divide C-side jump preparation from the assembly EL-exit path
- it avoids widening into assembly changes before the C boundary is exhausted
- it keeps the generic fast lane as a strong regression gate

## Explicitly Deferred

- assembly changes in `_init.S`
- board-specific Pi 4 hacks
- kernel-side instrumentation

## Acceptance Criteria

- the next patch touches only `plo/hal/aarch64/generic/hal.c`
- the generic lane still reaches the kernel banner after the new `hal:` jump markers
- the Pi 4 lane shows one of:
  - no new `hal:` jump marker after `go: jump`
  - `hal: jump entry`
  - `hal: jump irq off`
  - `hal: jump exit el1`
  - `hal: jump returned`

## Selected Next Step

- implement filtered Pi 4 `hal_cpuJump()` visibility
