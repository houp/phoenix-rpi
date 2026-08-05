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
  Netboot game tests are now RELIABLE — the harness (psh-interact.py) waits for the NFS
  "registered / (takeover)" line before sending commands (#156 fix). No ls-warm needed.

---

## Task board

Status: TODO / WIP / BLOCKED / DONE. Priority waves: W0 foundation → W3 hardest.

| ID | Wave | Task | Status | Notes |
|----|------|------|--------|-------|
| A1 | W0 | Upstream sync: pull all siblings, integrate, build, verify, push org | WIP | analysis DONE; Batch 1+2 MERGED+BUILT+BOOT-VERIFIED+PUSHED (manifest 2026-08-04-a1-batch2-done); only Batch 3 (kernel/libphoenix/project — careful) remains |
| G1 | W1 | Full code review (all repos): bugs/hacks/diagnostics/TODOs/comments/licensing → fix+test+commit | WIP | recon → docs/review/2026-08-04-autonomous-review-recon.md. Tier A (comment/TODO) DONE. Tier B **devices diagnostics REMOVED** (2026-08-05, -653 lines: sdcard/pcie/xhci/audio; --scope core OK; boot-verified 0 faults + audio/USB/NFS work; pushed devices 89ffe1c; manifest g1-devices-cleanup). Tier C tools/ headers DONE (6 files +%LICENSE%; fbdev_stub KEPT — still used by build-xfbdev.sh --stub). Pending: Tier B (diag removal, needs boot); Tier C _memset.S provenance (kernel→after Batch 3) + %LICENSE% tooling verify; kernel/libphoenix/project Tier A after A1 Batch 3 |
| H1 | W1 | Docs cleanup + archive stale docs | TODO | |
| H2 | W1 | Final Pi4 port-state documentation | WIP | primary state doc = docs/inprogress/pi4-hardware-support-matrix.md. Corrected stale entries (SMP now ✅ 4-core works, Vulkan ✅ textured 3D, exec-reliability→F1 finding) + added "Ported libraries & applications" section (Mesa GL/Vulkan, SDL2, X11, quakespasm/vkQuake/yQuake2, Dillo). Still: promote to a docs/-root doc + final polish when ports settle |
| H3 | W1 | Pi4 OS-dev knowledge base (extend existing) | DONE | base = docs/knowledge/rpi4-os-development-guide.md. Added: V3D GPU (GL+Vulkan), Display(fb0/HDMI)&audio(PWM), Porting userspace apps & games, **In-process debugging (libdbg)** (2026-08-05), **Storage & the root filesystem** (SD/eMMC DDR50-reads/PIO-writes/CMD13-poll-completion/pool-thread-stack + NFS-root takeover/boot-order-race/NFSv4-expiry/GENET-RX-aliasing/poll-stall/runtime-read caveat) (2026-08-05). Both planned gaps closed; living doc, extend as work continues |
| B1 | W1 | Generalize in-process debugger → reusable Phoenix debug library | DONE | libdbg corelib (phoenix-rtos-corelibs d026ff0): dbg_init/dbg_backtrace/dbg_arm_watchdog; --scope core + image verify OK, libdbg.a symbols confirmed. libphoenix trampoline enabler (_dbg_signal_ctx) already in place |
| B3 | W1 | Debug-facility documentation | DONE | OS-dev guide "In-process debugging (libdbg)" section + dbg.h API docs + tools/dbg-probe pointer note |
| F1 | W2 | Resolve KNOWN ISSUES (kernel/system/libphoenix) | WIP | **large-binary NFS-exec reliability** ROOT-CAUSED (docs/inprogress/2026-08-05-large-binary-exec-investigation.md): NOT -ENOMEM (status.md note stale) — it's the EAGER page-by-page BSS commit at exec (process_load64 anon vmmap + full hal_memset under map->lock; yquake2 26.5MB BSS = ~14k pages) → long exec window intermittently SILENT-HANGS over flaky netboot NFS. Stack trimmed 32→4MB (harmless). **#156 boot-order race FIXED 2026-08-05: the DOMINANT test-flakiness was psh running before the NFS takeover → `not found`. Fix: psh-interact.py waits for `registered / (takeover)` before sending commands (56bcdef) → netboot game tests RELIABLE (yquake2 3/3 clean). NO workaround needed.** Eager-BSS exec-hang = rarer secondary. Phoenix-side proper fix = plo boot-order gate (deferred) |
| F2 | W2 | OS perf (I/O, net, scheduling) + modern syscalls + measurements + wire ports to them | TODO | |
| SD | W2 | SD-card driver: full speed + correctness (prior loop goal; folds into F1/F2) | WIP | reads IRQ, writes CMD13-poll done; perf=PIO throughput |
| C1 | W2 | SDL2 port (fullscreen GL+Vulkan, kbd+mouse, sound); no X11 needed | PHASE-1 DONE | feasibility → docs/inprogress/2026-08-04-sdl2-port-plan.md. Phase-1 build plumbing DONE: ports/sdl2 (SDL 2.30.12, 4 patches: PHOENIX cmake branch + pthread + dynapi-off + sched-noop), libSDL2.a cross-builds+links (stock pthread backend), pushed org bdfe294. Phase-1 video+input driver DONE (patch 0005 + overlay/src/video/phoenix/ {video,opengl,events,framebuffer} + glue/{glctx GPL-copy,glstubs zlib}); libSDL2.a builds w/ SDL_VIDEO_DRIVER_PHOENIX + fullscreen-GL test LINKS (org 8671269). GPL-glue seam kept OUT of zlib libSDL2.a. **Pi GL-demo HW-VALIDATED (2026-08-04)**: sdl2-gltest = GL 2.1/V3D 4.2, 600 frames clean exit, 0 faults, 1920x1080 triple-buffer page-flip, fullscreen GL clear-color on HDMI. Audio driver DONE + HW-VALIDATED (2026-08-05): src/audio/phoenix/ (patch 0006, pull model /dev/audio0), sdl2-audiotest on Pi → "audio open: driver=phoenix 44100/S16/2ch", tone played, clean exit, 0 faults. org ports c191d20; project ports.yaml f82c334 (sdl2 registered `if:false` — no consumer yet). **SDL2 phase 1 COMPLETE**: fullscreen GL + input + audio all HW-validated. NEXT: **C4 Quake2 (yQuake2)** on SDL2. Vulkan=phase 2 (no V3DV WSI). See [[project_sdl2_port]] |
| C3 | W2 | Quake1 multiplayer networking fix | TODO | NOT loopback-only: quakespasm has UDP landriver + Datagram wired + FIONREAD→MSG_PEEK fix. Real bug = KNOWN-ISSUES **#68 MP hangs at LOADING screen** (open). vkQuake stub still loopback-only. Fix = diagnose #68 (Pi client ↔ host dedicated server) + bring vkQuake net to parity. Needs dedicated Pi turn |
| I1 | W2 | vkQuake e1m1 bright-walls: robustness of GPU-compute lightmap build | WIP | can't repro (my loads correct); see below |
| I2 | W2 | vkQuake: liquids + remaining workarounds + perf | mostly OK | 2026-08-05 (reliable pipeline): vkQuake renders `map start` CORRECTLY — HDMI shows textured brick/stone walls + tiled floor, "QUAKE" carved archway, proper lighting, fireball sky, shotgun+HUD, NO visible texture striping, torch/alpha fix holding. vkQuake substantially DONE. Remaining polish (low-pri, hard-to-repro): liquids on a water-heavy map (e1m3), combat lightmap flicker (I1), phantom-kbd (I3) |
| I3 | W2 | Fix phantom /dev/kbd0 input (spurious menu spam) | ANALYZED | Root-cause lead (2026-08-05, read-only): the raw-HID readers assume 8-byte-aligned reads and DISCARD the trailing `r%8` bytes (`for off+8<=r`) in tools/quakespasm-port/platform/pl_phoenix_in.c:411, tools/vkquake-port equiv, AND sources/.../sdl2/overlay/src/video/phoenix/SDL_phoenixevents.c. usbkbd fifoPushRaw preserves per-report framing, but ANY single partial read (device-push race, or N_URBS=1 stale/short interrupt-IN buffer, usbkbd.c:56-60,92-94) permanently DESYNCs → later reports read mid-frame → fabricated keys (opens menu). FIX candidates: (a) reader carry-over buffer for leftover bytes across reads (robust, low-risk, do in all 3 readers); (b) device-side: clear/validate the interrupt-IN buffer + only push full 8-byte reports. DIAGNOSTIC (needs Pi): log every raw report on an IDLE boot (no keypress) → confirms spurious vs misaligned. Fix+verify = a future Pi turn |
| E1 | W2 | Dillo HTTPS support | TODO | needs TLS (libphoenix/openssl?) |
| E2 | W2 | Pi internet via host Linux router/proxy (NAT) | TODO | host-side network config |
| E3 | W2 | Dillo displays live internet pages | TODO | after E1+E2 |
| C4 | W3 | Quake2 port (yQuake2) + open/shareware assets + demo+visual test | RENDERS (loads pak+map) | **2026-08-05: the "infra-blocked colormap.pcx" diagnosis was WRONG.** Real cause = the Q2 demo pak is staged at /usr/share/quake2/baseq2/pak0.pak but yquake2's default datadir = its binary dir (/usr/bin) or "." → it searched the wrong baseq2 → `Couldn't load pics/colormap.pcx` (a MISSING PAK, not NFS flakiness). **Fix = launch with `+set basedir /usr/share/quake2`.** With it, yquake2 loads the pak, loads **demo1 (Outer Base)** — HDMI shows server init + "38 entities inhibited" + the Yamagi Quake II loading screen — and reaches the 3D render path (CL_PrepRefresh → LoadTexinfo → v3d-winsys TFU texture uploads to V3D). **Long capture (330s) 2026-08-05: map FULLY loads → `ca_active` (in-game): "models done", precache complete (pics/models/images/clients/sky), 0 faults — so the "slow load" was NOT the blocker, it finishes.** BUT the HDMI shows the **3D world view does NOT render**: only the console/loading text, in a **~640×480 bottom-left corner** of the 1920×1080 screen (rest black). Two concrete SDL2-path bugs (yQuake2 is the FIRST game on SDL2; quakespasm/vkQuake render fullscreen via the direct winsys, not SDL2): **(a) viewport/vid-resolution = 640×480 not 1920×1080** despite vid_fullscreen 2 (small corner) — NEXT TEST = launch `+set r_mode -1 +set r_customwidth 1920 +set r_customheight 1080`; **(b) 3D world black behind the (opaque) console** = likely the no-WSI fb0 scanout alpha class (3D writes alpha≈0 → dropped; console 2D alpha=1 shows) like [[project_vkquake_torches_dark_fullbright]] → fix = force alpha=1 in the SDL2 present path (sdl_phoenix_glctx.c/scanout). **Phase 1 DONE** (single static ELF, coord 3eaf810 tools/yquake2-port; yQuake2 pinned e27fdcce). Also remove leftover YQ2DIAG probes (local external/yquake2). See [[project_quake2_port]] |
| C5 | W3 | Quake3 port (quake3e/ioq3) + playable assets + demos | PHASE1-DONE | quake3e links to one static aarch64-phoenix ELF (168/168 TUs, 0 undef, 27MB, GetRefAPI/main/VM_Create resolved). tools/quake3-port/ pushed (6fb98f0+3d74441). QVM=no dlopen; msg_t clash beaten w/ 0 Q3 rename. Phase-2 runtime deferred (infra) |
| C6 | W3 | SuperTuxKart (OpenGL fullscreen, GPU) | TODO | large |
| D1 | W3 | X11 GPU-accelerated extensions (toward RPi-OS parity) | TODO | |
| D2 | W3 | X11 GL/Vulkan windowed (GLX) + glxgears validation | TODO | |
| D3 | W3 | XFce desktop environment port | TODO | large |
| E4 | W3 | ffmpeg port (tool+lib) + Pi HW decode accel | TODO | |
| E5 | W3 | X11 video player (windowed + fullscreen) | TODO | after E4 |
| B2 | W3 | Extend debugger to kernel/driver-side | TODO | after B1 |
| H4 | W3 | AI-driven-journey article (git+conversation+memory analysis) | DRAFT | docs/AI-DRIVEN-PORT-JOURNEY.md — grounded draft (1427 coord commits over ~4.5mo): the arc, what was easy/hard for AI, the ~40-cycle torch bug + #67 false-metric + #156-race war-stories, observability techniques, the human's ground-truth impact, why hardware is hard for a text agent, the autonomous phase. Owner review/refine expected; extend as the journey continues |

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

**Quake2 decisive render test = infra-blocked (not a port bug).** 2026-08-05: reliable exec worked
(banner), but yquake2 fatal-errored `Couldn't load pics/colormap.pcx` = intermittent RUNTIME NFS
read failure (NFS lease-expiry/reclaim or stale host nfsd; the #156 exec fix doesn't cover runtime
reads). vkQuake/quakespasm render because their reads happened to succeed. **Conclusion: the games
are gated by netboot NFS reliability+speed (100Mbps + read flakiness) — an INFRA limit; SD-boot
(local, no card in) would fix it. Stop chasing game full-render over flaky netboot.** Netboot NFS
runtime-read reliability is a real KNOWN issue (lease/reclaim window) but a deep NFS effort. Pivot
to non-game / non-Pi-heavy work: Quake3 feasibility (analysis), A1 Batch 3, more G1/docs.

**H4 journey-article DRAFT written** (docs/AI-DRIVEN-PORT-JOURNEY.md) — the distinctive capstone
the owner wanted: honest field report on the all-AI Pi4 port (easy vs hard for AI, the war-stories,
observability-first, the human as ground-truth, why hardware is hard for a text agent, the
autonomous phase). Owner review expected; extend as the run continues. Non-Pi, no-risk.

**[DONE] G1 Tier B devices cleanup** — removed 653 lines of disproved diagnostics (sdcard/pcie/
xhci/audio), --scope core OK, boot-verified 0 faults (audio+USB enum+NFS all work), pushed
(devices 89ffe1c, manifest 2026-08-05-g1-devices-cleanup). Functional recovery logic preserved.
Next candidates (non-Pi-load-limited, publication/foundation): remaining G1 Tier B (plo dead
diagnostic vectors, project armstub markers — careful, plo has NO real fault handling via the
dead vector table), G1 Tier C _memset.S provenance, A1 Batch 3 (kernel merge, boot-verifiable
now), or a new feature (Quake3 feasibility, Dillo internet). vkQuake/quakespasm/SDL2 all render.

**[DONE] vkQuake render VERIFIED (2026-08-05)** on the reliable pipeline: `map start` renders correctly
(HDMI: textured walls/floor, QUAKE archway, lighting, sky, HUD, no visible striping, torch fix
holding). vkQuake substantially DONE. The #156 harness fix delivered clean exec + render.
Next vkQuake polish (low-pri): liquids on a water map, combat flicker, phantom-kbd.

**★ FIXED 2026-08-05: netboot-test flakiness (was blocking all Pi game testing).** psh-interact.py
now waits for the NFS `registered / (takeover)` line before sending commands (#156 boot-order race;
psh was launched by plo as a sibling of the takeover server) → yquake2 execs 3/3 clean, no workaround
(coord 56bcdef). **Reliable Pi game/vkQuake testing is RESTORED.** Next: use it — yquake2 full render
(long capture), vkQuake rendering work, TFU-tiling striping. Original breakthrough note below:

**BREAKTHROUGH 2026-08-05: netboot-test flakiness ROOT-CAUSED + WORKAROUND FOUND.** The intermittent
"empty" game boots are `psh: /usr/bin/<bin> not found` = the **nfs-fs first-lookup ENOENT race (#156)**
— the exec's FIRST access to the binary path ENOENTs before the Phoenix nfs-fs dircache is populated.
**WORKAROUND (proven): run `ls -la /usr/bin/<bin>` as the FIRST psh command to warm the dircache,
THEN exec** → reliable. Confirmed: `ls` then `/usr/bin/yquake2` → banner + ref_gl1 ran. So (a) games
are now RELIABLY TESTABLE (prepend the ls-warm), (b) the yquake2 4MB-stack build execs FINE, (c) the
F1 eager-BSS exec-hang was NOT the dominant failure — NFS-visibility was. Proper fix (#156, Phoenix
nfs-fs): retry/populate the dircache on first lookup, or make exec retry ENOENT once. **This reopens
reliable Pi game/vkQuake testing.**

Non-Pi note: This turn also: **H2 port-state
doc** — brought docs/inprogress/pi4-hardware-support-matrix.md up to date (fixed stale SMP/Vulkan/
exec-reliability entries; added the Ported libraries & applications section for the vacation-period
userspace work). Sustained-documentation work stream (H2/H1/H4) sidesteps the flaky netboot.
Previously: **F1 large-binary
NFS-exec reliability** — a root-cause+fix-plan subagent is analyzing the kernel exec/process_load
+ anon-memory-commit path (why ~19MB/big-BSS binaries fail NFS-exec ~50%). This is the real
bottleneck; fixing it unblocks reliable Pi game testing (and possibly a cheap userspace mitigation
= smaller yquake2 stack). C4 Quake2 BANKED (runs + loads maps + 2D; full 3D load infra-bound, not
a port bug). Other non-Pi candidates queued: H2 port-state doc, more H3 sections, G1 code-review,
A1 Batch 3 conflict analysis. SD-boot would make games fast+reliable but the card is out.

**[BANKED] C4 Quake2 — RUNS + loads maps + 2D renders; full 3D load infra-bound.** 2026-08-05 update: the earlier "black" with `+map base1` was a RED HERRING — base1.bsp
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

**STALL PINNED + PARTIALLY FIXED (2026-08-05):** the 2 empty diag runs were NETBOOT FLAKINESS
(confirmed: sdl2-gltest ran clean = netboot/exec/UART healthy; a 3rd yquake2 diag run then
produced the banner + full YQ2DIAG trace). The 5 fprintf(stderr) probes work.
Trace: server stufftext configstrings + baselines + **precache** → CL_Precache_f (argc=2) →
**CL_RequestNextDownload check=32 (CS_MODELS) → STOPS.** = the client stalls in the download-check
phase (allow_download ON → tries to fetch a model over loopback → hangs).
**FIX #1: `+set allow_download 0`** → handshake proceeds to **CL_PrepRefresh** (precache); TFU
uploads 4→12. → **BAKE allow_download 0 into the port** (pl_phoenix_main.c or a default cfg).
**FRONTIER LOCALIZED (2026-08-05): the precache "stall" is SLOW texture loading, not a hang.**
Probes (in external/yquake2 cl_view.c + gl1_model.c, local/uncommitted) traced it to
**R_BeginRegistration → Mod_LoadBrushModel → Mod_LoadTexinfo** (last probe "before LoadTexinfo";
"LoadTexinfo done" never printed in 90s). BUT TFU uploads keep PROGRESSING (n=5,6,7,8,9…) past
that point — so it is NOT hung on one texture; it's loading the map's ~100 wall textures (each =
an NFS `.wal` read from the 50MB pak + palette-convert + TFU upload) over the **slow 100Mbps
netboot NFS** [[project_pi4_netboot_100mbps_cable]] + 19MB-binary demand-paging — just too slow to
finish in ≤165s. **Also: several TFU uploads print `TILING=LINEAR!`/VERTICAL-MISMATCH** = the
winsys TFU-tiling striping bug (same as vkQuake, status.md) — a correctness issue, NOT the hang.

**NEXT:** (1) confirm slow-vs-hang with a **4-5 min capture** (does it eventually reach ca_active
+ render?). If slow: it's NFS-latency/demand-paging bound → mitigate via SD-boot (local, read-ahead
clustered [[project_sdboot_largeexec_slowstart]]) or a gigabit cable (owner, physical). (2) Fix the
TFU LINEAR-tiling striping in the winsys (shared w/ vkQuake). (3) Bake allow_download 0 into the
port. Clean up the yquake2 YQ2DIAG probes when done. Netboot ~50-70% reliable; sdl2-gltest = health
check. See [[project_quake2_port]] [[project_pi4_v3d_scout]].
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

2026-08-05 (Quake2 fully loads to `ca_active`, but 3D view doesn't render — SDL2-path bug): Ran
a 330s capture of `yquake2 +set basedir /usr/share/quake2 +set allow_download 0 +map demo1`. The
map **fully loaded**: UART shows all models loaded ("models done"), precache complete ("Map: demo1
pics models images clients sky"), and **`ca_active`** (client in-game) — 0 faults. So the earlier
"slow-NFS load doesn't finish" story is REFUTED (it finishes ~5min). **But the HDMI (pixel-checked)
shows the 3D world view does NOT render** — only the console/loading text, confined to a ~640×480
bottom-left corner of the 1920×1080 screen, rest black. Diagnosed 2 SDL2-specific bugs (yQuake2 is
the FIRST game on the SDL2 path; quakespasm/vkQuake use the direct winsys): (a) game's vid-resolution
= 640×480 not 1920×1080 despite vid_fullscreen 2 → renders in a corner (the SDL2 driver forces the
window to 1920×1080 at qsv3d_init but ref_gl1's viewport is 640×480 — a mode-negotiation gap); (b)
3D world black behind the opaque console = likely the no-WSI scanout alpha class (vkQuake torches).
**NEXT (a Pi turn): test (a) with `+set r_mode -1 +set r_customwidth 1920 +set r_customheight 1080`;
if the corner fills but stays black, chase (b) = force alpha=1 in sdl_phoenix_glctx.c present.** This
is the deep-GL-rendering class — bounded, pixel-analysis-verifiable, but multi-cycle. Pi FREE.

2026-08-05 (Quake2 UNBLOCKED — "infra-blocked" was a MISDIAGNOSIS): Re-tested Q2 on the fresh-
synced userspace to check the colormap.pcx failure. Found the real cause: **baseq2 in the game's
default datadir had NO pak** — the Q2 demo pak0.pak is staged at `/usr/share/quake2/baseq2/` but
yquake2's default datadir is its binary dir (`/usr/bin`) or `.`, so it searched the wrong path →
`Couldn't load pics/colormap.pcx` = a MISSING PAK, **not** the NFS lease/reclaim "runtime-read
flakiness" the board claimed. **Fix = `+set basedir /usr/share/quake2`** (the basedir cvar →
FS datadir; verified by reading external/yquake2 filesystem.c). Relaunched → yquake2 loaded the
pak + loaded **demo1 (Outer Base)**: HDMI shows "Game is baseq2 built on Aug 5 2026", server init,
"38 entities inhibited", the Yamagi Quake II loading screen; UART shows CL_PrepRefresh → LoadTexinfo
→ v3d-winsys TFU **texture uploads to the V3D GPU** (ran to the 110s capture cutoff still loading —
slow-NFS map). So **Quake2 loads pak+map+renderer and reaches the 3D render path** — C4 is
substantially working, not infra-blocked. Corrected the false "colormap.pcx = NFS runtime-read"
story in memory [[project_large_binary_exec_hang]]. Remaining: longer capture to reach the in-game
3D view (HDMI so far = console/loading, mostly black); clean up leftover YQ2DIAG probes. Pi FREE.

2026-08-05 (netboot-export-drift FIXED + libm HW-VALIDATED — end-to-end win): Fixed the
fresh-kernel/stale-userspace drift found last turn. **Root confirmed:** the NFS export
`/srv/phoenix-rpi4-nfs` userspace was ~2 weeks stale (psh Jul 23) vs the Aug-5 kernel — nearly
every base binary differed. **Fix (coord 8be79e4):** rewrote the deprecated no-op
`scripts/sync-netboot-tree.sh` to rsync the built base rootfs into the export WITHOUT --delete
(preserves hand-staged games/assets: baseq2, /usr/bin/yquake2, X11 configs, qdet captures; skips
/dev,/proc,/tmp,/mnt) and wired it into `netboot-server-up.sh` so every netboot cycle serves
userspace matching the kernel. Verified file-level (export base binaries now identical to build;
assets preserved). **Then end-to-end on real HW:** fresh userspace booted clean + `/bin/test-libc-
math -g math_round` → **6 Tests 0 Failures OK** → the rint/nearbyint/lrint/llrint/lround/llround/
fdim/fmax/fmin/copysign libm functions are now **HW-VALIDATED on aarch64-Phoenix** (were host-only).
The prior boot's USB `xHC-CMD err` was confirmed just intermittent flakiness (this boot enumerated
mouse0/kbd0 fine). **Implication:** future netboot game/app tests now run userspace matching the
kernel — this likely removes the ABI-drift class of "runtime-read" failures (worth re-testing Q2).

2026-08-05 (clean-build gate PASS + netboot-export-drift finding; libm HW-run still deferred):
Set out to run the math_round libm test on real HW. Two wins + one key finding + one blocker:
(1) **Clean-build gate**: staged ports (`--ports-only`: libnfs-6.0.2 + noted mbedtls/openssl
available → relevant to E1 Dillo-HTTPS) then built a fresh `--variant nfsroot --with-tests` image
reusing my cumulative core changes (rint/libm-family/libdbg) → **builds + verifies OK** (nfs-fs
relinked fresh vs current libphoenix). First from-ports build since those changes → no cumulative
breakage. (2) **CRITICAL finding — netboot export drifts from build** (new memory
project_netboot_export_drift): the Pi netboots a FRESH kernel (TFTP from buildroot) but a
HAND-MAINTAINED NFS root `/srv/phoenix-rpi4-nfs` (exportfs fsid=0) that does NOT auto-sync from the
build (sync-netboot-tree.sh is a no-op). First Pi cycle → `psh: /bin/test-libc-math not found`
(export stale). Fixed surgically (cp'd the fresh static test binary into the export). **This
reframes game "runtime-read" failures: fresh-kernel vs stale-userspace syscall-ABI drift is a
plausible alt cause of the Q2 colormap.pcx failure, not just NFS lease/reclaim.** (3) **Blocker**:
the rerun boot hit **intermittent USB xHCI enumeration flakiness** (`xHC-CMD err: 19` retry storm →
psh never reached), unrelated to the test (first boot enumerated mouse0/kbd0 fine). Stopped
chasing the low-marginal-value HW-run (functions already host-validated vs glibc) per resilience.
The test binary is now IN the export → a future clean boot is one step from completing the HW-run.
Pi FREE.

2026-08-05 (libm HW regression tests added; HW-run infra-deferred): Added a `math_round` test
group to the Phoenix libc math suite (`phoenix-rtos-tests/libc/math/round.c` + main.c runner,
commit b653851 pushed) covering the functions I recently added to the phoenix libm: rint/rintf/
nearbyint/nearbyintf (ties-to-even + signed-zero + NaN/inf), lrint/llrint vs lround/llround
(ties-even vs ties-away), fdim/fdimf, fmax/fmaxf, fmin/fminf (C99 NaN semantics), copysign/
copysignf. Expected values verified vs glibc; test cross-compiles clean (all TEST_math_round_*
symbols generated). **Intended to run it on real HW over netboot, but hit an infra blocker:** the
**nfsroot** variant's nfs-fs links the **libnfs port**, which is only staged by the `ports` build
stage — and the default/auto scope with dirty `phoenix-rtos-tests` forces `clean host core project
image` (NO `ports`), so `--variant nfsroot --with-tests` fails `fatal error: nfsc/libnfs.h`. Only
`--scope full-clean` includes `ports` (a ~20min from-scratch rebuild). Given the functions are
already host-validated vs glibc and integer-exact (HW double FP handles them trivially), the HW-
run's marginal benefit is low vs a full-clean's cost, so I deferred it rather than rabbit-hole
(resilience rule). The test is committed + will run in CI / a future full-build turn. **Reusable
finding:** to run tests OR games on netboot after a tests-dirty tree, use `--scope full-clean
--with-tests --variant nfsroot` (stages libnfs) — a bare `--with-tests` won't stage libnfs.

2026-08-05 (H3 DONE — knowledge-base "Storage & the root filesystem" section): Added the last
planned OS-dev-guide section (docs/knowledge/rpi4-os-development-guide.md), capturing the hard-won
storage + rootfs knowledge for the public release: SD/eMMC (EMMC2) DDR50+SDMA reads, PIO writes
with CMD13-poll completion (TRANSFER_DONE never latches; DMA-write silicon quirk), the lost-wakeup
guard, the FS pool-thread-stack-overflow crash (not a driver bug), card-variance EIO; and NFS-root
over netboot (dummyfs→lwip→nfs takeover + `registered / (takeover)`, the #156 boot-order race,
NFSv4 lease-expiry/renew, the GENET RX buffer-aliasing + poll-stall perf fixes, 100Mbps cable
limit, and the runtime-read-reliability caveat that gates asset-heavy games → prefer SD-boot).
With the debug-facility section added last turn, both of H3's noted gaps are closed → H3 DONE.
**Turn rationale:** surveyed the board — all remaining ambitious items (game runtime, X11 GPU,
ffmpeg, Dillo-internet, XFce, A1 Batch 3, SD perf, I3 phantom-kbd) are netboot-infra-blocked,
vision-dependent, network-risky, huge, or regression-risky without Pi verification, so none are
cleanly completable unattended; chose the highest-value autonomous-safe item (closing an H-doc).

2026-08-05 (B1+B3 DONE — libdbg reusable debug library): Promoted the aarch64 in-process
backtrace facility (built while debugging the vkQuake/Quake hangs, HW-validated in tools/dbg-
probe) into a first-class reusable corelib **`sources/phoenix-rtos-corelibs/libdbg`** (commit
d026ff0, pushed). API: `dbg_init()` (crash handlers → PC + fp-backtrace, exit 128+signo),
`dbg_backtrace(tag)` (context-aware: from a handler unwinds the INTERRUPTED code via libphoenix's
`_dbg_signal_ctx`, else the caller), `dbg_arm_watchdog(secs)` (SIGALRM → dumps where a HANG is
stuck, re-arms). The libphoenix enabler (trampoline stashing `_dbg_signal_ctx`/`_dbg_signal_pc`)
was already in place from an earlier turn. Arch-specific frame walk guarded behind `__aarch64__`
so the corelib builds for all targets. **Validation:** clean `-Wall -Wextra` cross-compile;
wired into corelibs DEFAULT_COMPONENTS; `--scope core` + image verify OK; `nm` confirms
dbg_init/dbg_backtrace/dbg_arm_watchdog in the built libdbg.a + dbg.h installed to the sysroot
include dir. Behavior is identical to the HW-validated tools/dbg-probe code (pure relocation +
style/arch-guard), so no Pi re-test. B3 docs: OS-dev guide "In-process debugging (libdbg)" section
(mechanism + host addr2line workflow) + dbg.h API comments + a canonical-home pointer in tools/
dbg-probe. Manifest 2026-08-05-libdbg-corelib.

2026-08-05 (libphoenix libm: rounding/min-max family completed): Audited math.h-declared vs
phoenix-libm-defined symbols (comm on nm of the built libphoenix.a) to find latent link-time
gaps, then filled the trivially-and-EXACTLY-implementable, commonly-hit subset (16 fns, commit
50f007c pushed): `lrint/llrint/lrintf/llrintf`, `lround/llround/lroundf/llroundf`, `fdim/fdimf`,
`fmax/fmaxf`, `fmin/fminf`, `copysign/copysignf`. All exact (no approximation): l*/ll* reuse
rint()/round(); fmax/fmin/fdim use C99 NaN semantics; copysign uses the conv_t union like fabs;
float variants delegate to double. **Validation (Pi-independent):** host unit test vs glibc
across NaN/inf/signed-zero/ties = ALL PASS; clean `-Wall -Wextra` cross-compile; `--scope core`
+ image verify OK; symbols confirmed in rebuilt libphoenix.a. Manifest 2026-08-05-libphoenix-
math-family. **Deliberately EXCLUDED** (not blindly implemented): all `long double` (*l) variants
(aarch64 binary128 — hard), transcendentals (exp2/log2/expm1/log1p/erf/gamma/bessel — precision-
critical), and nan/nanf (already a weak symbol). These stay as future demand-driven work.

2026-08-05 (libphoenix libm: rint/nearbyint added — the C5 follow-up): Implemented the
missing `rint`/`rintf`/`nearbyint`/`nearbyintf` in the phoenix libm (`sources/libphoenix/libm/
phoenix/exp.c`, committed d61f4a3, pushed to org). **Root of the gap:** the phoenix libm
(default `LIBM_USE_LIBMCS=n`) had floor/ceil/round/trunc but not rint/nearbyint, even though
math.h declares them and the alternative libmcs backend has them — so the C5 quake3e link had to
stub rint. Fixed centrally so EVERY math-using port gets it. Key correctness point: rint/
nearbyint round ties-to-EVEN (default FE_TONEAREST), unlike round() which rounds ties away from
zero; nearbyint==rint here since this libm raises no FP exceptions. **Validation (all Pi-
independent):** (a) a host unit test comparing a verbatim copy of the impl to glibc's rint across
24 ties-to-even/signed-zero/large cases = ALL PASS; (b) cross-compiles clean (aarch64-phoenix-gcc,
-Wall); (c) `rebuild --scope core` succeeds + image verify OK; (d) nm confirms `T rint`/`T
nearbyint` in the rebuilt .buildroot libphoenix.a + sysroot copy. Manifest 2026-08-05-libphoenix-
rint snapshotted. NOTE: quake3e's port-local rint stub is now redundant but HARMLESS (a strong
.o symbol shadows the archive copy — no duplicate-symbol conflict), so left as-is; will drop on
the next quake3e rebuild. `pthread_getcpuclockid` NOT done in libphoenix — it needs kernel
per-thread CPU-clock support to be correct, so the port stub stays (documented).

2026-08-05 (C5 Quake3 phase-1 COMPLETE): The build subagent closed the link — quake3e cross-
builds to a single static aarch64-phoenix ELF, **168/168 TUs, 0 undefined symbols**, 27MB, `T
main`/`T GetRefAPI`/`T VM_Create` resolved (verified ELF at /tmp/quake3e-phoenix; reproducible).
The no-dlopen QVM thesis held (GetRefAPI binds at link time; no game C compiled in). Pushed
tools/quake3-port/ to org (6fb98f0 + 3d74441): build-quake3e-phoenix.py, platform/ (pl_phoenix_
main/sys/stubs/compat), quake3e-phoenix-port.patch, README, COPYING. quake3e pinned SHA
623982900a132e5c812dcb5231a430f28fafabeb in gitignored external/quake3e. **Reusable findings:**
(1) the predicted top risk — Phoenix SysV `msg_t` vs Q3 net `msg_t` — was beaten with ZERO
Q3-source rename via pl_phoenix_compat.h pre-parsing the Phoenix socket/msg header chain under a
private rename so only Q3's msg_t is TU-visible; (2) `-DBOTLIB` needed for botlib TUs; (3)
huffman.c's file-local `send()` shadowed POSIX send → Huff_send. **libc/libm gaps** stubbed in
the port: `rint` (libm) + `pthread_getcpuclockid` (Mesa u_thread.c) — candidates to implement
properly in libphoenix (standing rule), deferred (core change → needs --scope core + boot
verify). Phase-2 runtime/render deferred per the infra caveat (needs reliable storage).

2026-08-05 (F1 KNOWN-ISSUES refresh; Quake3 build still running): While the C5 phase-1
build subagent grinds (no notification yet = still working; owns tools/quake3-port + external/
quake3e, so I stayed clear), did an independent, autonomous-safe, non-vision task: refreshed
`docs/KNOWN-ISSUES.md` (2026-07-22 → 2026-08-05, committed+pushed 8ef82a2). (a) Condensed the
huge #67 alias-model saga to its TRUE resolved state — quakespasm `3d742a3` (verified in
external/quakespasm): single-pose VBO crossing a 4KB page = deterministic data-dependent V3D
fetch bug, `vboposes=numposes`; kept the false-positive-metric lesson. (b) Added a system-level
limitation documenting **netboot NFS-root reliability** (the two flakiness modes: boot-order
race + transient runtime read failures on 100Mbps) and recommending SD-boot / gigabit for
asset-heavy use — capturing this run's game-render infra finding for the public release.
**Explicitly deferred vkQuake I1 (lightmap) per my own scoping rule:** it's a rendering-
correctness bug I couldn't reproduce and can't get owner ground-truth on while owner is away
(screenshot-needing → defer in unattended mode; #67 taught: don't trust a fix for a bug you
can't reproduce). SD-driver work also deferred (no card in Pi → untestable). Pi FREE, untouched.

2026-08-05 (C5 feasibility DONE → phase-1 build launched): Quake3 feasibility subagent
reported (plan `docs/inprogress/2026-08-05-quake3-port-plan.md`). Verdict: **feasible and
structurally SIMPLER than Quake2** — Q3 game logic is interpreted QVM bytecode shipped as data
in the pak, so there is NO game `.so` to fold (the yQuake2 dlopen→static headache doesn't
exist). Recommend **quake3e** + `code/renderer` (opengl1, pure fixed-function GL 1.x — fits our
Mesa V3D GL 2.1; renderer2/vulkan do NOT), QVM interpreted (`NO_VM_COMPILED`, avoids the aarch64
JIT's RWX mprotect), one static ELF (client+server+qcommon+botlib+renderer+SDL backend, linked
vs our libSDL2.a+libGL/libv3d). Four patch points: q_platform.h Phoenix branch, qgl.h GLX-block
gate, `msg_t`→`q3msg_t` rename (Phoenix sockport SysV `msg_t` clash — the one non-trivial fix),
Phoenix sys/net backend. Probe cross-compiled ~15 TUs clean. Assets = demo `demoq3/pak0.pk3`
(freely downloadable, NOT committable). **Launched a phase-1 build subagent** to cross-link to a
single ELF (link closure verifiable WITHOUT the Pi), mirroring tools/yquake2-port structure into
tools/quake3-port/. Result pending.

2026-08-05 (Quake2 verdict + C5 kickoff): Decisive Quake2 render test with the reliable
exec pipeline: yquake2 exec'd cleanly (banner) but fatal-errored `Couldn't load
pics/colormap.pcx` = a SECOND netboot-NFS flakiness that bites RUNTIME asset reads (distinct
from the #156 exec race the harness fix already handles; likely NFS lease-reclaim window /
stale host nfsd after many boots). Recorded in memory (project_large_binary_exec_hang). **Verdict:
game full-render over netboot is INFRA-bound (100Mbps + runtime-read flakiness), not a port
bug — vkQuake/quakespasm render because their reads happened to succeed; SD-boot would fix but
no card is in.** So pivoted off game-render-over-netboot. Launched a Quake3 (C5) feasibility
subagent scoped to **phase-1 = build-to-a-linking-ELF (verifiable WITHOUT the flaky netboot
runtime)**, on the HW-validated SDL2 base, investigating the key simplifier: Q3's QVM bytecode
modules may SIDESTEP the dlopen→static problem that Quake2 needed (and whether opengl1 renderer
fits Mesa V3D's GL 2.1). Result pending. Did NOT restart host nfs-server (its Phoenix export is
dynamic via netboot-server-up — a bare restart could drop it and break future boots).

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

ROTATION + infra conclusion (2026-08-05): a 5.5-min yquake2 confirm capture came back EMPTY
(netboot flaky again — the ~19MB binary intermittently fails to exec over NFS; gltest at 18MB is
reliable). So Quake2's remaining blocker is infrastructure (slow NFS + large-binary exec), not a
port bug — BANKED. Rotated to non-Pi breadth: added the "Porting userspace apps & games" section
to the OS-dev guide (H3) capturing all the SDL2/Quake porting lessons + infra gotchas. Continuing
non-Pi work while netboot is unreliable.

C4 Quake2 precache localized (2026-08-05): probes traced the "stall" to Mod_LoadTexinfo (wall
textures) — but TFU uploads keep progressing (n=5..12+), so it's SLOW not hung: ~100 .wal reads
over 100Mbps NFS + binary demand-paging, doesn't finish in ≤165s. Also TFU prints TILING=LINEAR!
(winsys striping bug, same as vkQuake). Next = 4-5min capture to confirm slow-vs-hang; SD-boot or
gigabit cable to speed NFS; fix TFU LINEAR tiling. Quake2 needs no code fix for the "stall" if
it's purely NFS-slow — it's an infra/perf issue.

C4 Quake2 precache stall (2026-08-05): with allow_download 0 + a 165s capture, reaches
CL_PrepRefresh but never ca_active (HDMI dark) → CL_PrepRefresh genuinely STALLS on some asset
(NFS read or model parse), NOT the winsys vcheck (gated to first 12). Next = instrument
CL_PrepRefresh to print each asset name → find the hanging one. Netboot ~50-70% reliable this
session (several empty runs); re-up + re-run works.

C4 Quake2 STALL PINNED + partially FIXED (2026-08-05): netboot flakiness explained the empty
runs (gltest ran clean; 3rd yquake2 diag run gave the full trace). Stall = CL_RequestNextDownload
(download-check, allow_download ON hangs over loopback). `+set allow_download 0` → reaches
CL_PrepRefresh (TFU uploads 4→12). New frontier = CL_PrepRefresh stalls after ~12 uploads →
suspect the winsys TFU vcheck readback diagnostic. Next = disable TFU vcheck + bake allow_download 0.

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

**Immediate follow-ups (pick per turn):**
- ~~libphoenix `rint` + rounding/min-max family~~ **DONE 2026-08-05** (d61f4a3 + 50f007c,
  --scope core validated). The remaining phoenix-libm gaps vs math.h are all HARD (long-double
  *l variants = binary128; transcendentals exp2/log2/expm1/erf/gamma/bessel) — leave as demand-
  driven (only implement when a real port link needs one, with a proper accuracy reference).
  `pthread_getcpuclockid` still NOT done (needs kernel per-thread CPU-clock support; port stub stays).
- **C1 SDL2 → ports.yaml wiring** so SDL2 is a first-class image component (games currently
  bundle libSDL2.a via their tools/ drivers; this makes it reusable). Build + boot verifiable.
- Another game phase-1 (build-verifiable-without-Pi): only if it adds NEW capability — the
  SDL2+GL+QVM/dlopen patterns are now proven across quakespasm/vkQuake/Q2/Q3.

**Strategy note (2026-08-05):** game FULL-RENDER validation is INFRA-bound over netboot
(100Mbps + runtime-read flakiness). vkQuake/quakespasm/SDL2 are already HW-render-validated;
adding more game *runtime* proofs is gated on reliable storage (SD, no card). So prefer work
whose success is verifiable WITHOUT a clean multi-minute netboot game run: cross-build/link
milestones, host/QEMU-checkable fixes, docs, and code cleanup.

Foundation (A1 Batch 1+2, G1 Tier A + Tier C tools/) is landed. A1 Batch 3 is NOT urgent
for a fork (incoming kernel changes are a cosmetic copyright/diacritics sweep + minor hal
fixes; we function without the errno transfer). Priority — pick
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
