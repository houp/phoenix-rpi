# 2026-03-21: refresh the host-visible Pi 4 SD image after staged HDMI progress

## Scope

- Step: `STEP-0299`
- Goal: refresh the host-visible Pi 4 SD image so it includes the staged HDMI
  progress panel

## Repositories Touched

- coordination repo

## Sequence Run

1. `./scripts/assemble-rpi4b-bootfs.sh`
2. `./scripts/assemble-rpi4b-bootfs-img.sh`
3. `./scripts/assemble-rpi4b-sdimg.sh`
4. `./scripts/export-rpi4b-sdimg.sh`

## Validation

Refreshed host-visible artifact:

- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`

Refreshed host-visible SHA-256:

- `b2f3a33fe7b4e96d364b6e7579350d7c548359701cbaf0e9ac6b86fbf18860b0`

Verified that the host artifact matches the VM-local source image by SHA-256.

Verified the exported boot partition still carries the intended firmware config:

```text
hdmi_force_hotplug=1
disable_overscan=1
```

using:

```sh
mtype -i /Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img@@1048576 ::config.txt
```

## Result

- the current host-visible Pi 4 SD image now corresponds to the staged HDMI
  progress implementation
- the no-UART real board can now be tested against the latest early HDMI
  visibility path
- the next practical move is the first manual Pi 4 board trial with this
  refreshed image

