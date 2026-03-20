# Manifest: Generic PL011 Target-Integration Scope

- Date: `2026-03-20`
- Step: `STEP-0082`
- Result: `completed`

## Scope

- inspect the current generic devices target and the new `pl011-tty` driver
- identify the smallest useful target-integration step
- keep the result smaller than board-config or `user.plo` integration

## Upstream Repositories

- none

## Findings

- the generic devices target currently has an intentionally empty `DEFAULT_COMPONENTS :=`
- the new `pl011-tty` driver already builds directly on the generic target
- the next smallest useful integration is therefore to make the generic devices target build `pl011-tty` by default

## Notes

- this step is preferred over immediate `board_config.h` or `user.plo` changes because it keeps the next implementation repo-local and prepares the broader generic core lane first
- board-config population and runtime image integration remain follow-up steps

## Selected Next Step

- add `pl011-tty` to the generic AArch64 devices target default components and validate `phoenix-rtos-devices all`
