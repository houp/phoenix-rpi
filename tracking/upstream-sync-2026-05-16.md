# Upstream Sync Note: 2026-05-16

## Purpose

This note is for the main Raspberry Pi 4 cache/MMU bring-up agent. A separate
upstream-following pass fetched all official Phoenix RTOS `origin` remotes and
merged upstream where it was safe to do so without touching the dirty kernel
cache worktree.

## What changed

Fast-forwarded local `master` branches that were only behind upstream:

- `sources/phoenix-rtos-doc`
- `sources/phoenix-rtos-hostutils`
- `sources/phoenix-rtos-lwip`
- `sources/phoenix-rtos-ports`
- `sources/phoenix-rtos-posixsrv`
- `sources/phoenix-rtos-tests`

Created `codex/upstream-sync-20260516` branches and merged `origin/master`
there for clean-diverged repos:

- `sources/libphoenix`
- `sources/phoenix-rtos-build`
- `sources/phoenix-rtos-devices`
- `sources/phoenix-rtos-filesystems`
- `sources/phoenix-rtos-project`
- `sources/phoenix-rtos-utils`
- `sources/plo`

No merge was attempted in `sources/phoenix-rtos-kernel` because the active
cache/MMU branch is dirty.

## Kernel warning

`sources/phoenix-rtos-kernel` has 44 incoming upstream commits on
`origin/master`. A synthetic merge reports a real content conflict in:

- `proc/name.c`

Relevant local-only commits touching that path include:

- `70e561a0 proc: trace devfs lookup state`
- `60703368 rpi4b: stabilize devfs lookup during TD-14`

Relevant upstream incoming commits touching that path include:

- `447f8169 !syscalls: add sys_portUnregister syscall`
- `64f2e8eb !syscalls/portRegister: pass string len from userspace`
- `1c7fac5e posix: add proc_destroy() function`

Recommendation: before any kernel upstream merge, either commit or shelve the
current dirty cache diagnostics, then resolve the `proc/name.c` conflict in a
dedicated kernel sync branch. Do not merge upstream directly into the dirty
cache/MMU working branch.

## Upstream themes to account for

- Kernel/libphoenix: signal bounds, waitpid, spawn argv/env handling,
  port register/unregister, `destroy_dev()`, VM error handling.
- Build/project/ports: port-manager feature updates, LittleFS image helper,
  submodule pointer updates, CI/lint updates.
- Devices/lwIP: STM32N6 USB/client work, usbwlan and Wi-Fi/WHD updates,
  GRLIB GPIO/NAND, sensors, and half-duplex tty support.
- Plo: STM32U3/N6 support and stricter command/device validation.
