# Upstream sweep 11 — census only, adoption DEFERRED

**Status: CENSUS DONE, NOTHING MERGED.** Deferred by the owner on 2026-09-21
while the ext2/USB correctness work is in flight. No branch was created, no
master was moved. Re-run `./scripts/upstream-status.sh` before adopting — these
counts will have moved.

## Tracking-ref check

All 17 siblings have an upstream tracking ref. **No `?` / "NO UPSTREAM" row**, so
nothing was silently skipped this time (the trap that hid `phoenix-rtos-usb` from
ten consecutive sweeps).

## What is behind — 14 commits across 4 repos

| repo | behind | what |
|---|---|---|
| `libphoenix` | 5 | `__INLINE`/`__EXPORT_INLINE`, drop `_ansi.h`, `ioctl_setResponse` size, **`termios: fix tcsetattr`**, `fbcon.h` FBCONGETMODE |
| `phoenix-rtos-devices` | 5 | **libtty race/locking fixes + `uart: refactor to match new libtty API`** |
| `phoenix-rtos-posixsrv` | 1 | `pty: adapt to driver interface change` |
| `phoenix-rtos-project` | 3 | submodule pointer + CI |

## Conflict census (`git merge-tree --write-tree`, read-only)

Three real file conflicts. Two are trivial; one is not.

| repo | rc | conflicts |
|---|---|---|
| `libphoenix` | 1 | `include/complex.h` (modify/delete), `include/sys/wait.h` (content) |
| `phoenix-rtos-devices` | 1 | `tty/libtty/libtty.c` (content) |
| `phoenix-rtos-posixsrv` | **0** | none |
| `phoenix-rtos-project` | 1 | submodule pointer for `phoenix-rtos-posixsrv` only |

### libphoenix — both from one upstream commit, both easy

`1b8d987 add __INLINE and __EXPORT_INLINE` is the cause of both.

* **`include/sys/wait.h` — take UPSTREAM, drop our local hack.** This is a
  *convergent* change: our `3a74c04 use __inline__ so the header builds under
  -ansi` is the same fix done worse. Upstream replaces `static inline` with
  `__INLINE` from `<sys/cdefs.h>`, which is correct for both c89 and c99 extern
  semantics. Our commit becomes redundant and should be dropped in the merge.
* **`include/complex.h` — keep DELETED.** We removed it deliberately in
  `d0a2884 libm: Move libphoenix math library implementation`; the header now
  lives at `libm/libmcs/libm/include/complex.h`. Upstream only retitled its
  `static inline`s to `__INLINE`, which is a linkage nicety, not a fix we need at
  the old path.

### phoenix-rtos-devices — this is the one with teeth

`libtty` gains real locking (`!libtty: fix races`, `synchronize libtty_ioctl data
access`, `add locking to libtty_{poll_status,ioctl,close}`) **and changes its
API**; upstream adapted every tty driver it ships in the same series — `pc-tty`,
`spike-tty`, `uart16550`, `zynq-uart`, `cmsdk-apbuart`, `grlib-uart`,
`imx6ull-uart`, and the four `multi/*` drivers. 733 insertions, 626 deletions
across 23 files.

⚠ **`tty/pl011-tty/` receives ZERO upstream changes, because it is ours — and it
is the Pi 4 console.** So adopting this series means porting our own console
driver to the new libtty API by hand, with no upstream reference for it, and the
failure mode is "the Pi has no console". Our one local `libtty.c` change worth
preserving is `b247643 tty/libtty: implement FIONREAD (was -EINVAL)`.

That is the whole reason this sweep is deferred rather than adopted: it is not a
big merge, it is a small merge plus an unreviewed console-driver port, and it
wants its own turn with a boot gate, not a slot between two data-corruption
fixes.

## When it is picked up

1. Branch per repo; master untouched until gates pass.
2. Order: `libphoenix` → `phoenix-rtos-devices` (+ pl011-tty port) →
   `phoenix-rtos-posixsrv` → `phoenix-rtos-project` submodule pointer.
3. No syscall arity/removal in this batch, so the stale-binary census does not
   apply; `--scope core` suffices, since libtty is a static lib and all its
   consumers are core.
4. Gate: boot + **psh interactive** first (console is the risk), then the
   six-app showcase gate. `termios: fix tcsetattr` also lands on that path.
5. Manifest + resolutions appended here.
