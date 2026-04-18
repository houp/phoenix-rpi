# Current Step

## Metadata

- Step ID: `STEP-0516`
- Title: `Classify the restored Pi 4 O-to-P seam from the last better baseline`
- Status: `in_progress`
- Date: `2026-04-18`
- Milestone / phase: `Phase 1`

## Objective

- use the restored `... X3NO` real-board baseline to classify the finer
  `_core_0_virtual` seam before the syspage copy completes
- distinguish whether the current kernel failure is on:
  - `relOffs` store
  - `hal_syspage` store
  - `syspage->size` load
  - first copy entry
  - first 8-byte copy
  - later copy loop
- keep the restored rollback baseline frozen except for this bounded split

## Scope

In scope:

- flashing and booting the already-built image
- UART capture and log interpretation on the real Pi 4
- if needed, one tightly bounded follow-up based on the first hardware result
- tracker and manifest updates recording the real-board evidence

Out of scope:

- restarting broad probe churn from the weaker `3C` baseline
- unrelated cleanup in `plo`, armstub, DTB parsing, or user-space services
- speculative LED instrumentation unless UART becomes insufficient again

## Acceptance Criteria

- a real Pi 4 UART log is captured on the re-split restored-baseline image
- that log classifies the seam as one of:
  - `NO`
  - `NOU`
  - `NOUV`
  - `NOUVW`
  - `NOUVWZ`
  - `NOUVWZY`
  - `NOUVWZYP`
- the docs record the exact image SHA, log path, and resulting next step

## Validation Plan

- flash image:
  - `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  - SHA-256 `ff1b0ca7b4bb89f4f8812537750487566160fc4e583368748976f80b4c200cb4`
- capture UART with:
  - `/Users/witoldbolt/phoenix-rpi/scripts/capture-rpi4b-uart.sh`
- summarize with:
  - `/Users/witoldbolt/phoenix-rpi/scripts/summarize-rpi4b-uart-log.py`

## Rollback / Baseline

- recent neutral hardware retries:
  - `phoenix-rtos-kernel 6cd294fd`
    image `5ac0d1290867556a78fe19bad048b1cfe98e8c5328053c2d588ed0d8691006fe`
    log `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260418-115137.log`
  - `phoenix-rtos-kernel 136b4cae`
    image `f44385750b37adc49bb279156e812e561c61ec8d31b983fae457215cd0fab469`
    log `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260418-220352.log`
- restored rollback baseline proved on hardware:
  - `phoenix-rtos-kernel 91f5f9d5`
  - `phoenix-rtos-project e8f794f`
  - image `be8c2773306870a5b66b75f64677d68d0a344f01ee348d2e1598aea969ca4fb1`
  - log `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260418-222500.log`
  - restored seam: `A2 KLM X1 X2 X3 NO`
- current re-split baseline just completed:
  - `phoenix-rtos-kernel 5f3bf75e`
  - `phoenix-rtos-project e8f794f`
  - image SHA-256 `ff1b0ca7b4bb89f4f8812537750487566160fc4e583368748976f80b4c200cb4`
- target seam to classify:
  - `N O U V W Z Y P`

## Notes

- the stale-image theory has already been disproved for this artifact chain
- `STEP-0515` is complete: the rollback successfully restored the last
  objectively better real-board seam, `... X3NO`
- there is no hardware-backed evidence in the project history for a later UART
  seam than `... X3NO`; apparent later milestones in manifests are QEMU-only or
  HDMI-only observations
