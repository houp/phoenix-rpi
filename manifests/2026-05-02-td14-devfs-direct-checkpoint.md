# 2026-05-02 TD-14 devfs-direct checkpoint

## Scope

Checkpoint for the Pi 4 TD-14 namespace/console bring-up work.

## Sibling repository revisions

- `phoenix-rtos-kernel`: `60703368` (`rpi4b: stabilize devfs lookup during TD-14`)
- `phoenix-rtos-devices`: `63f1d438` (`rpi4b: keep console alias usable during TD-14`)
- `phoenix-rtos-utils`: `50cf5605` (`psh: trace ttyopen errno during TD-14`)
- `libphoenix`: `43e050de`
- `phoenix-rtos-project`: `21bda559`

## Image

- Path: `artifacts/rpi4b/rpi4b-sd.img`
- SHA256: `06071d7aac0de7d54b635d297cca9474ff4eacda13a6be3471f044ba454bb3a4`
- Export verification: OK

## Validation

- QEMU Pi 4 smoke: PASS, reaches `(psh)% help`.
- Real Pi netboot:
  `artifacts/rpi4b-uart/rpi4b-uart-20260502-211848-netboot-td14-devfs-direct.log`

## Hardware result

The previous TD-14 wall (`lookup("devfs")` repeatedly falling through to
root dummyfs and sometimes taking 21-43 s) is bypassed. The latest Pi 4 log
shows:

```text
name: devfs direct hit
pl011-tty: tty0 lookup ok
pl011-tty: tty0 ready
pl011-tty: register console
name: devfs direct hit
pl011-tty: console ready
threads: psh user scheduled
```

No `(psh)%` prompt appears within the 240 s capture. The active blocker moves
to the post-schedule psh path before `psh: root ready`.

## Warnings / expected noise

- Firmware `HDMI1:EDID` failures are expected because HDMI0 is connected and
  HDMI1 is not.
- Firmware SD-card failures are expected in netboot-first testing when no
  valid SD boot media is inserted.
- `picocom` exits with watchdog SIGTERM (`exit 143`) at capture timeout; this
  is expected for timed UART captures.
- `cmdline.txt` is absent; firmware reports it, but this image does not rely
  on it.

## Next step

Instrument the psh/libc boundary after `threads: psh user scheduled`:
`lookup("/")`, `open("/dev/console")` stat/resolve/sys_open split, and first
shell app entry.
