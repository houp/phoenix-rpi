# Manifest: GDB Console Open Path

- Date: `2026-03-21`
- Step: `STEP-0251`
- Focus: prove the real live `psh_ttyopen("/dev/console")` call path before
  changing more code

## Scope

- inspect the built generic `psh` image for `psh_ttyopen()`, `open()`,
  `stat()`, `resolve_path()`, and `sys_open()`
- attach `gdb-multiarch` to a generic `virt` QEMU gdbstub session
- stop at the live `psh_ttyopen()` and `open()` call sites and then at the
  direct post-call return points inside `open()`

## Commands Used

- Built-image inspection in `phoenix-dev`:
  - `/home/witoldbolt.guest/phoenix-toolchains/aarch64-phoenix/bin/aarch64-phoenix-nm -n _build/aarch64a53-generic-qemu/prog/psh`
  - `/home/witoldbolt.guest/phoenix-toolchains/aarch64-phoenix/bin/aarch64-phoenix-objdump -d _build/aarch64a53-generic-qemu/prog/psh`
- QEMU gdbstub lane:
  - `/usr/bin/qemu-system-aarch64 -machine virt,secure=on,gic-version=2 -cpu cortex-a53 -smp 1 -m 1G -serial mon:stdio -serial null -display none -kernel _boot/aarch64a53-generic-qemu/plo.elf -device loader,file=_boot/aarch64a53-generic-qemu/loader.disk,addr=0x48000000,force-raw=on -S -gdb tcp::1238`
  - `gdb-multiarch -q -batch -x /tmp/psh-open-step.gdb`

## Key Runtime Stops

- `psh_ttyopen()` entry at `0x412620`
  - `ttydev = "/dev/console"`
- `open()` entry at `0x422f20`
  - `filename = "/dev/console"`
- post-`stat()` stop at `0x422f64`
  - `w0 = -1`
- post-`resolve_path()` stop at `0x422f90`
  - `x0 = NULL`
- post-`open()` return to `psh_ttyopen()` at `0x412634`
  - `w0 = -1`

## Source Correlation

- Built `psh` disassembly proves `psh_ttyopen()` calls the linked libphoenix
  `open()` symbol directly.
- `sources/libphoenix/unistd/file.c` shows the relevant `open()` control flow:
  - `stat(filename, &st)` for writable opens
  - then `resolve_path(filename, NULL, 1, 1)`
  - then `sys_open(canonical, ...)` only if canonicalization succeeded
- `sources/libphoenix/unistd/dir.c` implements `resolve_path()`, which returns
  `NULL` when `_resolve_abspath()` fails and leaves `errno` set.

## Conclusion

- The live failure is not a bad `psh` string, not a symbol/link mismatch, and
  not an early kernel-side `sys_open()` failure.
- The shared blocker is now exactly:
  `resolve_path("/dev/console", ..., allow_missing_leaf = 1) -> NULL`
- The next bounded step should therefore target `resolve_path()` /
  `_resolve_abspath()` for `/dev/console` rather than adding more `open()` or
  kernel syscall traces.
