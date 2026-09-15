# `premain-hang` — the full 2026-09-14/15 hunt

Moved out of `docs/inprogress/WEEK-2026-W38.md` when the bug was closed. The short version lives in
the weekly log; this is the working record, including the two bisects that were correct but pointed
at victims rather than the cause, and the claims that were withdrawn along the way.

**Verdict: `socket()` resolved `/dev/netsocket` by path on every call, so the single-threaded
`nfs-fs` — which owns `/` — sent that lookup to itself and deadlocked.** Fixed in
phoenix-rtos-kernel `7c021702` and libphoenix `47dde32`; gate `scripts/test-nfs-recovery.sh`.

---

## 3. ★★★ `premain-hang` SPLIT AT LAST — the child never reaches user mode

**The fault is KERNEL-SIDE. crt0 is exonerated.**

A one-character tick (`hal_consolePutch('~')`, kernel `21192f0f`) as the last statement before the
EL1→EL0 hand-off, paired with `LIBC_STARTUP_TRACE=min` as an **amplifier**:

| trials | ticks after the command echo | libc markers |
|---|---|---|
| 25 healthy | **1** (every one) | 1 |
| 1 silent | **0** | 0 |

⇒ the child never reaches `hal_jmp`, so the fault is in **exec / program loading or scheduling**, not
in crt0 and not in libc.

★★★ **Narrowed to the vfork child's FIRST USERSPACE INSTRUCTIONS.** Ten single-character ticks along
psh's `vfork` + `execve` path. Healthy baseline is exactly **1 of each**; five independent silent
launches all give the identical pattern:

| tick | where | healthy | silent |
|---|---|---|---|
| `!` | `process_vforkThread` entry | 1 | **1** |
| `&` | `posix_clone()` returned | 1 | **1** |
| `\|` | vfork handshake completed | 1 | **1** |
| `"` | `proc_resourcesCopy()` returned | 1 | **1** |
| `` ` `` | about to `proc_longjmp` into userspace | 1 | **1** |
| `$` | `proc_execve` entry | 1 | **0** |

⇒ the child completes **all** of `process_vforkThread` and resumes in userspace, then never issues
the `execve` syscall. Eliminated along the way: `posix_clone` taking the parent's posix lock (a
plausible AB deadlock), a lost wakeup on the handshake's infinite wait, `proc_resourcesCopy`, the map
switch, exec, the ELF load, the hand-off, crt0 and libc.

❌ **The vfork-child job-control theory is REFUTED — properly this time.** `psh_runfile()`'s vfork
child called `getpid()`, `setpgid()` and `tcsetpgrp()` (an ioctl, i.e. a blocking IPC) before
`execv()`, which is undefined in a vfork child and fitted the tick data exactly. Moved all of it into
the parent and re-ran **with the amplifier on**, so this arm had the power the earlier attempt lacked:

| psh | silent | trials | rate |
|---|---|---|---|
| stock (job control in the child) | 6 | 44 | 13.6% |
| job control moved to the parent | 3 | 23 | 13% |

**Fisher p = 1.000 — no effect whatsoever.** Experiment reverted; nothing shipped.
ⓘ This also settles the earlier `fork()` result I recorded as "refuted, p = 0.139": it was
underpowered, but the conclusion happens to have been right.

★★★ **ROOT CAUSE LOCALISED: the child hangs inside `open()`.** psh's vfork child reaches
libphoenix's `execve()`, which does six blocking things before the syscall. Ticked each boundary
(libphoenix `cb71255`, `2c58473`; healthy baseline exactly 1 of each):

| tick | after | healthy | silent |
|---|---|---|---|
| `\2` | `fflush(NULL)` | 1 | **1** |
| `\3` | the two `calloc(PATH_MAX)` | 1 | **1** |
| `\4` | about to call `shebang()` | 1 | **1** |
| `\a` | about to call `open()` | 1 | **1** |
| `\b` | **`open()` returned** | 1 | **0** |

⇒ the stall is **inside `open()`** — the filesystem IPC asking the fs server for the very binary the
child is about to exec. `read()`, `close()`, `resolve_path()` and `stat()` are never reached;
`fflush(NULL)` and the heap allocations are **cleared**.
ⓘ Eliminated over this bisect, in order: crt0, libc init, exec, the ELF load, the EL1→EL0 hand-off,
all of `process_vforkThread`, psh's job-control calls, and the rest of `execve`.
★★★★ **ROOT CAUSE REFRAMED: the NFS filesystem server is WEDGED. The launched process is an
innocent victim.** `nfs-fs` is **single-threaded** — one `msgRecv` loop — so a handler that blocks
stops every `open()` from every process. Added a one-character tick immediately after `msgRecv()`
returns (filesystems `8f269b0`, `-DNFS_MSG_TICK` via a new `FS_DIAG` hook):

| run | ticks after the command echo |
|---|---|
| healthy boot + launch | **~2074** |
| **silent launch (300 s)** | **0** |

The tick fires the instant `msgRecv` returns, so the server **never even received** the child's
`open()` — the message sits unclaimed on the port while the server is blocked somewhere else.
⇒ Everything this hunt eliminated in turn — crt0, libc init, exec, the ELF load, the hand-off,
`process_vforkThread`, psh's job control, the rest of `execve` — was eliminated correctly: **none of
them was ever the problem.** The process hangs because the filesystem it is being loaded from stopped
answering, on an IPC that has **no timeout**.
ⓘ Consistent with the earlier side probe (0 failures in 19 execs from tmpfs) and with the hang being
**size-independent** (`/bin/sh` stalls like an 18 MB game).
★★★★★ **ROOT CAUSE, EXACT STATEMENT: the server blocks inside `nfs_renew()` — the NFSv4 lease RPC.**
Using the on-demand reproducer (`scripts/repro-nfs-wedge.sh`, which restarts the host's nfsd), the
wedge report reads:

```
last ops:  L L L ... L ?        ('?' = the self-sent NFS_MSG_RENEW tick)
phase=20                        (set immediately BEFORE nfs_renew(fs->nfs))
```

Phase **21**, set immediately after that call returns, **never happens**. So the single-threaded
`nfs-fs` loop is stuck inside a synchronous libnfs RPC, and every filesystem request behind it waits
forever on an IPC with no timeout — which is the whole of `premain-hang`: a launch opens its own
binary and hangs before printing anything.

⇒ **The full chain:** host drops our NFSv4 state → the periodic lease renewal blocks forever →
`nfs-fs` stops servicing → every process that touches the filesystem hangs.
ⓘ The open-path phases were **stale** (`phase=2 rc=0` from the last successful open), which is what
showed the stall is not in the open path — even though the *spontaneous* failures end in `mtOpen`.
Whichever operation is in flight when the lease lapses is the one that hangs; the single-threaded
loop is the amplifier, not any one handler.
⚠ `nfs_set_timeout(nfs, 5000)` is set and still does not bound the call: `rpc->timeout` is a
**per-PDU** timeout, and after mount libnfs runs hard-mount semantics (`retrans=2`,
`auto_reconnect=-1`), so `rpc_timeout_scan()` **requeues** a timed-out PDU onto a fresh connection
instead of failing it. `wait_for_nfs_reply()` had no overall deadline (its RPC-level twin
`wait_for_reply()` does). Added one — ports `a44623a`, patch `04-sync-call-overall-deadline`: 4x
`rpc->timeout`, then `rpc_disconnect` (which completes every queued PDU, so the caller's stack
`cb_data` cannot outlive the frame) and `-ETIMEDOUT`; filesystems `427aabd` reclaims on that code as
well as on state expiry.

★★★★★ **That is necessary but NOT sufficient — the real blocking point is now pinned exactly.** With
the deadline built and running, the reproducer still wedges, and the new sync marks (a temporary
libnfs + `NFS_SYNC_MARK` instrument, watchdog re-reporting 8x) read the same value for **80 s**:

```
nfs-fs: WEDGE, last ops: LLLL...L      (x8, ~10 s apart)
nfs-fs:   open phase=2 iter=3 rc=0
nfs-fs:   sync mark=5 iter=4           (frozen — same mark, same iteration, every report)
```

Four successive mark refinements walked it in — `5` (pre-`nfs_service`) → `31`
(pre-`rpc_connect_sockaddr_async`) → `35`:

### ★★★★★ ROOT CAUSE: `socket()` deadlocks the process that owns `/`

`mark=35` is set immediately before `socket()` and `36` immediately after; `35` was frozen across
every report. The kernel's `posix/inet.c socksrvcall()` resolved `PATH_SOCKSRV` (`/dev/netsocket`)
with a **`proc_lookup` on every `socket()` call**, and a path lookup is answered by the filesystem
that owns `/`. `nfs-fs` **is** that filesystem and is **single-threaded**, so its loop thread sent
the lookup to the port it was itself supposed to be servicing. Nothing ever answers it.

libnfs opens a fresh socket as soon as it has to reconnect — which is exactly what a dropped server
forces — so the whole filesystem stopped, and every process behind it blocked on its first request.
A launch hung while opening its own binary: **that is `premain-hang`.**

**Fix (kernel `7c021702`): resolve `/dev/netsocket` once and cache it.** The socket server's port
does not change for the life of the system. Release/acquire ordering publishes the oid.
Verified: the reproducer no longer wedges — `nfs-fs: renew failed (-34), reclaiming client state`
now prints, i.e. the RPC **returns** instead of hanging.
The **same defect had a second door**: libphoenix's own `socksrvcall()` (`getaddrinfo`,
`getnameinfo`) also resolved `/dev/netsocket` per call, so the reclaim then stalled at `phase=4`
inside `nfs_mount()`. Cached there too — libphoenix `47dde32`, `sys/socket.c`.

### ✅ VERIFIED FIXED on a CLEAN build (zero diagnostic markers in the image)

`./scripts/repro-nfs-wedge.sh` — restart the host's nfsd under a running Pi:

```
38 Tests 0 Failures 0 Ignored            (unix-socket suite, after the restart)
nfs-fs: renew failed (-34), reclaiming client state
nfs-fs: reclaimed NFSv4 client state (re-mounted 10.42.0.1:/)
quakespasm: LOAD-TIME main->Host_Init = 2.239 s
```

No WEDGE report, no fault, and a game launches normally **immediately after** the server was pulled
out from under it. Before these commits the same script wedged 2/2 and nothing launched again.
Manifest: `manifests/2026-09-15-premain-hang-fixed.md`.

**Gate: `./scripts/test-nfs-recovery.sh`** (new) — same reproducer, but it grades the three lines and
exits non-zero, so this stays proven instead of eyeballed:
`noticed=1 reclaimed=1 launched=1 wedged=0 faults=0 → PASS`.
ⓘ Two wedges showed `phase=2` (open path), two `phase=20` (renew). Same blocking point, different
caller — consistent with "whichever op is in flight when the server drops us".
ⓘ **Scope / regression question:** `nfs-fs` exists only on the **netboot / nfsroot** variant. The
showcase videos and the gated demo image are **SD-boot** (ext2 root, no NFS), so this failure mode
cannot occur there, and every `premain-hang` observation to date came from a netboot cycle. **Regression question, precisely:** the
per-`socket()` lookup is **old** — it is not a regression from the recent kernel/libphoenix merges.
But two of our own earlier changes made it *reachable*, and then *periodic*: the NFS-root takeover
(filesystems `05f049a`, 2026-06-28) made `nfs-fs` own `/`, and the NFSv4 lease keepalive thread
(filesystems `8231627` + libnfs patch 01, 2026-08-03) put a synchronous RPC on the loop thread every
45 s. Before that, only an OPEN could trigger a reconnect, so the wedge was rare. So: **an old latent
defect, exposed by NFS-root and turned from rare into periodic on 2026-08-03.** To be confirmed with
an SD bench that `premain-hang` never occurs there.

## 3b. Rate and the withdrawn regression claim

**On the clean shipped build a cold app launch fails to start about 1 in 52** (plus 1 in 6 on one
small STK soak) — not the 1-in-6 I previously reported. Workaround unchanged: **relaunch it**.

⚠ **My "regression from the 2026-09-09 window, p = 0.022" is withdrawn.** It compared a rolled-back
**clean** tree against a current **instrumented** one. The diagnostic libc trace *raises* the failure
rate ~7× (1/52 clean vs 6/44 traced, p = 0.045) and the exec-entry printf *suppresses* it (0/33).
Controlled, rolled-back vs current clean is **0/27 vs 1/52, p = 1.00 — no difference.** Witold's
hypothesis is neither confirmed nor refuted; my evidence never tested it.
⏭ **Blocked on instrumentation, not on Pi time:** every probe tried so far changes the rate in one
direction or the other. A non-perturbing reproducer is the prerequisite for any further work.
ⓘ Established: the board stays **fully alive on the network** through the 300 s hang (156 consecutive
pings) ⇒ a localised process hang, not a console or scheduler wedge. `vfork` refuted as the cause.

