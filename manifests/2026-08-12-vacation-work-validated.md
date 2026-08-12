# Integration State: vacation-work-validated

## Summary

- Date: 2026-08-12
- Note: 2026-08-12 known-good rollback point: all vacation-run work (WiFi/BT drivers, 15 code-review fixes, RAM-staging, SDL consolidation) at the SHAs validated by the 2026-08-12 regression/health check (boot + thermal/gpio/WiFi-scan/game render, 0 faults). Kernel d8baae66 (readahead-cluster experiment reverted). Supersedes 2026-08-12-readahead-cluster-64 as the current-state reference (that one records the reverted-away kernel 8c465fbb).
- Generator: scripts/snapshot-integration-state.sh

## Repositories

| Repository | Branch | Commit SHA | Remote |
| --- | --- | --- | --- |
| _build | main | 98fbb35 (dirty(18)) | https://github.com/houp/phoenix-rpi.git |
| libphoenix | master | 3a74c04 (clean) | https://github.com/phoenix-rtos/libphoenix.git |
| phoenix-rtos-build | master | 4abd7a0 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-build.git |
| phoenix-rtos-corelibs | master | d026ff0 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-corelibs.git |
| phoenix-rtos-devices | master | 8d95c9b (dirty(2)) | https://github.com/phoenix-rtos/phoenix-rtos-devices.git |
| phoenix-rtos-doc | master | a982407 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-doc.git |
| phoenix-rtos-filesystems | master | bec497c (clean) | https://github.com/phoenix-rtos/phoenix-rtos-filesystems.git |
| phoenix-rtos-hostutils | master | 31f3431 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-hostutils.git |
| phoenix-rtos-kernel | master | d8baae66 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-kernel.git |
| phoenix-rtos-lwip | master | fb8af75 (dirty(4)) | https://github.com/phoenix-rtos/phoenix-rtos-lwip.git |
| phoenix-rtos-ports | master | 94ee607 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-ports.git |
| phoenix-rtos-posixsrv | master | ff04a1b (clean) | https://github.com/phoenix-rtos/phoenix-rtos-posixsrv.git |
| phoenix-rtos-project | master | fa84866 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-project.git |
| phoenix-rtos-tests | master | d049606 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-tests.git |
| phoenix-rtos-usb | master | d592025 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-usb.git |
| phoenix-rtos-utils | master | b64a8b1 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-utils.git |
| plo | master | 0033722 (clean) | https://github.com/phoenix-rtos/plo.git |

## Machine-Parseable State

Consumed by `scripts/restore-integration-state.sh`. Fields: `<repo>\t<sha>\t<branch>`.

```integration-state-v1
_build	98fbb35fe64375492d69a1397ecb48e09b05e027	main
libphoenix	3a74c04986527acd6ece39016b256cf1be8ab8f3	master
phoenix-rtos-build	4abd7a039319b9399356025b970ba365dd42b386	master
phoenix-rtos-corelibs	d026ff096456eaaf70db4b6c2c93004a39c8bbf9	master
phoenix-rtos-devices	8d95c9be1ee85f7e6cdd4ffd16f855636a15d1df	master
phoenix-rtos-doc	a98240766bbc1fd7b59acc5641bb9d272f6adb5e	master
phoenix-rtos-filesystems	bec497cbe53c2cbf4b9084c4fadb5fbe51076fbb	master
phoenix-rtos-hostutils	31f3431bbe49fd5df77eaeb0213b65d9b8eacf68	master
phoenix-rtos-kernel	d8baae66393ee55a1e1022a68a7ae8a405e57355	master
phoenix-rtos-lwip	fb8af750d75371dd4ffac2cde2dbb1b07e88f955	master
phoenix-rtos-ports	94ee60771eb1eb9375b42b169271f53072edb33d	master
phoenix-rtos-posixsrv	ff04a1b3a669238147ef8c7c5bc28c2e3652f76d	master
phoenix-rtos-project	fa84866aba6b1a3d08323a63e76e92d1caa36c99	master
phoenix-rtos-tests	d049606107d45303814392b56cbff114d7d8e7b8	master
phoenix-rtos-usb	d592025f0706f3302bea93ec2c629435aa1f125d	master
phoenix-rtos-utils	b64a8b16001fd6d974b74b5ce7761519e644b9dd	master
plo	003372296ee16f71d382ae10ed19ba934c8d5e48	master
```
