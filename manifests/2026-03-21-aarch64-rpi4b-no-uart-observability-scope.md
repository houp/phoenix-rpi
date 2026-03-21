# Manifest: Pi 4 No-UART Observability Scope

- Date: `2026-03-21`
- Step: `STEP-0282`
- Focus: choose the smallest technical path to produce a meaningful runtime
  signal on a Pi 4 board without USB-TTL serial

## Decision

- the next bounded technical move should be a `plo`-side Raspberry Pi mailbox
  framebuffer visibility step

## Rationale

- the current user hardware includes HDMI but not USB-TTL serial
- a full network-visible path is strategically valuable but too wide as the next
  bounded step
- a one-off GPIO or LED signal would be narrower but much less reusable
- official and project-local references indicate that current `raspi4b` QEMU
  supports mailbox plus VideoCore property-firmware behavior, which makes a
  mailbox framebuffer path testable before real hardware
- external references already analyzed in this repo provide concrete mailbox and
  framebuffer examples:
  - `external/rpi4-osdev/part5-framebuffer/mb.c`
  - `external/rpi4-osdev/part5-framebuffer/fb.c`
  - `external/circle/lib/bcmmailbox.cpp`
  - `external/circle/lib/bcmframebuffer.cpp`
- Phoenix already has a `plo`-side `graphmode` handoff structure:
  - `plo/syspage.c:syspage_graphmodeSet()`
- but the current AArch64 kernel path does not yet expose an equivalent
  `pctl_graphmode` consumer like IA32, so the next step should stop at a
  visible `plo`-side framebuffer signal rather than widening into a full runtime
  fb console

## Next Bounded Move

- implement one minimal Pi 4 mailbox-framebuffer step in `plo` that:
  - performs a property-mailbox call
  - allocates a simple 32-bpp framebuffer
  - produces one visible HDMI-side sign of life
  - keeps the first change narrowly scoped to early visibility, not full
    graphics or runtime display support
