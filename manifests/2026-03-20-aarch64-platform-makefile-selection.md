# AArch64 Platform Makefile Selection

## Summary

- Date: `2026-03-20`
- Step name: `STEP-0009`
- Scope: generalize the top-level AArch64 platform Makefile selection in `phoenix-rtos-kernel` and `plo` while preserving the existing `aarch64a53-zynqmp` build path
- Validation lanes used:
  - Linux VM build validation in `phoenix-dev`
- Result: success

## Repositories

| Repository | Remote URL | Branch | Commit SHA | Status |
| --- | --- | --- | --- | --- |
| `phoenix-rtos-kernel` | `https://github.com/phoenix-rtos/phoenix-rtos-kernel.git` | `codex/common-aarch64-platform-makefiles` | `6d5e9b9e4077d2f2018bedc68eec8b68a8255c47` | modified and validated |
| `plo` | `https://github.com/phoenix-rtos/plo.git` | `codex/common-aarch64-platform-makefiles` | `6c79154885b2c0167b0eba2736243d9a9ba37a0d` | modified and validated |
| `phoenix-rtos-build` | `https://github.com/phoenix-rtos/phoenix-rtos-build.git` | `master` | `488bdb942a1aca100ab28ecfd7fcabb56046ba91` | unchanged, used for validation |
| `phoenix-rtos-project` | `https://github.com/phoenix-rtos/phoenix-rtos-project.git` | `master` | `f25a4953501e1b63a7835164a08d77cf08570bc0` | unchanged, used for validation |

## Validation Evidence

- Copied buildroot refresh command:
  - `./scripts/prepare-buildroot.sh --copy-components`
- Validation buildroot:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy`
- Validation command:
  - `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project`
- Result summary:
  - the existing `aarch64a53-zynqmp-qemu` build completed successfully after the Makefile generalization
  - both `phoenix-aarch64a53-zynqmp.elf` and the `plo` images were rebuilt in the copied buildroot
- Emulator:
  - not run in this step
- Hardware:
  - not applicable
- Image or boot-tree location:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy/_build/aarch64a53-zynqmp-qemu`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy/_boot/aarch64a53-zynqmp-qemu`

## Notes

- New constraints discovered:
  - copied buildroots intentionally exclude `.git`, so some builds emit harmless version-probe noise such as `fatal: not a git repository`; do not treat that message alone as a build failure when the build continues and exits successfully
- Docs updated:
  - `docs/status.md`
  - `tracking/current-step.md`
  - `tracking/step-history.md`
- Next smallest task:
  - define a narrow first step toward a generic non-Xilinx AArch64 QEMU lane
