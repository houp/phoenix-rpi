# Current Step

## Metadata

- Step ID: `STEP-0152`
- Title: Implement architectural-timer write-barrier experiment
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- determine whether explicit instruction barriers after architectural timer sysreg writes are enough to make the selected timer IRQ dispatch on the current fast lanes

## Scope

In scope:

- `sources/phoenix-rtos-kernel/hal/aarch64/aarch64.h`
- add instruction barriers after:
  - `cntp_ctl_el0`
  - `cntp_tval_el0`
  - `cntv_ctl_el0`
  - `cntv_tval_el0`
- keep timer-source policy, GIC configuration, and retry logic unchanged
- validate on the generic `virt` lane first, then on the Pi 4 DTB-backed `raspi4b` lane

Out of scope:

- broader timer redesign
- GIC policy changes
- secure-physical timer support
- changing `pl011-tty` retry semantics
- changing scheduler policy
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- `sources/phoenix-rtos-kernel`
- coordination repo

## Expected Files Or Subsystems

- `sources/phoenix-rtos-kernel/hal/aarch64/aarch64.h`
- relevant generic and Pi 4 QEMU smoke notes
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the generic lane still reaches timer-handler registration and timer arming
- the generic lane exposes whether `gic: timer dispatch` begins to appear after explicit timer-write barriers
- the experiment remains local to architectural timer sysreg writes
- neither QEMU lane regresses from current known-good boot output

## Validation Plan

- Review:
  confirm the patch stays localized to `hal/aarch64/aarch64.h` and only changes timer-write synchronization
- Build:
  rebuild the affected generic and Pi 4 project lanes in `phoenix-dev`
- Emulator:
  rerun:
  - generic `virt`
  - Pi 4 DTB-backed `raspi4b`
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-gic-ppi-config-experiment.md`

## Notes

- Risks:
  avoid widening a bounded timer-write synchronization experiment into a broader timer redesign
- Dependencies:
  completed `STEP-0151` scope decision
- User-visible control point before next step:
  after this step lands, the next bounded move should come from whether explicit timer-write barriers restore the first dispatch
