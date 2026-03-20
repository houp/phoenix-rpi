# Manifest: AArch64 DTB Timer Source Selection Step

- Date: `2026-03-20`
- Step: `STEP-0020`
- Result: `completed`

## Scope

- extend the AArch64 DTB API so it exposes an explicit selected generic timer source and IRQ
- encode the current common EL1 policy:
  - prefer non-secure physical timer
  - fall back to virtual timer
  - do not select secure or hypervisor timer in this path
- validate the existing `aarch64a53-zynqmp-qemu` build

## Touched Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Upstream Commits

- `phoenix-rtos-kernel`: `51558e82` (`aarch64: select generic timer source from dtb`)

## Validation

- Refreshed the copied buildroot in `phoenix-dev`:
  `./scripts/prepare-buildroot.sh --copy-components`
- Rebuilt the existing AArch64 QEMU lane in `phoenix-dev`:
  `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project`
- Build result: success

## Key Findings

- The DTB API now exposes a reusable selected generic timer source instead of only raw timer interrupt slots.
- The common EL1 policy is now explicit in one place, which will simplify the first common AArch64 timer backend.
- The next step can focus on programming the chosen timer rather than also deciding which timer to use.

## Selected Next Step

- define the first directly selectable common AArch64 generic timer backend step from the new DTB-selection baseline
