# 2026-03-21: implement the Pi 4 HDMI visibility smoke helper

## Scope

- Step: `STEP-0285`
- Goal: turn the current Pi 4 `plo` HDMI marker path into a one-command QEMU
  regression check

## Change

- added:
  - `scripts/qemu-rpi4b-hdmi-smoke.sh`

The helper:

- runs the current `raspi4b` QEMU lane inside `phoenix-dev`
- uses the already-built copied-buildroot artifacts
- captures a framebuffer dump through the QEMU monitor
- validates the current early HDMI marker signature:
  - `1024 x 768` image size
  - bright marker samples at `(20, 20)` and `(100, 40)`
  - background samples at `(200, 120)` and `(639, 479)`
- exits non-zero on regression

## Validation

Command:

```sh
./scripts/qemu-rpi4b-hdmi-smoke.sh
```

Observed result:

```text
Pi 4 HDMI smoke passed
Framebuffer: 1024x768
(20, 20): (240, 240, 240)
(100, 40): (240, 240, 240)
(200, 120): (160, 96, 48)
(639, 479): (160, 96, 48)
```

## Result

- the first Pi 4 HDMI visibility path is now guarded by a repeatable QEMU smoke
  helper
- future sessions no longer need to re-create the monitor plus `screendump`
  sequence manually just to confirm the early framebuffer marker still works
