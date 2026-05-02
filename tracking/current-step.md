# Current Implementation Step

## Step: Root-cause post-console psh stall (TD-14, next boundary)

**Status**: ACTIVE — Pi 4 now reaches `pl011-tty: console ready`, but no
`psh: root ready` or `(psh)%` appears within the latest 240 s hardware
capture.

**Date**: 2026-05-02 night

**Manifest**: `manifests/2026-05-02-td14-devfs-direct-checkpoint.md`

## What just changed

Sibling commits:
- kernel `60703368` — relative `proc_portLookup("devfs")` fix, direct
  stored OID for the `devfs` namespace, bounded TD-14 `proc_send("devfs")`
  timing probe.
- devices `63f1d438` — PL011 minimal stat/attr support plus direct
  `/dev/console` alias.
- utils `50cf5605` — psh `ttyopen` errno diagnostics.

Validation:
- QEMU Pi 4 smoke reaches `(psh)% help`.
- Real Pi image SHA256:
  `06071d7aac0de7d54b635d297cca9474ff4eacda13a6be3471f044ba454bb3a4`.
- Real Pi log:
  `artifacts/rpi4b-uart/rpi4b-uart-20260502-211848-netboot-td14-devfs-direct.log`.

## New known boundary

The old repeated `lookup("devfs")` root-query wall is cleared:

```text
name: devfs direct hit
pl011-tty: tty0 lookup ok
pl011-tty: tty0 ready
pl011-tty: register console
name: devfs direct hit
pl011-tty: console ready
threads: psh user scheduled
```

The log then stops before:
- `psh: root ready`
- `psh: app run`
- `psh: tty open`
- `(psh)%`

## Next action

Instrument one narrow path only:
- psh process entry / first user instruction if practical.
- `psh_run()` before and after the `lookup("/")` loop.
- `open("/dev/console")`: separate `stat`, `resolve_path`, and `sys_open`
  outcomes.
- Kernel `proc_lookup("/")` and `/dev/console` only if user-space prints do
  not establish the boundary.

Then run:

```bash
./scripts/rebuild-rpi4b-fast.sh
./scripts/qemu-shell-smoke.sh rpi4b
./scripts/test-cycle-netboot.sh --label td14-psh-boundary --capture-secs 300 --dhcp-wait-secs 90
python3 scripts/summarize-rpi4b-uart-log.py artifacts/rpi4b-uart/<latest>.log
```
