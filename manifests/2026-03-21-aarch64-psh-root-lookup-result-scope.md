# Manifest: `psh` Root-Lookup First-Result Scope

- Date: `2026-03-21`
- Step: `STEP-0238`
- Status: `completed`

## Goal

- choose the smallest next hook that can distinguish a failed `psh`
  `lookup("/")` loop from “no root lookup reached”

## Evidence Reviewed

Current cross-lane result:

- both generic and Pi 4 print:
  - `threads: psh user scheduled`
- neither lane prints:
  - `syscalls: psh root lookup ok`

Relevant source path:

- `sources/phoenix-rtos-kernel/syscalls.c`

Current hook limitation:

- the current `psh` root-lookup trace prints only on success
- silence still leaves two possibilities:
  - `psh` is calling `lookup("/")` and getting a negative result
  - `psh` is not reaching that syscall path yet

## Selected Next Implementation Step

- keep the trace in `syscalls_lookup()` only
- change it to print once on the first `psh` lookup of `/`, regardless of
  success or failure
- include the integer result code in the marker

## Why This Is The Right Next Step

- it changes only one existing narrow trace hook
- it avoids broad syscall spam by staying one-time, process-filtered, and
  path-filtered
- it directly answers the remaining ambiguity from `STEP-0237`

## Selected Next Step

- implement bounded first-result visibility for `psh` `lookup("/")` in:
  - `sources/phoenix-rtos-kernel/syscalls.c`
