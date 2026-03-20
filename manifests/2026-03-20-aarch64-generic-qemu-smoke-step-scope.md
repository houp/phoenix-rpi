# Manifest: Generic AArch64 QEMU Smoke Step Scope

- Date: `2026-03-20`
- Step: `STEP-0058`
- Result: `completed`

## Scope

- select the first end-to-end runtime smoke command for `aarch64a53-generic-qemu`
- define the first accepted success signal and the first failure signals
- keep the step planning-only and stop before runtime execution or code changes

## Upstream Repositories

- none

## Selected Smoke Command

Run the first smoke lane inside the copied buildroot in `phoenix-dev`:

`timeout 10s ./scripts/aarch64a53-generic-qemu.sh`

## Selected Success Signal

- serial output on stdout containing `Phoenix-RTOS loader v.`

## Selected Failure Signals

- QEMU exits immediately with a launch or image-loading error
- no serial output appears before the timeout expires
- early exception or panic text appears before the loader banner

## Notes

- this smoke step comes before any emulated test-target integration because the launcher semantics, serial path, and earliest boot evidence must be stable first
- the selected command intentionally uses the existing launcher script unchanged so the next step validates the current runtime lane before introducing more helper logic

## Selected Next Step

- run the first end-to-end `aarch64a53-generic-qemu` smoke command on QEMU `virt`
