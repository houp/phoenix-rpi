# Manifest: Distinct-Output External-Applet Scope

- Date: `2026-03-21`
- Step: `STEP-0267`
- Focus: pick the smallest external-applet command whose output is distinct from
  its invocation text

## Selected Command

- `echo -h`

## Expected Success Markers

- command echo after the prompt:
  - `echo -h`
- distinct applet output:
  - `Usage: echo [options] [string]`
- returned prompt:
  - `(psh)%`

## Why This Command

- it still exercises the external `echo` applet
- its help output is distinct from the echoed command line
- the usage string is stable and easy to match on both lanes
