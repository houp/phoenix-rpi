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
| A1 | W0 | Upstream sync: pull all siblings, integrate, build, verify, push org | WIP | analysis DONE; Batch 1+2 MERGED+BUILT+BOOT-VERIFIED+PUSHED (manifest 2026-08-04-a1-batch2-done); only Batch 3 (kernel/libphoenix/project — careful) remains |
| G1 | W1 | Full code review (all repos): bugs/hacks/diagnostics/TODOs/comments/licensing → fix+test+commit | WIP | recon → docs/review/2026-08-04-autonomous-review-recon.md. Tier A (comment/TODO) DONE. Tier C tools/ headers DONE (6 files +%LICENSE%; fbdev_stub KEPT — still used by build-xfbdev.sh --stub). Pending: Tier B (diag removal, needs boot); Tier C _memset.S provenance (kernel→after Batch 3) + %LICENSE% tooling verify; kernel/libphoenix/project Tier A after A1 Batch 3 |
| H1 | W1 | Docs cleanup + archive stale docs | TODO | |
| H2 | W1 | Final Pi4 port-state documentation | TODO | after most ports land; start skeleton |
| H3 | W1 | Pi4 OS-dev knowledge base (extend existing) | WIP | base = docs/knowledge/rpi4-os-development-guide.md. Added V3D GPU section (OpenGL+Vulkan) + Display(fb0/HDMI)&audio(PWM) section (mailbox fb alloc, tall-virtual-fb flip, /dev/audio0 PWM, userspace-MMIO-driver pattern). Still to add: storage+NFS-root, in-process debug facility, the ported-app stack (SDL2/X11/Quake) |
| B1 | W1 | Generalize in-process debugger → reusable Phoenix debug library | TODO | from vkQuake debugger |
| B3 | W1 | Debug-facility documentation | TODO | with B1 |
| F1 | W2 | Resolve KNOWN ISSUES (kernel/system/libphoenix) | TODO | see docs/KNOWN-ISSUES.md; debugger-driven |
| F2 | W2 | OS perf (I/O, net, scheduling) + modern syscalls + measurements + wire ports to them | TODO | |
| SD | W2 | SD-card driver: full speed + correctness (prior loop goal; folds into F1/F2) | WIP | reads IRQ, writes CMD13-poll done; perf=PIO throughput |
| C1 | W2 | SDL2 port (fullscreen GL+Vulkan, kbd+mouse, sound); no X11 needed | PHASE-1 DONE | feasibility → docs/inprogress/2026-08-04-sdl2-port-plan.md. Phase-1 build plumbing DONE: ports/sdl2 (SDL 2.30.12, 4 patches: PHOENIX cmake branch + pthread + dynapi-off + sched-noop), libSDL2.a cross-builds+links (stock pthread backend), pushed org bdfe294. Phase-1 video+input driver DONE (patch 0005 + overlay/src/video/phoenix/ {video,opengl,events,framebuffer} + glue/{glctx GPL-copy,glstubs zlib}); libSDL2.a builds w/ SDL_VIDEO_DRIVER_PHOENIX + fullscreen-GL test LINKS (org 8671269). GPL-glue seam kept OUT of zlib libSDL2.a. **Pi GL-demo HW-VALIDATED (2026-08-04)**: sdl2-gltest = GL 2.1/V3D 4.2, 600 frames clean exit, 0 faults, 1920x1080 triple-buffer page-flip, fullscreen GL clear-color on HDMI. Audio driver DONE + HW-VALIDATED (2026-08-05): src/audio/phoenix/ (patch 0006, pull model /dev/audio0), sdl2-audiotest on Pi → "audio open: driver=phoenix 44100/S16/2ch", tone played, clean exit, 0 faults. org ports c191d20; project ports.yaml f82c334 (sdl2 registered `if:false` — no consumer yet). **SDL2 phase 1 COMPLETE**: fullscreen GL + input + audio all HW-validated. NEXT: **C4 Quake2 (yQuake2)** on SDL2. Vulkan=phase 2 (no V3DV WSI). See [[project_sdl2_port]] |
| C3 | W2 | Quake1 multiplayer networking fix | TODO | NOT loopback-only: quakespasm has UDP landriver + Datagram wired + FIONREAD→MSG_PEEK fix. Real bug = KNOWN-ISSUES **#68 MP hangs at LOADING screen** (open). vkQuake stub still loopback-only. Fix = diagnose #68 (Pi client ↔ host dedicated server) + bring vkQuake net to parity. Needs dedicated Pi turn |
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

**C4 — Quake2 (yQuake2)** feasibility, on the HW-validated SDL2 base. A background subagent is
RUNNING the analysis: source choice (yQuake2 renderers — our Mesa V3D is GL **2.1**, so ref_gl1
fits, ref_gl3 needs GL3.2 core = NO; ref_soft fallback), build against libSDL2.a + the GL glue
(reuse sdl2-gltest link recipe), open/shareware pak0 assets, libphoenix/system gaps, and a
phased port plan. **Do NOT launch duplicate Quake2 work.** A concurrent heartbeat may advance an
independent item (vkQuake, docs, C3). SDL2 (C1) phase 1 COMPLETE (GL+input+audio HW-validated).

Note: coord working tree carries PRE-EXISTING uncommitted vkQuake/v3d WIP (v3dv_harness.c,
vkquake_shaders.c, triangle_spirv*, drm*.h, texprobe/, two 2026-07-2x analysis docs) from
before the vacation handoff — NOT ours; leave untouched (always `git add <path>`, never -A).

## Last progress

2026-08-04 (setup + analysis): Board + memory + heartbeat cron (df8363ff) created &
pushed. Both read-only analysis subagents reported: (a) A1 upstream-delta survey →
"A1 integration plan" above; (b) G1 code-review recon → saved to
`docs/review/2026-08-04-autonomous-review-recon.md` with Tier A/B/C/D execution order.
A1 Batch 1 DONE: phoenix-rtos-doc (ff), -ports, -tests merged clean and pushed.
`git -C <abs> merge/push` confirmed working in this bg session (no permission block).

A1 Batch 2 DONE (2026-08-04): snapshot pre-a1-batch2 → merged filesystems/usb/utils/
devices (all 0 conflicts; devices +6567 lines of imx6ull/spacewire/sensors/uart16550,
none Pi4) → `rebuild --scope core` OK (image verify OK) → netboot boot-verify HEALTHY
(psh prompt + lwip + genet link/IP + xHCI + fbcon + NFS-root mount, 0 faults) → pushed
all 4 to org → manifest 2026-08-04-a1-batch2-done. Pi powered off, lock FREE.

G1 Tier A DONE (2026-08-04, via synchronous subagent): text-only comment/TODO cleanups
in usb (usb.c refuted-silicon-story + runStateSelftest ref, mem.c diag-udp ref, hub.c 5×
#129), lwip (genet-rxcache-bench dc ivac→civac), devices (xhci #129, pcie warm-up TODOs,
sdstorage/sdcard/pl011-tty stale TODOs incl. TD-14-pl011-retry), plo (_init.S/hal.c/
video.c stale-history comments). `--scope core` build PASS; committed + pushed per repo.
**lwip caveat**: its org publish tip is a git-filter-repo-scrubbed lineage (WiFi blobs
stripped) — the fix was cherry-picked onto publish/master's tip via a worktree, NOT
force-pushed. See [[project_git_topology]]. Kernel/libphoenix/project Tier A comment
fixes intentionally NOT done yet (would worsen the A1 Batch 3 merges).

SDL2 AUDIO driver DONE + HW-VALIDATED (2026-08-05): subagent added src/audio/phoenix/
(SDL_phoenixaudio.c/.h, patch 0006, pull model over /dev/audio0 44100/stereo/S16LE) + a
tone-test; libSDL2.a builds w/ SDL_AUDIO_DRIVER_PHOENIX; sdl2-audiotest on Pi → "audio open:
driver=phoenix freq=44100 format=0x8010 channels=2", "smoke test done", 0 faults (audible
sign-off deferred — no mic unattended). Pushed org: ports c191d20, project ports.yaml f82c334
(sdl2 registered `if:false` — a plain listing builds unconditionally which would risk unrelated
image builds, so gated until a consumer game lands; built via scripts/build-sdl2-port.sh),
coord test helpers 73d2158. => **SDL2 phase 1 COMPLETE** (GL+input+audio HW-validated).

SDL2 video+GL+input driver HW-VALIDATED (2026-08-04): rebuilt+deployed sdl2-gltest to NFS
root, netboot Pi cycle → UART: GL_VERSION 2.1 Mesa 26.2.0-rc1, GL_RENDERER V3D 4.2.14.0,
"600 frames, clean exit", 0 faults, qsv3d scanout FBO 1920x1080 n=3 (direct-render+page-
flip). HDMI: clean fullscreen GL clear-color fill (animating, triple-buffered). => the
phoenix SDL2 video/GL path WORKS on hardware. Pi off, lock FREE. (Input drain exercised 600
frames w/o fault; keypress→event unverifiable unattended.)

SDL2 video+input driver DONE (2026-08-04): patch 0005 + overlay/src/video/phoenix/
{SDL_phoenixvideo,opengl,events,framebuffer}.c + glue/{sdl_phoenix_glctx.c (GPL copy of the
quakespasm pl_phoenix_glctx.c, kept OUTSIDE libSDL2.a),sdl_phoenix_glstubs.c (zlib,
pthread_getcpuclockid)}. libSDL2.a builds w/ SDL_VIDEO_DRIVER_PHOENIX+SDL_VIDEO_OPENGL;
sdl2-gltest LINKS to aarch64-phoenix ELF (qsv3d_init/SDL_GL_CreateContext resolve T). Pushed
org 8671269; coord test helpers (build-sdl2-port.sh + tools/sdl2-port/) pushed 2908483. Memory
[[project_sdl2_port]] created. NOT yet Pi-tested — that's the next step (de-risks the GL seam).

H3 increment (2026-08-04, heartbeat parallel to SDL2 video-driver subagent): added a
"V3D GPU: 3D acceleration (OpenGL + Vulkan)" section to docs/knowledge/rpi4-os-development-
guide.md (was missing the entire graphics stack) — VC6-vs-V3D distinction, reuse-Mesa,
in-process winsys (flat 128MiB PT, CT0/CT1, SLCACTL-per-submit, EZ, no ray_query), the
no-WSI color-buffer-alpha scanout gotcha, mailbox serialization (libvcmbox), firmware pin.
Future H3 increments: fb0/HDMI, storage+NFS-root, audio, userspace-MMIO pattern, debugger.

C3 scoping (2026-08-04, heartbeat parallel to SDL2 build): read the net glue — Quake1 MP
is NOT loopback-only. quakespasm-port/pl_phoenix_stubs.c registers Loopback + Datagram
net_drivers and the UDP net_landriver (UDP_Init…), and net_udp.c is patched to replace the
unimplemented ioctl(FIONREAD) with an MSG_PEEK non-blocking probe. But KNOWN-ISSUES #68
("Quake multiplayer hangs at the LOADING screen") is OPEN — so the driver is wired yet MP
doesn't complete connect/spawn. vkQuake-port stub is still Loopback-only (comment line 13).
C3 = reproduce+fix #68 (Pi client ↔ host quakespasm dedicated server; diagnose where the
LOADING handshake/precache/spawn stalls) + port the UDP net_drivers table into the vkQuake
stub for parity. Dedicated Pi turn (not while SDL2 builds).

C1 SDL2 feasibility DONE (2026-08-04): full analysis → docs/inprogress/2026-08-04-sdl2-
port-plan.md. Verdict: port real SDL 2.30.x (ports/sdl2, CMake, mimic zlib) with phoenix
drivers reusing pl_phoenix_* prims; header-shim approach won't scale to Quake2/3/STK. Cross-
configure probed: passes arch/ABI/atomics; first blocker = SDL threads-detection (needs
patch). Vulkan (no V3DV WSI) = phase 2, risk #1. Then a phase-1 build-plumbing
subagent DELIVERED: `sources/phoenix-rtos-ports/sdl2/` (SDL 2.30.12 + patches 0001 pthread-
detection, 0002 PHOENIX cmake platform branch, 0003 dynapi-off, 0004 systhread-priority-
noop). libSDL2.a cross-builds via port_manager + a trivial SDL_Init/SDL_GetTicks program
LINKS to aarch64-phoenix. Uses STOCK SDL pthread backend (libphoenix pthreads are real +
recursive — a custom backend would be redundant; pl_phoenix_sdlcompat.c kept as fallback).
Pushed org bdfe294. Deferred libphoenix gaps (bite at full-game link, not archive): sem_*
(SDL auto-uses its generic sem), pthread_mutex_timedlock, pthread_{get,set}schedparam
(proper home = libphoenix + toolchain re-sync, would let patch 0004 drop). Then launched
the phase-1 video+input driver subagent (src/video/phoenix/, running).

G1 Tier C (tools/) DONE (2026-08-04): added `Copyright 2026 Phoenix Systems  %LICENSE%`
to 6 unheadered coord-repo files (v3d-driver-port phoenix_mesa_compat.h +
test_ident_decode.c, x11-port mouseprobe.c + fbdev_stub.c, dbg-probe dbg.h + dbg.c),
committed + pushed to coord org (d4cb38e). Corrected recon's "delete fbdev_stub.c" — it
is STILL used by build-xfbdev.sh --stub, so kept (removing the --stub option = attended
decision). Remaining Tier C: _memset.S ARM-provenance (kernel → do with Batch 3) +
confirm publish tooling substitutes %LICENSE% (owner/tooling).

2026-08-04: Plan created. vkQuake torch fix already landed+pushed (d3e329c). vkQuake
e1m1 bright-walls (I1): could not reproduce — fresh `map e1m1`, `start→e1m1`,
`r_rtshadows=1`, and weapon-fire all render lightmaps matching GLQuake (diff <0.2%)
once the build settles; owner reports it steady/persistent on the same netboot build.
Leading theory: GPU-compute lightmap update skips unmodified lightmaps and clears the
per-frame modified flag, so a disrupted initial build could leave surfaces stuck at
the bright default. Robustness fix candidate for I1.

## Next step

Foundation (A1 Batch 1+2, G1 Tier A + Tier C tools/) is landed. A1 Batch 3 is NOT urgent
for a fork (incoming kernel changes are a cosmetic copyright/diacritics sweep + minor hal
fixes; we function without the errno transfer). Pivot to feature work. Priority — pick
ONE focus per turn (use subagents to parallelize analyze/implement/test):

1. **C1 — SDL2 finish phase 1** (video+GL+input HW-VALIDATED ✓ 2026-08-04): (a) **audio
   driver** src/audio/phoenix/ (pull model over /dev/audio0, ref pl_phoenix_snd.c) — no-Pi
   build then one Pi tone test; (b) **wire sdl2 into** `sources/phoenix-rtos-project/_targets/
   aarch64a72/generic/ports.yaml` so it builds into images / games can `depends` on it.
   Delegatable to a subagent. Then phase-1 complete → **C4 Quake2 (yQuake2)** begins on SDL2.
2. **vkQuake continuation** (explicit standing ask): I1 lightmap robustness (make the
   GPU-compute lightmap build not get stuck at the bright default — re-mark modified until
   an upload is confirmed) — implement + verify no regression via HDMI/pixel/host pipeline;
   and/or I2 liquids + remaining workarounds. I3 phantom-kbd bug.
3. **C3 — Quake1 MP**: fix KNOWN-ISSUES #68 (MP hangs at LOADING) — UDP driver is wired;
   diagnose the connect→spawn stall (Pi client ↔ host dedicated server) + vkQuake net parity.
4. Dedicated-turn / lower-urgency: **A1 Batch 3** (careful kernel/libphoenix/project merges
   — snapshot first; restore to manifest 2026-08-04-a1-batch2-done or tag
   known-good/2026-04-19-map-relocation-complete on trouble; then the deferred kernel G1
   Tier A comment fixes + _memset.S provenance); **G1 Tier B** (diagnostic removal, needs
   build+boot); **F1** KNOWN-ISSUES; **B1** debugger library.
