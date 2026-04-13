# Current Step

## Metadata

- Step ID: `STEP-0477`
- Title: Await the next Pi 4 board retry with stabilized HDMI visibility
- Status: `in_progress`
- Date: `2026-04-12`
- Milestone / phase: `Phase 1`

## Objective

- verify that the "Phoenix-RTOS HDMI console" text returns to the screen
- observe the extended libklog status and tty0 registration logs on HDMI
- capture a successful UART log using the 103448 baud rate (if 115200 remains broken)
- observe the first Phoenix shell (psh) output on HDMI or UART

## Scope

In scope:
- analysis of the next real-device trial results
- verification of stabilized HDMI-mirrored logs
- verification of thread stack robustness

Out of scope:
- broad kernel or driver changes before seeing the next feedback

## Acceptance Criteria

- HDMI console shows the initial banner AND subsequent libklog/tty progress
- a readable UART log is captured (either 115200 or 103448)
- no userspace hangs in the pl011-tty driver

## Validation Plan

- analyze the next log and video/screenshot
- use the results to choose between baud-rate refinement, devfs/dummyfs debugging, or shell integration

## Rollback / Baseline

- latest stabilized image:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  (SHA-256: `35318a72268e95662d86ea220b9fc3a915764df1aadcd8b25be55deb33ec7d24`)

## Notes

- the regression was likely caused by usleep/LED-loop timer dependencies or stack overflow
- thread stacks in pl011-tty are now 4KB (up from 1KB/2KB)
- config.txt is back to the last known working state for clocks
