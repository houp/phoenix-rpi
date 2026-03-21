# Manifest: Generic `virt` Shell Stdin Scope

- Date: `2026-03-21`
- Step: `STEP-0259`
- Focus: choose the smallest generic-QEMU runtime change after the partial
  shell-smoke result

## Current Difference

- Pi 4 working smoke launch:
  - `-nographic -monitor none`
- generic failing smoke launch:
  - `-serial mon:stdio -serial null -display none`

## Selected Next Change

- replace the generic interactive smoke launch with the same stdio model used
  by the working Pi 4 lane:
  - `-nographic -monitor none`

## Rationale

- Pi 4 already proves the shell command path itself
- generic failure happens only at automated stdin delivery
- `mon:stdio` is explicitly a muxed monitor-plus-serial path and is the
  strongest current candidate for the generic-only input problem
- this keeps the next step runtime-only and limited to one QEMU invocation

## Expected Success Markers For Next Step

- generic `virt` log contains:
  - `(psh)% help`
  - `Available commands:`
  - returned `(psh)%`
