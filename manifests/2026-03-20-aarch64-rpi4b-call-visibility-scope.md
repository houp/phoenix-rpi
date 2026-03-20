# Manifest: Scope Pi 4 Loader User-Script Visibility

- Date: `2026-03-20`
- Step: `STEP-0165`
- Status: `completed`

## Goal

- define the smallest next visibility patch that will show whether the Pi 4 lane blocks before opening `user.plo`, while reading it, or while executing its first parsed command

## Reviewed Paths

- `sources/phoenix-rtos-project/_targets/aarch64a53/generic/preinit.plo.yaml`
- `sources/phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/user.plo.yaml`
- `sources/plo/cmds/call.c`
- `sources/plo/cmds/kernel.c`
- `sources/plo/phfs/phfs.c`
- `sources/plo/devices/ram-storage/ramdrv.c`

## Findings

- the generated pre-init script for the official-DTB Pi 4 build ends with:
  - `call ram0 user.plo dabaabad`
- the generated `user.plo` begins with:
  - `kernel ram0`
  - `blob ram0 system.dtb ddr`
  - `app ram0 -x dummyfs;-N;devfs;-D ddr ddr`
  - `app ram0 -x pl011-tty ddr ddr`
  - `app ram0 -x psh ddr ddr`
  - `go!`
- `ram0` still maps to `RAM_ADDR 0x48000000`, so the previously suspected raw loader-image placement mismatch is not the best current explanation
- the narrowest unresolved split is inside `cmd_call()`:
  - `phfs_open()` may fail
  - the magic read may fail
  - the line-read loop may block before the first newline
  - `cmd_parse()` may start and then block on the first command, most likely `kernel ram0`

## Decision

The next implementation step should change only `sources/plo/cmds/call.c` and add tightly filtered one-time visibility markers for:

- successful `phfs_open()`
- successful magic check
- each parsed script line immediately before `cmd_parse()`

## Why This Step

- it is the smallest change that can split the current Pi 4 loader boundary into actionable subcases
- it avoids broad PHFS or kernel instrumentation before the first blocking phase is identified
- it preserves the current working generic `virt` lane as the fast regression gate while directly targeting the Pi 4 loader boundary

## Explicitly Deferred

- instrumenting PHFS internals
- changing Pi 4 image layout or memory placement
- changing DTB contents
- widening into kernel-side Pi 4 work before the loader call boundary is split

## Acceptance Criteria

- the next patch touches only `plo/cmds/call.c`
- the next `raspi4b` run will show one of:
  - no `call: opened` marker
  - `call: opened` but no `call: magic ok`
  - `call: magic ok` but no `call: exec ...`
  - `call: exec kernel ram0` and then stop
- the generic `virt` lane remains a working regression lane after the patch

## Selected Next Step

- implement filtered Pi 4 loader call visibility in `plo/cmds/call.c`
