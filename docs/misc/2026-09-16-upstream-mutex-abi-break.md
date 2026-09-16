# The 2026-09-16 upstream merge is a syscall ABI break

*Root cause for the `w38-upstream` gate failures (SuperTuxKart aborts, vkQuake hung on LOADING).*

## What changed

Kernel `2350f170` / `b0d34fff` (`!proc: allow to pass clock id to phMutexLock` / `phCondWait`) change
how the syscall reads its arguments:

```c
-	return proc_mutexLock(h);
+	GETFROMSTACK(ustack, time_t, timeout, 1U);
+	GETFROMSTACK(ustack, int, clock, 2U);
+	return proc_mutexLock(h, timeout, clock);
```

`phMutexLock` now takes **three** stack arguments where it took one. libphoenix `f57a03d` updates the
caller to match (`phMutexLock(m, 0, PH_CLOCK_MONOTONIC)`).

## Why it breaks userspace

**Every binary linked against the OLD libphoenix passes only the handle.** The kernel then reads two
words of whatever happens to be on that thread's stack as `timeout` and `clock`. If the garbage lands
on a valid clock id the call works; otherwise `pthread_clock_id_to_phx_clock()` rejects it and
**`pthread_mutex_lock()` returns EINVAL** — on a plain, correctly-initialised mutex.

That is what the apps show. `test-libc-pthread`: **28 tests, 7 failures**, six of them
`err1 = pthread_mutex_lock(...)` returning **22 (EINVAL)**. SuperTuxKart: `std::system_error: Invalid
argument` thrown at the first `std::mutex`, immediately after DATADIR, before any rendering.

## ⚠ The failure is NONDETERMINISTIC per binary — do not bisect by app

Measured on one build:

| binary | rebuilt since the merge | result |
|---|---|---|
| quakespasm | no | passes |
| vkquake | no | passes |
| `test-libc-pthread` | no | **fails** |
| stk | yes | **fails** |

Because the deciding value is uninitialised stack content, "which apps broke" tells you nothing about
which change is at fault, and a passing app is **not** evidence that a binary is ABI-clean. Grade this
by `test-libc-pthread`, which exercises the primitive directly.

## Remedy

The merge requires **every userspace binary to be rebuilt**, not just core. `--scope core` rebuilds the
kernel, libphoenix and the drivers and leaves the prebuilt ports staged on the NFS export, which is how
a half-updated system arises. Either:

1. rebuild all ports against the new libphoenix (`--scope full-clean`, hours), or
2. revert the two `!` kernel commits plus libphoenix `f57a03d` and keep the rest of the merge.

★ Related standing rule: `feedback_owner_td21_syscall_upstream_order` — mutex syscalls were deliberately
kept append-only so prebuilt netboot binaries keep working. This merge breaks that property by changing
an existing syscall's argument count rather than adding a new one.

ⓘ Separately and genuinely: libphoenix `4ab9ad9` (default cond clock → `CLOCK_REALTIME`) is a *second*
real regression on this board, fixed by the local revert `c283f2d` — vkQuake went from hung-on-LOADING
to fully playable with no other change. Phoenix's wall clock starts at 1970 and jumps when time is set
mid-boot, so absolute REALTIME deadlines are meaningless across the jump.
