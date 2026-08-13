# GNU coreutils port to Phoenix/RPi4 (owner "full bash/zsh + coreutils beyond busybox")

Scouted 2026-08-13. coreutils 9.5 (https://ftp.gnu.org/gnu/coreutils/coreutils-9.5.tar.xz,
sha256 cd328edeac92f6a665de9f323c93b712af1858bc2e0d88f3f7100469470a1b8a, 6007136 bytes).
**Status: DEDICATED MULTI-CYCLE PROJECT — banked with this precise resume note.** Unlike bash (a short flat list of
libphoenix gaps), coreutils drags in gnulib, whose modules substitute their own impls and each pull the next Phoenix
gap — a dependency *tree*. Advisor-endorsed to timebox + bank once the walls turned to whack-a-mole.

## DONE
- **configure now PASSES** (exit 0). The one fatal configure wall was gnulib mountlist: *"could not determine how to
  read list of mounted file systems"* — Phoenix's `<mntent.h>` was an empty file (header present → configure "yes",
  functions absent → "no"). **Fixed: libphoenix 29f5373** implements the getmntent family (`mntent/mntent.c` +
  populated `include/mntent.h`). configure then reports `getmntent... yes`, `one-argument getmntent... yes`.
  - PENDING for 29f5373: `--scope core` + Pi boot-verify (additive, 0 regression risk — no in-tree consumer yet) +
    org push. Batch with the first real coreutils build cycle.
- Cross-configure invocation (drop `--enable-static`, coreutils warns it's unrecognized; static comes from the
  toolchain/LDFLAGS): `./configure --host=aarch64-phoenix CC=aarch64-phoenix-gcc --disable-nls`.

## BUILD WALLS (make -k → 325 errors, clustered) — the real work, in priority order
1. **[122 errors] `gettime`/`settime` namespace collision (HIGHEST LEVERAGE).** Phoenix `sys/time.h:34,37` declares
   NON-STANDARD `int gettime(time_t *raw, time_t *offs)` + `int settime(time_t offs)` (Phoenix's native time API).
   gnulib `lib/timespec.h:93,94` declares `void gettime(struct timespec *)` + `int settime(struct timespec const *)`.
   Every TU including both → conflicting-types. **Fix options:** (a) port-local gnulib patch renaming its gettime/
   settime → gl_gettime/gl_settime (contained, but touches many gnulib files); (b) the RIGHT long-term fix — stop
   Phoenix `sys/time.h` from exposing bare `gettime`/`settime` in the default namespace (guard behind a
   `_PHOENIX_SOURCE`-style feature macro). (b) is a broad libphoenix change with existing in-tree users (drivers/
   kernel-adjacent) — do it deliberately, grep all callers first, NOT rushed. Prefer (a) for the port initially.
2. **[39 errors] `assure.h`: implicit `assert`.** gnulib assure.h expects `<assert.h>` in scope. Check Phoenix
   `<assert.h>` (exists? NDEBUG behavior?) — likely a one-line include or a config.cache/gnulib fix.
3. **[~15 errors] `struct statfs` / `statfs()` undefined (stat.c).** Phoenix lacks `<sys/statfs.h>`/`<sys/vfs.h>` +
   `statfs()`. Feeds `stat -f` and df. Either add a minimal `struct statfs` + `statfs()` stub to libphoenix, or
   exclude `stat`/`df` from the built subset (see below).
4. **Singles:** `getprogname` "module not ported to this OS" (#error — add getprogname/program_invocation_name to
   libphoenix, or gnulib port stub); `pthread_sigmask` implicit (pselect.c — Phoenix pthread gap); `lchown` implicit
   (add to libphoenix or gnulib substitutes); `struct rlimit` redefinition (sort.c — Phoenix sys/resource.h vs
   gnulib); `stty.c` expected-expression (termios macro gap); `mini-gmp.c` assert (same as #2).

## STRATEGY (when this project is picked up)
- Scope to a value SUBSET (advisor): ls, cat, cp, mv, rm, mkdir, echo, printf, wc, sort, head, tail, cut, tr, uniq,
  seq, true, false, env, basename, dirname, tee, touch, ln, pwd, sleep, yes, comm, join, paste, nl, tac, rev.
  EXCLUDE the exotic/OS-heavy ones whose gnulib deps aren't worth chasing: df, stat (-f), who, pinky, users, uptime,
  chcon, runcon, stty (maybe), df. coreutils build order still compiles gnulib lib/ wholesale, so walls #1/#2 must be
  cleared regardless; only #3/#4 are subset-avoidable.
- Verify built utils non-interactively through psh-interact exactly like `bash /t.sh` (stage into the NFS root, run
  `/bin/ls -la /`, `/bin/wc /etc/...`, etc., grade on clean output + 0 faults).
- Formalize as `sources/phoenix-rtos-ports/coreutils/port.def.sh` (autoconf template, like the bash port) + patches
  (the gnulib gettime rename, getprogname) + a config.cache once the run-tests settle.

## NEXT ACTION when resumed
Clear wall #1 first (it blocks everything; 122 errors). Try the port-local gnulib timespec rename patch; if that's
clean, rebuild and reassess from wall #2. Then decide subset vs full from the remaining wall count.
