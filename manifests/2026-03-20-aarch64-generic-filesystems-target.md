# Manifest: Generic AArch64 Filesystems Target

- Date: `2026-03-20`
- Step: `STEP-0071`
- Result: `completed`

## Scope

- add the generic AArch64 target file in `phoenix-rtos-filesystems`
- keep the default component set aligned with the existing board-agnostic `aarch64a53-zynqmp` file
- validate the repo directly on the generic target in `phoenix-dev`

## Upstream Repositories

### `phoenix-rtos-filesystems`

- Commit: `a25dde5`

## Files

- `phoenix-rtos-filesystems/_targets/Makefile.aarch64a53-generic`

## Validation

- refreshed the VM-local copied buildroot in `phoenix-dev`
- first attempted:
  `TARGET=aarch64a53-generic-qemu make -C phoenix-rtos-filesystems all`
- reran with the generic project path set so `board_config.h` is available:
  `TARGET=aarch64a53-generic-qemu PROJECT_PATH=/home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy/_projects/aarch64a53-generic-qemu make -C phoenix-rtos-filesystems all`

## Validation Evidence

- the first direct repo build exposed the expected current generic workflow requirement:
  `fatal error: board_config.h: No such file or directory`
- with `PROJECT_PATH` set to the generic project directory, the repo builds successfully for the generic target:
  - `libdummyfs.a`
  - `dummyfs`
  - `libext2.a`
  - `libjffs2.a`

## Notes

- the new target file itself is correct; the validation also reaffirmed that direct generic repo builds currently need the project path to supply `board_config.h`
- the next smallest repo-local unblock should stay board-agnostic and move to `phoenix-rtos-utils`

## Selected Next Step

- define the generic AArch64 `phoenix-rtos-utils` target unblock step
