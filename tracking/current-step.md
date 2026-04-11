# Current Step

## Metadata

- Step ID: `STEP-0463`
- Title: Await the first Pi 4 UART-assisted board retry
- Status: `in_progress`
- Date: `2026-04-11`
- Milestone / phase: `Phase 1`

## Objective

- capture the first real Pi 4 boot attempt with the USB-TTL adapter connected
- treat UART as the primary runtime boundary signal and LED video as the
  secondary earliest-entry signal
- use the combined evidence to replace the current ambiguous LED-only boundary
  with a direct boot-stage classification

## Scope

In scope:

- one UART log captured through the canonical host helper
- one matching LED video in parallel if practical
- log summary and failure classification
- deciding the next smallest real boot fix from the combined UART-plus-LED data

Out of scope:

- network-boot lab setup
- unrelated Pi 4 driver work
- speculative armstub or `plo` instrumentation before the first UART-equipped
  retry is actually captured

## Expected Repositories

- coordination repo
- potentially `phoenix-rtos-project`
- potentially `plo`

## Expected Files Or Subsystems

- `/Users/witoldbolt/phoenix-rpi/scripts/capture-rpi4b-uart.sh`
- `/Users/witoldbolt/phoenix-rpi/scripts/summarize-rpi4b-uart-log.py`
- `/Users/witoldbolt/phoenix-rpi/docs/manual-operator-instructions.md`
- `/Users/witoldbolt/phoenix-rpi/docs/testing-automation.md`
- `/Users/witoldbolt/phoenix-rpi/docs/status.md`

## Acceptance Criteria

- a raw UART log is captured through the canonical helper
- the log is summarized and classified
- the next boot failure band is identified more directly than the current
  LED-only ambiguity allows

## Validation Plan

- [capture-rpi4b-uart.sh](/Users/witoldbolt/phoenix-rpi/scripts/capture-rpi4b-uart.sh) `--list`
- [capture-rpi4b-uart.sh](/Users/witoldbolt/phoenix-rpi/scripts/capture-rpi4b-uart.sh) `--device /dev/cu.usbserial-XXXX --label pi4-boot`
- [summarize-rpi4b-uart-log.py](/Users/witoldbolt/phoenix-rpi/scripts/summarize-rpi4b-uart-log.py) `/path/to/log`
- optional parallel ACT-LED video decode

## Rollback / Baseline

- latest UART host-lane manifest:
  `/Users/witoldbolt/phoenix-rpi/manifests/2026-04-11-pi4-uart-tio-first-host-lane.md`

## Notes

- default helper behavior now prefers `tio`
- `picocom` remains the explicit fallback path
- if early bootloader text is still absent, the next manual board-side action
  is to enable EEPROM `BOOT_UART=1` on a known-good Raspberry Pi OS card
