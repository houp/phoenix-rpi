# Current Step

## Metadata

- Step ID: `STEP-0057`
- Title: Implement first `aarch64a53-generic-qemu` project entry point
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add the first `phoenix-rtos-project` target and project files needed to boot the generic AArch64 loader on QEMU `virt`

## Scope

In scope:

- add the generic AArch64 target-level build, NVM, preinit, and user-script files
- add the first generic AArch64 QEMU project directory with the minimum board-config stub
- add the first runtime script for `qemu-system-aarch64` on `virt`
- keep the first project step boot-first and avoid rootfs, test-target, or user-space console-driver expansion

Out of scope:

- `phoenix-rtos-tests` target additions
- Raspberry Pi-specific code
- solving runtime boot bugs beyond the first project entry-point build and launch path

## Expected Repositories

- `phoenix-rtos-project`
- coordination repo

## Expected Files Or Subsystems

- `phoenix-rtos-project/_targets/aarch64a53/generic-qemu/`
- `phoenix-rtos-project/_projects/aarch64a53-generic-qemu/`
- `phoenix-rtos-project/scripts/aarch64a53-generic-qemu.sh`
- `docs/status.md`
- tracking files and manifest updates for this step

## Acceptance Criteria

- `TARGET=aarch64a53-generic-qemu ./build.sh clean core project image` completes in the copied buildroot
- the build produces the first generic QEMU boot artifacts under `_boot/aarch64a53-generic-qemu/`
- the runtime script provides the first launchable QEMU `virt` command for the new generic target

## Validation Plan

- Review:
  follow the existing generic-QEMU and AArch64 project patterns while keeping the first user script kernel-only
- Build:
  refresh the copied buildroot and build the new `aarch64a53-generic-qemu` project image lane in `phoenix-dev`
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-generic-qemu-project-step-scope.md`

## Notes

- Risks:
  the first project entry point must use the new `ram-storage`-backed loader path and must not widen into test-target or user-space driver work in the same patch
- Dependencies:
  completed planning step `STEP-0056`
- User-visible control point before next step:
  after this implementation step lands, the next slice should be the first emulated smoke-lane step
