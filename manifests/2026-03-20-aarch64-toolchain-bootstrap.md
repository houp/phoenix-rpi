# AArch64 Toolchain Bootstrap

## Summary

- Date: `2026-03-20`
- Step name: `STEP-0008`
- Scope: provision the Phoenix `aarch64-phoenix` toolchain in `phoenix-dev` and document the writable copied-buildroot workflow required for the current AArch64 lane
- Validation lanes used:
  - Linux VM environment validation in `phoenix-dev`
- Result: success

## Validated Environment Artifacts

- Copied buildroot preparation command:
  - `./scripts/prepare-buildroot.sh --copy-components`
- Copied buildroot path:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy`
- Toolchain build command:
  - `env -u CC -u CFLAGS -u LIBS -u CPPFLAGS -u CXX -u CXXFLAGS -u CPP -u CXXCPP -u CXXFILT ./phoenix-rtos-build/toolchain/build-toolchain.sh aarch64-phoenix "$HOME/phoenix-toolchains"`
- Toolchain install path:
  - `/home/witoldbolt.guest/phoenix-toolchains/aarch64-phoenix`
- Toolchain sysroot:
  - `/home/witoldbolt.guest/phoenix-toolchains/aarch64-phoenix/aarch64-phoenix`

## Validation Evidence

- Verified commands:
  - `/home/witoldbolt.guest/phoenix-toolchains/aarch64-phoenix/bin/aarch64-phoenix-gcc`
  - `/home/witoldbolt.guest/phoenix-toolchains/aarch64-phoenix/bin/aarch64-phoenix-ld`
  - `/home/witoldbolt.guest/phoenix-toolchains/aarch64-phoenix/bin/aarch64-phoenix-ar`
- Verified version:
  - `aarch64-phoenix-gcc (GCC) 14.2.0`
- Notes from the successful toolchain run:
  - the final `strip` phase reported harmless "file format not recognized" warnings for `.la` files and shell scripts, but the script exited successfully and the toolchain binaries resolve correctly afterward

## Notes

- Upstream Phoenix source repositories were not modified in this step.
- New constraints discovered:
  - the current AArch64/libphoenix flow still generates files inside component source trees, so the linked buildroot is not sufficient for toolchain or current AArch64 validation work in the read-only Lima mount
  - use the copied buildroot mode for the toolchain and refresh it before later AArch64 validations after source changes
- Docs updated:
  - `docs/status.md`
  - `docs/git-repository-strategy.md`
  - `docs/manual-operator-instructions.md`
  - `docs/session-playbook.md`
  - `tracking/current-step.md`
- Next smallest task:
  - generalize the top-level AArch64 platform Makefile selection in `phoenix-rtos-kernel` and `plo` while keeping the existing `aarch64a53-zynqmp` build working
