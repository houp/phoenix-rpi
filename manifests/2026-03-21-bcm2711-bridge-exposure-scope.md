# BCM2711 Bridge Exposure Scope

Date: `2026-03-21`

## Step

- `STEP-0321` Scope the smallest root-bridge memory-window and downstream-bus exposure step

## Repositories

- coordination repo

## Summary

- reviewed the remaining gap between the outbound-window / root-bridge shaping
  step and meaningful downstream endpoint visibility
- selected the next smallest slice as:
  root-bridge memory-window programming plus explicit downstream bus exposure on
  bus `1`
- explicitly kept endpoint-specific validation, MSI, xHCI, and USB keyboard
  behavior out of scope

## Why This Comes Next

- Circle's `enable_bridge()` path programs bridge-side cache line, secondary
  bus, subordinate bus, and memory-window registers before it treats
  downstream devices as meaningfully exposable
- that makes bridge memory-window programming and bus exposure the smallest
  next move after outbound-window setup and root-bridge class shaping

## References

- `external/circle/lib/bcmpciehostbridge.cpp`
- `sources/phoenix-rtos-devices/pcie/server/pcie.c`

## Validation

- source review only
- no code changes in this step

## Next Logical Step

- implement bridge-side cache-line, bus-number, memory-window, and command
  exposure for the BCM2711 root bridge behind the sampled link-state gate
