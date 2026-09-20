# The USB mass-storage test stick (prepared 2026-09-20)

Prepared on the host for Pi 4 USB mass-storage bring-up. **The Pi does not need to boot
from it** — the goal is hot-plug enumeration and `mount` in a running system.

## What the device is

| | |
|---|---|
| Model / ID | SanDisk Ultra, USB ID **`0781:5581`** |
| Serial | `A2004263F6063324` |
| Capacity | 31 625 052 160 B = **29.45 GiB**, 61 767 680 × 512 B sectors |
| Logical / physical block | 512 B / 512 B |
| bcdUSB | 2.10 |
| Class / SubClass / Protocol | **08 / 06 / 50** = Mass Storage / **SCSI transparent** / **Bulk-Only Transport (BOT)** |
| Endpoints | 1 × bulk IN `0x81`, 1 × bulk OUT `0x02`, `wMaxPacketSize` **512** at High Speed |
| Configurations / interfaces | 1 / 1, `bConfigurationValue` 1, `MaxPower` 300 mA, bus-powered |

⚠ **It is not running at USB 3 speed here, and that is a host-side fact, not a device
limit.** The BOS descriptor advertises `wSpeedsSupported 0x000e` — Full, High **and
SuperSpeed** — but on this host it is plugged into a **USB 2.0 hub**, so it negotiated
**High Speed (480 Mbps)**. On the Pi 4's VL805 root ports it should come up SuperSpeed,
which changes the numbers the driver must handle: bulk `wMaxPacketSize` becomes **1024**
and a SuperSpeed endpoint companion descriptor (burst size) appears. Do not hard-code 512.

ⓘ It is **BOT, not UASP** — the simple profile. One command = CBW (31 B) out, data in or
out, CSW (13 B) in. No streams, no command queueing.

## What is on it now

Everything that was there (an Ubuntu 18.04.6 install image, iso9660 + a 2.3 MB EFI vfat
partition) was erased: `wipefs -a`, then 8 MiB of zeros at the head and 8 MiB at the tail,
so no stale iso9660 / hybrid-MBR / backup-GPT signature survives.

**MBR (DOS) label**, disk identifier `0x50485834`:

| part | start (LBA) | sectors | size | type | fs | label | UUID |
|---|---|---|---|---|---|---|---|
| `sda1` | 2048 | 4 194 304 | 2 GiB | 0x83 | ext2, **1 KiB blocks** | `PHXTEST1` | `1111…1111` |
| `sda2` | 4 196 352 | 57 571 328 | 27.5 GiB | 0x83 | ext2, **4 KiB blocks** | `PHXDATA` | `2222…2222` |

**The block size is the only deliberate difference between them**, and it is the point.
`sda1` is byte-for-byte the geometry class Phoenix already mounts on every SD boot
(`mke2fs -t ext2 -b 1024 -i 2048`, the same command `build-rpi4b-rootfs-ext2.sh` uses), so
if it fails, the fault is in the *storage* path, not in ext2. `sda2` is the realistic
large-volume case at 4 KiB blocks, which nothing in this port has ever mounted. Feature
sets are otherwise identical:

```
ext_attr resize_inode dir_index filetype sparse_super large_file   (rev 1, inode size 256)
```

Both are `e2fsck -fn` clean. MBR type 0x83 matters: `sdstorage_checkMBR` in
`bcm2711-emmc` only recognises partitions by table entry, and a USB storage driver that
reuses libstorage will want the same.

## Verifying a mount by CONTENT, not by rc

Each partition carries `/phoenix/` with a `SHA256SUMS` covering every file, so a mount can
be graded by reading rather than by an exit code:

* `README.txt` — names the partition and its block size
* `small.bin` — 3 KiB (fits in direct blocks on both geometries)
* `one-mib.bin` — 1 MiB (crosses the single-indirect boundary at 1 KiB blocks)
* `big.bin` — 17 MiB (**crosses the double-indirect boundary at 1 KiB blocks**: 12 direct
  + 256 single + 65 536 double = ~64 MiB ceiling, and the 17 MiB file is well into the
  double-indirect region)
* `deep/a/b/c/leaf.txt` — 4 levels of directory nesting

`sha256sum -c /phoenix/SHA256SUMS` on the Pi is the acceptance test. ⚠ A successful
`mount` that returns 0 proves nothing on its own — this port has repeatedly produced
green results from paths where nothing executed.

## Re-preparing it

Identity gate first, always — **`/dev/sda` is this stick on this host, not the SD card**:

```
lsblk -o NAME,SIZE,TRAN,MODEL,SERIAL        # expect SanDisk Ultra / A2004263F6063324
```
```

Then: `wipefs -a`, zero 8 MiB head and tail, `sfdisk` the DOS label above, and
`mke2fs -q -t ext2 -b 1024 -i 2048 -L PHXTEST1` / `-b 4096 -L PHXDATA`.

## What the driver side looks like (surveyed 2026-09-20)

**A BOT mass-storage driver already exists upstream** — `devices/storage/umass/` — and
matches this stick exactly: its filter is
`{ ANY, ANY, USB_CLASS_MASS_STORAGE, USB_SUBCLASS_SCSI, USB_PROTOCOL_BULK }`, i.e.
08/06/50. It was simply **never built for aarch64**. Now wired up:

* `libusbdrv-umass` added to `DEFAULT_COMPONENTS` for `aarch64a72-generic` **and** to
  `USB_HOSTDRV_LIBS` in `build-core-aarch64a72-generic.sh`. Both are needed — the
  component alone builds the lib while nothing links it, and the device then enumerates
  with no driver bound, which is precisely the shape of the #126 mouse bug.
* `UMASS_MOUNT_EXT2` enabled for aarch64 (libext2 is already built here; it backs the SD
  card's ext2 root).
* Two `-Werror=format` failures fixed — `%lld` against an `off_t` that is `long int` on
  this ABI, not `long long`. That is most likely *why* it stayed ia32-only.
* `mtStat` added, the same gap and the same fix as `bcm2711-emmc` got the same day.

⚠ **Known limitation, and it shapes what to expect:** `_umass_check()` reads the MBR and
takes **partition 0 only** (`/* Read only the first partition */`). So this stick will
present `sda1` = **`PHXTEST1`** and `sda2` will be invisible until the driver enumerates
all four table entries. That is a small, obvious follow-up — and it is why the
proven-geometry 1 KiB filesystem is on partition 1.

⚠ Also untested here: **hot-plug and hot-unplug**. The driver has `handleInsertion` /
`handleDeletion`, but nothing on this port has ever exercised removal of a *mounted*
block device, and the ext2 server holds state across it.

ⓘ Bulk transfer support is not a gap: `usb_transferBulk()` already exists in libusb and
is used by `usbacm` and `usbwlan` today.
