# Integration State: ext2-defects-12-21-and-short-read

## Summary

- Date: 2026-09-22
- Note: ext2 defects 12-21 fixed (four data-destroying) + libcache skip-identical-writeback + bcm2711-emmc short-read. GATED: SD lane test_sparse incl. new trunc-holes/trunc-tail 0 faults; showcase 6/6 visually verified; dd-to-EOF 1026+1 records byte-exact; full e2fsck of the SD root has 0 ref-count errors. Host: run-all.sh ALL GREEN, 7 harnesses x 2 block sizes.
- Generator: scripts/snapshot-integration-state.sh

## ⚠ Exactly what was gated on hardware, and what was not

| repo | SHA recorded | gated? |
|---|---|---|
| `phoenix-rtos-filesystems` | `d46ba88` | **yes** — defects 12-21; SD lane `test_sparse` (incl. the new `trunc-holes` / `trunc-tail`) 0 faults, showcase 6/6 visually verified, full `e2fsck` 0 ref-count errors |
| `phoenix-rtos-corelibs` | `a46399c` | **yes** — libcache skip-identical-writeback; 12.6x measured with the warm-cache confound controlled |
| `phoenix-rtos-devices` | `56fe0a5` | **partly** — the gate ran `822e933`; `56fe0a5` is a pure clamp-hoist restructure with no behaviour change, **not itself built or run** |

**Known-open, deliberately, both recorded in the weekly log:**
- a **lock-order inversion** in libext2 (`dir->lock` → `objs->lock` vs `objs->lock` → `obj->lock`);
  reachability depends on whether unmount can race an in-flight request. Not fixed — concurrency is
  untestable with the host harness and a locking mistake would not be caught.
- `storage_write()` returns `-EINVAL` where POSIX wants a short write or `ENOSPC`. No measured
  symptom; a short write on a storage device silently truncates the caller's data.

**Restoring from this manifest** gives a filesystem that is clean under ~182 000 randomised host
ops and gated on hardware, with those two items still open.

## Repositories

| Repository | Branch | Commit SHA | Remote |
| --- | --- | --- | --- |
| _build | main | 129cadfd3 (dirty(1)) | https://github.com/houp/phoenix-rpi.git |
| libphoenix | master | fa8521d (clean) | https://github.com/phoenix-rtos/libphoenix.git |
| phoenix-rtos-build | master | 2f0e5ad (clean) | https://github.com/phoenix-rtos/phoenix-rtos-build.git |
| phoenix-rtos-corelibs | master | a46399c (clean) | https://github.com/phoenix-rtos/phoenix-rtos-corelibs.git |
| phoenix-rtos-devices | master | 56fe0a5 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-devices.git |
| phoenix-rtos-doc | master | d4419df (clean) | https://github.com/phoenix-rtos/phoenix-rtos-doc.git |
| phoenix-rtos-filesystems | master | d46ba88 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-filesystems.git |
| phoenix-rtos-hostutils | master | 49a1fd9 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-hostutils.git |
| phoenix-rtos-kernel | master | 7348dd99 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-kernel.git |
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
_build	129cadfd3d2ae5067044404d94ddf53820dd6eaa	main
libphoenix	fa8521d3e9799fed312861c4753bd4dbe55b35b4	master
phoenix-rtos-build	2f0e5adf4203aafb3d2f323871313eca60cc1e61	master
phoenix-rtos-corelibs	a46399c2c42ac7beb4f947351dd4f259117be2b2	master
phoenix-rtos-devices	56fe0a50fd96763278bfbcb4db5eba6e506bce90	master
phoenix-rtos-doc	d4419dfae5428cb3b8081404c34b12c78c86770d	master
phoenix-rtos-filesystems	d46ba889e94d577cd1f21df0f51090fc8c0e5395	master
phoenix-rtos-hostutils	49a1fd996e5745a19cc7ec0b22179bd1e90906cf	master
phoenix-rtos-kernel	7348dd999d40ed2458d4b0506f575a48f1086703	master
phoenix-rtos-lwip	492b20badec9ea1132c7ce47cfabcd9d48a2ee1b	master
phoenix-rtos-ports	057891a98152e047839c7af4ffadcf5d2ea0bf9a	master
phoenix-rtos-posixsrv	df2f6049145503f584931a7abaf58a061106b845	master
phoenix-rtos-project	2f31e8e6c9b768d16a287d927cf6ef90ec567efd	master
phoenix-rtos-tests	558125935556fa1db2973da82db420a7f785489a	master
phoenix-rtos-usb	877caceb936556d144eea70af1063e269a4b5c63	master
phoenix-rtos-utils	ce472cb9050260310c57bc466eb1e682561dcad9	master
plo	3e22b52515ce705233ee97f48ee579bd05406ca6	master
```
