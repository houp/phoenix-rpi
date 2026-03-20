# Manifest: Generic AArch64 QEMU DTB Handoff Fix

- Date: `2026-03-20`
- Step: `STEP-0067`
- Result: `completed`

## Scope

- generate `system.dtb` into `${PREFIX_ROOTFS}` during the generic QEMU project build
- load that DTB from the generic AArch64 user script before `go!`
- rebuild the generic QEMU project/image lane and rerun the smoke command

## Upstream Repositories

### `phoenix-rtos-project`

- Commit: `f1a4c82`

## Files

- `phoenix-rtos-project/_projects/aarch64a53-generic-qemu/build.project`
- `phoenix-rtos-project/_targets/aarch64a53/generic/user.plo.yaml`

## Validation

- refreshed the VM-local copied buildroot in `phoenix-dev`
- rebuilt the generic QEMU project/image lane with:
  `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh host project image`
- confirmed the generated DTB exists at:
  `_fs/aarch64a53-generic-qemu/root/etc/system.dtb`
- reran:
  `timeout 10s ./scripts/aarch64a53-generic-qemu.sh`

## Validation Evidence

- the project build now generates and packages `system.dtb`:
  - `system.dtb (offs=0x23000, size=0x100000)` in `part_kernel.img`
  - `_fs/aarch64a53-generic-qemu/root/etc/system.dtb` exists after the build
- the generic `plo` output still reaches:
  - `Phoenix-RTOS loader v. 1.21`
  - `hal: Cortex-A53 Generic`
  - `cmd: Executing pre-init script`
  - `alias: Setting relative base address to 0x0000000000200000`
- the run remains silent after the handoff and times out later, so the next blocker is beyond the raw DTB presence fix

## Notes

- the AArch64 kernel DTB contract is now satisfied in the generic QEMU project/image lane
- the next likely blocker is console selection: the generic kernel console currently uses `serials[0]`, while the `virt,secure=on` DTB enumerates the secure PL011 before the non-secure `chosen.stdout-path` UART

## Selected Next Step

- define the smallest stdout-path-aware DTB or console-selection fix for the generic AArch64 kernel
