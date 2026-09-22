---
name: rpi4-core-change
description: >-
  Take a change to a Phoenix core sibling (kernel, devices, libphoenix, libcache,
  filesystems, usb, plo, lwip) from edit to pushed-and-verified on the Pi 4. Use
  for any code change in sources/* — gives the syntax-check → build → strings-verify
  → A/B gate → commit → push → manifest loop, and the specific ways each of those
  steps has silently passed on the wrong thing here.
---

# Landing a core change on the Pi 4

The order is: **validate cheaply, build, prove the blob contains your change,
gate it, commit, push, manifest.** Each step below has a known failure mode where
it reports success without having tested anything.

## 1. Syntax-check before spending a build

```
./scripts/syntax-check.sh <sibling-repo> <path/in/repo.c>
# e.g. ./scripts/syntax-check.sh phoenix-rtos-filesystems ext2/block.c
```

Compiles ONE file under the real flags including `-Werror`, writes no artifact,
and is safe to run while a Pi cycle holds the bench.

⚠ **It uses the INSTALLED sysroot headers.** If your change adds a new API to a
library (e.g. a new `libcache` function), a *consumer* of that library will fail
here with `implicit declaration of function` even though the code is correct —
the real build installs the updated header first. That is a **false failure**:
confirm by building, don't "fix" it by reverting.

ⓘ Historic trap, now fixed: this script used to take `grep -m1` of `make -n`
output and so compiled the *first* prerequisite rather than the file you asked
about, reporting CLEAN for a file the real build rejected. It now matches by
basename and refuses a non-unique match.

## 2. Build

```
./scripts/rebuild-rpi4b-fast.sh --scope core        # after a COMMITTED core change
./scripts/rebuild-rpi4b-fast.sh --with-ports        # when a ports.yaml entry changed
```

⚠ **Never build while a Pi cycle is running.** The build ends in the image stage
and overwrites the TFTP `loader.disk` underneath the running measurement.

⚠ **Stale-core hazard:** the default `--scope auto` runs only `project image` when
the sibling repos are *clean*, reusing cached objects — so after you **commit** a
core change, an `auto` build can ship a blob without it. Use `--scope core`.

## 3. Prove the blob actually contains the change

Never grep the build log — the core build prints short-form `CC file.c` and
cannot distinguish. Grep the shipped artifact:

```
strings .buildroot/_boot/aarch64a72-generic-rpi4b/rpi4b-bootfs/loader.disk | grep -c "<a string from your change>"
```

This is also how you prove **removal** — e.g. that temporary instrumentation is
gone (expect `0`). A commit message citing "0 occurrences in the build log" was
amended once for exactly this reason.

### When the change has no greppable string

Numeric changes — a cap, a constant, a bound — leave nothing for `strings`. Grade
the **artifact mtimes** against the commit time instead: every object on the path
from the edited file to the shipped blob must be NEWER than the commit.

```
git -C sources/<repo> log -1 --format=%cd --date=iso <the/edited/file.c>
stat -c '%y  %n' .buildroot/_build/aarch64a72-generic-rpi4b/lib/<lib>.a                  .buildroot/_build/aarch64a72-generic-rpi4b/prog/<host program>                  .buildroot/_boot/aarch64a72-generic-rpi4b/rpi4b-bootfs/loader.disk
```

⚠ **Know which programs HOST your library** — a filesystem library is not a
program. libext2 is linked into **both** `prog/bcm2711-emmc` (SD root) and
`prog/usb` (which absorbs `libusbdrv-umass.a`; there is no `prog/umass`). Check
every host, not the one you happened to test on.

⚠ **`/etc/build-versions` does NOT prove this.** It is stamped at image time, so
an image-only rebuild prints the new SHA over old objects — the version stamp and
the binary are independent. Grade the binary.

## 3b. If the fix is NOT Pi-specific, record it for upstream

`docs/PHOENIX-RTOS-RPI4-CHANGES.md` is written for **Phoenix-RTOS maintainers**, and its
"★ Start here: fixes to Phoenix that are not Pi-specific" shortlist is the part they will actually
read. A defect in shared code — libphoenix, the kernel, corelibs, libext2, lwip, a subsystem any
target uses — belongs there **as part of finishing the fix**, not as a later cleanup.

Ask: *would this bite a target that is not a Pi 4?* If yes:

1. add a numbered row to the shortlist table — `# | defect | where (repo + SHA) | why it matters to you`;
2. add the root-cause narrative to the matching `### ★ General bug fixes` section, which is what
   the shortlist's "described with its root cause in the section that follows" promises;
3. say what makes it **general** — the mechanism, not the symptom. "Every storage driver reads
   `msg->oid.id` and none reads `i.raw`" travels; "fsync was broken on the Pi" does not.

Match the file's style: specific, evidence-first, with the measurement. It states its own numbers
honestly (including what it excludes from the diff stats), so do not inflate.

⚠ Also record fixes whose *mechanism* generalises even when the register does not, and negative
results where they save someone a search. The owner asked for this explicitly (2026-09-22): the
non-Pi-specific fixes are the ones upstream maintainers care about.

## 4. Gate it — and make the gate able to fail

- For a behaviour change, **A/B one variable**: same blob, one constant flipped.
  The cleanest form is a one-line toggle (a granularity constant, a `#define`
  gate), built twice. That is how `21.02x → 1.00x` was established; a comparison
  against a number measured a different way would not have been trustworthy.
- **Predict the magnitude before measuring.** If the mechanism says "one block
  should leak" and `df` says 4 KiB, you understand it. Twice on 2026-09-21 a
  plausible theory was fixed without that check and was wrong both times.
- Grade by **individually tagged output lines**, never by rc. UART interleaves.
  `grep -c` returning 0 exits 1 — a "failed" command can be the desired result.
- Storage changes: use the `rpi4-storage-test` skill's `e2fsck` oracle.
- Console/tty/boot-path changes: gate on boot **and psh-interactive** before
  anything else. If you break the boot, restoring it is priority #1.

## 5. Commit, push, manifest

- Small commits in the touched sibling, then a coordination-repo commit recording
  the state. Sibling commits: `git -C sources/<repo> add/commit` (never
  `cd <repo> && git …`, which the harness flags).
- State the **scope** of a fix in the message: which configurations it affects and
  why it was not seen before. ("Affects every ext2 with blocks > 1 KiB; never seen
  here because the SD rootfs is `mke2fs -b 1024`.")
- Say so plainly when a commit is knowingly incomplete, with the evidence. A
  partial fix that is documented is useful; one presented as complete is a trap
  for the next session.
- Push only once verified: `git -C sources/<repo> push publish master`, then
  `git push publish main`. **NEVER force-push.**
- `./scripts/snapshot-integration-state.sh <slug> --note "…"` when the integration
  state changed, then commit `manifests/`.

## 6. Remove your instrumentation

Temporary `fprintf`s are how most of the hard bugs here were caught — and they
must come out before the step closes, with a rebuild and a `strings` check proving
it. A print in an allocator or a write path is per-operation noise on the console.

Record the *finding* in `docs/misc/` and the weekly log, not in the code.
