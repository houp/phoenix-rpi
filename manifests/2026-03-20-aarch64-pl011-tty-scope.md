# Manifest: First Reusable PL011 TTY Driver Scope

- Date: `2026-03-20`
- Step: `STEP-0080`
- Result: `completed`

## Scope

- inspect nearby tty-driver patterns in `phoenix-rtos-devices`
- inspect the current generic-QEMU board configuration expectations
- choose the smallest reusable PL011 driver slice

## Upstream Repositories

- none

## Findings

- `spike-tty` provides the smallest message-loop and `libklog` integration pattern
- `uart16550` provides the richer `libtty` + `/dev/console` registration pattern, but it is broader than needed for the first PL011 slice
- the current generic QEMU `board_config.h` is empty, so the first PL011 step should define a board-config contract explicitly instead of trying to add DT parsing immediately

## Selected Driver Shape

- add a new single-instance `pl011-tty` driver in `phoenix-rtos-devices/tty/pl011-tty/`
- keep the first version polling-based instead of interrupt-driven
- use `libtty` for tty semantics and `libklog` for kernel-log pumping
- register:
  - `/dev/tty0`
  - `/dev/console`
- take the first hardware parameters from `board_config.h`, with a contract shaped for reuse:
  - PL011 base address
  - PL011 input clock
  - console instance selection if needed

## Notes

- this keeps the first driver slice reusable for both generic QEMU `virt` and Raspberry Pi 4 early bring-up
- interrupt support, DT-driven probing, and multi-UART support are intentionally deferred

## Selected Next Step

- implement the first polling PL011 tty driver scaffold and validate it directly on the generic target
