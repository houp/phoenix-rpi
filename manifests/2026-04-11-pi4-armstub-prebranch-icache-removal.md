# Pi 4 Armstub Pre-Branch `ic iallu` Removal

- Date: `2026-04-11`
- Scope: Pi 4 late armstub handoff seam
- Step: `STEP-0467`

## Summary

The first real retry on the relocatable `kernel8.img` image showed:

- firmware UART still readable up to:
  - `uart: Set PL011 baud rate to 103448.300000 Hz`
  - `uart: Baud rate change done...`
- LED decode still reaches:
  - `2`
  - `3`
  - `23`
  - `24`
  - `25`
  - `26`
  - `4`
- then the special armstub exception stage `0`

That means the current failure still sits in the tiny late-armstub band between
stage `4` and the trampoline branch.

The narrowest suspicious instruction in that band was the manual cache
invalidate:

- `ic iallu`

So this step removes that instruction and keeps the rest of the handoff seam
intact.

## Touched Repositories

- `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project`
- `/Users/witoldbolt/phoenix-rpi`

## Touched Files

- `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S`
- `/Users/witoldbolt/phoenix-rpi/docs/status.md`
- `/Users/witoldbolt/phoenix-rpi/docs/testing-automation.md`
- `/Users/witoldbolt/phoenix-rpi/docs/manual-operator-instructions.md`
- `/Users/witoldbolt/phoenix-rpi/tracking/current-step.md`
- `/Users/witoldbolt/phoenix-rpi/tracking/step-history.md`

## Why This Is The Current Best Fix

After stage `4`, the remaining live instructions were:

- `mov x0, x5`
- `dsb sy`
- `ic iallu`
- `isb`
- `br x4`

The branch target is now firmware-provided and the trampoline itself already
does the cache maintenance needed after copying the high-linked `plo` payload.

So `ic iallu` was the strongest remaining candidate for the observed EL2
exception before trampoline entry.

## Validation

- `./scripts/rebuild-rpi4b-fast.sh --scope project --qemu-sanity`
  - result: pass
- surfaced warnings/errors during this step:
  - none from the rebuild/export/verify path
- direct Pi 4 QEMU serial sanity:
  - still reaches:
    - `call: exec go!`
    - `go: enter`
    - `hal: jump exit el1`
    - `A3`
    - `KLMconsole: pl011 init done`
  - still ends in the known later:
    - `Exception #37: Data Abort (EL1)`
- canonical SD-image export:
  - result: pass
- FAT-aware SD-image verification:
  - result: pass

## Refreshed Test Artifact

- path:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- SHA-256:
  `830da43f4e3ffc85347ea9522dd9ccf6ed5c6956dc52c821813037d2dc46f639`

## Expected Next Hardware Signal

The next retry should answer the narrowed question directly:

- if `TR0` now appears:
  removing `ic iallu` unblocked the seam
- if stage `0` still appears after LED stage `4`:
  suspicion shifts from the cache invalidate to the final branch through `x4`
