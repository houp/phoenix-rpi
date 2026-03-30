# 2026-03-30 Upstream Sync And Retest

## Scope

- fetch current upstream state from the Phoenix RTOS GitHub repositories
- merge upstream changes into the local sibling worktrees
- rebuild the validated host, generic AArch64 QEMU, and Pi 4 A72 lanes
- rerun the current QEMU smoke checks
- refresh the exported Pi 4 SD-card image and its checksum
- document the DTB regeneration requirement after `phoenix-dev` restarts

## Upstream Merge Result

Fetched all current sibling repositories from GitHub.

Repos with upstream changes merged into the local working branches:

- `phoenix-rtos-build`
  - local merge commit: `4cf07ce`
- `phoenix-rtos-devices`
  - local merge commit: `0724a24`
- `phoenix-rtos-filesystems`
  - local merge commit: `856b15f`
- `phoenix-rtos-project`
  - local merge commit: `1323778`
- `phoenix-rtos-kernel`
  - local merge commit on `codex/common-aarch64-platform-makefiles`: `a049189f`
- `plo`
  - local merge commit on `codex/common-aarch64-platform-makefiles`: `c0c4d8d`

Repos fast-forwarded to current upstream:

- `phoenix-rtos-lwip`
  - current HEAD: `806d01c`
- `phoenix-rtos-ports`
  - current HEAD: `fdabc22`
- `phoenix-rtos-tests`
  - current HEAD: `f7978fc`

Repos already current with upstream during this sync:

- `libphoenix`
- `phoenix-rtos-corelibs`
- `phoenix-rtos-doc`
- `phoenix-rtos-hostutils`
- `phoenix-rtos-posixsrv`
- `phoenix-rtos-usb`
- `phoenix-rtos-utils`

Post-merge result:

- all sibling repositories are now `behind 0`
- repositories carrying local Raspberry Pi port work remain ahead of upstream as expected
- all sibling repositories are clean after the merges

## Rebuild And Retest

### Rebuilt Successfully

- host baseline:
  - `TARGET=host-generic-pc ./phoenix-rtos-build/build.sh clean host core fs test project image`
- generic AArch64 QEMU lane:
  - `TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- Pi 4 A72 lane:
  - `TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

### Important Pi 4 DTB Note

The Pi 4 A72 rebuild initially failed after the `phoenix-dev` restart because
the VM-local DTB staging path had disappeared:

- `/tmp/rpi4b-dtb/bcm2711-rpi-4-b.dtb`

That path is ephemeral across VM restarts. The current documented recovery is:

- run:
  - `/Users/witoldbolt/phoenix-rpi/scripts/prepare-rpi4b-dtb.sh`
- then rebuild with:
  - `RPI4B_DTB_PATH=/tmp/rpi4b-dtb/bcm2711-rpi-4-b.dtb`
  - `RPI4B_QEMU_MEMORY_SIZE=80000000`

### QEMU Validation Result

Passed:

- generic shell smoke
  - `/Users/witoldbolt/phoenix-rpi/scripts/qemu-shell-smoke.sh generic`
- Pi 4 shell smoke
  - `/Users/witoldbolt/phoenix-rpi/scripts/qemu-shell-smoke.sh rpi4b`
- Pi 4 HDMI smoke
  - `/bin/bash /Users/witoldbolt/phoenix-rpi/scripts/qemu-rpi4b-hdmi-smoke.sh`

Observed Pi 4 shell-smoke success marker:

- `(psh)% help`
- `Available commands:`

Observed Pi 4 HDMI-smoke success marker:

- `Pi 4 HDMI smoke passed`
- `Framebuffer: 1024x768`

## Refreshed Pi 4 Artifact

Refreshed and exported after the merged rebuilds:

- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`

Current validated SHA-256:

- `d815e4c1b72bf0c170fb7fb6c00165d918d82f3d7b78bad97ec1c345a00e86db`

Current validated size:

- `69206016`

Validation helper:

- `/Users/witoldbolt/phoenix-rpi/scripts/verify-rpi4b-sdimg.sh`

## Operator-Facing Consequence

After an upstream sync or after restarting `phoenix-dev`, the operator or agent
should now assume that Pi 4 QEMU retests may require DTB regeneration first.

The current stronger lane after this sync remains the real Pi 4 hardware run
with the refreshed exported image.
