# STEP-0358

## Title

Implement the smallest cleanup step for the stale `create_dev` probes

## Date

2026-03-22

## Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Change Summary

Removed the stale kernel-side `create_dev` probe helpers and their call sites
from `syscalls.c`.

The cleanup is intentionally narrow:

- deletes the old `create_dev` lookup and send trace helpers
- removes their temporary `traced` plumbing in `syscalls_msgSend()` and
  `syscalls_lookup()`
- keeps runtime behavior unchanged apart from the obsolete console spam

This follows the repository rule that failed-hypothesis debug instrumentation
must be removed once it is no longer needed.

## Files

- `sources/phoenix-rtos-kernel/syscalls.c`

## Validation

Validated in `phoenix-dev` with the current DTB-prepared Pi 4 image and the
existing QEMU helpers:

```sh
./scripts/qemu-shell-smoke.sh generic
./scripts/qemu-shell-smoke.sh rpi4b
/bin/bash /Users/witoldbolt/phoenix-rpi/scripts/qemu-rpi4b-hdmi-smoke.sh
```

Observed results:

- generic shell smoke passed
- Pi 4 shell smoke passed cleanly and returned to `(psh)%` without the stale
  `create_dev` interleaving
- Pi 4 HDMI smoke passed

## Result

The fast-lane QEMU gates are clean again after the temporary shell-race
diagnostic work:

- generic shell smoke passes
- Pi 4 shell smoke passes
- Pi 4 HDMI smoke passes

## Upstream Commit

- `phoenix-rtos-kernel 3182c7e7`

## Next Step

- scope the smallest xHCI pre-run operational step beyond command-space
  register programming
