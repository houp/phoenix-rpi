# Manifest: Later-Boot Interactive Probe Scope

- Date: `2026-03-21`
- Step: `STEP-0230`
- Status: `completed`

## Goal

- choose the smallest next step after the syspage spawn loop is proven healthy

## Evidence Reviewed

- both generic and Pi 4 lanes now show:
  - `main: spawned dummyfs (2)`
  - `main: spawned pl011-tty (3)`
  - `main: spawned psh (4)`
- visible boot output then goes quiet in the same way on both lanes

## Selected Next Step

- use an interactive QEMU UART session, without code changes, to test whether
  `psh` is already alive and waiting for console input
- start with the generic `virt` guardrail lane
- if the shell reacts there, repeat on Pi 4

## Why This Is The Right Next Step

- it is read-only
- it can distinguish “boot is already interactive” from “`psh` never became
  ready”
- it avoids prematurely instrumenting user-space code

## Selected Next Step

- implement a bounded interactive console probe on generic and Pi 4 QEMU lanes
