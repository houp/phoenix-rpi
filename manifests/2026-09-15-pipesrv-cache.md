# Integration State: pipesrv-cache

## Summary

- Date: 2026-09-15
- Note: premain-hang fix + /dev/posix/pipes cache; 6/6 showcase CONTENT, nfs-recovery PASS, libc posixsrv/stdio/unix-socket green
- Generator: scripts/snapshot-integration-state.sh

## Repositories

| Repository | Branch | Commit SHA | Remote |
| --- | --- | --- | --- |
| _build | main | 597395f81 (dirty(1)) | https://github.com/houp/phoenix-rpi.git |
| libphoenix | master | 47dde32 (clean) | https://github.com/phoenix-rtos/libphoenix.git |
| phoenix-rtos-build | master | 95d9fca (clean) | https://github.com/phoenix-rtos/phoenix-rtos-build.git |
| phoenix-rtos-corelibs | master | 4814fed (clean) | https://github.com/phoenix-rtos/phoenix-rtos-corelibs.git |
| phoenix-rtos-devices | master | b8d4bbb (clean) | https://github.com/phoenix-rtos/phoenix-rtos-devices.git |
| phoenix-rtos-doc | master | d4419df (clean) | https://github.com/phoenix-rtos/phoenix-rtos-doc.git |
| phoenix-rtos-filesystems | master | d5db203 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-filesystems.git |
| phoenix-rtos-hostutils | master | 49a1fd9 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-hostutils.git |
| phoenix-rtos-kernel | master | 4e473590 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-kernel.git |
| phoenix-rtos-lwip | master | 492b20b (clean) | https://github.com/phoenix-rtos/phoenix-rtos-lwip.git |
| phoenix-rtos-ports | master | 41ebc7a (clean) | https://github.com/phoenix-rtos/phoenix-rtos-ports.git |
| phoenix-rtos-posixsrv | master | 211d49a (clean) | https://github.com/phoenix-rtos/phoenix-rtos-posixsrv.git |
| phoenix-rtos-project | master | e301d6d (clean) | https://github.com/phoenix-rtos/phoenix-rtos-project.git |
| phoenix-rtos-tests | master | 2a627bf (clean) | https://github.com/phoenix-rtos/phoenix-rtos-tests.git |
| phoenix-rtos-usb | master | bb33c9f (clean) | https://github.com/phoenix-rtos/phoenix-rtos-usb.git |
| phoenix-rtos-utils | master | e4dd587 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-utils.git |
| plo | master | 3e22b52 (clean) | https://github.com/phoenix-rtos/plo.git |

## Machine-Parseable State

Consumed by `scripts/restore-integration-state.sh`. Fields: `<repo>\t<sha>\t<branch>`.

```integration-state-v1
_build	597395f81d7ad4b84c360ad87f91b65b42efcf51	main
libphoenix	47dde3213ce2a3f27e8249e09ddeead10c65c7ef	master
phoenix-rtos-build	95d9fcaf5ee6df1077340705e02d573f2e1df47b	master
phoenix-rtos-corelibs	4814fedee83291da75b2c1d127e7ae68a3d99349	master
phoenix-rtos-devices	b8d4bbbf9119321a25589d9db59b7b797f236784	master
phoenix-rtos-doc	d4419dfae5428cb3b8081404c34b12c78c86770d	master
phoenix-rtos-filesystems	d5db203efe72d91606fe8be096d445763728a7f4	master
phoenix-rtos-hostutils	49a1fd996e5745a19cc7ec0b22179bd1e90906cf	master
phoenix-rtos-kernel	4e47359015ea4cd23f7772d2152091059f06ebd3	master
phoenix-rtos-lwip	492b20badec9ea1132c7ce47cfabcd9d48a2ee1b	master
phoenix-rtos-ports	41ebc7ab72ce13e9a02b198ae9f77e7702d169bc	master
phoenix-rtos-posixsrv	211d49a7a3f54b736c3c03296492ce5b7cc45539	master
phoenix-rtos-project	e301d6d65e9d14e599928fb719d06d69d0e464f8	master
phoenix-rtos-tests	2a627bf10a0e44bcb24ac03e7e25b5a2ba6808e6	master
phoenix-rtos-usb	bb33c9ff825c0f2cd85fadc5b4ca749911ac5f6d	master
phoenix-rtos-utils	e4dd587195c5b7ad5745884e2aa5010ec8e3687a	master
plo	3e22b52515ce705233ee97f48ee579bd05406ca6	master
```
