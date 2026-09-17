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


## Resolved on branches (2026-09-17, same day) — NOT built yet

`agent/upstream-sweep-10` exists in all five repos and is pushed to `publish`; **every master is
untouched** (`changes=0`, still `behind`). SHAs: libphoenix `434f6de` · kernel `00dd500a` ·
utils `ce472cb` · tests `c7ccfc8` · project `fe60daf`.

How each conflict went — the ones that were judgement calls, not mechanics:

| conflict | resolution |
|---|---|
| `libphoenix/signal/signal.c` | Took **upstream's file** (signal handling now lives in the kernel) and re-added the two pieces that are ours: the `_dbg_signal_ctx` / `_dbg_signal_pc` globals (read by `phoenix-rtos-corelibs/libdbg`) and `siginterrupt()`, which upstream **declares but does not implement** and which `phoenix-rtos-tests libc/signal/handler.c` asserts. |
| `libphoenix/arch/aarch64/signal.S` | Upstream restructured the trampoline (the kernel now pushes the handler address and the trampoline `blr`s it). Our libdbg stash was re-derived against the **new** stack layout, read off `hal_cpuPushSignal()`: `[sp+24]` = signalCtx, `[sp+32]` = interrupted pc. |
| `libphoenix` umask (`sys/stat.c`, `unistd/file.c`, `include/sys/stat.h`) | Took upstream's atomic `umask`/`__getumask()` and **deleted our `_libc_applyUmask`** helper and its three call sites — less divergence, and upstream's is thread-safe. ⓘ Behaviour change: upstream's `_stat_init()` starts the mask at **0**, not 022. The libc stat tests read the mask back rather than assuming it, so they are unaffected. |
| `libphoenix/misc/init.c` | Kept our `LIBC_TRACE` instrumentation, dropped `_signals_init()` (gone upstream), added upstream's `_stat_init()`, and fixed the trace comment that referred to the now-deleted "signals" stage. |
| `kernel/vm/map.c` | Kept our three-way fault path (a USER thread whose `process_t` is gone retires instead of asserting — the bug that turned one corrupt pointer into a **dead Pi** on an X11 exit) and adopted upstream's `SIGSEGV` numbering plus its new trailing `proc_threadEnd()`, which now expresses our "retire this thread" more cleanly than the `hal_cpuReschedule()` we used. |
| `kernel/proc/msg.c` | Took upstream's `proc_sendEx(..., interruptible)` split and kept our `MSG_SEND_WATCHDOG` timeout inside it (`WD_TIMEOUT` is 0 when the watchdog is compiled out, i.e. identical to upstream then). |
| `kernel/proc/process.c` | Kept both sides: our `p->magic = 0U` invalidation **and** upstream's `vm_kfree(p->sigactions)`; kept our exec-failure diagnostic print and took upstream's `SIGKILL` as the termination mechanism. |
| `kernel/proc/threads.c` (threadsinfo) | Took **upstream's** rewrite wholesale — our side carried only a commented-out `ppid` line and a TODO. |
| `kernel/proc/threads.c` (vfork kstack) | Both applied — upstream's restructure first, then our guard. ✅ **Reviewed (kernel `15fdea32`): they cover DIFFERENT threads, so neither is redundant.** `execdata`/`execkstack` are set on the vforked **child** (`process.c:1716` parks the child's own stack in `execkstack` while it borrows the parent's), and upstream's block restores it before the free. `lentKstack` is set on the **parent** (`process.c:1746`) for exactly the duration of that borrow, and our guard is what stops an asynchronous parent death from freeing the stack the child is running on. Upstream's `execdata != NULL` test is also equivalent to the `execkstack != NULL ? execkstack : kstack` choice our version made, since both are set and cleared together. |

ⓘ The `full-clean` build started 18:37 predates kernel `15fdea32` by a **comment only**.

**Still to do, in order:** `--scope full-clean` → `check-stale-binaries.sh` → `test-libc-pthread`
→ full libc suites → six-app gate → only then fast-forward master + manifest. Nothing has been
compiled yet: the branches are a *resolution*, not a verification.


## What the tests cannot prove absent: four semantic changes that came with the sweep

Everything below **passed** every gate (1 088 libc tests, six-app gate, three endurance runs, 6/6
desktop exits). These are recorded because a green suite does not mean "nothing changed" — it means
nothing changed that the suite looks at. If an intermittent shows up later, start here.

1. **An interruptible wait now checks for a pending signal BEFORE sleeping**
   (`957a1695 proc/threads: interruptible wait signal delivery`). It used to test only
   `thread->exit`; it now calls `_threads_checkSignal()` and returns **-EINTR** if a signal is
   pending. ⇒ blocking operations can return `EINTR` in windows where they previously blocked. Our
   libphoenix retry loops (`while ((err = sys_…) == -EINTR)`) absorb it; anything that forgot one is
   newly exposed. Watch for: a rare "operation failed" where the errno is EINTR.
2. **`proc_close` is now uninterruptible** (`62cf68ef`), via the new `proc_sendUninterruptible()` —
   upstream's own fix for a `posix_fileDeref` deadlock when a signal is pending during close. This is
   the other half of (1): they had to exempt close from the new interruptibility.
3. **Pending signals are inherited across `exec`** (`a7767afe`). A signal posted to a process that is
   mid-`exec` is no longer dropped. Affects the vfork/exec path this port has its own fixes in.
4. **Signals that cannot be ignored are no longer masked** (`256e764f`), and `sys_exit` now ignores
   bits above the 8th (`19000dae`) — an exit status of 0x100 no longer reads as 0.

ⓘ The thing to hold on to: the sweep's risk was never the merge conflicts (18 hunks, all mechanical
once read). It is that signal delivery is now a **kernel** concern with slightly different timing,
and timing changes are exactly what a pass/fail suite is blind to. The three endurance runs and the
20-start Quake II hunt exist for that reason.
