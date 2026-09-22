# Integration State: sync-chain-and-ext2-12-21

## Summary

- Date: 2026-09-22
- Note: ext2 defects 12-21 (four data-destroying) + libcache skip-identical-writeback + bcm2711-emmc short read + the sync/fsync durability chain fixed at three levels (libphoenix sync() stub, kernel fsync() zeroed oid, libext2 had no mtSync handler). Host suite run-all.sh ALL GREEN: 8 harnesses x 2 block sizes, randomised, and 2/4-thread concurrent.
- Generator: scripts/snapshot-integration-state.sh

## What is gated, and what is not

| repo | SHA | gated on hardware? |
|---|---|---|
| `phoenix-rtos-filesystems` | `017eb3a` | defects 12-21 **yes** (SD lane `test_sparse` incl. the new `trunc-holes`/`trunc-tail`, 0 faults; showcase 6/6 visually verified; full `e2fsck` 0 ref-count errors). The `mtSync` handler is **in flight** at the time of writing |
| `phoenix-rtos-corelibs` | `a46399c` | **yes** — 12.6x measured with the warm-cache confound controlled |
| `phoenix-rtos-devices` | `56fe0a5` | short read **yes** at `822e933` (`dd` to EOF: `1026+1 records`, byte-exact 1076594688); `56fe0a5` is a no-behaviour-change clamp hoist, **not** separately run |
| `libphoenix` | `a1bce81` | `sync()` **in flight** |
| `phoenix-rtos-kernel` | `482b54c2` | `fsync()` oid. ↩ **SQLite is NOT the test** — it `fsync()`s regular files, which reach `libext2_handler` and are dispatched to `libext2_sync(fdata)` **ignoring `msg->oid`**, so the zeroed oid never bit them. The fix bites only **raw block-device fds**: `storage_sync(0)` fails `IS_BLOCK_DEVICE_ID(0)` (`0 & DEVTYPE_BLOCK` == 0) and falls to the `else` branch, so `fsync()` on `/dev/mmcblk0*` returned **`-EINVAL` unconditionally**. Gate = `sync /dev/mmcblk0p2` with a positive control. |

**Open, deliberately** (all in the weekly log with reasons): a lock-order inversion reachable only
if unmount races an in-flight request; `storage_write()`'s `-EINVAL` where POSIX wants a short
write or `ENOSPC`; and a 1-inode/1-block residue on the SD card that is characterised but **not**
attributed — three inodes flagged by `e2fsck` against a used count that moved by one, which do not
reconcile, so no mechanism is claimed.

**Host position:** `tools/libext2-hosttest/run-all.sh` ALL GREEN — 8 harnesses x 2 block sizes,
randomised runs, and 2/4-thread concurrency with real mutexes.

## Repositories

| Repository | Branch | Commit SHA | Remote |
| --- | --- | --- | --- |
| _build | main | 6498fa86e (dirty(3)) | https://github.com/houp/phoenix-rpi.git |
| libphoenix | master | a1bce81 (clean) | https://github.com/phoenix-rtos/libphoenix.git |
| phoenix-rtos-build | master | 2f0e5ad (clean) | https://github.com/phoenix-rtos/phoenix-rtos-build.git |
| phoenix-rtos-corelibs | master | a46399c (clean) | https://github.com/phoenix-rtos/phoenix-rtos-corelibs.git |
| phoenix-rtos-devices | master | 56fe0a5 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-devices.git |
| phoenix-rtos-doc | master | d4419df (clean) | https://github.com/phoenix-rtos/phoenix-rtos-doc.git |
| phoenix-rtos-filesystems | master | 017eb3a (clean) | https://github.com/phoenix-rtos/phoenix-rtos-filesystems.git |
| phoenix-rtos-hostutils | master | 49a1fd9 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-hostutils.git |
| phoenix-rtos-kernel | master | 482b54c2 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-kernel.git |
| phoenix-rtos-lwip | master | 492b20b (clean) | https://github.com/phoenix-rtos/phoenix-rtos-lwip.git |
| phoenix-rtos-ports | master | 057891a (clean) | https://github.com/phoenix-rtos/phoenix-rtos-ports.git |
| phoenix-rtos-posixsrv | master | df2f604 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-posixsrv.git |
| phoenix-rtos-project | master | 2f31e8e (clean) | https://github.com/phoenix-rtos/phoenix-rtos-project.git |
| phoenix-rtos-tests | master | 5581259 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-tests.git |
| phoenix-rtos-usb | master | 877cace (clean) | https://github.com/phoenix-rtos/phoenix-rtos-usb.git |
| phoenix-rtos-utils | master | ce472cb (clean) | https://github.com/phoenix-rtos/phoenix-rtos-utils.git |
| plo | master | 3e22b52 (clean) | https://github.com/phoenix-rtos/plo.git |

## Machine-Parseable State

Consumed by `scripts/restore-integration-state.sh`. Fields: `<repo>\t<sha>\t<branch>`.

```integration-state-v1
_build	6498fa86ed6cad055f00a5e9a0187c88a461fca9	main
libphoenix	a1bce81b07646efbc5410b2492fcba3a6428bcb8	master
phoenix-rtos-build	2f0e5adf4203aafb3d2f323871313eca60cc1e61	master
phoenix-rtos-corelibs	a46399c2c42ac7beb4f947351dd4f259117be2b2	master
phoenix-rtos-devices	56fe0a50fd96763278bfbcb4db5eba6e506bce90	master
phoenix-rtos-doc	d4419dfae5428cb3b8081404c34b12c78c86770d	master
phoenix-rtos-filesystems	017eb3a38eada4b55087611cf64882b6c1b2d610	master
phoenix-rtos-hostutils	49a1fd996e5745a19cc7ec0b22179bd1e90906cf	master
phoenix-rtos-kernel	482b54c240daa8d5d0ea6072f57a0ce44f071a2c	master
phoenix-rtos-lwip	492b20badec9ea1132c7ce47cfabcd9d48a2ee1b	master
phoenix-rtos-ports	057891a98152e047839c7af4ffadcf5d2ea0bf9a	master
phoenix-rtos-posixsrv	df2f6049145503f584931a7abaf58a061106b845	master
phoenix-rtos-project	2f31e8e6c9b768d16a287d927cf6ef90ec567efd	master
phoenix-rtos-tests	558125935556fa1db2973da82db420a7f785489a	master
phoenix-rtos-usb	877caceb936556d144eea70af1063e269a4b5c63	master
phoenix-rtos-utils	ce472cb9050260310c57bc466eb1e682561dcad9	master
plo	3e22b52515ce705233ee97f48ee579bd05406ca6	master
```
