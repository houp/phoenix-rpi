# Manifest: External-Applet Shell Smoke Scope

- Date: `2026-03-21`
- Step: `STEP-0263`
- Focus: pick the smallest shell smoke that goes beyond the built-in `help`
  command

## Selected Command

- use:
  - `ls /`

## Expected Success Markers

- the command is echoed after the prompt
- output contains the current fast-lane root entries:
  - `dev`
  - `syspage`
- the prompt returns

## Why This Command

- it exercises a real applet path rather than a built-in command
- it uses the current root namespace that is already intentionally stabilized by
  the `dummyfs-root` plus `/dev` bind fast-lane setup
- the expected result is small and easy to match on both fast lanes
