# Current Step

## Metadata

- Step ID: `STEP-0470`
- Title: Diagnose the post-`plo` Pi 4 kernel-entry failure after the HDMI kernel-jump proof
- Status: `in_progress`
- Date: `2026-04-12`
- Milestone / phase: `Phase 1`

## Objective

- validate the next live boundary after real hardware proved that `plo`
  reaches the HDMI `kernel jump` panel
- stop treating missing trampoline UART `TR0..TR3` as strong evidence by
  itself
- push diagnosis into the post-`plo` EL1 / kernel-entry band
- prepare the next fix or diagnostic for that later boundary

## Scope

In scope:

- correlating the latest UART log, LED video, and HDMI screenshot
- classifying the real meaning of the three-square `plo` HDMI panel on hardware
- choosing the next smallest post-`plo` kernel-entry diagnostic or fix
- improving the post-firmware UART interpretation strategy as needed

Out of scope:

- unrelated USB or PCIe driver work
- network-boot lab setup
- unrelated Pi 5 work

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260412-000322.log`
- `/Users/witoldbolt/phoenix-rpi/manifests/2026-04-12-pi4-hdmi-kernel-jump-boundary.md`
- `/Users/witoldbolt/phoenix-rpi/scripts/capture-rpi4b-uart.sh`
- `/Users/witoldbolt/phoenix-rpi/scripts/summarize-rpi4b-uart-log.py`
- `/Users/witoldbolt/phoenix-rpi/docs/pi4-first-hardware-trial.md`
- `/Users/witoldbolt/phoenix-rpi/docs/status.md`

## Acceptance Criteria

- the new evidence is recorded clearly enough to prove that the board now
  reaches or passes the `plo` kernel-jump panel on real hardware
- the active failure boundary is restated as post-`plo` rather than armstub
- the next implementation step is selected for the post-`plo` boundary
- the docs explicitly record that missing `TR0..TR3` is no longer sufficient by
  itself to classify the failure as pre-trampoline

## Validation Plan

- analyze:
  [summarize-rpi4b-uart-log.py](/Users/witoldbolt/phoenix-rpi/scripts/summarize-rpi4b-uart-log.py)
- correlate:
  HDMI screenshot, UART log, and LED video
- record:
  the updated live boundary in status, tracker, and a manifest

## Rollback / Baseline

- latest implementation manifest:
  `/Users/witoldbolt/phoenix-rpi/manifests/2026-04-12-pi4-hdmi-kernel-jump-boundary.md`

## Notes

- the latest real retry now proves a much later boundary:
  - the UART log still shows low-memory placement active
  - the HDMI screenshot shows the `plo` three-stage progress panel with all
    three squares lit
  - source review confirms that this corresponds to `video_markKernelJump()`
    immediately before `hal_exitToEL1()`
- the next active blocker is therefore after the current `plo` HDMI panel, in
  the EL1 / kernel-entry band rather than in the old armstub handoff seam
