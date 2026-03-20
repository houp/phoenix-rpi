# Manifest: Pi 4 Firmware Payload Staging

- Date: `2026-03-20`
- Step: `STEP-0107`
- Status: `validated`

## Scope

- stage `loader.disk` into the Pi 4 firmware-facing boot tree
- update the Pi 4 project-local `config.txt` so Raspberry Pi firmware preloads that payload to generic `plo` `RAM_ADDR`
- keep the existing generic `ram0` / `loader.disk` contract unchanged

## Repositories And SHAs

- `phoenix-rtos-project`
  - commit: `0c228ac`
  - summary: `project: stage rpi4b loader payload for firmware preload`

## Files

- `sources/phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/build.project`
- `sources/phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/config.txt`

## Validation

Command run in `phoenix-dev` from a copied disposable buildroot:

```sh
LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh host core project image
```

Result:

- build succeeded
- staged boot tree contained:
  - `_boot/aarch64a53-generic-rpi4b/rpi4b/config.txt`
  - `_boot/aarch64a53-generic-rpi4b/rpi4b/kernel8.img`
  - `_boot/aarch64a53-generic-rpi4b/rpi4b/loader.disk`
- staged `config.txt` now contains:
  - `initramfs loader.disk 0x48000000`
- staged `loader.disk` size:
  - `2567152` bytes
- that size fits within generic `RAM_BANK_SIZE = 0x08000000` (`134217728` bytes)

## Notes

- this change keeps the Pi 4 path aligned with the existing generic `phfs ram0 4.0 raw` preinit flow
- the next Pi 4 blocker is now kernel DTB propagation, because the current Pi 4 payload still does not supply `system.dtb` to the generic AArch64 kernel
