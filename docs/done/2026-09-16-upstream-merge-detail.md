# 2026-09-16 — the upstream sweep that broke, and unbroke, the tree

Archived from the weekly log once the work closed. Analysis of the ABI break itself lives in
[`docs/misc/2026-09-16-upstream-mutex-abi-break.md`](../misc/2026-09-16-upstream-mutex-abi-break.md).

- **2026-09-16 sweep (9th) — 17 upstream commits merged, and the tree is GREEN again.** All 16 siblings
  **0 behind**. Getting there took finding two real regressions, both now fixed:
  - 🐞 **A syscall ABI break.** `phMutexLock`/`phCondWait` went from **one** stack argument to **three**.
    Any binary built against the old libphoenix passes only the handle, so the kernel reads
    uninitialised stack as the clock id and **`pthread_mutex_lock()` returns EINVAL on a good mutex**
    (`test-libc-pthread`: 7 of 28 failures). ⚠ The deciding value is garbage, so it fails
    **nondeterministically per binary** — QuakeSpasm and vkQuake "passed" while stale, which was luck,
    not health. **Fix: rebuild every userspace binary**; all five showcase ports now are. Analysis:
    [`docs/misc/2026-09-16-upstream-mutex-abi-break.md`](../misc/2026-09-16-upstream-mutex-abi-break.md).
    ⓘ This breaks the append-only syscall property TD-21 exists to protect. Per your fork-only policy
    I am **not** raising it upstream; the cost lands on us as "rebuild everything after a merge that
    touches a syscall's arity", which is now written down so the next sweep expects it.
  - 🐞 **Default cond clock → `CLOCK_REALTIME`** (POSIX-correct, unusable here). Phoenix's wall clock
    starts at 1970 and **jumps** when time is set mid-boot, so absolute REALTIME deadlines are
    meaningless across the jump; vkQuake hung on LOADING. Reverted locally, libphoenix `c283f2d`, with
    the two conditions that would let us drop the revert recorded in the commit.
  - ✅ **Confirmed at the PRIMITIVE, not just via the games: 898 libc tests across 11 suites, 0
    failures, 0 faults.** `test-libc-pthread` **28/0** — it was **7 failures** (six `Was 22`) on the
    stale build — plus the merge's blast radius (time, signal, semaphore, poll, pthread-tsd: it also
    changed `__timespecToUs` rounding and added `clock{lock,wait}` across mutex/cond/rwlock) and the
    core suites (string 208, misc 212, printf 118, stdlib 93, math 90, stdio 82).
    The app gate alone could not have proven this: apps can pass on **luck** when the deciding value is
    uninitialised stack. ⓘ The test binaries were themselves stale and are now rebuilt, so the next
    person running the suite gets a real answer instead of 7 phantom failures.
  - ✅ **Re-gated `w38-abifix`: 6/6 mechanical · 6/6 content · 0 faults · torches PRESENT (20/20).**
    STK 100.0% non-black, vkQuake 54.2% — both back to healthy numbers. Manifest
    `manifests/2026-09-16-w38-abifix-gate.md`. All siblings pushed.
  - ⏸ **Your call:** the merged tree is now gated and could produce an image. `da752ac1` (pre-merge,
    gated 12/12) remains the recommendation until you want otherwise — say the word and I cut one.
