# Manifest: Boot-First Fast-Lane Policy

- Date: `2026-03-20`
- Step: `STEP-0047`
- Result: `completed`

## Scope

- update the coordination repo so the current user priority is reflected explicitly
- keep the existing execution-control and commit discipline intact
- bias future step selection toward the earliest realistic Raspberry Pi 4 boot path

## Files

- `AGENTS.md`
- `docs/status.md`
- `docs/implementation-dossier.md`
- `docs/repository-work-breakdown.md`
- `docs/execution-control.md`
- `docs/unattended-agent-mode.md`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Result

- the repo rules now explicitly permit a boot-first fast lane when the user asks for the first Pi 4 boot as the main priority
- the implementation dossier and repository-order docs now state that generic timer runtime validation, generic `virt`, PL011 reuse, and Pi 4 `plo` bring-up should take precedence over non-blocking cleanup
- the control docs still state that this is a prioritization rule only, not a relaxation of step size, validation, or per-repo commit rules

## Validation

- reviewed the touched coordination documents after editing
- confirmed that the policy text is consistent with the existing execution-control model
- confirmed that no implementation repositories were modified by this planning step

## Selected Next Step

- define the first non-Xilinx generic AArch64 QEMU `virt` milestone and the first concrete code step that should unlock it
