# 2026-05-02 TD-14 UART shell prompt checkpoint

## Scope

Checkpoint for the first real Raspberry Pi 4 run that reaches the Phoenix
UART shell prompt.

## Sibling repository revisions

- `phoenix-rtos-kernel`: `60703368` (`rpi4b: stabilize devfs lookup during TD-14`)
- `phoenix-rtos-devices`: `3ee4702` (`tty: use requested process group in TIOCSPGRP`)
- `phoenix-rtos-utils`: `da2f541` (`psh: keep early TD-14 probes on debug syscall`)
- `libphoenix`: `3c76bba` (`rpi4b: trace and fast-path console open during TD-14`)
- `phoenix-rtos-project`: `21bda559`

## Image

- Path: `artifacts/rpi4b/rpi4b-sd.img`
- SHA256: `d219efa27dd617ea171465f601742427ca1c96f3d505fb3979a1c7a27d0c520e`
- Export verification: OK

## Validation

- QEMU Pi 4 smoke: PASS, reaches `(psh)% help`.
- Real Pi netboot:
  `artifacts/rpi4b-uart/rpi4b-uart-20260502-220314-netboot-td14-readcmd-long.log`

## Hardware result

The real Pi 4 reaches the shell prompt:

```text
open: console sys_open done
psh: tty isatty
psh: tty isatty done
psh: tty ready
psh: tcsetpgrp
psh: tcsetpgrp done
psh: readcmd
(psh)%
```

The run still needs a long capture window because the image has heavy TD-04
and TD-14 diagnostic output. The prompt appeared near the end of a 420 s
capture. The next step is cleanup/gating of unnecessary probes followed by
interactive UART smoke commands.

## Warnings / expected noise

- Firmware `HDMI1:EDID` failures are expected because HDMI0 is connected and
  HDMI1 is not.
- Firmware SD-card failures are expected in netboot-first testing when no
  valid SD boot media is inserted.
- Firmware `cmdline.txt` missing is expected; this image does not rely on it.
- `picocom` exits with watchdog SIGTERM (`exit 143`) at capture timeout; this
  is expected for timed UART captures.
- `!! STDIN is not a TTY !! Continue anyway...` is expected from scripted
  `picocom` capture.

## Next step

Strip or gate high-volume TD-04/TD-14 boot diagnostics, rebuild, verify QEMU,
then run a shorter real-Pi prompt check and an interactive UART smoke if the
current helper can send commands.
