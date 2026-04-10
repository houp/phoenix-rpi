# Pi 4 UART Host Lane Preparation

Date:

- `2026-04-11`

Purpose:

- prepare a canonical macOS-host UART debug lane before the USB-TTL adapter
  arrives, so the next Pi 4 board retry uses serial capture plus LED video
  instead of LED-only probing

What changed:

- added host capture helper:
  - [capture-rpi4b-uart.sh](/Users/witoldbolt/phoenix-rpi/scripts/capture-rpi4b-uart.sh)
- added host log summarizer:
  - [summarize-rpi4b-uart-log.py](/Users/witoldbolt/phoenix-rpi/scripts/summarize-rpi4b-uart-log.py)
- extended the first-trial report template with UART fields:
  - [create-rpi4b-first-trial-report.sh](/Users/witoldbolt/phoenix-rpi/scripts/create-rpi4b-first-trial-report.sh)
- updated the operator and testing docs for:
  - wiring
  - canonical capture command
  - `BOOT_UART=1` EEPROM guidance
  - dual-lane LED-plus-UART trials

Current UART assumptions:

- host tool:
  - `picocom`
- current serial mode:
  - `115200 8N1`
- preferred device path form on macOS:
  - `/dev/cu.*`
- current Pi 4 Phoenix `config.txt` already sets:
  - `enable_uart=1`
  - `uart_2ndstage=1`

Current official-documentation facts preserved in the docs:

- Raspberry Pi bootloader `BOOT_UART=1` enables UART debug on `GPIO14` and
  `GPIO15`
- official receiving terminal settings are `115200 8N1`
- official EEPROM edit/apply path is:
  - `sudo -E rpi-eeprom-config --edit`

Validation:

- `picocom --help`: confirms host support for `--logfile`
- `python3 -m py_compile /Users/witoldbolt/phoenix-rpi/scripts/summarize-rpi4b-uart-log.py`
- helper shell syntax checked locally before commit

Outcome:

- tomorrow's USB-TTL session can be run through one canonical host capture path
- UART logs can be preserved and summarized consistently
- LED-video decoding remains available in parallel for pre-console failures
