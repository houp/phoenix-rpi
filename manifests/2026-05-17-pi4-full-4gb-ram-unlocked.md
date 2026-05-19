# Integration State: 2026-05-17-pi4-full-4gb-ram-unlocked

## Summary

- Date: 2026-05-17
- Note: Goal 3 COMPLETE: kernel sees both Pi 4 4GB memory banks (948 MB low + 3008 MB high = 3956 MB total). Achieved via plo reading firmware-patched DTB pointer directly from armstub PA 0xf8 (bypassing the x0 propagation chain that arrived 0). UART proof: pmap: nBanks=2 with start=0x0/0x40000000 entries. Boot reaches psh tty open cleanly.
- Generator: scripts/snapshot-integration-state.sh

## Repositories

| Repository | Branch | Commit SHA | Remote |
| --- | --- | --- | --- |
| libphoenix | codex/upstream-sync-20260516 | d063d93 (clean) | https://github.com/phoenix-rtos/libphoenix |
| phoenix-rtos-build | codex/upstream-sync-20260516 | 3fd5c6b (clean) | https://github.com/phoenix-rtos/phoenix-rtos-build.git |
| phoenix-rtos-corelibs | master | ff6870b (clean) | https://github.com/phoenix-rtos/phoenix-rtos-corelibs/ |
| phoenix-rtos-devices | codex/upstream-sync-20260516 | 18eef25 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-devices.git |
| phoenix-rtos-doc | master | 5083598 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-doc |
| phoenix-rtos-filesystems | codex/upstream-sync-20260516 | c7a1401 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-filesystems.git |
| phoenix-rtos-hostutils | master | 2a894a3 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-hostutils/ |
| phoenix-rtos-kernel | agent/rpi4-program-reloc | effaeb38 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-kernel.git |
| phoenix-rtos-lwip | master | b63d44c (clean) | https://github.com/phoenix-rtos/phoenix-rtos-lwip |
| phoenix-rtos-ports | master | d3c6cf9 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-ports |
| phoenix-rtos-posixsrv | master | 0cecd86 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-posixsrv |
| phoenix-rtos-project | codex/upstream-sync-20260516 | dde9bb5 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-project.git |
| phoenix-rtos-tests | master | 8676250 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-tests.git |
| phoenix-rtos-usb | master | 2a35b16 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-usb |
| phoenix-rtos-utils | codex/upstream-sync-20260516 | 7800484 (dirty(1)) | https://github.com/phoenix-rtos/phoenix-rtos-utils/ |
| plo | codex/upstream-sync-20260516 | 84ffbea (clean) | https://github.com/phoenix-rtos/plo.git |

## Machine-Parseable State

Consumed by `scripts/restore-integration-state.sh`. Fields: `<repo>\t<sha>\t<branch>`.

```integration-state-v1
libphoenix	d063d9362d933e0160a5e51b16ea59f25060b79c	codex/upstream-sync-20260516
phoenix-rtos-build	3fd5c6b20cbf2d6c0caa9a36577753bf455dc5f5	codex/upstream-sync-20260516
phoenix-rtos-corelibs	ff6870be35405ee63bac73b155816f62d05f755d	master
phoenix-rtos-devices	18eef259a0e3fb4b877690eec3fe4510f757c26a	codex/upstream-sync-20260516
phoenix-rtos-doc	5083598b45ec21355a90467656c2c101ed217ea4	master
phoenix-rtos-filesystems	c7a14019c6a70b6e0a6ce8b93fd232c90684ed68	codex/upstream-sync-20260516
phoenix-rtos-hostutils	2a894a3d643df5b24d45ae5147993fb07e3b3bc0	master
phoenix-rtos-kernel	effaeb388e322d096774eec01cf1cb67c1e9ce0f	agent/rpi4-program-reloc
phoenix-rtos-lwip	b63d44c2fc998f63bde1c3e24d0faf5b0a188c46	master
phoenix-rtos-ports	d3c6cf99fbeba450cddff097765e1dfbd28ab33d	master
phoenix-rtos-posixsrv	0cecd86d396030dc5cf65a366d85ddb0a42e501b	master
phoenix-rtos-project	dde9bb5e0bda6a1db1550a5c68c15811efc8c82e	codex/upstream-sync-20260516
phoenix-rtos-tests	8676250f05f73fcc82192aa68e4dff4991a4c0a0	master
phoenix-rtos-usb	2a35b16927fb7e58329fe494e5176b374695fa87	master
phoenix-rtos-utils	780048412adbed1ece5e995fc5809135804deb24	codex/upstream-sync-20260516
plo	84ffbea2ba1e78783d1a3cfcaf6d4166f813d9f6	codex/upstream-sync-20260516
```
