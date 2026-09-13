# Integration State: afunix-upstream-verified

## Summary

- Date: 2026-09-13
- Note: AF_UNIX upstream endpoint/channel re-port VERIFIED on HW: test-libc-unix-socket 38/0 OK (baseline 27/0), X11 0 faults 89.7pct non-black, 11.64 fps vs 10.45 pre-merge. kernel 114ce7d3, tests 2a627bf, project bumped.
- Generator: scripts/snapshot-integration-state.sh

## Repositories

| Repository | Branch | Commit SHA | Remote |
| --- | --- | --- | --- |
| _build | main | 9e2276cd7 (dirty(1)) | https://github.com/houp/phoenix-rpi.git |
| libphoenix | master | 11286c0 (clean) | https://github.com/phoenix-rtos/libphoenix.git |
| phoenix-rtos-build | master | 95d9fca (clean) | https://github.com/phoenix-rtos/phoenix-rtos-build.git |
| phoenix-rtos-corelibs | master | 4814fed (clean) | https://github.com/phoenix-rtos/phoenix-rtos-corelibs.git |
| phoenix-rtos-devices | master | b8d4bbb (clean) | https://github.com/phoenix-rtos/phoenix-rtos-devices.git |
| phoenix-rtos-doc | master | d4419df (clean) | https://github.com/phoenix-rtos/phoenix-rtos-doc.git |
| phoenix-rtos-filesystems | master | 4f2d8fd (clean) | https://github.com/phoenix-rtos/phoenix-rtos-filesystems.git |
| phoenix-rtos-hostutils | master | 49a1fd9 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-hostutils.git |
| phoenix-rtos-kernel | master | 114ce7d3 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-kernel.git |
| phoenix-rtos-lwip | master | 492b20b (clean) | https://github.com/phoenix-rtos/phoenix-rtos-lwip.git |
| phoenix-rtos-ports | master | b5ce28f (clean) | https://github.com/phoenix-rtos/phoenix-rtos-ports.git |
| phoenix-rtos-posixsrv | master | 211d49a (clean) | https://github.com/phoenix-rtos/phoenix-rtos-posixsrv.git |
| phoenix-rtos-project | master | e301d6d (clean) | https://github.com/phoenix-rtos/phoenix-rtos-project.git |
| phoenix-rtos-tests | master | 2a627bf (clean) | https://github.com/phoenix-rtos/phoenix-rtos-tests.git |
| phoenix-rtos-usb | master | bb33c9f (clean) | https://github.com/phoenix-rtos/phoenix-rtos-usb.git |
| phoenix-rtos-utils | master | e4dd587 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-utils.git |
| plo | master | 3e22b52 (clean) | https://github.com/phoenix-rtos/plo.git |

## Machine-Parseable State

Consumed by `scripts/restore-integration-state.sh`. Fields: `<repo>\t<sha>\t<branch>`.

```integration-state-v1
_build	9e2276cd79938f0adf79ea781937c0eb94d7f975	main
libphoenix	11286c0cb62343073861a023f16e44e04c89a36a	master
phoenix-rtos-build	95d9fcaf5ee6df1077340705e02d573f2e1df47b	master
phoenix-rtos-corelibs	4814fedee83291da75b2c1d127e7ae68a3d99349	master
phoenix-rtos-devices	b8d4bbbf9119321a25589d9db59b7b797f236784	master
phoenix-rtos-doc	d4419dfae5428cb3b8081404c34b12c78c86770d	master
phoenix-rtos-filesystems	4f2d8fdf2201785b9be4026523ea4fd39301deed	master
phoenix-rtos-hostutils	49a1fd996e5745a19cc7ec0b22179bd1e90906cf	master
phoenix-rtos-kernel	114ce7d313eed7381cfac45dcc4d89d188aa430d	master
phoenix-rtos-lwip	492b20badec9ea1132c7ce47cfabcd9d48a2ee1b	master
phoenix-rtos-ports	b5ce28f3ca03e0162adbad133cc2dea90c3004bc	master
phoenix-rtos-posixsrv	211d49a7a3f54b736c3c03296492ce5b7cc45539	master
phoenix-rtos-project	e301d6d65e9d14e599928fb719d06d69d0e464f8	master
phoenix-rtos-tests	2a627bf10a0e44bcb24ac03e7e25b5a2ba6808e6	master
phoenix-rtos-usb	bb33c9ff825c0f2cd85fadc5b4ca749911ac5f6d	master
phoenix-rtos-utils	e4dd587195c5b7ad5745884e2aa5010ec8e3687a	master
plo	3e22b52515ce705233ee97f48ee579bd05406ca6	master
```
