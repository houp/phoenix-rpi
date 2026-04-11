# Pi 4 UART Host Lane: `tio`-First Promotion

- Date: `2026-04-11`
- Scope: macOS-host UART tooling only
- Step: `STEP-0462`

## Summary

The canonical macOS-host Pi 4 UART capture helper was updated to prefer
`tio` automatically when it is installed. `picocom` remains available as an
explicit fallback and as the automatic fallback when `--exit-after` is
requested for local dry runs.

## Touched Files

- `/Users/witoldbolt/phoenix-rpi/scripts/capture-rpi4b-uart.sh`
- `/Users/witoldbolt/phoenix-rpi/docs/host-macos-apple-silicon.md`
- `/Users/witoldbolt/phoenix-rpi/docs/manual-operator-instructions.md`
- `/Users/witoldbolt/phoenix-rpi/docs/testing-automation.md`
- `/Users/witoldbolt/phoenix-rpi/docs/session-playbook.md`
- `/Users/witoldbolt/phoenix-rpi/docs/status.md`
- `/Users/witoldbolt/phoenix-rpi/tracking/current-step.md`
- `/Users/witoldbolt/phoenix-rpi/tracking/step-history.md`

## Helper Behavior After This Step

- default tool selection:
  - `tio` if installed
  - otherwise `picocom`
- explicit override:
  - `--tool tio`
  - `--tool picocom`
- metadata now records:
  - `serial_tool`
- `--list` now:
  - prints likely `/dev/cu.*` USB serial candidates when they exist
  - otherwise emits an explicit warning
  - and, when `tio` is present, prints `tio --list`
- `--exit-after` behavior:
  - warns that `tio` does not support it
  - falls back to `picocom` if available

## Validation

- `bash -n scripts/capture-rpi4b-uart.sh`
  - result: pass
- `scripts/capture-rpi4b-uart.sh --help`
  - result: pass
- `scripts/capture-rpi4b-uart.sh --list`
  - result: pass
  - current host fact:
    - no likely USB serial adapter is attached right now
  - helper now surfaces that state explicitly instead of returning a silent
    empty result
- `scripts/capture-rpi4b-uart.sh --tool auto --device /dev/does-not-exist --exit-after 1000`
  - result: expected warning plus clean failure
  - warning:
    - `tio` does not support `--exit-after`
  - behavior:
    - helper fell back to `picocom`
    - then reported the missing device cleanly

## Surfaced Error During Validation

A synthetic pseudo-TTY smoke using `tio` inside the Codex sandbox failed with:

- `Error: Could not open tty device (Operation not permitted)`

Classification:

- sandbox limitation during synthetic PTY validation
- not evidence of a defect in the real macOS-host USB-TTL path

Process consequence:

- the helper no longer suppresses `tio` messages with `--mute`, so real device
  open/connect errors remain visible to the operator

## Current Practical Outcome

Tomorrow's first real USB-TTL board session should use:

- `/Users/witoldbolt/phoenix-rpi/scripts/capture-rpi4b-uart.sh --device /dev/cu.usbserial-XXXX --label pi4-boot`

and let the helper choose `tio` automatically.
