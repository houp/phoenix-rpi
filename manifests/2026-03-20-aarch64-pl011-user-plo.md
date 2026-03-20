# Manifest: Generic `user.plo` Console-Prep Integration

- Date: `2026-03-20`
- Step: `STEP-0087`
- Result: `completed`

## Scope

- add `dummyfs` and `pl011-tty` to the generic `user.plo`
- keep the change smaller than full shell bring-up
- rebuild the generic image artifacts and rerun the generic QEMU smoke lane

## Upstream Repositories

### `phoenix-rtos-project`

- Commit: `503e897`

## Files

- `phoenix-rtos-project/_targets/aarch64a53/generic/user.plo.yaml`

## Validation

- refreshed the VM-local copied buildroot in `phoenix-dev`
- rebuilt the required generic components:
  - `phoenix-rtos-filesystems all`
  - `phoenix-rtos-devices all`
- rebuilt the generic project/image lane with:
  `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh host project image`
- reran the generic QEMU smoke lane with:
  `timeout 12s ./scripts/aarch64a53-generic-qemu.sh`

## Validation Evidence

- the rebuilt generic image now packages:
  - `dummyfs`
  - `pl011-tty`
- the smoke lane still reaches only the current visible milestones:
  - Phoenix loader banner
  - loader pre-init output
  - first Phoenix kernel banner line

## Notes

- this confirms that the generic image composition is moving forward, but it does not yet produce new visible userspace-console output
- the next smallest fast-lane decision should come from that updated runtime state rather than from more packaging-only work

## Selected Next Step

- define the first generic `psh` integration step after the console-prep smoke rerun
