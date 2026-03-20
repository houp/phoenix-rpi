# Current Step

## Metadata

- Step ID: `STEP-0063`
- Title: Route generic QEMU non-secure PL011 to stdio and rerun smoke command
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- apply the smallest launcher-side serial fix that can turn the silent generic QEMU smoke lane into visible early boot output and rerun the smoke command

## Scope

In scope:

- adjust the generic QEMU launcher so stdio is routed to the non-secure PL011 used by the generic `plo` console
- refresh the copied buildroot as needed
- rerun `timeout 10s ./scripts/aarch64a53-generic-qemu.sh` in `phoenix-dev`
- record the earliest post-fix result

Out of scope:

- broader `plo` console changes
- `phoenix-rtos-tests` target additions
- Raspberry Pi-specific code
- fixing any later runtime issue beyond documenting it

## Expected Repositories

- coordination repo
- `phoenix-rtos-project`
- `plo`

## Expected Files Or Subsystems

- `phoenix-rtos-project/_targets/aarch64a53/generic-qemu/`
- `phoenix-rtos-project/_projects/aarch64a53-generic-qemu/`
- `phoenix-rtos-project/scripts/aarch64a53-generic-qemu.sh`
- `docs/status.md`
- tracking files and manifest updates for this step
- smoke output captured from the copied buildroot in `phoenix-dev`

## Acceptance Criteria

- the generic launcher routes the non-secure PL011 used by `plo` to stdio
- the unchanged smoke command is rerun successfully
- the result records whether early boot output now appears or what the next earliest runtime failure is

## Validation Plan

- Review:
  inspect the generic launcher and the QEMU `virt` UART layout as needed during result analysis
- Build:
  refresh the copied buildroot if needed
- Emulator:
  run `timeout 10s ./scripts/aarch64a53-generic-qemu.sh` inside the copied buildroot in `phoenix-dev`
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-generic-qemu-serial-fix-scope.md`

## Notes

- Risks:
  the result must stay as one launcher-side serial-routing change plus one rerun and must not silently turn into broader `plo` or kernel bring-up
- Dependencies:
  completed implementation step `STEP-0062`
- User-visible control point before next step:
  after this rerun lands, the next slice should be the smallest runtime-fix step implied by the earliest observed post-routing result
