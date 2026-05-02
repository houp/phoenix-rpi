# Current Implementation Step

## Step: Cleanup after first real Pi 4 psh prompt (TD-14)

**Status**: ACTIVE — Pi 4 reaches the UART shell prompt. The next task is
to reduce diagnostic backlog and validate interactive shell commands on real
hardware.

**Date**: 2026-05-02 night

**Manifest**: `manifests/2026-05-02-td14-uart-shell-prompt.md`

## What just changed

Sibling commits:
- kernel `60703368` — relative `proc_portLookup("devfs")` fix, direct
  stored OID for the `devfs` namespace, bounded TD-14 `proc_send("devfs")`
  timing probe.
- devices `63f1d438` — PL011 minimal stat/attr support plus direct
  `/dev/console` alias.
- devices `3ee4702` — `TIOCSPGRP` now stores the requested foreground
  process-group ID directly.
- libphoenix `3c76bba` — temporary `/dev/console` open trace plus a narrow
  fast path that skips the second `resolve_path()` walk for the direct console
  alias.
- utils `da2f541` — psh early probes use `debug()` and bracket tty open,
  `isatty`, `tcsetpgrp`, and first `readcmd`.

Validation:
- QEMU Pi 4 smoke reaches `(psh)% help`.
- Real Pi image SHA256:
  `d219efa27dd617ea171465f601742427ca1c96f3d505fb3979a1c7a27d0c520e`.
- Real Pi log:
  `artifacts/rpi4b-uart/rpi4b-uart-20260502-220314-netboot-td14-readcmd-long.log`.

## New known boundary

The first real Pi 4 UART prompt is reached:

```text
psh: tty ready
psh: tcsetpgrp
psh: tcsetpgrp done
psh: readcmd
(psh)%
```

## Next action

Run a cleanup-focused iteration:
- Strip or gate the highest-volume TD-04/TD-14 boot probes that are no longer
  needed for the prompt boundary.
- Keep the functional fixes: `devfs` direct OID, PL011 stat/attr support,
  `TIOCSPGRP` semantics, and the temporary direct console alias/fast path.
- Rebuild and run QEMU smoke.
- Run real Pi netboot long enough to verify `(psh)%`, then run an interactive
  UART smoke if the current helper supports sending commands.

Then run:

```bash
./scripts/rebuild-rpi4b-fast.sh
./scripts/qemu-shell-smoke.sh rpi4b
./scripts/test-cycle-netboot.sh --label td14-clean-prompt --capture-secs 240 --dhcp-wait-secs 90
python3 scripts/summarize-rpi4b-uart-log.py artifacts/rpi4b-uart/<latest>.log
```
