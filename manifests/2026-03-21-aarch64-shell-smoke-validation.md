# Manifest: First Interactive Shell Smoke Validation

- Date: `2026-03-21`
- Step: `STEP-0258`
- Scope: validate the built-in `help` command through the new `(psh)%` prompt

## Validation Method

- use `expect` inside `phoenix-dev`
- wait for `(psh)%`
- send `help`
- look for:
  - `Available commands:`
  - returned `(psh)%`

## Result

- Pi 4 `raspi4b` passes with the current QEMU launch:
  - prompt matched
  - `help` executed
  - `Available commands:` appeared
  - the prompt returned
- generic `virt` does not yet pass with the current QEMU launch:
  - the prompt match was good enough for `expect` to send `help`
  - the log shows `help` echoed
  - the expected `Available commands:` output never appears
  - the prompt does not return

## Supporting Evidence

- Pi 4 log:
  - `/tmp/pi4-shell-smoke.log`
  - key lines include:
    - `(psh)% help`
    - `Available commands:`
    - returned `(psh)%`
- generic log:
  - `/tmp/generic-shell-smoke.log`
  - the log ends after echoed `help`

## Conclusion

- the first command-level shell smoke is now proven on the Pi 4 fast lane
- the generic fast lane still needs a narrower stdin-path adjustment before the
  same smoke is reliable there
- the current strongest hypothesis is the generic validation launch itself:
  `-serial mon:stdio` is likely still muxing stdin in a way that is fine for
  passive boot logs but not for automated interactive shell input
