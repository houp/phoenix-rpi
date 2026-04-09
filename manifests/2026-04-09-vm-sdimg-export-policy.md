# Pi 4 VM SD-Image Export Policy

Date: `2026-04-09`

## Problem

Multiple ad hoc VM-to-host copy methods were tried for the Pi 4 SD-card image,
and some of them produced corrupted host-visible copies even when the VM-local
image was valid.

That made the workflow noisy, slow, and unpredictable.

## Canonical Rule

For the Pi 4 SD-card image exported from `phoenix-dev` to the macOS host, use
only:

- [scripts/export-rpi4b-sdimg.sh](/Users/witoldbolt/phoenix-rpi/scripts/export-rpi4b-sdimg.sh)

This helper is now the only approved export path for that artifact.

## Required Behavior

The helper must:

- read the VM-local image size first
- read the VM-local image SHA-256 first
- transfer the image through a text-safe path
- verify the transferred temp file on the host against the VM-derived size and
  SHA-256
- also run the FAT-aware boot-partition checks before replacing the exported
  artifact

## Disallowed Fallbacks

Do not use these as alternative export methods for the Pi 4 SD image:

- `scp`
- `sftp`
- `rsync`
- `limactl copy`
- streamed `dd`
- manual `cat` pipelines

If export reliability is ever in doubt again, fix the canonical helper and keep
the docs current. Do not start a new round of copy-method experiments.
