# Manifest: Generic AArch64 Kernel Stdout-Path Console Fix Scope

- Date: `2026-03-20`
- Step: `STEP-0068`
- Result: `completed`

## Scope

- inspect the generic AArch64 DTB serial API after DTB handoff was fixed
- compare generic kernel console selection with the local `virt,secure=on` DTB serial ordering
- choose the smallest clean console-selection fix

## Upstream Repositories

- none

## Findings

- the generic kernel console currently initializes PL011 from `serials[0]`
- the current DTB API exposes only raw serial enumeration, not a preferred console choice
- the local `virt,secure=on` DTB enumerates the secure PL011 before the normal `chosen.stdout-path` PL011

## Selected Fix

- extend the AArch64 DTB layer with a helper that prefers `chosen.stdout-path` when it points at a PL011 node
- switch the generic kernel console to use that helper instead of taking `serials[0]` directly

## Notes

- this is preferred over choosing the last serial or hardcoding a `0x09000000` console base because it generalizes the intended DTB contract instead of encoding a QEMU-specific ordering quirk
- the first implementation only needs to support the currently observed `/pl011@...` `stdout-path` form, with safe fallback to the first parsed serial when no match exists

## Selected Next Step

- implement `chosen.stdout-path`-aware console serial selection in the AArch64 DTB layer and generic kernel console, then rerun the generic QEMU smoke lane
