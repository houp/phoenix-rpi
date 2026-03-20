# Manifest: Generic Console-Ready Diagnostic Scope

- Date: `2026-03-20`
- Step: `STEP-0092`
- Result: `completed`

## Scope

- inspect the new generic QEMU smoke result that includes `pl011-tty: started`
- choose the smallest follow-up diagnostic that distinguishes “driver initialized” from “console fully registered”
- stop before implementing that diagnostic

## Findings

- the current smoke evidence already proves that the generic runtime path reaches `pl011-tty` initialization
- the next smallest unknown is whether `/dev/tty0` and `_PATH_CONSOLE` registration succeed before the shell tries to open the console
- the most localized follow-up is a second raw PL011 banner emitted only after successful `_PATH_CONSOLE` registration

## Selected Next Step

- add a raw `pl011-tty: console ready` banner immediately after successful `_PATH_CONSOLE` registration in `pl011-tty`
- rebuild the needed generic artifacts and rerun the generic QEMU smoke lane
