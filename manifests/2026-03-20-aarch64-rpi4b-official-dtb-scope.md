# Manifest: Scope Pi 4 Validation With an Official Firmware DTB

- Date: `2026-03-20`
- Step: `STEP-0163`
- Status: `completed`

## Goal

- define the smallest next experiment that replaces the current stub Pi 4 DTB with an official Raspberry Pi firmware DTB for `raspi4b` validation

## Decision

The next implementation step is bounded to:

- acquire an official `bcm2711-rpi-4-b.dtb` from the Raspberry Pi firmware repository
- record the exact firmware repo revision used for the validation
- rebuild the Pi 4 project with `RPI4B_DTB_PATH` pointing to that real DTB
- rerun only the Pi 4 `raspi4b` QEMU lane
- update the docs and manifests with the exact DTB source used

## Why This Step

- `STEP-0162` proved the Pi 4 lane already reaches `plo` in EL3, so the missing boundary is not a loader entry-level mismatch
- the current `RPI4B_DTB_PATH` input resolves to a 274-byte stub DTB:
  - `compatible = "raspberrypi,4-model-b", "brcm,bcm2711"`
  - one memory bank only
- that DTB is too small to represent a real Pi 4 board tree and is very likely masking the next real blocker

## Explicitly Deferred

- changing kernel or loader code
- checking in firmware blobs to the repo before the validation proves they are needed in-tree
- any real-hardware-only conclusions

## Acceptance Criteria

- the next validation uses an official Raspberry Pi firmware DTB rather than the current stub
- the exact DTB source and revision are recorded in the resulting manifest
- the Pi 4 QEMU result shows whether a real board DTB moves the lane beyond `tty0 lookup retry`

## Selected Next Step

- validate the Pi 4 QEMU lane with an official firmware DTB
