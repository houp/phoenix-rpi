# Manifest: Generic AArch64 QEMU Serial Routing Fix

- Date: `2026-03-20`
- Step: `STEP-0063`
- Result: `completed`

## Scope

- route the generic QEMU non-secure PL011 used by `plo` to stdio
- refresh the copied buildroot in `phoenix-dev`
- rerun the unchanged generic QEMU smoke command

## Upstream Repositories

### `phoenix-rtos-project`

- Commit: `746d44e`

## Files

- `phoenix-rtos-project/scripts/aarch64a53-generic-qemu.sh`

## Validation

- refreshed the VM-local copied buildroot in `phoenix-dev` with:
  `cd /Users/witoldbolt/phoenix-rpi && ./scripts/prepare-buildroot.sh --copy-components /home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy`
- reran the unchanged smoke command there:
  `timeout 10s ./scripts/aarch64a53-generic-qemu.sh`

## Validation Evidence

- the generic QEMU lane now produces visible `plo` serial output:
  - `Phoenix-RTOS loader v. 1.21`
  - `hal: Cortex-A53 Generic`
  - `cmd: Executing pre-init script`
- the next runtime blocker is now inside `plo` script loading:
  - `Can't open user.plo, on ram0`

## Notes

- this is the first successful end-to-end visible boot milestone for the generic AArch64 QEMU lane
- the current output strongly suggests the next smallest fix belongs in the generic pre-init PHFS setup rather than in serial, EL3 entry, or kernel code

## Selected Next Step

- define the smallest generic pre-init fix needed to let `plo` open `user.plo` from `ram0`
