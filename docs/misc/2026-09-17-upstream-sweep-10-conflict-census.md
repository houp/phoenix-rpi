# 10th upstream sweep: what the merge actually costs, measured

*2026-09-17. `git merge-tree` against every sibling — no checkout, no working tree touched, so this
costs nothing and can be re-run any time.*

27 commits are waiting upstream. Four are marked breaking by upstream itself and move together:
`!syscalls: move signal handling to kernel`, **`!syscalls: remove signalPost system call`**,
`!syscalls/threadsinfo: return more granular thread information`, `!sys/threads: modify threadsinfo
declaration`.

## The conflict census

```
git -C sources/<repo> merge-tree --write-tree --name-only master origin/master
```

| repo | behind | conflicted files | hunks | conflicted lines |
|---|---|---|---|---|
| libphoenix | 5 | 7 | 8 | ~259 |
| phoenix-rtos-kernel | 13 | 4 | 7 | ~169 |
| phoenix-rtos-utils | 3 | 1 | — | — |
| phoenix-rtos-tests | 5 | **0** | — | — |
| phoenix-rtos-project | 1 | **0** | — | — |

Per file:

| file | hunks | conflicted lines |
|---|---|---|
| `libphoenix/signal/signal.c` | 1 | 159 |
| `kernel/proc/threads.c` | 2 | 101 |
| `kernel/vm/map.c` | 1 | 35 |
| `libphoenix/arch/aarch64/signal.S` | 1 | 24 |
| `libphoenix/sys/stat.c` | 1 | 24 |
| `kernel/proc/msg.c` | 2 | 18 |
| `kernel/proc/process.c` | 2 | 15 |
| `libphoenix/unistd/file.c` | 2 | 15 |
| `libphoenix/misc/init.c` | 2 | 14 |
| `libphoenix/include/sys/stat.h` | 1 | 12 |
| `libphoenix/include/signal.h` | 1 | 11 |
| `utils/psh/bind/bind.c` | 1 | — |

**12 files, 18 hunks, ~430 conflicted lines.**

↩ **This corrects my own estimate.** The weekly log said "the kernel's 310-commit divergence will be
worse [than libphoenix's 7 files]" — it is **not**: 4 files against 7, and the syscall table itself
does **not** conflict (TD-21 aligned our numbering with upstream's, which is exactly the payoff that
change was made for). The work is a focused signal-handling reconciliation, not a rewrite.

## What still makes it risky, independent of the conflict count

1. **A syscall is REMOVED.** Every binary that references `signalPost` must be rebuilt; a timestamp
   census (`scripts/check-stale-binaries.sh`) is the only check that sees a half-updated tree. This
   is the same shape as the 09-16 ABI break, where 189 of 333 binaries were stale while the six-app
   gate passed 6/6.
2. **`threadsinfo` changes shape**, and `psh` reads it — so the shell itself changes behaviour.
3. Signal delivery moves into the kernel, which touches `vfork`, `exec` and thread teardown — paths
   this port has its own fixes in (detached-thread stack race, premain hang).

## The order to do it in

1. Branch per repo: `agent/upstream-sweep-10` off master.
2. Merge, resolve the 18 hunks, commit on the branch.
3. `--scope full-clean` rebuild (a removed syscall invalidates every binary; nothing less is honest).
4. `scripts/check-stale-binaries.sh` **first** — the census sees what a passing gate cannot.
5. `test-libc-pthread` (signals and threads are what changed), then the full libc suites.
6. Six-app gate, both lanes.
7. Only then fast-forward master, and snapshot a manifest.

Rollback is `scripts/restore-integration-state.sh manifests/2026-09-17-w38-malloc-heap-provenance.md`.
The demo card is not affected either way: it boots `b95e983a` from its own rootfs.
