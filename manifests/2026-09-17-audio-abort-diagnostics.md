# Integration State: audio-abort-diagnostics

## Summary

- Date: 2026-09-17
- Note: rpi4-audio self-test abort now prints DMA_CS + ring cursors (devices HEAD); healthy path HW-verified unchanged
- Generator: scripts/snapshot-integration-state.sh

## Repositories

| Repository | Branch | Commit SHA | Remote |
| --- | --- | --- | --- |
| _build | main | d6c86156c (dirty(2)) | https://github.com/houp/phoenix-rpi.git |
| libphoenix | master | c283f2d (clean) | https://github.com/phoenix-rtos/libphoenix.git |
| phoenix-rtos-build | master | 0db6edd (clean) | https://github.com/phoenix-rtos/phoenix-rtos-build.git |
| phoenix-rtos-corelibs | master | 4814fed (clean) | https://github.com/phoenix-rtos/phoenix-rtos-corelibs.git |
| phoenix-rtos-devices | master | d13778d (clean) | https://github.com/phoenix-rtos/phoenix-rtos-devices.git |
| phoenix-rtos-doc | master | d4419df (clean) | https://github.com/phoenix-rtos/phoenix-rtos-doc.git |
| phoenix-rtos-filesystems | master | d5db203 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-filesystems.git |
| phoenix-rtos-hostutils | master | 49a1fd9 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-hostutils.git |
| phoenix-rtos-kernel | master | c88690cc (clean) | https://github.com/phoenix-rtos/phoenix-rtos-kernel.git |
| phoenix-rtos-lwip | master | 492b20b (clean) | https://github.com/phoenix-rtos/phoenix-rtos-lwip.git |
| phoenix-rtos-ports | master | b0fc510 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-ports.git |
| phoenix-rtos-posixsrv | master | 211d49a (clean) | https://github.com/phoenix-rtos/phoenix-rtos-posixsrv.git |
| phoenix-rtos-project | master | 20a8b70 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-project.git |
| phoenix-rtos-tests | master | be80bfb (clean) | https://github.com/phoenix-rtos/phoenix-rtos-tests.git |
| phoenix-rtos-usb | master | e8e1092 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-usb.git |
| phoenix-rtos-utils | master | 6f7a991 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-utils.git |
| plo | master | 3e22b52 (clean) | https://github.com/phoenix-rtos/plo.git |

## Machine-Parseable State

Consumed by `scripts/restore-integration-state.sh`. Fields: `<repo>\t<sha>\t<branch>`.

```integration-state-v1
_build	d6c86156c44504ec4a9eae4d463c412482bf3b06	main
libphoenix	c283f2d5f4e7c8f8ece6bb6cf9379dc2722360a1	master
phoenix-rtos-build	0db6eddf42477159b0424fba16f13c9f3f5cdd76	master
phoenix-rtos-corelibs	4814fedee83291da75b2c1d127e7ae68a3d99349	master
phoenix-rtos-devices	d13778d128d75a3d89fec6d6829200dadf5fb3b6	master
phoenix-rtos-doc	d4419dfae5428cb3b8081404c34b12c78c86770d	master
phoenix-rtos-filesystems	d5db203efe72d91606fe8be096d445763728a7f4	master
phoenix-rtos-hostutils	49a1fd996e5745a19cc7ec0b22179bd1e90906cf	master
phoenix-rtos-kernel	c88690cc6ad1ffde8cfb119d20e75d812d6bbb50	master
phoenix-rtos-lwip	492b20badec9ea1132c7ce47cfabcd9d48a2ee1b	master
phoenix-rtos-ports	b0fc510705ebf0169d1beb4b365b8c2c5b3147d7	master
phoenix-rtos-posixsrv	211d49a7a3f54b736c3c03296492ce5b7cc45539	master
phoenix-rtos-project	20a8b709b5890127ffc68c9042eabf8d67a82f62	master
phoenix-rtos-tests	be80bfbc1aae2806b923dd0f0d24d5f3651bdf67	master
phoenix-rtos-usb	e8e10929ab2494ee9709e17510e9f2c0a7f1255c	master
phoenix-rtos-utils	6f7a991a4adb275d5de246c455db8234e220713a	master
plo	3e22b52515ce705233ee97f48ee579bd05406ca6	master
```
