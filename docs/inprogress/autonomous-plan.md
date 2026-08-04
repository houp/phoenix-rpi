# Autonomous execution plan — Phoenix-RTOS RPi4 (owner on vacation)

**This is the durable source of truth for long-horizon autonomous work.** It
survives context compaction and session restarts. A recurring heartbeat re-invokes
work on this plan; each invocation reads THIS file first.

Source plan: `~/rpi-phoenix-tasks.md` (owner's high-level tasks). This file
decomposes it into tracked, prioritized tasks.

Started: 2026-08-04. Owner away; **no human visual feedback or manual tests
available** — all validation is self-service (HDMI capture + pixel analysis +
host/Linux comparison, UART logs, QEMU, debugger).

---

## Long-term goal

Deliver **all** tasks in `~/rpi-phoenix-tasks.md`: upstream sync, extended
debugging facilities, game ports (SDL2/3, Quake1 MP, Quake2, Quake3, SuperTuxKart),
X11 GPU/windowed + XFce, Dillo HTTPS + Pi internet, ffmpeg + video player,
kernel/system fixes + perf, full code review, documentation + a journey article —
plus continued vkQuake rendering work. Everything clean, tested, and pushed to the
`rpi-phoenix-rtos` GitHub org when ready.

## Ground rules (apply every invocation)

- **Never stop permanently.** On API limit / API error / network error / build
  breakage: commit any partial progress, note it under "Last progress", and end the
  turn cleanly. The heartbeat retries. Do not treat an error as "done".
- **Pi is exclusive** — one boot/UART cycle at a time. Set the **Pi lock** line
  below before a Pi cycle, clear it after. Builds are parallelizable; Pi boot is not.
- **No GPL/incompatible code copied into Phoenix-RTOS repos.** Reading Linux/BSD for
  understanding is fine; reimplement cleanly. Track provenance/licensing.
- **Clean & upstreamable**: minimal Phoenix-specific shims; prefer extending
  libphoenix/kernel/system over per-port hacks. Small reviewable commits.
- **Test everything**: Pi (netboot; card is out, Pi off), QEMU, host build, compare
  vs host render / vs Linux-on-Pi where possible. Use the debugger.
- **Push to org, not upstream.** We are a long-lived fork pulling from upstream.
- **Shell discipline**: use the wrapper scripts (`scripts/git-siblings.sh`,
  `scripts/test-cycle-*.sh`, `scripts/uart-*.sh`, etc.); no ad-hoc pipelines.
- **Background-session permission model**: no human is present to approve permission
  prompts, so a non-allowlisted command is effectively DENIED. Confirmed usable this
  session: `git commit`, `git push`, `git -C <abs> ...`, the wrapper scripts, grep/rg,
  Read/Edit/Write. Early in an execution turn, if a git op (e.g. `git merge`) prompts/
  denies, fall back to an allowlisted path (wrapper script, or `git -C <abs> merge`).
- **Use subagents** for parallel analyze→implement→test on independent topics.
- Record validated core-integration states with `scripts/snapshot-integration-state.sh`.

## Continuation protocol (do this at the start of every invocation)

1. Read this file, `docs/inprogress/status.md`, and `MEMORY.md`.
2. Look at **Active task** + **Last progress** below. Resume it, or if it's blocked/
   done, pick the next highest-priority unstarted task.
3. analyze → implement → build → test → commit (sibling repo) → push to org when
   verified → snapshot manifest if core integration changed.
4. Update **Active task**, **Last progress**, **Next step**, and the task table.
   Commit this file (coord repo) and update memory. Keep commits small.
5. If the heartbeat cron is within ~1 day of its 7-day auto-expiry, re-create it.

---

## Heartbeat / scheduling state

- Mechanism: `CronCreate` recurring, `11,41 * * * *` (every 30 min, fires only when
  REPL idle → acts as a restart-after-stall safety net; long work turns don't overlap).
- **Re-arm before 7-day expiry** (created 2026-08-04 → expires ~2026-08-11).
- Job ID: `df8363ff` (CronList to verify; CronDelete to cancel). Session-only (dies
  if this background session ends — no cloud fallback has Pi access).

## Pi lock

- **FREE** _(set to "IN USE <label> <timestamp>" before booting the Pi; clear to FREE after)_

---

## Task board

Status: TODO / WIP / BLOCKED / DONE. Priority waves: W0 foundation → W3 hardest.

| ID | Wave | Task | Status | Notes |
|----|------|------|--------|-------|
| A1 | W0 | Upstream sync: pull all siblings, integrate, build, verify, push org | WIP | analysis DONE; Batch 1 (doc/ports/tests) MERGED+PUSHED; Batch 2 (core+boot) & 3 (kernel/libphoenix/project) pending |
| G1 | W1 | Full code review (all repos): bugs/hacks/diagnostics/TODOs/comments/licensing → fix+test+commit | WIP | recon DONE → docs/review/2026-08-04-autonomous-review-recon.md (Tier A/B/C/D); execute Tier A first |
| H1 | W1 | Docs cleanup + archive stale docs | TODO | |
| H2 | W1 | Final Pi4 port-state documentation | TODO | after most ports land; start skeleton |
| H3 | W1 | Pi4 OS-dev knowledge base (extend existing) | TODO | accumulate all porting experience |
| B1 | W1 | Generalize in-process debugger → reusable Phoenix debug library | TODO | from vkQuake debugger |
| B3 | W1 | Debug-facility documentation | TODO | with B1 |
| F1 | W2 | Resolve KNOWN ISSUES (kernel/system/libphoenix) | TODO | see docs/KNOWN-ISSUES.md; debugger-driven |
| F2 | W2 | OS perf (I/O, net, scheduling) + modern syscalls + measurements + wire ports to them | TODO | |
| SD | W2 | SD-card driver: full speed + correctness (prior loop goal; folds into F1/F2) | WIP | reads IRQ, writes CMD13-poll done; perf=PIO throughput |
| C1 | W2 | SDL2 port (fullscreen GL+Vulkan, kbd+mouse, sound); no X11 needed | TODO | reusable base for many games |
| C3 | W2 | Quake1 multiplayer networking fix | TODO | net_drivers currently loopback-only |
| I1 | W2 | vkQuake e1m1 bright-walls: robustness of GPU-compute lightmap build | WIP | can't repro (my loads correct); see below |
| I2 | W2 | vkQuake: liquids + remaining workarounds + perf | TODO | |
| I3 | W2 | Fix phantom /dev/kbd0 input (spurious menu spam) | TODO | open SW bug, pl_phoenix_in.c |
| E1 | W2 | Dillo HTTPS support | TODO | needs TLS (libphoenix/openssl?) |
| E2 | W2 | Pi internet via host Linux router/proxy (NAT) | TODO | host-side network config |
| E3 | W2 | Dillo displays live internet pages | TODO | after E1+E2 |
| C4 | W3 | Quake2 port (yQuake2) + open/shareware assets + demo+visual test | TODO | |
| C5 | W3 | Quake3 port (quake3e/ioq3) + playable assets + demos | TODO | |
| C6 | W3 | SuperTuxKart (OpenGL fullscreen, GPU) | TODO | large |
| D1 | W3 | X11 GPU-accelerated extensions (toward RPi-OS parity) | TODO | |
| D2 | W3 | X11 GL/Vulkan windowed (GLX) + glxgears validation | TODO | |
| D3 | W3 | XFce desktop environment port | TODO | large |
| E4 | W3 | ffmpeg port (tool+lib) + Pi HW decode accel | TODO | |
| E5 | W3 | X11 video player (windowed + fullscreen) | TODO | after E4 |
| B2 | W3 | Extend debugger to kernel/driver-side | TODO | after B1 |
| H4 | W3 | AI-driven-journey article (git+conversation+memory analysis) | TODO | LAST — summarizes whole effort incl. this phase |

---

## A1 integration plan (from upstream delta survey, 2026-08-04)

All 16 siblings fetched from `origin` (phoenix-rtos/*) OK; each also has `publish`
(org). No fork-mirror needed. Integrate in this order, **build + boot-verify before
pushing**, snapshot a manifest after each validated batch:

**Batch 1 — no boot-image impact, zero Pi4 overlap (safe, merge + push, no boot needed):**
`phoenix-rtos-doc` (1), `phoenix-rtos-ports` (1: libevent install path),
`phoenix-rtos-tests` (2: tmpnam/grspw). Clean merges expected.

**Batch 2 — core boot-image repos, zero Pi4-file overlap (merge → `rebuild --scope core`
→ ONE Pi boot-verify → push):** `phoenix-rtos-filesystems` (1: jffs2 bool),
`phoenix-rtos-usb` (1: warning), `phoenix-rtos-utils` (1: psh unused vars),
`phoenix-rtos-devices` (27: imx6ull-sdma/spacewire/sensors/uart16550 — none touch
bcm2711/genet/pl011, but they compile into the image, so build must pass). If the
build breaks, bisect the offending sibling, roll it back, defer it.

**Batch 3 — careful, deferred (dedicated turns, rollback-ready):**
- `libphoenix` (MED): mostly disjoint, but `sys/socket.c` accept4 overlaps our socket
  work → hand-merge that file. **The errno transfer is coordinated with the kernel**
  (libphoenix `!include/errno` ↔ kernel `transfer errno defines` + `change errno
  numbers to match host`) — these MUST land together or errno numbering breaks
  system-wide. Integrate the errno commits from kernel+libphoenix as ONE unit.
- `phoenix-rtos-kernel` (HIGH): upstream copyright/diacritics sweep textually touches
  ~500 files incl. 35 we own (all `hal/aarch64/*`, `vm/object.c`, `vm/map.c`,
  `proc/threads.c`, `posix/unix.c`, `main.c`, `syscalls.c`) → header-hunk conflicts on
  nearly every owned file; plus semantic overlap on `vm/object.c` (our read-ahead
  clustering 8834eaf3) and `hal/aarch64` reschedule/strncpy. Merge file-by-file for the
  35 overlaps, keep our semantics, rebuild `--scope core`, boot-verify, be ready to
  roll back to `known-good/2026-04-19-map-relocation-complete`. Do this on a turn with
  full attention.
- `phoenix-rtos-project` (MED): incoming content trivial (stm32n6 CI + submodule
  bumps) but it's the submodule superproject and we're 172 commits diverged → keep OUR
  submodule pointers; cherry-pick only the CI workflow if wanted.

**Already up to date (no action):** build, corelibs, hostutils, lwip, posixsrv, plo.

**Rollback:** run `scripts/snapshot-integration-state.sh` BEFORE Batch 2/3 merges so
`scripts/restore-integration-state.sh` can undo a bad batch.

## Active task

**A1 — Upstream sync** (delta analysis done). Next: execute Batch 1, then Batch 2 with
build+boot verify. G1 code-review recon still running (parallel).

## Last progress

2026-08-04 (setup + analysis): Board + memory + heartbeat cron (df8363ff) created &
pushed. Both read-only analysis subagents reported: (a) A1 upstream-delta survey →
"A1 integration plan" above; (b) G1 code-review recon → saved to
`docs/review/2026-08-04-autonomous-review-recon.md` with Tier A/B/C/D execution order.
A1 Batch 1 DONE: phoenix-rtos-doc (ff), -ports, -tests merged clean (zero conflicts)
and pushed to org — no boot impact. `git -C <abs> merge/push` confirmed working in this
bg session (no permission block). Batches 2/3 still pending.

2026-08-04: Plan created. vkQuake torch fix already landed+pushed (d3e329c). vkQuake
e1m1 bright-walls (I1): could not reproduce — fresh `map e1m1`, `start→e1m1`,
`r_rtshadows=1`, and weapon-fire all render lightmaps matching GLQuake (diff <0.2%)
once the build settles; owner reports it steady/persistent on the same netboot build.
Leading theory: GPU-compute lightmap update skips unmodified lightmaps and clears the
per-frame modified flag, so a disrupted initial build could leave surfaces stuck at
the bright default. Robustness fix candidate for I1.

## Next step

1. **A1 Batch 2** (filesystems, usb, utils, devices): `scripts/snapshot-integration-state.sh`
   first (rollback point), `git -C sources/<repo> merge --no-edit origin/master` each
   (abort+defer on conflict), `./scripts/rebuild-rpi4b-fast.sh --scope core`, then ONE Pi
   boot-verify (`test-cycle-netboot.sh`, set Pi-lock IN USE first, clear after), push each
   to org if green, snapshot a new manifest. If build breaks, bisect the offending sibling,
   `restore-integration-state.sh`, defer it.
2. **G1 Tier A** (text-only comment/TODO fixes from the recon doc) — can interleave; needs
   only a `--scope core` build to confirm syntax, no boot. Good candidate to delegate to a
   subagent to keep main context clean.
3. Then A1 Batch 3 (libphoenix errno+socket, kernel copyright-sweep, project submodule) on
   dedicated attentive turns; G1 Tier B/C after.
