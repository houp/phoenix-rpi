# Integration State: 2026-09-19-w38-sdma-writes-fixed

## Summary

- Date: 2026-09-19
- Note: SDMA write root cause fixed: emmc2bus BUS-address translation (SDCARD_DRAM_BUS) was never applied — the SD driver was the only DMA master in the tree skipping it. Writes bit-correct over 4 rounds with a read control. Also: cmdFrame zero-init (bit 3 = Auto-CMD23, forbidden with SDMA), dsb submit barrier, TC-wait before the spin. PIO stays default on validation, not speed: measured PIO 12.2 MB/s vs SDMA 12.3 MB/s with coreutils dd. Six-app gate sdgate 6/6, 0 faults, torches present.
- Generator: scripts/snapshot-integration-state.sh

## Repositories

| Repository | Branch | Commit SHA | Remote |
| --- | --- | --- | --- |
| _build | main | d66fab710 (dirty(1)) | https://github.com/houp/phoenix-rpi.git |
| libphoenix | master | 5b954c8 (clean) | https://github.com/phoenix-rtos/libphoenix.git |
| phoenix-rtos-build | master | 0db6edd (clean) | https://github.com/phoenix-rtos/phoenix-rtos-build.git |
| phoenix-rtos-corelibs | master | 4814fed (clean) | https://github.com/phoenix-rtos/phoenix-rtos-corelibs.git |
| phoenix-rtos-devices | master | 4a61c96 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-devices.git |
| phoenix-rtos-doc | master | d4419df (clean) | https://github.com/phoenix-rtos/phoenix-rtos-doc.git |
| phoenix-rtos-filesystems | master | d5db203 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-filesystems.git |
| phoenix-rtos-hostutils | master | 49a1fd9 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-hostutils.git |
| phoenix-rtos-kernel | master | 7348dd99 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-kernel.git |
| phoenix-rtos-lwip | master | 492b20b (clean) | https://github.com/phoenix-rtos/phoenix-rtos-lwip.git |
| phoenix-rtos-ports | master | dd3628a (clean) | https://github.com/phoenix-rtos/phoenix-rtos-ports.git |
| phoenix-rtos-posixsrv | master | 211d49a (clean) | https://github.com/phoenix-rtos/phoenix-rtos-posixsrv.git |
| phoenix-rtos-project | master | fe60daf (clean) | https://github.com/phoenix-rtos/phoenix-rtos-project.git |
| phoenix-rtos-tests | master | c7ccfc8 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-tests.git |
| phoenix-rtos-usb | master | e8e1092 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-usb.git |
| phoenix-rtos-utils | master | ce472cb (clean) | https://github.com/phoenix-rtos/phoenix-rtos-utils.git |
| plo | master | 3e22b52 (clean) | https://github.com/phoenix-rtos/plo.git |

## Machine-Parseable State

Consumed by `scripts/restore-integration-state.sh`. Fields: `<repo>\t<sha>\t<branch>`.

```integration-state-v1
_build	d66fab7105e0aee6a05fab6cf57967920af24e37	main
libphoenix	5b954c8228f50144d4f649af1249783d16f85775	master
phoenix-rtos-build	0db6eddf42477159b0424fba16f13c9f3f5cdd76	master
phoenix-rtos-corelibs	4814fedee83291da75b2c1d127e7ae68a3d99349	master
phoenix-rtos-devices	4a61c9634059324feeb9599e34014b6fb77fe230	master
phoenix-rtos-doc	d4419dfae5428cb3b8081404c34b12c78c86770d	master
phoenix-rtos-filesystems	d5db203efe72d91606fe8be096d445763728a7f4	master
phoenix-rtos-hostutils	49a1fd996e5745a19cc7ec0b22179bd1e90906cf	master
phoenix-rtos-kernel	7348dd999d40ed2458d4b0506f575a48f1086703	master
phoenix-rtos-lwip	492b20badec9ea1132c7ce47cfabcd9d48a2ee1b	master
phoenix-rtos-ports	dd3628a4b79d9262703e50cc129af82990ab8317	master
phoenix-rtos-posixsrv	211d49a7a3f54b736c3c03296492ce5b7cc45539	master
phoenix-rtos-project	fe60dafe9e69cae78f62fa6afef1cbb3d7205315	master
phoenix-rtos-tests	c7ccfc8a5c16fd08186ea8665b2d53f9828ade75	master
phoenix-rtos-usb	e8e10929ab2494ee9709e17510e9f2c0a7f1255c	master
phoenix-rtos-utils	ce472cb9050260310c57bc466eb1e682561dcad9	master
plo	3e22b52515ce705233ee97f48ee579bd05406ca6	master
```
