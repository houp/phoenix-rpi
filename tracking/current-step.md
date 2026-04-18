# Current Step

## Metadata

- Step ID: `STEP-0513`
- Title: `Validate the identity-first Pi 4 kernel MMU image on real hardware`
- Status: `in_progress`
- Date: `2026-04-18`
- Milestone / phase: `Phase 1`

## Objective

- validate on the real Pi 4 that the newly implemented identity-first kernel
  MMU bootstrap path moves the board beyond the long-standing `3C` boundary
- confirm whether the hardware now reaches the restored post-MMU kernel path
  and normal early console output
- keep the software baseline frozen unless the first post-fix hardware retry
  proves the new strategy ineffective

## Scope

In scope:

- flashing and booting the already-built image
- UART capture and log interpretation on the real Pi 4
- if needed, one tightly bounded follow-up based on the first hardware result
- tracker and manifest updates recording the real-board evidence

Out of scope:

- restarting broad probe churn without new evidence
- unrelated cleanup in `plo`, armstub, DTB parsing, or user-space services
- speculative LED instrumentation unless UART becomes insufficient again

## Acceptance Criteria

- a real Pi 4 UART log is captured on the identity-first image
- that log either proves progress beyond `3C` or establishes a new, better
  bounded failure seam
- the docs record the exact image SHA, log path, and resulting next step

## Validation Plan

- flash image:
  - `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  - SHA-256 `5ac0d1290867556a78fe19bad048b1cfe98e8c5328053c2d588ed0d8691006fe`
- capture UART with:
  - `/Users/witoldbolt/phoenix-rpi/scripts/capture-rpi4b-uart.sh`
- summarize with:
  - `/Users/witoldbolt/phoenix-rpi/scripts/summarize-rpi4b-uart-log.py`

## Rollback / Baseline

- implementation baseline just completed:
  - `phoenix-rtos-kernel 6cd294fd`
  - image SHA-256 `5ac0d1290867556a78fe19bad048b1cfe98e8c5328053c2d588ed0d8691006fe`
- previous hardware-proven failing boundary:
  - `A2`, `KLM`, `X1`, `X2`, `3C`, then silence
  - log:
    `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260418-004543.log`

## Notes

- the stale-image theory has already been disproved for this artifact chain
- `STEP-0512` finished the structural software change; `STEP-0513` is the
  first real-board proof step on that new baseline
- if the real board still stops at `3C`, the next move should be based on the
  new hardware evidence, not a return to the older TTBR1-from-start model
