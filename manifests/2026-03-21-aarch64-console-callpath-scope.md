# Manifest: Console Open Call-Path Scope

- Date: `2026-03-21`
- Step: `STEP-0250`
- Status: `completed`

## Goal

- choose the smallest next step after both the libphoenix and kernel console
  open probes stayed silent

## Evidence Reviewed

Runtime evidence:

- `psh_ttyopen()` still reports `open -2`
- libphoenix `open()` trace stays silent
- kernel `posix_open()` trace stays silent

Conclusion:

- the next plausible seam is no longer pathname handling alone
- the next smallest high-signal step is binary or symbol inspection of the
  `psh` image to determine what `psh_ttyopen()` is actually calling for `open`

## Selected Next Step

- inspect the built `psh` image and symbol references before adding more
  runtime traces
