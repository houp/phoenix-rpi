# Manifest: Generic `user.plo` PL011 Integration Scope

- Date: `2026-03-20`
- Step: `STEP-0086`
- Result: `completed`

## Scope

- inspect the current generic `user.plo`
- inspect comparable QEMU `user.plo` sequences
- choose the smallest ordering and component set that can make `/dev/console` creation viable

## Upstream Repositories

- none

## Findings

- the current generic `user.plo` still loads only:
  - the kernel
  - `system.dtb`
- comparable QEMU and generic targets that bring up userspace consoles load:
  - `dummyfs` first
  - the tty driver second
  - `psh` later
- `pl011-tty` uses `create_dev()` for `/dev/tty0` and `/dev/console`, so the smallest viable script ordering is:
  - `dummyfs;-N;devfs;-D`
  - `pl011-tty`

## Notes

- this keeps the next runtime-image step smaller than full shell bring-up
- `psh` should remain a separate follow-up after the console path itself is proven

## Selected Next Step

- add `dummyfs` and `pl011-tty` to the generic `user.plo` in that order and rerun the generic QEMU smoke lane
