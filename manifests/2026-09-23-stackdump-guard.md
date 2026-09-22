# Integration State: 2026-09-23-stackdump-guard

## Summary

- Date: 2026-09-23
- Note: C2 re-measured and found STALE: the stack-exhaustion kernel DoS does not reproduce (3 boots, 5 runs, 3 bombs in one boot; process dies, system lives). The EL1 abort is exceptions_dumpUserStack reading sp-128 on an unmapped page, not hal_cpuPushSignal. Guarded with at s1e1r + PAR_EL1.F. Gated both ways: stackbomb 2 EL1 aborts -> 0 with the new diagnostic line; nullderef control keeps its full stack dump.
- Generator: scripts/snapshot-integration-state.sh

## Repositories

| Repository | Branch | Commit SHA | Remote |
| --- | --- | --- | --- |
| _build | main | 90c29fb70 (dirty(4)) | https://github.com/houp/phoenix-rpi.git |
| libphoenix | master | a1bce81 (clean) | https://github.com/phoenix-rtos/libphoenix.git |
| phoenix-rtos-build | master | 2f0e5ad (clean) | https://github.com/phoenix-rtos/phoenix-rtos-build.git |
| phoenix-rtos-corelibs | master | a46399c (clean) | https://github.com/phoenix-rtos/phoenix-rtos-corelibs.git |
| phoenix-rtos-devices | master | 7315705 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-devices.git |
| phoenix-rtos-doc | master | d4419df (clean) | https://github.com/phoenix-rtos/phoenix-rtos-doc.git |
| phoenix-rtos-filesystems | master | 522ecc8 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-filesystems.git |
| phoenix-rtos-hostutils | master | 49a1fd9 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-hostutils.git |
| phoenix-rtos-kernel | master | 20179bfe (clean) | https://github.com/phoenix-rtos/phoenix-rtos-kernel.git |
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
_build	90c29fb70aeed4c13ae668d5fae73d41890eca8d	main
libphoenix	a1bce81b07646efbc5410b2492fcba3a6428bcb8	master
phoenix-rtos-build	2f0e5adf4203aafb3d2f323871313eca60cc1e61	master
phoenix-rtos-corelibs	a46399c2c42ac7beb4f947351dd4f259117be2b2	master
phoenix-rtos-devices	7315705b45443d83d4d20d7fa66618bceccc2f10	master
phoenix-rtos-doc	d4419dfae5428cb3b8081404c34b12c78c86770d	master
phoenix-rtos-filesystems	522ecc82d721f4f952bd6281d410bb18a1faa01a	master
phoenix-rtos-hostutils	49a1fd996e5745a19cc7ec0b22179bd1e90906cf	master
phoenix-rtos-kernel	20179bfe7472e8c28dea18a88d08b28427d7fe8e	master
phoenix-rtos-lwip	492b20badec9ea1132c7ce47cfabcd9d48a2ee1b	master
phoenix-rtos-ports	057891a98152e047839c7af4ffadcf5d2ea0bf9a	master
phoenix-rtos-posixsrv	df2f6049145503f584931a7abaf58a061106b845	master
phoenix-rtos-project	2f31e8e6c9b768d16a287d927cf6ef90ec567efd	master
phoenix-rtos-tests	558125935556fa1db2973da82db420a7f785489a	master
phoenix-rtos-usb	877caceb936556d144eea70af1063e269a4b5c63	master
phoenix-rtos-utils	ce472cb9050260310c57bc466eb1e682561dcad9	master
plo	3e22b52515ce705233ee97f48ee579bd05406ca6	master
```
