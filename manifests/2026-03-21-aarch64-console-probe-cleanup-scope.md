# Manifest: Console Probe Cleanup Scope

- Date: `2026-03-21`
- Step: `STEP-0255`
- Focus: identify the smallest safe cleanup set after the prompt-reaching fast
  lane milestone

## Selected Cleanup Set

Remove the console-path probes that were only introduced to isolate the old
`/dev/console` failure:

- `sources/phoenix-rtos-utils/psh/psh.c`
  - `psh_ttyopenTraceDone`
  - `psh_ttyopenTrace()`
  - the `open` / `isatty` failure trace calls
- `sources/libphoenix/unistd/file.c`
  - `open_consoleTraceDone`
  - `open_consoleTrace()`
  - the `"/dev/console"`-specific trace calls in `open()`
- `sources/phoenix-rtos-kernel/syscalls.c`
  - the `syscalls: psh console lookup ...` one-shot trace
- `sources/phoenix-rtos-kernel/posix/posix.c`
  - the `posix: psh console open ...` one-shot trace

## Keep For Now

- broader fast-lane boot visibility that is still helping later bring-up, such
  as:
  - `psh: root ready`
  - `psh: run enter`
  - `create_dev: ...`
  - later shared boot-band markers in `plo` and the kernel

## Rationale

- the old console-open blocker is now solved by the project-local `/dev` bind
  startup fix
- these console-path probes are no longer needed for the current fast lane
- removing them now matches the repository rule that false-hypothesis or
  diagnosis-only probes should not accumulate once the blocker is resolved
