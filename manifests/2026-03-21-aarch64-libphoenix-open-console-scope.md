# Manifest: libphoenix `open("/dev/console")` Scope

- Date: `2026-03-21`
- Step: `STEP-0248`
- Status: `completed`

## Goal

- choose the smallest next trace after both kernel-side console-open probes
  stayed silent

## Evidence Reviewed

Source paths:

- `sources/libphoenix/unistd/file.c`
- `sources/libphoenix/unistd/dir.c`
- `sources/phoenix-rtos-kernel/posix/posix.c`

Key findings:

- `open()` first does `stat(filename, &st)` for `O_RDWR`
- `open()` then calls `resolve_path(filename, NULL, 1, 1)`
- only after that does it call `sys_open(canonical, oflag, mode)`
- the current kernel-side `posix_open()` trace remains silent on both lanes

## Conclusion

- the next blocker is likely still in libphoenix before the kernel open syscall
- the tightest next seam is therefore the libphoenix `open()` wrapper itself

## Selected Next Step

- add one bounded libphoenix trace that distinguishes `stat()`,
  `resolve_path()`, and `sys_open()` on `"/dev/console"`
