# Current Step

## Metadata

- Step ID: `STEP-0485`
- Title: Await the next Pi 4 board retry with clean stabilized image
- Status: `in_progress`
- Date: `2026-04-12`
- Milestone / phase: `Phase 1`

## Objective

- verify that the stable userspace boot path is restored (psh prompt on HDMI)
- confirm that the legacy LED blinks are gone, resulting in a faster boot process
- verify that the UART lane is now stable at 115200 baud from kernel entry onward
- confirm that the PCIe `va2pa` fix persists and eliminates the `SError` exception

## Scope

In scope:
- analysis of the next real-device trial results
- verification of clean boot sequence (no old LED bursts)
- verification of `psh` accessibility

Out of scope:
- broad driver changes before seeing the next feedback

## Acceptance Criteria

- `psh` prompt appears on HDMI console
- no legacy LED diagnostic signals are seen
- a readable 115200 baud UART log is captured through kernel entry and userspace

## Validation Plan

- analyze the next log and video/screenshot
- use the results to choose between shell-based hardware tests or network/USB integration

## Rollback / Baseline

- latest clean image:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  (SHA-256: `2f2be2e7bc97500e5202ee55f960a9b1423a79d611112d527fd35868bdec5527`)

## Notes

- all manual HDMI mirroring was removed to fix the recent regression
- legacy LED probes were purged to satisfy the speed-up request
- the hardcoded 115200 baud UART init in the kernel is now active
