# 2026-03-22: Pi 4 flash-helper scope

## Scope

Close `STEP-0400` by recording the one remaining operator-side blocker that can
still be improved before the first board run:

- safe verification of the exported SD image
- safe generation of the exact macOS flashing commands

## Why This Still Fits The Boundary

This is not new runtime work. It only reduces the chance that the first board
trial fails because of:

- using the wrong image
- using the wrong checksum expectation
- typing the flashing commands incorrectly

## Selected Next Step

Add two small host-side helpers:

- one script to verify the exported image path, size, and SHA-256
- one script to print the exact macOS flashing commands for a chosen `diskN`

The flashing helper should stay non-destructive by default and should not write
to disks by itself.
