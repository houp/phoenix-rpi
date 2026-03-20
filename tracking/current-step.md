# Current Step

## Metadata

- Step ID: `STEP-0209`
- Title: Implement the Pi 4 local interrupt routing experiment
- Status: `planned`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add one bounded Pi 4-only local interrupt controller routing hook for the
  non-secure physical timer path and capture the resulting local pending-state
  evidence

## Scope

In scope:

- add a Pi 4-only local interrupt controller base hook
- enable `ARM_LOCAL_TIMER_INT_CONTROL0` bit `1` for the physical timer path on
  core 0
- extend the existing bounded timer probe with one local pending readback from
  `ARM_LOCAL_IRQ_PENDING0`
- validate the Pi 4 A72 patched lane and use the generic guardrail lane if the
  common code path is touched

Out of scope:

- scheduler or VM changes
- broad interrupt-controller redesign
- Pi 5 or RP1 work

## Expected Repositories

- coordination repo
- `phoenix-rtos-kernel`
- `phoenix-rtos-project`

## Expected Files Or Subsystems

- Pi 4 timer registration evidence after the restore
- Circle local-interrupt reference paths
- `sources/phoenix-rtos-kernel/hal/aarch64/interrupts_gicv2.c`
- `sources/phoenix-rtos-kernel/hal/aarch64/gtimer_timer.c`
- `sources/phoenix-rtos-kernel/hal/aarch64/generic/config.h`
- `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/board_config.h`
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the Pi 4 lane clearly reports whether the local route enable changed
  `ARM_LOCAL_IRQ_PENDING0` or GIC dispatch behavior
- the generic guardrail lane remains healthy if common code is touched
- the result narrows the next move to one concrete follow-up on the same seam

## Validation Plan

- Review:
  inspect that the change stays bounded to the Pi 4 local timer-routing path
- Build:
  - `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
  - `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb RPI4B_QEMU_MEMORY_SIZE=80000000 TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- Emulator:
  - run the Pi 4 A72 `raspi4b` lane and compare:
    - `ARM_LOCAL_IRQ_PENDING0` readback
    - `gtimer: pending`
    - `gic: timer dispatch`
  - run the generic `virt` guardrail lane if the common path changes
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-rpi4b-local-interrupt-routing-scope.md`

## Notes

- Risks:
  do not widen this into full BCM2711 local-interrupt support or SMP work
- Dependencies:
  completed `STEP-0208` local-interrupt-routing scope
- Source reminder:
  official Raspberry Pi kernel DTS files on `rpi-6.19.y` and `rpi-7.0.y` are currently identical for Pi 4 and keep `memory@0` bootloader-filled plus `stdout-path` on `serial1` (aux UART); Raspberry Pi documentation also confirms that firmware applies overlays and `dtparam`s before handing the merged DTB to the OS; this step specifically targets the root memory-node cell layout, not UART alias handling
- Architecture reminder:
  Raspberry Pi 4 Model B is based on BCM2711 with a quad-core Cortex-A72 CPU; treat `aarch64a53-generic-rpi4b` only as a temporary diagnostic lane and keep new target work centered on `aarch64a72-generic-rpi4b`
- User-visible control point before next step:
  after this step lands, the next bounded move should depend only on whether
  the local route-enable changes local pending or GIC-dispatch evidence
