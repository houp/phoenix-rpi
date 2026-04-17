# Pi 4 Tracker Reconciliation

Date: `2026-04-17`

## Purpose

Reconcile the coordination-repo tracker against the actual committed source
state after recent April 14-16 updates landed through multiple AI-assisted
sessions.

## Key Findings

1. `docs/status.md` had drifted behind the actual source-repo heads and still
   described older images such as `b9b61d48...` and `19928dd6...`, while the
   current exported host image sidecar points to:
   - `2f2be2e7bc97500e5202ee55f960a9b1423a79d611112d527fd35868bdec5527`

2. `tracking/current-step.md` overstated the cleanup status:
   - it claimed that legacy LED probes were purged
   - the committed tree still contains:
     - `PLO_RPI_ACTLED_DIAG 1` in
       `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/board_config.h`
     - the Stage-5 GPIO42 signal in
       `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-filesystems/dummyfs/srv.c`

3. `plo` still has an uncommitted cleanup diff:
   - file:
     `/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S`
   - effect:
     removes the remaining `PLO_RPI_ACTLED_DIAG` stage-code machinery from the
     earliest generic AArch64 loader path
   - status:
     not committed, therefore not part of the last reproducible baseline

4. Recent coordination commits advanced the tracker without a matching manifest
   for the April 14-16 integration state:
   - `4214d11` `phase1: clean up legacy diagnostics and stabilize userspace boot`
   - `f8ff625` `phase1: clean up and stabilize userspace boot attempt 5`

5. The committed source history now points to a later live blocker than the old
   armstub/UART-seam era:
   - latest committed userspace-facing evidence comes from:
     - `phoenix-rtos-filesystems 1ae1cbf`
     - `phoenix-rtos-devices f0f97ae`
     - `phoenix-rtos-kernel 79fa82e8`
     - `phoenix-rtos-project 67f280f`
   - the current project-level blocker is the Pi 4 userspace boot path around
     `dummyfs` / `devfs` registration and `pl011-tty` lookup, not the old
     pre-`plo` telemetry seam

## Current Committed Repo Heads

- coordination repo:
  - `/Users/witoldbolt/phoenix-rpi`
  - HEAD before this reconciliation commit: `f8ff625`
- `phoenix-rtos-project`:
  - `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project`
  - HEAD: `67f280f`
- `phoenix-rtos-kernel`:
  - `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-kernel`
  - HEAD: `79fa82e8`
- `phoenix-rtos-devices`:
  - `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-devices`
  - HEAD: `f0f97ae`
- `phoenix-rtos-filesystems`:
  - `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-filesystems`
  - HEAD: `1ae1cbf`
- `plo`:
  - `/Users/witoldbolt/phoenix-rpi/sources/plo`
  - HEAD: `1ae4d5d`
  - warning: dirty working tree in `_init.S`

## Exported Image State

- current host image:
  - `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- current sidecar metadata:
  - `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img.meta.txt`
- image SHA-256 from sidecar:
  - `2f2be2e7bc97500e5202ee55f960a9b1423a79d611112d527fd35868bdec5527`

## Warning Status

- project warning:
  the current exported image cannot be treated as fully reproducible from
  committed SHAs until the dirty `plo` cleanup diff is either committed or
  explicitly excluded and a fresh image is rebuilt

- tool warning observed during this audit:
  a first inspection command used the wrong `dummyfs` source path
  (`dummyfs/dummyfs_srv.c`) and failed with:
  `No such file or directory`
  this was a command-path mistake in the audit, not a project build issue; the
  correct committed file is `dummyfs/srv.c`

## Next Recommended Step

Do not start another Pi 4 board retry from the current mixed state.

First:

1. decide whether the dirty `plo` ACT-LED cleanup is still desired
2. commit it or drop it
3. rebuild and export a fresh Pi 4 image from the fully committed tree
4. record that image SHA-256 and exact repo heads in a new manifest
