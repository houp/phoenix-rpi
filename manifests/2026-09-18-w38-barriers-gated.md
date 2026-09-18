# Integration State: 2026-09-18-w38-barriers-gated

## Summary

- Date: 2026-09-18
- Note: Five DMA barrier fixes + V3D wedge dump + ARMTRIALS degrade + allocator extent/overlap/munmap checks + kernel map-overlap report. Six-app gate barriergate 6/6, 0 faults, torches present. Relink verified on the staged ELF (OVERLAPPING marker 0 before, 1 after). cb-race A/B measured the barrier race: 0/5000 stale with dsb sy, 146/5000 without.
- Generator: scripts/snapshot-integration-state.sh

## Repositories

| Repository | Branch | Commit SHA | Remote |
| --- | --- | --- | --- |
| _build | main | a54a43484 (dirty(1)) | https://github.com/houp/phoenix-rpi.git |
| libphoenix | master | 5b954c8 (clean) | https://github.com/phoenix-rtos/libphoenix.git |
| phoenix-rtos-build | master | 0db6edd (clean) | https://github.com/phoenix-rtos/phoenix-rtos-build.git |
| phoenix-rtos-corelibs | master | 4814fed (clean) | https://github.com/phoenix-rtos/phoenix-rtos-corelibs.git |
| phoenix-rtos-devices | master | fc2e0ba (clean) | https://github.com/phoenix-rtos/phoenix-rtos-devices.git |
| phoenix-rtos-doc | master | d4419df (clean) | https://github.com/phoenix-rtos/phoenix-rtos-doc.git |
| phoenix-rtos-filesystems | master | d5db203 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-filesystems.git |
| phoenix-rtos-hostutils | master | 49a1fd9 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-hostutils.git |
| phoenix-rtos-kernel | master | fb7fd4ca (clean) | https://github.com/phoenix-rtos/phoenix-rtos-kernel.git |
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
_build	a54a434840da016da53563dec09b8eb878eb5ef8	main
libphoenix	5b954c8228f50144d4f649af1249783d16f85775	master
phoenix-rtos-build	0db6eddf42477159b0424fba16f13c9f3f5cdd76	master
phoenix-rtos-corelibs	4814fedee83291da75b2c1d127e7ae68a3d99349	master
phoenix-rtos-devices	fc2e0ba541e62a070f6bcb7ebde3e5e01f726ab4	master
phoenix-rtos-doc	d4419dfae5428cb3b8081404c34b12c78c86770d	master
phoenix-rtos-filesystems	d5db203efe72d91606fe8be096d445763728a7f4	master
phoenix-rtos-hostutils	49a1fd996e5745a19cc7ec0b22179bd1e90906cf	master
phoenix-rtos-kernel	fb7fd4ca972623b3673770c5600a09142676d92c	master
phoenix-rtos-lwip	492b20badec9ea1132c7ce47cfabcd9d48a2ee1b	master
phoenix-rtos-ports	dd3628a4b79d9262703e50cc129af82990ab8317	master
phoenix-rtos-posixsrv	211d49a7a3f54b736c3c03296492ce5b7cc45539	master
phoenix-rtos-project	fe60dafe9e69cae78f62fa6afef1cbb3d7205315	master
phoenix-rtos-tests	c7ccfc8a5c16fd08186ea8665b2d53f9828ade75	master
phoenix-rtos-usb	e8e10929ab2494ee9709e17510e9f2c0a7f1255c	master
phoenix-rtos-utils	ce472cb9050260310c57bc466eb1e682561dcad9	master
plo	3e22b52515ce705233ee97f48ee579bd05406ca6	master
```
