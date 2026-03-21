# Manifest: Unique-Token External-Applet Scope

- Date: `2026-03-21`
- Step: `STEP-0265`
- Focus: pick the smallest external-applet command with a non-ambiguous output
  marker

## Selected Command

- `echo codex-smoke-echo`

## Expected Success Markers

- command echo after the prompt:
  - `echo codex-smoke-echo`
- unique applet output:
  - `codex-smoke-echo`
- returned prompt:
  - `(psh)%`

## Why This Command

- `echo` is an external applet, not the built-in `help` path
- the token `codex-smoke-echo` is unique enough to avoid collisions with the
  current boot-band output
- the command has no filesystem or environment dependencies beyond the current
  shell command-dispatch path
