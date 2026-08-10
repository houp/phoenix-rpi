# Integration State: 2026-08-10-dlopen-phase-a

## Summary

- Date: 2026-08-10
- Note: libphoenix dlopen/dlsym/dlclose/dlerror (Phase-A dynamic linking, no kernel change); real <dlfcn.h> API HW-validated on Pi with auto host-.symtab resolution, 0 faults
- Generator: scripts/snapshot-integration-state.sh

## Repositories

| Repository | Branch | Commit SHA | Remote |
| --- | --- | --- | --- |
| _build | main | 8ad9aed (dirty(19)) | https://github.com/houp/phoenix-rpi.git |
| libphoenix | master | 3f98897 (clean) | https://github.com/phoenix-rtos/libphoenix.git |
| phoenix-rtos-build | master | 4abd7a0 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-build.git |
| phoenix-rtos-corelibs | master | d026ff0 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-corelibs.git |
| phoenix-rtos-devices | master | 89ffe1c (clean) | https://github.com/phoenix-rtos/phoenix-rtos-devices.git |
| phoenix-rtos-doc | master | a982407 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-doc.git |
| phoenix-rtos-filesystems | master | bec497c (clean) | https://github.com/phoenix-rtos/phoenix-rtos-filesystems.git |
| phoenix-rtos-hostutils | master | 31f3431 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-hostutils.git |
| phoenix-rtos-kernel | master | d8baae66 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-kernel.git |
| phoenix-rtos-lwip | master | 8fe3cf6 (dirty(4)) | https://github.com/phoenix-rtos/phoenix-rtos-lwip.git |
| phoenix-rtos-ports | master | bc5e7ae (clean) | https://github.com/phoenix-rtos/phoenix-rtos-ports.git |
| phoenix-rtos-posixsrv | master | ff04a1b (clean) | https://github.com/phoenix-rtos/phoenix-rtos-posixsrv.git |
| phoenix-rtos-project | master | 22376b7 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-project.git |
| phoenix-rtos-tests | master | d049606 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-tests.git |
| phoenix-rtos-usb | master | d592025 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-usb.git |
| phoenix-rtos-utils | master | b64a8b1 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-utils.git |
| plo | master | 0033722 (clean) | https://github.com/phoenix-rtos/plo.git |

## Machine-Parseable State

Consumed by `scripts/restore-integration-state.sh`. Fields: `<repo>\t<sha>\t<branch>`.

```integration-state-v1
_build	8ad9aed7bbb1ef03a38b785adfc8e9049abe61ba	main
libphoenix	3f98897118977a2a89a5cf7785f4973d59c5fb5e	master
phoenix-rtos-build	4abd7a039319b9399356025b970ba365dd42b386	master
phoenix-rtos-corelibs	d026ff096456eaaf70db4b6c2c93004a39c8bbf9	master
phoenix-rtos-devices	89ffe1cc79270fcadb990016829117e249b97c8d	master
phoenix-rtos-doc	a98240766bbc1fd7b59acc5641bb9d272f6adb5e	master
phoenix-rtos-filesystems	bec497cbe53c2cbf4b9084c4fadb5fbe51076fbb	master
phoenix-rtos-hostutils	31f3431bbe49fd5df77eaeb0213b65d9b8eacf68	master
phoenix-rtos-kernel	d8baae66393ee55a1e1022a68a7ae8a405e57355	master
phoenix-rtos-lwip	8fe3cf652b94b28f5269567d89faed770720296e	master
phoenix-rtos-ports	bc5e7aed16df6fa767736240e36d7fbb1b9305be	master
phoenix-rtos-posixsrv	ff04a1b3a669238147ef8c7c5bc28c2e3652f76d	master
phoenix-rtos-project	22376b76f7c518ec103a78e50f8a639870684395	master
phoenix-rtos-tests	d049606107d45303814392b56cbff114d7d8e7b8	master
phoenix-rtos-usb	d592025f0706f3302bea93ec2c629435aa1f125d	master
phoenix-rtos-utils	b64a8b16001fd6d974b74b5ce7761519e644b9dd	master
plo	003372296ee16f71d382ae10ed19ba934c8d5e48	master
```
