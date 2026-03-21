# 2026-03-21: scope the refreshed Pi 4 SD-image handoff after staged HDMI progress

## Scope

- Step: `STEP-0298`
- Goal: select the minimal artifact-refresh sequence needed to get the staged
  Pi 4 HDMI progress indicator onto the host-visible SD image

## Repositories Touched

- coordination repo

## Decision

No new sequencing is needed. The correct refresh path remains the full helper
chain already established in `STEP-0293`:

1. `./scripts/assemble-rpi4b-bootfs.sh`
2. `./scripts/assemble-rpi4b-bootfs-img.sh`
3. `./scripts/assemble-rpi4b-sdimg.sh`
4. `./scripts/export-rpi4b-sdimg.sh`

## Why This Is Still Needed

The currently exported host artifact:

- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`

was produced before the new staged `plo` HDMI progress change.

So even though the staged panel is validated in QEMU, the real board will not
show it until the full artifact chain is rerun and the exported image checksum
is updated.

## Result

- `STEP-0298` is complete
- the next active step is to rerun the known-good full helper chain and record
  the refreshed host-visible image hash

