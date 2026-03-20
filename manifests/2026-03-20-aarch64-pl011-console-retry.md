# Manifest: `pl011-tty` Console-Retry Validation

- Date: `2026-03-20`
- Step: `STEP-0123`
- Status: `completed`

## Goal

- test the smallest shared console-readiness fix by adding bounded driver-local `create_dev()` retries in `pl011-tty`

## Temporary Patch Outcome

- a bounded retry helper was added locally in `pl011-tty`
- the patch was validated on:
  - generic `virt`
  - Pi 4 DTB-backed `raspi4b`
- neither lane advanced beyond:
  - `pl011-tty: started`

Observed runtime result:

- generic `virt`
  - still reached the kernel banner and `pl011-tty: started`
  - still did not reach `tty0 ready` or `console ready`
- Pi 4 DTB-backed `raspi4b`
  - still reached loader startup and `pl011-tty: started`
  - still did not reach `tty0 ready` or `console ready`

## Decision

- the retry patch did not produce new console output on either lane
- the patch was reverted and not committed upstream

## Conclusion

- a blind driver-local `create_dev()` retry is not enough to move the current boundary
- the next smallest useful step is diagnostic, not another speculative workaround

## Selected Next Step

- add raw UART-side diagnostics around the `pl011-tty` registration path so the next run can distinguish:
  - failure before `/dev/tty0` registration starts
  - blocking inside registration
  - failure after a specific registration attempt
