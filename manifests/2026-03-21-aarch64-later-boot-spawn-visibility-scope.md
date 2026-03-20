# Manifest: Later-Boot Syspage Spawn Visibility Scope

- Date: `2026-03-21`
- Step: `STEP-0228`
- Status: `completed`

## Goal

- choose the smallest shared later-boot visibility step after
  `dummyfs: initialized`

## Evidence Reviewed

Current generic and Pi 4 parity:

- both lanes now reach:
  - `pl011-tty: console ready`
  - `main: Starting syspage programs ...`
  - `dummyfs: initialized`
- neither lane exposes later visible progress within the current QEMU timeout
  window

Relevant source path:

- `sources/phoenix-rtos-kernel/main.c`
  contains the syspage program spawn loop in `main_initthr()`

## Selected Next Visibility Step

- instrument the syspage spawn loop in `main_initthr()` only:
  - print before `proc_syspageSpawn()`
  - print success after spawn returns
  - keep the existing failure print unchanged

## Why This Is The Right Next Step

- it stays outside the solved early GIC / timer path
- it is shared by generic and Pi 4
- it can distinguish:
  - all syspage apps spawned successfully and later silence is expected
  - boot stalls in one specific spawn

## Selected Next Step

- implement bounded syspage spawn visibility in `sources/phoenix-rtos-kernel/main.c`
