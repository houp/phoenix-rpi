# Manifest: `pl011-tty` Registration Diagnostic Scope

- Date: `2026-03-20`
- Step: `STEP-0124`
- Status: `completed`

## Goal

- choose the smallest next diagnostic step after the negative `create_dev()` retry experiment

## Evidence

- both lanes still stop at `pl011-tty: started`
- the retry experiment produced no new output
- stderr-based failure messages are not visible on the current QEMU UART captures, so they are not sufficient for this boundary

## Conclusion

- the next step must emit raw UART-side progress markers around `/dev/tty0` and `/dev/console` registration
- this is smaller and higher-signal than another speculative retry or init-order change

## Selected Next Step

- add raw PL011 banners immediately:
  - before `/dev/tty0` registration
  - after `/dev/tty0` success or failure
  - before `/dev/console` registration
  - after `/dev/console` success or failure
- validate on both:
  - generic `virt`
  - Pi 4 DTB-backed `raspi4b`
