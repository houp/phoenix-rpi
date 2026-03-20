# Manifest: `pl011-tty` Console-Retry Scope

- Date: `2026-03-20`
- Step: `STEP-0122`
- Status: `completed`

## Goal

- choose the smallest shared implementation step that can move both the generic and Pi 4 DTB-backed QEMU lanes beyond `pl011-tty: started`

## Evidence

From the current `pl011-tty` source:

- `main()` currently:
  - initializes the UART
  - calls `create_dev(&oid, "/dev/tty0")` once
  - exits immediately on failure
  - then calls `create_dev(&oid, _PATH_CONSOLE)` once
  - exits immediately on failure

From `libphoenix` `create_dev()`:

- `lookup("devfs", ...)` is retried only three times with `100 ms` sleeps
- after lookup succeeds, the actual create request is still one-shot
- there is no driver-local retry around transient create-time failures

From current runtime evidence:

- both generic and Pi 4 DTB-backed QEMU lanes reach:
  - `pl011-tty: started`
- neither lane reaches:
  - `pl011-tty: tty0 ready`
  - `pl011-tty: console ready`

## Selected Patch Shape

- keep the change local to:
  - `sources/phoenix-rtos-devices/tty/pl011-tty/pl011-tty.c`
- add a small helper that retries device registration for a bounded interval
- use it for:
  - `/dev/tty0`
  - `_PATH_CONSOLE`
- keep the existing diagnostic banners and extend them only if the retry path needs clearer failure reporting

## Why This Is The Smallest Safe Step

- it directly targets the shared runtime boundary
- it avoids a broader loader-script or devfs redesign
- it matches the already established hypothesis of transient early namespace readiness
- it is one-file, reviewable, and easy to validate on both QEMU lanes

## Selected Next Step

- implement bounded `create_dev()` retry handling in `pl011-tty`
- validate on:
  - generic `virt`
  - Pi 4 DTB-backed `raspi4b`
