# Pi 4 VL805 Fast-Path Constants

Date: `2026-03-21`

## Step

- `STEP-0328` Implement the board-level Pi 4 VL805 fast-path constants

## Repositories

- `phoenix-rtos-project` `0db048e`
- coordination repo

## Summary

- added the first board-level constants for the Pi 4 downstream USB controller
  fast path:
  fixed BDF, fixed class code, and fixed outbound-window MMIO base
- kept this step strictly declarative:
  no xHCI code, no runtime staging, and no new boot-path risk

## Key Files

- `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/board_config.h`

## Validation

Validated in `phoenix-dev`:

```sh
export PATH="$HOME/phoenix-toolchains/aarch64-phoenix/bin:$PATH"
cd /Users/witoldbolt/phoenix-rpi
tmpdir=$(mktemp -d ~/phoenix-buildroots/pi4-vl805-const.XXXXXX)
./scripts/prepare-buildroot.sh --copy-components "$tmpdir"
cd "$tmpdir"
TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image
```

Observed result:

- the Pi 4 A72 project build still succeeds from a fresh disposable buildroot
- the new board-level constants do not change the staged runtime image

## Remaining Gap

- there is still no xHCI HCD or Pi 4-specific xHCI discovery helper using these
  constants

## Next Logical Step

- scope the first compile-only Pi 4 xHCI HCD skeleton and discovery stub
