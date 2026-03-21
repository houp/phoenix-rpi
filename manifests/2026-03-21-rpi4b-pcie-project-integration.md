# Pi 4 PCIe Project Integration

Date: `2026-03-21`

## Step

- `STEP-0326` Implement the smallest Pi 4 project-integration step for the
  current PCIe server

## Repositories

- `phoenix-rtos-devices` `7af4e98`
- `phoenix-rtos-project` `ce18718`
- coordination repo

## Summary

- the Pi 4 A72 device target now includes the `pcie` server in its default
  component set
- the Pi 4 `user.plo` script now stages `pcie` before `psh`
- this does not claim successful PCIe runtime behavior yet; it only makes the
  existing transport work reachable on the actual Pi 4 image path

## Key Files

- `sources/phoenix-rtos-devices/_targets/Makefile.aarch64a72-generic`
- `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/user.plo.yaml`

## Validation

Validated in `phoenix-dev`:

```sh
export PATH="$HOME/phoenix-toolchains/aarch64-phoenix/bin:$PATH"
cd /Users/witoldbolt/phoenix-rpi
tmpdir=$(mktemp -d ~/phoenix-buildroots/pi4-pcie-project.XXXXXX)
./scripts/prepare-buildroot.sh --copy-components "$tmpdir"
cd "$tmpdir"
TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image
```

Observed result:

- the Pi 4 A72 project build still succeeds from a fresh disposable buildroot
- the image build now includes `pcie` in the staged program set:
  `pcie (offs=0xb6000, size=0xaae8)` before `psh`

## Remaining Gap

- the current Pi 4 path still lacks first real downstream endpoint visibility
  evidence and any xHCI-specific runtime support

## Next Logical Step

- scope the smallest first direct downstream endpoint-readback step now that
  the `pcie` server is part of the Pi 4 image path
