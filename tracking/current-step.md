# Current Step

## Metadata

- Step ID: `STEP-0179`
- Title: Split generic kernel early init around the A53-specific setup block
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add the smallest post-entry visibility markers that show whether the Pi 4 lane fails before, inside, or after the A53-specific system-register block in generic kernel early init

## Scope

In scope:

- add only tiny raw markers in:
  - `phoenix-rtos-kernel/hal/aarch64/_init.S`
- keep the existing `K` marker in place
- place new markers immediately before and after the `__TARGET_AARCH64A53` setup block
- validate the new markers on:
  - generic `virt`
  - Pi 4 `raspi4b`
- update manifests and docs with the result

Out of scope:

- changes to target-family naming or selection
- changing Pi 4 image layout
- changing DTB content or selection
- semantic kernel-init changes beyond visibility
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Expected Files Or Subsystems

- generic AArch64 kernel earliest entry path
- generic `virt` and Pi 4 `raspi4b` post-entry early-init notes
- A53-specific kernel setup block visibility
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the generic kernel emits the new pre/post-block markers on the working generic lane
- the Pi 4 lane is rerun and the presence or absence of those markers is documented
- the result is specific enough to justify the next smallest follow-up step

## Validation Plan

- Review:
  inspect the touched early init assembly for minimality
- Build:
  - `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
  - `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- Emulator:
  - generic `virt` with `-smp 1`
  - Pi 4 `raspi4b` with `-smp 4`
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-kernel-a53-block-scope.md`

## Notes

- Risks:
  keep the change visibility-only and avoid widening into a target-family refactor before the failure region is pinned down
- Dependencies:
  completed `STEP-0178` post-entry A53-block split scoping
- User-visible control point before next step:
  after this step lands, the next bounded move should be chosen directly from whether Pi 4 prints the pre-block marker, the post-block marker, or neither
