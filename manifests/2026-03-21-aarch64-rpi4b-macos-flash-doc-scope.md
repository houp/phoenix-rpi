# Manifest: Pi 4 macOS Flashing-Workflow Documentation Scope

- Date: `2026-03-21`
- Step: `STEP-0280`
- Focus: select the smallest operator-facing documentation step now that a
  host-visible Pi 4 SD-card image exists

## Decision

- the next bounded move should be one concrete macOS flashing workflow in the
  operator runbook using the current host-visible artifact:
  - `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`

## Rationale

- the project now has a flashable Pi 4 disk image on the host
- the operator already has a Pi 4 board and microSD card
- the remaining immediate practical gap before the first manual hardware trial
  is explicit host-side flashing guidance, not another artifact transformation
- the same documentation step should also capture the current no-UART
  expectations so the first manual hardware trial is not misinterpreted

## Next Bounded Move

- update the runbook with:
  - one explicit macOS SD-card flashing procedure
  - the current artifact path
  - the current no-UART expectations and limitations for the first boot attempt
