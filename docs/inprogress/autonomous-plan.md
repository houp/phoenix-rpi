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
| A1 | W0 | Upstream sync: pull all siblings, integrate, build, verify, push org | TODO | foundation for everything |
| G1 | W1 | Full code review (all repos): bugs/hacks/diagnostics/TODOs/comments/licensing → fix+test+commit | TODO | safe, no-visual; ties to prepub cleanup |
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

## Active task

**A1 — Upstream sync** (starting). Establish a clean, up-to-date base before other work.

## Last progress

2026-08-04: Plan created. vkQuake torch fix already landed+pushed (d3e329c). vkQuake
e1m1 bright-walls (I1): could not reproduce — fresh `map e1m1`, `start→e1m1`,
`r_rtshadows=1`, and weapon-fire all render lightmaps matching GLQuake (diff <0.2%)
once the build settles; owner reports it steady/persistent on the same netboot build.
Leading theory: GPU-compute lightmap update skips unmodified lightmaps and clears the
per-frame modified flag, so a disrupted initial build could leave surfaces stuck at
the bright default. Robustness fix candidate for I1.

## Next step

Kick off A1 (upstream sync) — survey upstream deltas across all siblings (read-only),
then integrate + build + push. Use `scripts/git-siblings.sh` and
`scripts/git-pull-upstream-all.sh`.
