# W38 detail — the SD lane, and the fixes it surfaced (2026-09-16/17)

*Archived from `docs/inprogress/WEEK-2026-W38.md` to keep the weekly log short. This is the full
working detail for the night the SD card lane was exercised for the first time.*

## 3c. ✅ vkQuake now LOOPS its demos (HW-verified)

vkQuake played **one** demo (~50 s) then dropped to console while QuakeSpasm looped. It was **one line**:
the bring-up patch in `Host_Startdemos_f` set `cls.demonum = -1`, which permanently disarms Quake's demo
loop, so when the port's `playdemo` ended `Host_EndGame` disconnected instead of calling `CL_NextDemo`.

**HW-verified** (`vkq-loop2`): `demo2 → demo1 → demo2 → demo3 → demo1` — four self-driven transitions,
including the wrap. Flip rate stays in the variable 25–86 fps band with **no flat ~42 fps console block**.
Fork `c55d7d8`, ports `b0fc510`. Only applies when `id1/phoenix-demo.cfg` is staged; the shipped image has
none and boots `map start`, which disarms the loop by itself — so **the default image is unchanged**.

⚠ **Attempt 1 failed on hardware, and the cause was my own fix.** It armed `demonum` and then inserted
`menu_main`; `M_Menu_Main_f` stashes `demonum` in `m_save_demonum` and sets it to **-1**, restoring it only
from `M_Main_Key`'s ESCAPE case — a keypress that never arrives on a headless board. So the menu call
silently disarmed the loop it had just armed. Worth keeping: the old hack's explicit `-1` was only ever
redundant with the menu's own effect. It was caught in one cycle because the test demands ≥2 ordered
`Playing demo from …` lines and `flipstat-summary.sh --seq` shows the console as a flat block.
Both attempts are on `publish`: `c55d7d8`/`b0fc510` is the verified one, `abd06c4`/`8169648` the
intermediate hardware rejected — history, not a rollback target.

## 3d. 🐞 FIXED — every rebuild was silently wiping the Pi's shader cache

`sync-netboot-tree.sh` keeps the shader cache unless the GPU driver changed, keyed on a fingerprint file
at the buildroot root — but **`prepare-buildroot.sh`'s `rsync --delete` deleted that fingerprint on every
prepare**, so the next sync wiped the cache regardless. The 2026-09-08 optimisation had been inert since
it landed. Fixed (`4bfdfb135`), proven with both controls and end-to-end (`prepare` → sync = `KEPT`).

**Why it matters:** a cold cache is **~67 s of black screen for vkQuake**, misread as a hang twice.
⚠ **Correction, mine:** the cache does *not* save STK ~35 s — a controlled probe shows 50 shaders
"compiling" with zero new entries, all hits, run still 49.8 s. Real saving ~5.5 s. Fixed in
`KNOWN-ISSUES.md`, the README, and the script comment that spawned it.
👁 Invalidation now rests entirely on the fingerprint; green speckle after a driver change points there.
Full map: [`docs/misc/2026-09-16-shader-cache-map.md`](../misc/2026-09-16-shader-cache-map.md).

## 3e. ✅ Gate re-run on the post-loop-fix tree — 6/6, 0 faults, torches PRESENT

Two things had landed since the last gate — the vkQuake port (`b0fc510`) and the build tooling — and the
vkQuake change touches `Host_Startdemos_f`, which runs on **every** boot including the shipped
`map start` path that #67's torch check grades. So it needed re-gating rather than assuming.

**6/6 mechanical** (rc 0, prompt, 0 faults, launched, 23–24 frames each) · **#67 torches PRESENT**
(20/20 at-viewpoint frames) · **6/6 content** by eye: vkQuake on the START map with both torches lit,
QuakeSpasm 48 fps, Quake II 38.8, Quake III 27 on q3dm1 with a bot in frame, STK racing lap 2/2 at
Hacienda, and the X desktop with all five clients drawn (GL pinwheel, XBill, xclock, Life, top).
Manifest `manifests/2026-09-16-w38-postloop-gate.md`.

## 3g. ★ SD BOOT WORKS — first time ever on this bench, and the cache persists

You flashed nothing by hand: the card went to `/dev/sda`, I wrote `b95e983a` (read-back sha256
byte-identical) and you put it in the Pi. It boots.

- **Phoenix boots from the card**: plo → kernel → fbcon → pcie → xhci → **psh**, `mmcblk0p2` ext2 root,
  **0 faults**. Verified twice.
- **QuakeSpasm runs off the card** at **37–42 fps**, 11 419 frames, real gameplay.
- ★ **The shader cache lands on the SD root AND survives a reboot.** Boot 1 (cold, ran QuakeSpasm) wrote
  it; boot 2 ran **no GPU app** and still listed **27 blobs** in `/.mesa-shader-cache/v1`, directory
  stamped from boot 1. ⇒ **A presenter pays the cold start once per card per app, not every launch.**
  That was the open question in the shader-cache map — now answered on the real lane.

★★ **The whole showcase passes ON THE CARD — 6/6, 0 faults, torches PRESENT.** New
`run-showcase-gate.sh --sd-boot` mode; every gate before tonight graded the netboot tree, never the
artifact you present from. Content-checked by eye too: Quake III mid-deathmatch with a live bot
scoreboard, vkQuake's START map with both torches lit, STK racing lap 1/2, Quake II lit corridor,
QuakeSpasm 31 fps, and the X desktop with all five clients drawn. Confirmed genuinely SD-booted — the
Pi's last DHCP was 23:32, before the gate began.

✅ **Correction — the card is NOT an either/or, leave it in.** I said earlier tonight that netboot was
unavailable while the card is in. That is true only of a *blank* card. With the **written** card in:
dnsmasq **up** → the Pi netboots (verified: nfs takeover, psh, 0 faults); dnsmasq **down** → it
SD-boots. So both lanes are available and switching costs one `netboot-server-{up,down}.sh` plus a power
cycle — nothing to unplug.

## 3f. ✅ Stability soak — 10× SuperTuxKart, all clean

10 back-to-back STK races (`stksoak2`) — the app the "page reads back zero" family hits hardest — with
the 2026-09-15 guards in place. **10/10 raced, 0 faults, 0 allocator events, 0 guard fires**, boot stages
10/10, frame counts tight at 1607–1672 (±2%) so nothing degraded silently either.
⚠ Ten clean runs **bound** an intermittent, they do not clear it — at the historic ~1-in-10 rate that is
about a coin flip's worth of evidence. The family stays open and livable (§4).

## 3i. 🐞 Two graders that could not fail — and the bug one of them was hiding

Running the SD showcase **twice** (reported 12/12 at the time; **re-graded 2026-09-17 as 11/12** —
`sdgate2`'s Quake II hung in `SDL_OpenAudio` and drew nothing, which the gate could not see because
its `frames` column counted HDMI snapshots) turned up better
evidence than the timing I was after:

- ✅ **`q2-sdl-openaudio-hang` — ROOT-CAUSED AND FIXED** (devices `efac488`, manifest
  `2026-09-17-audio-selftest-bound.md`). `rpi4-audio`'s boot self-test runs **before** its message loop,
  so until it finishes nothing is served — while `/dev/audio0` already exists and has printed `ready`,
  which is why `open()` *blocks* instead of failing. It was never infinite: `audio_write` bounds a stuck
  DMA at ~10 s and the self-test feeds 35 chunks (the healthy `fed 8960 samples` = 35×256), so the stall
  is up to **~350 s** — longer than the 300 s capture, which is what made it look like a hang. Now it
  stops at the first short write and says so: worst case **~350 s → ~10 s**. HW-verified: healthy path
  unchanged (8960 samples, underruns=0, DMA), Quake II reaches `SDL audio initialized` and renders 6331
  frames. ⓘ The underlying DMA stall is not fixed — it is bounded and announced instead of silent.
  ✅ **Re-gated after the core change: 6/6, 0 faults, torches present**, and every GPU app rendered —
  Quake II **10 622** frames (the app that hung), QuakeSpasm 9 640, Quake III 10 050, vkQuake 8 094,
  STK 1 824; X desktop drawn with all five clients. Manifest `2026-09-17-audio-selftest-bound.md`.
  🔧 `uart-summary.sh` now prints a **WARNINGS** section for `self-test ABORTED` and `shader cache COLD`
  — bounding that stall is only useful if somebody notices it happened.
- 🐞 **How it was found.** In gate 2 Quake II rendered **zero frames** — while the
  gate scored it `rc=0, prompt yes, 0 faults, launched yes`. It printed `SDL audio driver is "phoenix"`
  and then nothing for 300 s, still on the console. Localised to `SDL_OpenAudio()` never *returning*
  (its failure branch would have printed, and did not) → `PHOENIXAUDIO_OpenDevice`, whose only blocking
  syscall is `open("/dev/audio0", O_WRONLY)`. **Rate 2/136** runs (~1.5%), 2026-09-04 netboot and
  tonight on SD — **pre-existing, not an SD regression**. Relaunch works. Next step: bound that open so
  a stall becomes an error instead of silence.
- **Two harness stages that could never fail**, both fixed: `uart-summary.sh`'s "netif has IP" matched
  `dhcp_start: 0` and literally `netif waits for OFFER` — it passed when the netif had *no* address
  (caught on SD with dnsmasq down: link up, no DHCP server, "has IP" ✓). Split into "dhcp requested" +
  a "netif has IP" that requires a real dotted address. And the gate's `launched` column greps for the
  command string, which **psh echoes** — renamed `cmd-echo`, since the real evidence is `frames`.
- ⚠ **Grade GPU apps by flipstat frames, never by the gate's mechanical columns.** That is the lesson
  the Quake II hang paid for.

## 3h. 🧹 Tech-debt register reconciled against the code (pre-publication)

Audited every `TD-xx` entry against the tree. Five real discrepancies, **all fixed as documentation —
no code touched**:
- **TD-19**, the one the register itself asked to settle before publishing: both its hypotheses are
  false. Neither the generic TLBI helpers (`aarch64.h:198-239`) nor `_pmap_writeTtl3` has an `isb`;
  `pmap.c`'s only two bracket the TTBR switch. The `dsb; isb` claim is **retracted**. Per ARM ARM
  D8.16.1 the sequence is arguably incomplete — ⏸ **left for you**: adding a barrier to every TLBI is a
  global change to a correctness-sensitive path and should not be made unattended overnight.
- `TD-Git-Branches` described four branches that no longer exist (all repos are on `master`);
  `TD-Eth-DHCP` described a static-IP workaround the driver no longer has; `TD-14-psh-retry` was marked
  "superseded, n/a" while its marker and its deviation from upstream are both live; **TD-21**,
  `TD-13-mtxbypass` and `TD-14-startup-settle` had no row in the checklist the header calls authoritative.
- Registered three debt namespaces the series never mentioned: `TD-USB` (VL805 async-firmware race),
  `TD-STK-SWPRINTF` (⚠ its condition is genuinely unmet — libphoenix still has no `swprintf`), and
  `TODO(vkquake-port)`. Reviewed the stubbed `vkCmdSetDepthBias`: a deliberate guard against a
  PC-alignment fault, cost is z-fighting cosmetics — not an unfinished port.

## 3j. ⚠ SD boot-reliability bench NOT delivered — my measurement broke, the Pi is fine

Wanted a boot-success rate for the card. Three attempts died to the tool timeout (`rc=143`, killed
mid-run — the truncated output reads exactly like a script bug, and I went looking for one before
checking the return code). I then hand-rolled a `for` loop, which **corrupted the next boot into a
runaway kernel print loop**: a 60 MB log, **2 057 279 lines** before the psh prompt, 3 unique lines.
A healthy boot has **one** `map: enter` and a ~7 KB log.

✅ **Bench restored** — `pi_power_off` + 45 s settle, then a clean SD cycle: 7 KB, psh, 0 faults. The
Pi and the card are fine; nothing to act on. Both lessons are in the `rpi4-run` skill (use `setsid` for
long runs; never hand-roll cycle loops — the bench/gate scripts pace themselves).
⏭ The reliability number is still unmeasured; `test-cycle-bench.sh --sd-boot` now exists for it.



---

# Weekly-log sections 3–3e, archived 2026-09-17 08:57

*Moved out of `docs/inprogress/WEEK-2026-W38.md` to keep the board short; content verbatim.*

## 3. ✅ Verified this week (detail: [`docs/done/2026-09-16-w38-verification-detail.md`](../done/2026-09-16-w38-verification-detail.md))

- **`vkq-tile-flicker` FIXED** (ports `7ed2949`) — vkQuake rendered into the buffer being scanned out;
  it now page-flips. Not a lightmap bug. The winsys already had the machinery; vkQuake never opted in.
- **Upstream merge adopted** — 17 commits, two regressions found and fixed (syscall ABI break; the
  `CLOCK_REALTIME` cond default). Merged tree **12/12** across two gate passes.
- **The desktop survives a talk** — `startx_gpu action` **31.7 min, 0 faults**, still *animating* at the
  end (tick-to-tick 4.32 late vs 5.95 mid; presence alone would not have shown that).
- **STK's ~8.5 fps is not fill-bound** — 720p gives 8.43–8.52 vs 1080p 8.40–8.53. No cheap win, left
  alone. [`docs/misc/2026-09-16-stk-fps-not-fill-bound.md`](../misc/2026-09-16-stk-fps-not-fill-bound.md)
- **1050 libc tests, 17 suites, 0 failures** · stale-binary census clean · netboot proven by a real boot.
- 🔧 New: `scripts/check-stale-binaries.sh`, `scripts/verify-demo-reel.py`, `tools/demo-apps/webroot/`.

## 3b. ✅ This week's fixes — detail in [`docs/done/2026-09-17-w38-sd-lane-and-fixes.md`](../done/2026-09-17-w38-sd-lane-and-fixes.md)

★ **SD boot works, first time ever on this bench** — you flashed the card via me and it boots: psh,
`mmcblk0p2` ext2 root, 0 faults. The **showcase passes on the card: 17 of 18 app-runs over three
passes** (one Quake II hit `q2-sdl-openaudio-hang`), **0 faults,
torches present.** The shader cache lands on the SD root and **survives a reboot** (27 blobs after a
boot that ran no GPU app), so a cold start is paid once per card per app.
✅ **Both lanes stay available with the card in** — dnsmasq up ⇒ netboot, down ⇒ SD boot.

Fixed this week:
- **vkQuake loops its demos** (one line: `cls.demonum = -1` disarmed the loop). Fork `c55d7d8`.
- **Every rebuild was silently wiping the shader cache** — `prepare-buildroot.sh` deleted the driver
  fingerprint, so the 2026-09-08 keep-optimisation had been inert since it landed. Coord `4bfdfb135`.
- **`q2-sdl-openaudio-hang` root-caused and fixed** (devices `efac488`): `rpi4-audio`'s boot self-test
  runs *before* its message loop, so a stuck DMA left `/dev/audio0` unopenable for up to ~350 s.
  Bounded to ~10 s and announced. Re-gated **6/6, 0 faults**, every GPU app rendering.
- **Two graders that could not fail**: `uart-summary`'s "netif has IP" passed with no address; the
  gate's `launched` column passed on psh's own echo. Both fixed — ⚠ grade GPU apps by flipstat frames.
- **Tech-debt register reconciled** against the code (6 stale entries, incl. TD-19's retracted
  `dsb; isb` claim — code deliberately untouched, it needs an attended decision).

✅ **SD boot reliability: 6/6 clean boots off the card** (`sdrel5`) — kernel banner, fbcon, psh prompt,
lwIP and genet link all 6/6, **0 faults**, every trial confirmed on the SD root (`mmcblk0p2:ext2`) and
`uname` answering, logs a tight 7 265–7 380 bytes. ⓘ `netif has IP` reads **0/6**, which is the
*correct* answer — dnsmasq is down for SD boot, and that stage only started telling the truth after
this morning's grader fix; it would have falsely reported 6/6 before.
↩ Earlier in the night I could not get this number: three benches were killed by the tool timeout and a
hand-rolled loop corrupted a boot into a 2-million-line print loop (bench restored, Pi verified fine).
The fix was to route trials through the psh-interact path the gate already loops reliably.

## 3c. Evidence behind the §1 table (all on the card unless noted)

- **X endurance ~33 min, 0 faults.** At the end Life still ticking (gen 31 722, 17.6 gen/s), `xclock`,
  `xbill`, `top` live. ⚠ The GL window is **bounded at 20 000 frames** (24.7 fps → 12.3 fps, ~27–33 min)
  and exits normally — that is why it is missing from the final frame. Relaunch it for a longer talk.
- **#67 torches 6/6** (`sdtorch`): 15 at-viewpoint frames per trial, 599/553 lit px vs a threshold of 8,
  0 faults. `#67` was closed five times historically on single screenshots, so it is scored by rate.
- **Audio stall hunted, did not reproduce**: 20 Quake II starts on the fixed build (netboot — the card
  lacks the fix), 20/20 reached `SDL audio initialized`. **44 boots since the fix, 0 aborts.**
  ⚠ That narrows the rate; it does not clear the bug. At ~1.5% a clean 20 is ~3-in-4 likely anyway, and
  the abort path still has never fired on hardware.

## 3d. ✅ Quake III's QVM pak is reproducible from source — one fewer publication blocker

`q3-qvm-recipe` was the last **reproducibility** gap on the open list: Quake III's `pak1.pk3` (three
QVMs, needed because the free demo's 1999 UI reports API 3 and the engine wants 6) was described as a
hand-staged binary with no recipe. That has not been true since 2026-09-03 — the row just never caught
up, and an earlier interrupted run left `tools/quake3-vm/build/vm/` empty, which made it look unfixed.

Ran `tools/quake3-vm/build-quake3-vms.sh` end to end: it builds the whole host toolchain (`lburg` →
`q3rcc`, `q3cpp`, `q3lcc`, `q3asm`) and all three QVMs from ioquake3 pinned at `5883936`, with the
UI/GAME/CGAME API cross-checks passing. **Verified against the committed pak rather than assumed:**
`cgame.qvm` and `ui.qvm` are **byte-identical**; `qagame.qvm` differs in exactly **5 bytes**, which are
the embedded `gamedate` string (`__DATE__`). So it is **bit-for-bit deterministic apart from the build
date**. Committed asset untouched (no `--install`).

## 3e. 🧹 Pre-publication sweep — deliverables re-verified, three doc/tree mismatches closed

- **Deliverables intact and still passing their own gates**: image sha matches its name
  (`b95e983a56be…`) and `verify-sd-image-contents.sh` **PASSES** (FAT boot set, `arm_64bit=1`, ext2
  consistent 10 505 files, `loader.disk` boots `mmcblk0p2:ext2` with no NFS root); reel decodes at
  239.0 s and `verify-demo-reel.py` is **11/11**.
- **`q3-qvm-recipe` closed** — reproducible from source, verified byte-wise (§3d).
- **`tools/hevc-decode/testdata/README.md` understated what is committed**: it claimed the tracked
  `.265` files are "conformance vectors (~12 kB each)", but 35 files / **13.3 MB** are tracked including
  two multi-MB demo clips. Corrected — and the line that matters is intact: your **personal** footage
  (`IMG_8331-phoenix.265`, 19.7 MB) is **not** committed; the non-personal `showcase1080.265` is.
- **`freebin` signature B census on the current build: 0 in 31 GPU runs.** ⚠ A bound, not a cure — the
  honest denominator is GPU runs (31), not every run reaching psh (47).

