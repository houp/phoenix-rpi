# Current Step

## Metadata

- Step ID: `STEP-0481`
- Title: Await the next Pi 4 board retry with fixed UART and PCIe
- Status: `in_progress`
- Date: `2026-04-12`
- Milestone / phase: `Phase 1`

## Objective

- verify that the UART lane is now stable at 115200 baud from kernel entry onward
- confirm that the PCIe `SError` is resolved and the VL805 firmware notify succeeds
- observe the Phoenix shell (psh) prompt on the UART console
- verify if `libklog` messages correctly appear on both HDMI and UART

## Scope

In scope:
- analysis of the next real-device trial results
- verification of fixed UART baud rate
- verification of PCIe/XHCI initialization success

Out of scope:
- broad driver changes before seeing the next log

## Acceptance Criteria

- a readable 115200 baud UART log is captured through kernel entry and userspace
- the `SError` exception no longer appears in the `pcie` or `usb` threads
- the `psh` prompt is reachable on the UART

## Validation Plan

- analyze the next log and video/screenshot
- use the results to choose between USB driver debugging, network integration, or filesystem hardening

## Rollback / Baseline

- latest fixed image:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  (SHA-256: `5a251b167f6d6bbfc299bfb8ce1f022c89bb06a171fe44afd1075e1a587327cf`)

## Notes

- the firmware choice of 103448 baud is now overridden by the kernel to 115200
- PCIe `SError` was traced to invalid `va2pa` usage in mailbox communication
- HDMI remains active as a secondary diagnostic channel
