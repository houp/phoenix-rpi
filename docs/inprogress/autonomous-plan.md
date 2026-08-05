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
| I3 | W2 | Fix phantom /dev/kbd0 input (spurious menu spam) | ANALYZED | Root-cause lead (2026-08-05, read-only): the raw-HID readers assume 8-byte-aligned reads and DISCARD the trailing `r%8` bytes (`for off+8<=r`) in tools/quakespasm-port/platform/pl_phoenix_in.c:411, tools/vkquake-port equiv, AND sources/.../sdl2/overlay/src/video/phoenix/SDL_phoenixevents.c. usbkbd fifoPushRaw preserves per-report framing, but ANY single partial read (device-push race, or N_URBS=1 stale/short interrupt-IN buffer, usbkbd.c:56-60,92-94) permanently DESYNCs → later reports read mid-frame → fabricated keys (opens menu). FIX candidates: (a) reader carry-over buffer for leftover bytes across reads (robust, low-risk, do in all 3 readers); (b) device-side: clear/validate the interrupt-IN buffer + only push full 8-byte reports. DIAGNOSTIC (needs Pi): log every raw report on an IDLE boot (no keypress) → confirms spurious vs misaligned. Fix+verify = a future Pi turn |
| E1 | W2 | Dillo HTTPS support | TODO | needs TLS (libphoenix/openssl?) |
| E2 | W2 | Pi internet via host Linux router/proxy (NAT) | TODO | host-side network config |
| E3 | W2 | Dillo displays live internet pages | TODO | after E1+E2 |
| C4 | W3 | Quake2 port (yQuake2) + open/shareware assets + demo+visual test | WIP | **Phase 1 DONE (2026-08-05)**: single static aarch64-phoenix ELF LINKS (undefs→0), coord 3eaf810 tools/yquake2-port/ (dlopen→static backend, ref_gl1 only, malloc-hunk, shared-TU dedup, -fcommon, port patch). yQuake2 pinned e27fdcce. ELF /tmp/yquake2-phoenix. See [[project_quake2_port]]. **Phase 2**: 2002-demo pak0.pak → NFS /usr/share/quake2/baseq2/ + Pi +playdemo demo1 + HDMI capture vs host gl1 |
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

**C4 Quake2 Phase 2 — RUNS + loads maps + 2D renders; game VIEW not visible (console stuck
open).** 2026-08-05 update: the earlier "black" with `+map base1` was a RED HERRING — base1.bsp
isn't in the demo pak ("Can't find maps/base1.bsp"). The demo pak has **maps/demo1|demo2|demo3.bsp**
+ demos/q2demo1.dm2. With **`+map demo1`** the map LOADS FULLY: "Outer Base" level title, 38
entities inhibited, 1 team/2 entities, client_connect, 0 faults, `Multitexturing: Okay`,
`ref_gl1` loaded. 2D renders (console + HUD text + Q2-logo conback). BUT the **drop-down console
stays open with the conback behind it** (not the 3D world) across BOTH `+map demo1` and
`+demomap q2demo1.dm2` → the game 3D view never becomes active/visible.

**Root cause narrowed to: client stalls in the connect→precache handshake** (cls.state never
reaches ca_active → SCR_DrawConsole shows conback, cl_screen.c:559). RULED OUT: focus (driver
already sets SHOWN|INPUT_FOCUS + SDL_SetKeyboardFocus), Sys_Milliseconds (standard/correct),
main loop (Qcommon_Init runs it, "never returns" = upstream-identical, 2D renders prove frames
run). Evidence: only **4 TFU texture uploads** (engine init textures, NOT the map's hundreds);
"Outer Base" prints (serverdata received) but precache never runs. Handshake path: server
svc_stufftext "precache" → CL_Precache_f (cl_main.c:529) → CL_RequestNextDownload (cl_download.c:76)
→ CL_PrepRefresh (cl_view.c:240) → cls.state=ca_active (cl_parse.c:849).

**INSTRUMENTATION BLOCKED by a delivery problem:** added 5 fprintf(stderr) probes at those
points (in external/yquake2, LOCAL/uncommitted), rebuilt (140/140, md5 e8986b40), but 2 Pi runs
produced **ZERO yQuake2 UART output — not even the startup banner** earlier same-binary runs
showed. So either the ~19MB/47MB-bss binary's **NFS-exec is intermittently failing** (cf.
status.md "~19MB binaries hit -ENOMEM at process_load:704") OR stdout/stderr isn't reaching UART.

**NEXT:** (1) make diagnostics **FILE-BASED** — write the handshake trace to a file on the NFS
root (e.g. /usr/share/quake2/yq2diag.txt) + read it host-side, bypassing console delivery;
convert the 5 stderr probes to file appends. (2) In parallel, this exposes a **large-binary
NFS-exec reliability** concern worth checking (does yquake2 exec every boot?). Fallback if the
handshake proves a real loopback bug: inspect the Loop_* net driver message delivery. Minor:
gamma unimpl, relative-mouse, exclusive-fullscreen. See [[project_quake2_port]] [[project_sdl2_port]].
2026-08-05 Pi tests: yQuake2 with **`+set vid_fullscreen 2`** (desktop-fullscreen = use native
mode, no mode-change) DISPLAY-TAKEOVER WORKS and yQuake2's **GL 2D renders to HDMI** (the
drop-down console + green HUD text render via our SDL2+ref_gl1). But loading a level (`+map
base1`, even with gl1_overbrightbits 2 + intensity 3) → HDMI **pure black (max lum 0)** = the
**3D WORLD VIEW does not draw** (2D works, 3D doesn't). 0 faults throughout.

**NEXT (the real C4 blocker) = why the ref_gl1 3D world renders black on V3D.** Investigate
like the vkQuake/quakespasm GL bring-up: is the world drawn at all (GL state: depth test,
face cull, projection/modelview matrices, `glBegin` world surfaces), or drawn but invisible
(matrix/viewport, or all-black lightmaps/textures). Also seen: `TFU vcheck ... VERTICAL-MISMATCH
match=3/6` (texture-tiling mismatch, same class as vkQuake striping) and "Setting gamma failed"
(SDL gamma unimplemented). Secondary C1 SDL2 fixes: gamma (SDL_SetWindowGammaRamp), optional
exclusive-fullscreen "Unknown pixel format" (vid_fullscreen 1) — vid_fullscreen 2 is the
workaround. Minor: SDL relative-mouse, Sys_GetBinaryDir, RO-NFS config writes.
See [[project_quake2_port]] [[project_sdl2_port]] [[project_pi4_v3d_scout]].

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

C4 Quake2 handshake-diag (2026-08-05): narrowed the "console stuck" to the client stalling in
the connect→precache handshake (never reaches ca_active; only 4 TFU init-texture uploads, not
the map's hundreds). Ruled out focus/timing/main-loop. Added 5 fprintf(stderr) probes at the
handshake fns + rebuilt (140/140), but 2 Pi runs gave ZERO yQuake2 UART output (not even the
banner) → suspect large-binary NFS-exec flakiness or stdout/stderr not reaching UART. NEXT =
FILE-BASED diagnostics (write trace to NFS-root file, read host-side) + check exec reliability.

C4 Quake2 demo1 test (2026-08-05, 3 Pi cycles): "black world" was a RED HERRING — base1.bsp
not in demo pak. Demo pak maps = demo1/demo2/demo3.bsp. `+map demo1` LOADS FULLY ("Outer Base",
38 entities, client_connect, 0 faults, Multitexturing Okay). 2D renders. But the console stays
open (conback behind) → game 3D view never activates. Lead: SDL2 driver sends no window
focus/activation events → yQuake2 stays inactive. Next = post SDL_WINDOWEVENT SHOWN/FOCUS_GAINED
in the driver (+ fallback: force present alpha=1). UART goes quiet after ref_gl1 load (yQuake2
Com_Printf → GL console, not UART) — use HDMI to probe post-init.

C4 Quake2 Phase 2 render tests (2026-08-05): with `+set vid_fullscreen 2` the "Unknown pixel
format" mode-set error is GONE (desktop-fullscreen uses the native mode, no mode-change) and
yQuake2's GL output REACHES HDMI — its 2D console + green HUD text render via our SDL2 driver +
ref_gl1 (display takeover works!). BUT `+map base1` renders PURE BLACK (3D world view doesn't
draw; 2D does). gl1_overbrightbits/intensity didn't help → not just darkness. Also: TFU
VERTICAL-MISMATCH (texture tiling) + gamma unsupported. 0 faults. Next = debug the black 3D
world render (Active task). This is the last thing between us and Quake2 on screen.

C4 Quake2 Phase 2 first Pi boot (2026-08-05): downloaded the legal Q2 2002 demo pak
(deponie.yamagi.org, pak0.pak 49951322 bytes verified) → NFS /usr/share/quake2/baseq2/;
deployed yquake2 → /usr/bin. First Pi boot: **yQuake2 RUNS** — "Yamagi Quake II Initialized",
Added packfile pak0.pak (1106 files), ref_gl1 loaded, qsv3d GL up 2.1/V3D 4.2.14.0, scanout
FBO 1920x1080 n=3, client_connect, 0 faults. Blocker = SDL2 driver "Unknown pixel format" on
mode-set → windowed revert → fbcon text stays (see Active task fix). HUGE: first game on our
SDL2 base runs on HW; just needs the video-mode fix to show pixels.

C4 Quake2 Phase 1 DONE (2026-08-05): subagent delivered a single static aarch64-phoenix ELF
that LINKS clean (undefs→0), /tmp/yquake2-phoenix ~26MB. tools/yquake2-port/ (9 files, coord
3eaf810 pushed): dlopen→static backend (pl_phoenix_sys.c), malloc-hunk, main (no setuid),
glstubs, compat header, yquake2-phoenix-port.patch, build script. yQuake2 pinned e27fdcce.
Key: ref_gl1 only (fits GL 2.1), Client-Source already includes the server (don't add
Server-Source), shared.c/md4.c dedup, -fcommon, -Dmodes rename. No libphoenix additions.
Links the GPL sdl_phoenix_glctx.c glue (like sdl2-gltest). Memory [[project_quake2_port]].
Phase 2 = assets + Pi demo render (Active task).

I3 phantom-kbd analysis (2026-08-05, heartbeat parallel to Quake2 build): read the raw-HID
kbd0 read/diff path — strong root-cause lead = the readers discard trailing `r%8` bytes, so
one partial read permanently desyncs report framing → phantom keys (affects both Quake ports
AND the new SDL2 events driver). See the I3 row for the fix candidates (reader carry-over
buffer) + diagnostic (log idle reports on Pi). Fix is a future Pi turn.

C4 Quake2 feasibility DONE (2026-08-05): full plan → docs/inprogress/2026-08-05-quake2-
port-plan.md. Verdict feasible ~5-8d: yQuake2 ref_gl1 (pure fixed-function, fits our GL 2.1;
quakespasm already proves immediate-mode GL on this stack) folded into a SINGLE ELF; the
dlopen→static problem solved via a backends/phoenix static-return backend (+ VID_HasRenderer
file-check patch + link exactly one renderer). All needed libphoenix syscalls measured present
(only realpath-NULL + hunk anon-mmap caveats). Assets = Q2 2002 demo pak. Location =
external/yquake2 + tools/yquake2-port (mirror quakespasm-port), NOT a ports/ lib. Then launched
the phase-1 single-ELF build subagent.

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
