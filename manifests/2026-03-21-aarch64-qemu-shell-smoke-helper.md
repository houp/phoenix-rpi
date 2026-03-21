# Manifest: Reusable QEMU Shell-Smoke Helper

- Date: `2026-03-21`
- Step: `STEP-0262`
- Scope: package the validated `help` smoke into a reusable helper

## Change

- add:
  - `scripts/qemu-shell-smoke.sh`
- supported targets:
  - `generic`
  - `rpi4b`
- fixed smoke command:
  - `help`

## Validation

- `./scripts/qemu-shell-smoke.sh generic`
- `./scripts/qemu-shell-smoke.sh rpi4b`

## Result

- both helper runs now print the same compact smoke markers:
  - `(psh)% help`
  - `Available commands:`
  - `help       - prints this help message`
  - returned `(psh)%`
- the helper also prints the corresponding VM-side log path:
  - `/tmp/generic-shell-smoke.log`
  - `/tmp/pi4-shell-smoke.log`

## Conclusion

- the project now has a small reusable QEMU shell-smoke baseline for both fast
  lanes
- the next small shell-level step should move beyond a built-in command and
  validate one external applet path
