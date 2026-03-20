# Manifest: Generic AArch64 QEMU Launcher Mode Fix

- Date: `2026-03-20`
- Step: `STEP-0061`
- Result: `completed`

## Scope

- make the generic QEMU launcher executable in `phoenix-rtos-project`
- refresh the copied buildroot in `phoenix-dev`
- rerun the unchanged first smoke command

## Upstream Repositories

### `phoenix-rtos-project`

- Commit: `761d18d`

## Files

- `phoenix-rtos-project/scripts/aarch64a53-generic-qemu.sh`

## Validation

- refreshed the VM-local copied buildroot in `phoenix-dev` with:
  `cd /Users/witoldbolt/phoenix-rpi && ./scripts/prepare-buildroot.sh --copy-components /home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy`
- reran the unchanged smoke command there:
  `timeout 10s ./scripts/aarch64a53-generic-qemu.sh`

## Validation Evidence

- the smoke command now starts QEMU successfully
- QEMU remained running until the timeout and then exited with:
  `qemu-system-aarch64: terminating on signal 15 from pid ... (timeout)`
- no serial output appeared before the timeout

## Notes

- the launcher-level blocker is now removed
- the next blocker is a true early-runtime silence issue somewhere between QEMU serial routing, generic `plo` console assumptions, and earliest AArch64 entry behavior

## Selected Next Step

- define the smallest first-boot-output debugging step for the silent generic QEMU lane
