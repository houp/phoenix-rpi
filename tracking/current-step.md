# Current Step

## Metadata

- Step ID: `STEP-0207`
- Title: Restore the Pi 4 timer-group baseline
- Status: `planned`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- remove the temporary Pi 4-only `TIMER_IRQ_GROUP 0U` override so the next
  runtime experiment starts from the validated pre-step baseline

## Scope

In scope:

- remove only the Pi 4 A72 board-level timer-group override
- keep the generic kernel override hook intact
- validate that the Pi 4 lane returns to the pre-experiment `grp 1 en 1`
  registration state
- update manifests and docs with the restored baseline

Out of scope:

- scheduler or VM changes
- new timer or interrupt-routing experiments
- broad interrupt-controller redesign
- Pi 5 or RP1 work

## Expected Repositories

- `phoenix-rtos-project`
- coordination repo

## Expected Files Or Subsystems

- `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/board_config.h`
- Pi 4 timer registration evidence after the revert
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the Pi 4 A72 lane returns to `gic: timer handler set grp 1 en 1`
- the temporary board-level Group 0 override is removed from the tree
- the next runtime experiment can start from the restored baseline without
  ambiguity about the active timer-group setting

## Validation Plan

- Review:
  inspect that the revert removes only the Pi 4 board override
- Build:
  - `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb RPI4B_QEMU_MEMORY_SIZE=80000000 TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- Emulator:
  - run the automated Pi 4 A72 `raspi4b` lane and confirm the timer
    registration readback returns to `grp 1 en 1`
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-rpi4b-timer-group-override.md`

## Notes

- Risks:
  do not mix the baseline restore with the next timer-routing experiment
- Dependencies:
  completed `STEP-0206` timer-group override experiment
- Source reminder:
  official Raspberry Pi kernel DTS files on `rpi-6.19.y` and `rpi-7.0.y` are currently identical for Pi 4 and keep `memory@0` bootloader-filled plus `stdout-path` on `serial1` (aux UART); Raspberry Pi documentation also confirms that firmware applies overlays and `dtparam`s before handing the merged DTB to the OS; this step specifically targets the root memory-node cell layout, not UART alias handling
- Architecture reminder:
  Raspberry Pi 4 Model B is based on BCM2711 with a quad-core Cortex-A72 CPU; treat `aarch64a53-generic-rpi4b` only as a temporary diagnostic lane and keep new target work centered on `aarch64a72-generic-rpi4b`
- User-visible control point before next step:
  after the baseline is restored, the next bounded move should change exactly
  one new variable on the Pi 4 timer-to-GIC seam
