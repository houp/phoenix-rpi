# Manifest: Generic `psh` Integration Scope

- Date: `2026-03-20`
- Step: `STEP-0088`
- Result: `completed`

## Scope

- inspect the updated generic smoke result after packaging `dummyfs` and `pl011-tty`
- inspect comparable minimal console-plus-shell scripts
- choose the smallest useful follow-up runtime step

## Upstream Repositories

- none

## Findings

- the generic image now already has the minimal console prerequisites packaged:
  - `dummyfs`
  - `pl011-tty`
- the comparable `riscv64-generic-spike` target uses the next minimal step:
  - `dummyfs`
  - tty driver
  - `psh`
- the generic AArch64 utils target already builds `psh`, so adding `psh` to `user.plo` is a small, direct next step

## Notes

- this is preferred over a temporary driver-only diagnostic because it advances the generic QEMU lane toward the real boot goal
- the step should use plain `psh` first, without introducing a new `rc.psh` overlay yet

## Selected Next Step

- add `psh` to the generic `user.plo`, rebuild the needed artifacts, and rerun the generic QEMU smoke lane
