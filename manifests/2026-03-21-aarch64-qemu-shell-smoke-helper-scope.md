# Manifest: QEMU Shell-Smoke Helper Scope

- Date: `2026-03-21`
- Step: `STEP-0261`
- Focus: choose the smallest reusable wrapper for the validated QEMU `help`
  smoke

## Selected Helper Shape

- add one host-side shell script:
  - `scripts/qemu-shell-smoke.sh`

## Scope

- supported targets:
  - `generic`
  - `rpi4b`
- fixed smoke command:
  - `help`
- execution model:
  - invoke `limactl shell -y phoenix-dev`
  - run a VM-side `expect` smoke
  - reuse the already validated QEMU arguments for each target

## Why This Shape

- keeps the helper small and readable
- does not require changes in Phoenix upstream repos
- removes repeated long command lines from future sessions
- stays narrow enough that the next step can implement and validate it in one
  pass
