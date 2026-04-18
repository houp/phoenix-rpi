# Current Step

## Metadata

- Step ID: `STEP-0517`
- Title: `Re-freeze the restored Pi 4 X3NO baseline after the failed O-to-P re-split`
- Status: `in_progress`
- Date: `2026-04-18`
- Milestone / phase: `Phase 1`

## Objective

- remove the regressing `U / V / W / Z / Y / P` post-MMU re-split and return
  the active image to the last objectively better hardware seam
- rebuild, export, and verify a fresh SD image from the restored `... X3NO`
  baseline so later retries do not accidentally reuse the regressed image
- record clearly that the fine `NO -> P` re-split was a negative result on
  real hardware

## Scope

In scope:

- restoring the kernel early-MMU path to the last better `X3NO` lineage
- rebuilding, exporting, and verifying the refreshed rollback image
- recording the real-board regression evidence from the failed re-split image
- tracker and manifest updates recording the real-board evidence

Out of scope:

- introducing another speculative MMU or post-MMU probe on top of the
  regressed image
- restarting broad probe churn from the weaker `3C` baseline
- unrelated cleanup in `plo`, armstub, DTB parsing, or user-space services

## Acceptance Criteria

- the regressing `U / V / W / Z / Y / P` split is removed from the active
  kernel tree
- a fresh exported SD image exists from the restored `X3NO` lineage
- the docs record:
  - the failing log from the re-split image
  - the refreshed rollback image SHA
  - the fact that `STEP-0516` is now a disproved hardware experiment

## Validation Plan

- flash image:
  - `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  - SHA-256 `576bacf524d115f8f99361d0434eac32a92d0f1354f8169fb5c7fa24502f39d8`
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
- disproved re-split experiment:
  - `phoenix-rtos-kernel 5f3bf75e`
  - image `ff1b0ca7b4bb89f4f8812537750487566160fc4e583368748976f80b4c200cb4`
  - log `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260418-234332.log`
  - regressed seam: `A2 KLM X1 X2 X3`
- refreshed rollback image now ready:
  - `phoenix-rtos-kernel a4883d37` restoring the `91f5f9d5` lineage in
    `_init.S`
  - `phoenix-rtos-project e8f794f`
  - image `576bacf524d115f8f99361d0434eac32a92d0f1354f8169fb5c7fa24502f39d8`

## Notes

- the stale-image theory has already been disproved for this artifact chain
- `STEP-0515` is still the strongest real-board checkpoint restoration so far:
  the rollback successfully restored `... X3NO`
- `STEP-0516` is now closed as a negative hardware result: the finer
  `NO -> P` re-split regressed back to `X3`
- there is no hardware-backed evidence in the project history for a later UART
  seam than `... X3NO`; apparent later milestones in manifests are QEMU-only
  or HDMI-only observations
