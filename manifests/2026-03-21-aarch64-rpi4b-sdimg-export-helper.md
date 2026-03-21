# Manifest: Pi 4 Full SD-Card Image Export Helper

- Date: `2026-03-21`
- Step: `STEP-0279`
- Scope: export the VM-local Pi 4 full disk image into a stable host-visible
  artifact path

## Change

- add:
  - `scripts/export-rpi4b-sdimg.sh`

## Validation

- run:
  - `./scripts/export-rpi4b-sdimg.sh`
- confirm the exported host artifact exists:
  - `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- confirm the host-visible file matches the VM-local source:
  - host:
    - `shasum -a 256 /Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  - VM:
    - `sha256sum /home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy/_boot/aarch64a72-generic-rpi4b/rpi4b-sd.img`

## Result

- the current Pi 4 full disk image now exports into a stable host-visible path:
  - `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- the current validated size is:
  - `69206016`
- the current validated SHA-256 is:
  - `d480e6d35d91a6e9b4d56971fd8973feb45140d570c099ee4c638fa5179cb0bc`

## Conclusion

- the project now has a host-visible flashable Pi 4 disk image artifact
- the next bounded move should document the first manual flashing workflow on
  macOS and the current no-UART expectations for the first hardware boot attempt
