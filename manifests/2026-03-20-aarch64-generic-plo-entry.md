# Manifest: Generic PLO Multi-EL Entry And Handoff

- Date: `2026-03-20`
- Step: `STEP-0105`
- Status: `validated`

## Scope

- extend generic AArch64 `plo` startup to accept EL1, EL2, and EL3 entry
- extend generic AArch64 `plo` kernel handoff to work from EL1, EL2, and EL3
- preserve the existing EL3 behavior

## Repositories And SHAs

- `plo`
  - commit: `cce0dba`
  - summary: `aarch64/generic: allow plo startup outside EL3`

## Files

- `sources/plo/hal/aarch64/generic/_init.S`

## Validation

### 1. Build validation

Commands run in `phoenix-dev` from copied disposable buildroots:

```sh
LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh host core project image
LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh host core project image
```

Result:

- both targets built successfully after the `_init.S` change

### 2. Generic QEMU runtime validation

All runtime checks used the same generic QEMU image from the copied buildroot and terminated QEMU with `timeout` after visible boot progress was reached.

#### EL3 baseline

Command:

```sh
qemu-system-aarch64 -machine virt,secure=on,gic-version=2 ...
```

Observed output included:

- `Phoenix-RTOS loader v. 1.21`
- `Phoenix-RTOS microkernel v. 3.3.1`
- `pl011-tty: started`

#### EL1 lane

Command:

```sh
qemu-system-aarch64 -machine virt,secure=off,gic-version=2 ...
```

Observed output included:

- `Phoenix-RTOS loader v. 1.21`
- `Phoenix-RTOS microkernel v. 3.3.1`
- `pl011-tty: started`
- `pl011-tty: tty0 ready`
- `pl011-tty: console ready`
- `dummyfs: initialized`

#### EL2 lane

Command:

```sh
qemu-system-aarch64 -machine virt,secure=off,virtualization=on,gic-version=2 ...
```

Observed output included:

- `Phoenix-RTOS loader v. 1.21`
- `Phoenix-RTOS microkernel v. 3.3.1`
- `pl011-tty: started`
- `pl011-tty: tty0 ready`
- `pl011-tty: console ready`
- `dummyfs: initialized`

## Notes

- the EL1 lane initially faulted in `interrupts_init()` on a SIMD register load; the final patch enables FP/AdvSIMD access in the EL1 startup path
- generic non-EL3 exception-context save code is still not independently hardened as a diagnostic path; the validated result here is the no-fault boot and handoff fast path
- this step removes the main generic loader-side obstacle for Raspberry Pi 4 firmware entry and shifts the next Pi 4 blocker to payload staging after firmware boots `kernel8.img`
