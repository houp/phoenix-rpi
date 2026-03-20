# Current Step

## Metadata

- Step ID: `STEP-0064`
- Title: Define smallest generic pre-init fix for `user.plo` loading
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest generic project or pre-init change needed to let `plo` open `user.plo` from the RAM-backed loader image

## Scope

In scope:

- inspect the current generic target pre-init scripts and NVM layout
- compare the failing path with existing RAM-backed PHFS patterns in other Phoenix targets
- choose the narrowest fix that should let `plo` open `user.plo`

Out of scope:

- broader `plo` or kernel bring-up changes
- `phoenix-rtos-tests` target additions
- Raspberry Pi-specific code
- implementing the fix in this planning step

## Expected Repositories

- coordination repo
- `phoenix-rtos-project`
- `plo`

## Expected Files Or Subsystems

- `phoenix-rtos-project/_targets/aarch64a53/generic-qemu/`
- `phoenix-rtos-project/_targets/aarch64a53/generic/preinit.plo.yaml`
- `phoenix-rtos-project/_targets/aarch64a53/generic/user.plo.yaml`
- `phoenix-rtos-project/_targets/aarch64a53/generic/nvm.yaml`
- `phoenix-rtos-project/scripts/aarch64a53-generic-qemu.sh`
- `docs/status.md`
- tracking files and manifest updates for this step
- smoke output captured from the copied buildroot in `phoenix-dev`

## Acceptance Criteria

- the result names the smallest concrete pre-init or project-layout fix to try next
- the result explains why that fix is preferred over broader `plo` or kernel changes
- the step remains planning-only

## Validation Plan

- Review:
  inspect the generic pre-init, user script, NVM layout, and comparable RAM-backed PHFS target patterns
- Build:
  not applicable
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-generic-qemu-serial-fix.md`

## Notes

- Risks:
  the result must stay as one pre-init-fix planning step and must not silently turn into broader generic bring-up
- Dependencies:
  completed implementation step `STEP-0063`
- User-visible control point before next step:
  after this planning step lands, the next slice should be the selected first `user.plo` loading fix
