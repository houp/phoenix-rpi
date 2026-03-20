# Current Step

## Metadata

- Step ID: `STEP-0081`
- Title: Implement the first polling PL011 tty driver scaffold
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add the first reusable polling PL011 tty driver scaffold needed for generic QEMU `virt` and early Raspberry Pi 4 console work

## Scope

In scope:

- add a new `phoenix-rtos-devices/tty/pl011-tty/` driver directory
- keep the first version single-instance and polling-based
- validate the new driver directly on the generic target

Out of scope:

- all upstream source changes
- Pi 4 board-specific code
- Raspberry Pi-specific code
- `phoenix-rtos-tests` target additions

## Expected Repositories

- coordination repo
- `phoenix-rtos-devices`

## Expected Files Or Subsystems

- `phoenix-rtos-devices/_targets/*`
- `phoenix-rtos-devices/tty/*`
- `phoenix-rtos-devices/tty/pl011-tty/*`
- `docs/status.md`
- tracking files and manifest updates for this step
- direct code references and, if needed, direct generic-target validation output

## Acceptance Criteria

- the new `pl011-tty` driver builds directly for `aarch64a53-generic-qemu`
- the driver stays single-instance and polling-based in this first slice
- the change stays inside `phoenix-rtos-devices`

## Validation Plan

- Review:
  inspect the new driver against nearby tty-driver style and keep the first slice minimal
- Build:
  validate `phoenix-rtos-devices` directly for `aarch64a53-generic-qemu` in `phoenix-dev`
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-pl011-tty-scope.md`

## Notes

- Risks:
  the result must stay as one repo-local PL011 driver scaffold and must not silently turn into interrupt support, DT parsing, or generic-QEMU `user.plo` integration
- Dependencies:
  completed implementation step `STEP-0080`
- User-visible control point before next step:
  after the driver scaffold lands, the next step should scope the first generic target integration of that driver
