# Manifest: Generic `psh` Integration

- Date: `2026-03-20`
- Step: `STEP-0089`
- Result: `completed`

## Scope

- add plain `psh` to the generic `user.plo`
- rebuild the required generic utils/project artifacts
- rerun the generic QEMU smoke lane

## Upstream Repositories

### `phoenix-rtos-project`

- Commit: `eccd0f5`

## Files

- `phoenix-rtos-project/_targets/aarch64a53/generic/user.plo.yaml`

## Validation

- refreshed the VM-local copied buildroot in `phoenix-dev`
- rebuilt the required generic components:
  - `phoenix-rtos-filesystems all`
  - `phoenix-rtos-devices all`
  - `phoenix-rtos-utils all`
- rebuilt the generic project/image lane with:
  `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh host project image`
- reran the generic QEMU smoke lane with:
  `timeout 12s ./scripts/aarch64a53-generic-qemu.sh`

## Validation Evidence

- the rebuilt generic image now packages:
  - `dummyfs`
  - `pl011-tty`
  - `psh`
- the visible smoke output still stops at the same point:
  - loader banner
  - loader pre-init output
  - first kernel banner line

## Notes

- this rules out the simple “missing `psh` package” explanation for the current lack of visible userspace progress
- the next smallest step should now focus on proving whether userspace startup and the PL011 console app are being reached at all

## Selected Next Step

- define the first generic userspace-start diagnostic step
