# Manifest: `posix_open()` Console Scope

- Date: `2026-03-21`
- Step: `STEP-0246`
- Status: `completed`

## Goal

- choose the smallest next trace on the real failing `/dev/console` open path

## Evidence Reviewed

Source paths:

- `sources/phoenix-rtos-kernel/posix/posix.c`
- `sources/phoenix-rtos-kernel/proc/name.c`
- `sources/phoenix-rtos-kernel/syscalls.c`

Key findings:

- `syscalls_sys_open()` passes control to `posix_open()`
- `posix_open()` calls `proc_lookup(filename, &ln, &oid)` directly
- the earlier `syscalls_lookup()` trace stays silent, which matches that code
  path

## Conclusion

- `posix_open()` is the tightest next seam
- the next bounded step should expose the first `proc_lookup()` result seen by
  `psh` for `"/dev/console"` inside `posix_open()`

## Selected Next Step

- add one one-shot `posix_open()` trace for `psh` opening `"/dev/console"`
