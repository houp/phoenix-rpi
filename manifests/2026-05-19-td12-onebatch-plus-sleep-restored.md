# Integration State: td12-onebatch-plus-sleep-restored

## Summary

- Date: 2026-05-19
- Note: Working baseline after build-cache lesson: TD-12 speed bundle + pl011_thr one-batch fix + sleep(1) restored. Intermittent cold-boot race observed in test cycle (devfs lookup occasionally hangs ~21 s) but morning's interactive verification confirms psh + help/mem/pwd/top all work over serial.
- Generator: scripts/snapshot-integration-state.sh

## Repositories

| Repository | Branch | Commit SHA | Remote |
| --- | --- | --- | --- |
| libphoenix | codex/upstream-sync-20260516 | bd61195 (clean) | https://github.com/phoenix-rtos/libphoenix |
| phoenix-rtos-build | codex/upstream-sync-20260516 | 3fd5c6b (clean) | https://github.com/phoenix-rtos/phoenix-rtos-build.git |
| phoenix-rtos-corelibs | master | ff6870b (clean) | https://github.com/phoenix-rtos/phoenix-rtos-corelibs/ |
| phoenix-rtos-devices | codex/upstream-sync-20260516 | 3899d38 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-devices.git |
| phoenix-rtos-doc | master | 5083598 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-doc |
| phoenix-rtos-filesystems | codex/upstream-sync-20260516 | c7a1401 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-filesystems.git |
| phoenix-rtos-hostutils | master | 2a894a3 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-hostutils/ |
| phoenix-rtos-kernel | agent/rpi4-program-reloc | 334638ee (clean) | https://github.com/phoenix-rtos/phoenix-rtos-kernel.git |
| phoenix-rtos-lwip | master | b63d44c (clean) | https://github.com/phoenix-rtos/phoenix-rtos-lwip |
| phoenix-rtos-ports | master | d3c6cf9 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-ports |
| phoenix-rtos-posixsrv | master | 0cecd86 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-posixsrv |
| phoenix-rtos-project | codex/upstream-sync-20260516 | dde9bb5 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-project.git |
| phoenix-rtos-tests | master | 8676250 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-tests.git |
| phoenix-rtos-usb | master | 404e646 (clean) | https://github.com/phoenix-rtos/phoenix-rtos-usb |
| phoenix-rtos-utils | codex/upstream-sync-20260516 | 3783dcc (clean) | https://github.com/phoenix-rtos/phoenix-rtos-utils/ |
| plo | codex/upstream-sync-20260516 | 6a5dfdd (clean) | https://github.com/phoenix-rtos/plo.git |

## Machine-Parseable State

Consumed by `scripts/restore-integration-state.sh`. Fields: `<repo>\t<sha>\t<branch>`.

```integration-state-v1
libphoenix	bd61195eb188d383c0163f5a22a461f7160c2fd8	codex/upstream-sync-20260516
phoenix-rtos-build	3fd5c6b20cbf2d6c0caa9a36577753bf455dc5f5	codex/upstream-sync-20260516
phoenix-rtos-corelibs	ff6870be35405ee63bac73b155816f62d05f755d	master
phoenix-rtos-devices	3899d38a88fd9c74275b0f2c5f236087dda43322	codex/upstream-sync-20260516
phoenix-rtos-doc	5083598b45ec21355a90467656c2c101ed217ea4	master
phoenix-rtos-filesystems	c7a14019c6a70b6e0a6ce8b93fd232c90684ed68	codex/upstream-sync-20260516
phoenix-rtos-hostutils	2a894a3d643df5b24d45ae5147993fb07e3b3bc0	master
phoenix-rtos-kernel	334638ee2ac82390b5c06626c1ed6ca45fb2ee66	agent/rpi4-program-reloc
phoenix-rtos-lwip	b63d44c2fc998f63bde1c3e24d0faf5b0a188c46	master
phoenix-rtos-ports	d3c6cf99fbeba450cddff097765e1dfbd28ab33d	master
phoenix-rtos-posixsrv	0cecd86d396030dc5cf65a366d85ddb0a42e501b	master
phoenix-rtos-project	dde9bb5e0bda6a1db1550a5c68c15811efc8c82e	codex/upstream-sync-20260516
phoenix-rtos-tests	8676250f05f73fcc82192aa68e4dff4991a4c0a0	master
phoenix-rtos-usb	404e646354a81c884834249315df1b5c4f601367	master
phoenix-rtos-utils	3783dcc5921eb6a95b9b6b5643eac3615b32a4ad	codex/upstream-sync-20260516
plo	6a5dfddbb9a7569ff713071c5228858772362572	codex/upstream-sync-20260516
```
