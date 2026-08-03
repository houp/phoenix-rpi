# Integration State: nfs4-expiry-recovery

## Summary

- Date: 2026-08-03
- Note: nfs-fs recovers from NFSv4 lease/state expiry (NFS4ERR_EXPIRED): renew thread + reclaim/retry; libnfs nfs_renew patch. HW-validated netboot NFS-root, no regression, 0 ERANGE, survives 100s idle.
- Generator: scripts/snapshot-integration-state.sh

## Repositories

| Repository | Branch | Commit SHA | Remote |
| --- | --- | --- | --- |
| _build | main | 4e57fda (dirty(11)) | https://github.com/houp/phoenix-rpi.git |
| libphoenix | master | 75c60e7 (clean) | https://github.com/phoenix-rtos/libphoenix.git |
| phoenix-rtos-build | master | 4abd7a0 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-build.git |
| phoenix-rtos-corelibs | master | 2311290 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-corelibs.git |
| phoenix-rtos-devices | master | bff0e89 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-devices.git |
| phoenix-rtos-doc | master | 50602f3 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-doc.git |
| phoenix-rtos-filesystems | master | 8231627 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-filesystems.git |
| phoenix-rtos-hostutils | master | 31f3431 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-hostutils.git |
| phoenix-rtos-kernel | master | 7e6cbe37 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-kernel.git |
| phoenix-rtos-lwip | master | 7091a41 (dirty(4)) | https://github.com/phoenix-rtos/phoenix-rtos-lwip.git |
| phoenix-rtos-ports | master | d84e902 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-ports.git |
| phoenix-rtos-posixsrv | master | ff04a1b (clean) | https://github.com/phoenix-rtos/phoenix-rtos-posixsrv.git |
| phoenix-rtos-project | master | c1428e6 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-project.git |
| phoenix-rtos-tests | master | 19b11eb (clean) | https://github.com/phoenix-rtos/phoenix-rtos-tests.git |
| phoenix-rtos-usb | master | 8e8316f (clean) | https://github.com/phoenix-rtos/phoenix-rtos-usb.git |
| phoenix-rtos-utils | master | 5e9e3c9 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-utils.git |
| plo | master | d6c0969 (clean) | https://github.com/phoenix-rtos/plo.git |

## Machine-Parseable State

Consumed by `scripts/restore-integration-state.sh`. Fields: `<repo>\t<sha>\t<branch>`.

```integration-state-v1
_build	4e57fdac6277534cc8e901222c3604d78598237b	main
libphoenix	75c60e71dde3715bf4d2512a717a6e9f820bfe0d	master
phoenix-rtos-build	4abd7a039319b9399356025b970ba365dd42b386	master
phoenix-rtos-corelibs	2311290343e37cde2440ea7056119743150bf631	master
phoenix-rtos-devices	bff0e895e41cb1b01c2fd683362551a5d1daa32b	master
phoenix-rtos-doc	50602f302e3f59f0d73de3f5b3a873f2ac3c5428	master
phoenix-rtos-filesystems	8231627b95f004ec0da7b30f975415464e004aca	master
phoenix-rtos-hostutils	31f3431bbe49fd5df77eaeb0213b65d9b8eacf68	master
phoenix-rtos-kernel	7e6cbe37960401170a7026507741c9b7aff21fb0	master
phoenix-rtos-lwip	7091a415864222c3a38f8bd83cef21da18a3a17a	master
phoenix-rtos-ports	d84e902c6d9f6e6bea20b39e659ed6cf35694335	master
phoenix-rtos-posixsrv	ff04a1b3a669238147ef8c7c5bc28c2e3652f76d	master
phoenix-rtos-project	c1428e63ca6eace471faf3524891bb8c805a4a97	master
phoenix-rtos-tests	19b11eb2e165aad59a3d39772568c42e55eabaaf	master
phoenix-rtos-usb	8e8316f95adf6933180d55f31a40064d95b2fb46	master
phoenix-rtos-utils	5e9e3c9bdb52c2caea352ac6d538348041741d1d	master
plo	d6c09690a9a1502d9e38796c6fe4f047958857a7	master
```
