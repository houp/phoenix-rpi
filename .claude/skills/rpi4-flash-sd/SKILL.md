---
name: rpi4-flash-sd
description: Flash a new Phoenix-RTOS SD image to the Pi's card WITHOUT touching the hardware — netboot Phoenix, dd the image from the NFS root onto /dev/mmcblk0, then switch lanes and boot it. Use whenever the card needs updating, including unattended overnight.
---

# Updating the Pi's SD card without a human

The card **stays in the Pi**. Phoenix flashes its own card: netboot Phoenix (the card is
irrelevant to lane choice — dnsmasq picks it), `dd` the image from the NFS root onto
`/dev/mmcblk0`, drop dnsmasq, power-cycle, and the Pi boots what you just wrote.

No Linux netboot image is needed. Phoenix has the `bcm2711-emmc` driver (`/dev/mmcblk0`) and
coreutils `dd` on the export — that is the whole toolchain.

First proven end-to-end 2026-09-19: `b95e983a` → `6012dd0d` with no human intervention.

## Use `/usr/bin/dd`, NOT `/bin/dd`

⛔ **`/bin/dd` is busybox and runs at ~0.65 MB/s** — it is what made the first flash take 28
minutes. **`/usr/bin/dd` is coreutils and runs at ~12.3 MB/s**, a ~19x difference on the identical
transfer, and it also prints its own `bytes copied, N s, X MB/s` line, which is the only
trustworthy timing available here (see the warning below).

**Budget ~2-3 minutes for a 1.1 GB image** with coreutils dd.

⚠ **Each binary can do only one direction.** coreutils `dd` **cannot read** `/dev/mmcblk0` —
`dd: cannot fstat '/dev/mmcblk0': Function not implemented` — and it then produces a **0-byte file**,
which looks exactly like a broken driver rather than a broken tool. So:

* write **to** the device → `/usr/bin/dd` (coreutils, fast, reports its rate)
* read **from** the device → `/bin/dd` (busybox)

⚠ **Do not time transfers with the harness.** `test-cycle-psh-interact.sh --idle-secs N` waits N
seconds of UART idle **after each command**, so bracketing a command with `date` measures the
harness's cadence, not the device: three unrelated operations once all "took" exactly 312 s that
way. Read the rate coreutils `dd` reports instead.

## The procedure

### 1. Build and stage the image

```
./scripts/rebuild-rpi4b-fast.sh --variant sd --with-tests --with-ports --with-showcase
cp artifacts/rpi4b/rpi4b-sd-2part.img /srv/phoenix-rpi4-nfs-gcc16/sdimage.img
```

⚠ `--variant sd` **overwrites the TFTP `loader.disk`** with an SD blob that has no `nfs;/`, so
every netboot cycle then refuses. Rebuild the netboot loader **before** flashing — you need
netboot working to do the flash at all:

```
./scripts/rebuild-rpi4b-fast.sh --with-tests --with-ports --with-showcase
./scripts/check-netboot-blob.sh        # must print: rootfs: nfsroot
```

The staged image appears on the Pi as `/sdimage.img` (the export is the Pi's `/`).

### 2. Verify the image before writing it

Never flash an unverified image — a bad one costs 30 min plus a recovery flash:

```
./scripts/qemu-boot-sdimage.sh          # must print: SD variant, as expected + PASS
```

### 3. Flash from Phoenix (netboot lane, dnsmasq UP)

```
./scripts/test-cycle-psh-interact.sh --label sdflash \
    --idle-secs 1500 --max-cmd-secs 1600 -- \
    "/usr/bin/dd if=/sdimage.img of=/dev/mmcblk0 bs=1M"
```

With coreutils dd this finishes in a couple of minutes, so it fits in one foreground call; keep
`nohup … &` for the busybox path or a very large image.

Success looks like `1087+1 records in / 1087+1 records out` (records = image bytes / 1 MiB).
⚠ **Check the record count against the image size.** A short write is the one failure that
still leaves a plausible-looking log.

When dd finishes, the cycle sits out `--idle-secs` doing nothing. `kill -TERM` the
**`psh-interact.py`** child; the parent's EXIT trap then powers the Pi off.

### 4. Switch lanes and boot what you wrote

```
./scripts/netboot-server-down.sh
./scripts/test-cycle-netboot.sh --sd-boot --capture-secs 200 --label sdboot-new
```

**Grade by the ROOT ARG, never by `bcm2711-emmc`** — that driver starts on *both* lanes, so its
presence proves nothing:

* SD boot  ⇒ `bcm2711-emmc;-r;/dev/mmcblk0p2:ext2`, and **no** `nfs;/;10.42.0.1`
* netboot  ⇒ `nfs;/;10.42.0.1;/;v4;takeover`

Then confirm you are running the image you *meant* to write — `rpi4-sysinfo` prints
`/etc/build-versions` on every boot, so compare those SHAs against the tree you built from.
A card that boots the *old* image looks identical to success until you check this.

### 5. Restore the bench

```
./scripts/netboot-server-up.sh     # netboot is the default lane; leave it up
```

## Safety, and the one way this bricks the bench

**Keep dnsmasq UP for the whole of steps 1–3.** While the server is up the Pi netboots, so a
failed or interrupted flash is recoverable: netboot again and re-flash. Only drop it once dd
has reported the full record count.

⛔ **The unrecoverable case:** a **blank** card stops the Pi booting *at all* — 0 DHCP, 0 UART,
black HDMI, observed over four power-ons 2026-09-16, and it does **not** fall through to the
network despite `BOOT_ORDER=0xf12`. An interrupted `dd` leaves a *partially written* card,
which is untested territory. The dangerous window is small (dd writes the MBR in its first
moments, so the card has a valid partition table almost immediately) but it is real. If the Pi
ever stops responding on both lanes, that is the one situation needing the owner.

⚠ **Probe writes damage the card.** `seek=` into the ext2 area corrupts the running rootfs, so
any throughput probe commits you to completing a full flash. Do not "just measure" unless you
intend to flash.

## Related

* `rpi4-run` — running commands on the Pi generally.
* `feedback_selfflash_sd_via_netboot_linux` (memory) — lane selection, the blank-card finding,
  and the older Linux-netboot variant of this loop (no longer needed).
* `docs/misc/2026-09-19-ntp-clock-step-breaks-app-startup.md` — why a bench waits for the clock
  step before launching timing-sensitive commands.
