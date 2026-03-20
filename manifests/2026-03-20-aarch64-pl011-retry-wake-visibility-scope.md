# Manifest: Scope `pl011-tty` Retry Wake-Return Visibility Step

- Date: `2026-03-20`
- Step: `STEP-0139`
- Status: `completed`

## Goal

- define the smallest next diagnostic step that can prove whether the retry loop stalls inside `usleep(100000)`

## Decision

The next step is bounded to:

- `sources/phoenix-rtos-devices/tty/pl011-tty/pl011-tty.c`
- keep the current bounded retry window unchanged
- add only one new raw UART marker immediately after `usleep(100000)` returns

## Why This Step

- the kernel name-service trace now proves there is no second `lookup("devfs")` call during the observed retry window
- the only bounded gap left inside the current helper is the sleep / wake-up path between `pl011-tty: tty0 lookup retry` and the next iteration

## Explicitly Deferred

- changing the retry duration
- replacing `usleep()` with a busy wait
- broader scheduler or timer debugging
- Pi 4 real-hardware work

## Acceptance Criteria

- the generic lane exposes whether the retry loop wakes up after the first sleep
- the resulting output distinguishes “sleep never returned” from “sleep returned and the second lookup blocked”

## Selected Next Step

- implement the post-`usleep()` wake-return marker and rerun both QEMU lanes
