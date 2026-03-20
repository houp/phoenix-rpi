# Manifest: AArch64 Timer Implementation Override Hook

- Date: `2026-03-20`
- Step: `STEP-0044`
- Result: `completed`

## Scope

- update `hal/aarch64/Makefile`
- update `hal/aarch64/zynqmp/Makefile`
- add an explicit override hook for the public timer implementation object
- preserve the current ZynqMP timer implementation as the default and validate the existing `aarch64a53-zynqmp-qemu` build in `phoenix-dev`

## Touched Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Upstream Commits

- `phoenix-rtos-kernel`: `3c4fc0c5` (`aarch64: add timer implementation override hook`)

## Validation

- Refreshed the copied buildroot in `phoenix-dev`:
  `./scripts/prepare-buildroot.sh --copy-components`
- Rebuilt the existing AArch64 QEMU lane in `phoenix-dev`:
  `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project`
- Build result: success

## Key Findings

- The AArch64 build can now select a non-default public timer implementation explicitly without changing the default ZynqMP timer path.
- This creates a clean validation path for the first common public timer wrapper file without forcing a new target immediately.
- The next step can focus on the first common public `hal_timer*` wrapper file instead of more helper or build-glue preparation.

## Selected Next Step

- define the first common public AArch64 timer wrapper-file step
