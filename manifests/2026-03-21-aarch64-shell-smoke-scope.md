# Manifest: Interactive Shell-Smoke Scope

- Date: `2026-03-21`
- Step: `STEP-0257`
- Focus: pick the smallest command-level validation for the new QEMU shell
  prompt

## Selected Smoke Command

- use the built-in `help` command as the first interactive smoke

## Expected Success Markers

- command accepted after the `(psh)%` prompt appears
- output contains:
  - `Available commands:`
- the prompt returns after the command output:
  - `(psh)%`

## Why This Command

- `help` is implemented inside `psh`, so it does not depend on extra `/bin`
  alias wiring beyond the shell itself
- its output is deterministic and easy to match
- it exercises:
  - console input delivery
  - shell line parsing
  - command dispatch
  - console output back to the prompt

## Execution Method For Next Step

- start QEMU in an interactive TTY session
- wait for the first `(psh)%`
- send `help` followed by newline
- capture the first command output and the returned prompt on:
  - generic `virt`
  - Pi 4 `raspi4b`
