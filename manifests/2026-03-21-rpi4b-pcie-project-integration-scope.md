# Pi 4 PCIe Project Integration Scope

Date: `2026-03-21`

## Step

- `STEP-0325` Scope the smallest Pi 4 project-integration step for the current
  PCIe server

## Repositories

- coordination repo

## Summary

- reviewed the current Pi 4 project composition after the new BCM2711 PCIe
  server work
- found the more practical remaining gap:
  the current Pi 4 image does not stage the `pcie` server at all
- selected the next bounded step accordingly:
  include `pcie` in the Pi 4 A72 device target and stage it in the Pi 4
  `user.plo` script before `psh`

## Design Notes

- this step comes before any further downstream endpoint-readback work because
  real-device PCIe progress is impossible if the `pcie` server never reaches
  the image
- the current `aarch64a72-generic` device target is Pi 4-specific in this repo
  state, so adding `pcie` there does not currently widen across multiple boards

## Validation

- source inspection against:
  - `sources/phoenix-rtos-devices/_targets/Makefile.aarch64a72-generic`
  - `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/user.plo.yaml`
  - `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/build.project`

## Next Logical Step

- implement the smallest Pi 4 project-integration step for the current PCIe
  server
