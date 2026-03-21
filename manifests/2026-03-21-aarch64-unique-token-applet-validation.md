# Manifest: Unique-Token External-Applet Validation

- Date: `2026-03-21`
- Step: `STEP-0266`
- Scope: validate a unique-token external-applet smoke on the fast QEMU lanes

## Command Under Test

- `echo codex-smoke-echo`

## Result

- the shell clearly echoes the command line:
  - `echo codex-smoke-echo`
- but that makes the token unusable as a proof of applet output:
  - the same token appears in the command echo before the applet's stdout is
    independently proven

## Conclusion

- the next smallest external-applet smoke should use a command whose output is
  distinct from its command line
- the most direct next candidate is:
  - `echo -h`
  because its output begins with `Usage: echo ...`, which does not collide with
  the command echo
