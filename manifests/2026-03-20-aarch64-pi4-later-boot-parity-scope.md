# Manifest: Pi 4 Later-Boot Parity Scope

- Date: `2026-03-20`
- Step: `STEP-0227`
- Status: `completed`

## Goal

- determine whether Pi 4 is still uniquely behind generic after the fixed early
  GIC / timer / console path

## Evidence Reviewed

- generic `virt` 15-second QEMU window
- Pi 4 `raspi4b` 15-second QEMU window

## Result

- within the current observed 15-second window, the Pi 4 lane is no longer
  uniquely behind the generic lane
- both lanes reach the same visible later-boot band through:
  - `pl011-tty: console ready`
  - `main: Starting syspage programs ...`
  - `dummyfs: initialized`
- the next blocker is therefore not currently Pi 4-specific in the observed
  window; it is a shared later-boot visibility / progress question

## Selected Next Step

- scope the smallest shared later-boot visibility step after
  `dummyfs: initialized`, with focus on syspage app launch progress and `psh`
  readiness
