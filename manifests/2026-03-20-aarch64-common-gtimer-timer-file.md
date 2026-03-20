# Manifest: Common Public AArch64 Gtimer Timer File

- Date: `2026-03-20`
- Step: `STEP-0046`
- Result: `completed`

## Scope

- add the first common public AArch64 timer implementation file in `phoenix-rtos-kernel`
- fix the Makefile override path so the new file can be selected through `AARCH64_TIMER_IMPL_OVERRIDE`
- validate the new file without changing the default ZynqMP timer selection

## Upstream Repositories

### `phoenix-rtos-kernel`

- Branch: `codex/common-aarch64-platform-makefiles`
- Commit: `5f37bb2a`

## Files

- `phoenix-rtos-kernel/hal/aarch64/Makefile`
- `phoenix-rtos-kernel/hal/aarch64/gtimer_timer.c`

## Validation

- Refreshed the copied buildroot inside `phoenix-dev` with:
  `./scripts/prepare-buildroot.sh --copy-components`
- Rebuilt the existing AArch64 lane with the timer override enabled:
  `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project`
- Validation environment:
  `AARCH64_TIMER_IMPL_OVERRIDE='$(addprefix $(PREFIX_O)hal/aarch64/, gtimer_timer.o)'`

## Validation Evidence

- the kernel build compiled `hal/aarch64/gtimer_timer.c`
- the kernel build did not compile `hal/aarch64/zynqmp/timer.c` in the overridden kernel lane
- the overall copied-buildroot build completed successfully in `phoenix-dev`
- the default target selection remains unchanged outside the override validation lane

## Notes

- `plo` still builds its platform-specific ZynqMP timer in the same project build, which is expected because this step only affects the kernel timer implementation selection
- the next planning step should explicitly bias the project toward the earliest realistic Pi 4 boot path, now that the common AArch64 timer file exists
