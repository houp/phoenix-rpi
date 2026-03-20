# Current Step

## Metadata

- Step ID: `STEP-0196`
- Title: Scope the first Pi 4 patched-lane timer follow-up
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- select the smallest runtime follow-up after the automated Pi 4 QEMU DTB
  memory hook, with focus on the current post-`dummyfs` timer or wakeup stall

## Scope

In scope:

- review the current patched-lane evidence without widening into implementation
- identify the single highest-signal next runtime experiment
- keep the focus on timer-source, timer-interrupt, or wakeup delivery only
- update manifests and docs with the chosen next move

Out of scope:

- new implementation code
- broader VM or scheduler changes
- firmware-bundle or real-device work
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Expected Files Or Subsystems

- Pi 4 A72 patched-lane runtime evidence after `dummyfs: devfs initialized`
- the current timer-source, GIC, and wakeup diagnostics
- manifests and tracking updates for this planning step

## Acceptance Criteria

- one concrete next runtime experiment is selected
- that experiment stays inside the current Pi 4 patched-lane timer or wakeup
  boundary
- the result is documented precisely enough that the next code change can start
  without reopening broader DTB or VM scope

## Validation Plan

- Review:
  inspect the current runtime evidence and keep the next step limited to one
  timer or wakeup follow-up
- Build:
  not applicable
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-rpi4b-qemu-dtb-memory-hook.md`

## Notes

- Risks:
  do not let the next move widen into generic timer redesign or broad Pi 4
  subsystem work before one narrow runtime experiment is selected
- Dependencies:
  completed `STEP-0195` automated Pi 4 QEMU DTB memory hook
- Source reminder:
  official Raspberry Pi kernel DTS files on `rpi-6.19.y` and `rpi-7.0.y` are currently identical for Pi 4 and keep `memory@0` bootloader-filled plus `stdout-path` on `serial1` (aux UART); Raspberry Pi documentation also confirms that firmware applies overlays and `dtparam`s before handing the merged DTB to the OS; this step specifically targets the root memory-node cell layout, not UART alias handling
- Architecture reminder:
  Raspberry Pi 4 Model B is based on BCM2711 with a quad-core Cortex-A72 CPU; treat `aarch64a53-generic-rpi4b` only as a temporary diagnostic lane and keep new target work centered on `aarch64a72-generic-rpi4b`
- User-visible control point before next step:
  after this planning step lands, the next bounded move should be exactly one
  code experiment in the current Pi 4 patched-lane timer or wakeup path
