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



## Weekly-log §2/§2b as it stood 2026-09-17 (moved here to keep the log short)

## 2. ✅ CARD FLASHED, IN THE PI, AND TESTED — nothing left for you here

You put the card on `/dev/sda`, I wrote `b95e983a` (read-back sha256 **byte-identical**, `e2fsck`
clean, full boot set + all showcase binaries; device checked before writing — USB `SDDR-409`,
removable, root on `nvme0n1p2`). You then put it in the Pi, and it has since been exercised hard:
**SD boot works, the showcase passes on it 17 of 18 app-runs across three passes** (the miss is one
Quake II hitting the known audio hang, §4), **0 faults, torches present**, and the
shader cache survives a reboot (§3b).

**Leave the card in.** Both lanes stay available: `netboot-server-up.sh` ⇒ netboot,
`netboot-server-down.sh` ⇒ SD boot. Nothing to unplug.
↩ **Two of my earlier claims here were wrong and are retracted:** a *blank* card does stop the Pi
booting entirely (0 DHCP, 0 UART, black HDMI over 4 power-ons) — but a *written* one does not, so the
"card vs netboot is either/or" line I wrote is false for the card you now have. The EEPROM's
network-first `BOOT_ORDER=0xf12` behaves exactly as documented.

ⓘ The card carries `b95e983a`, which **predates** tonight's two fixes (vkQuake demo loop, the
`rpi4-audio` self-test bound). Neither is needed for the demo as configured — the image ships no
`phoenix-demo.cfg`, and the audio stall is ~1.5% — so I have **not** re-flashed
it. Say the word and I will cut and write a fresh image.
⚠ **If Quake II ever hangs on the card, REBOOT — do not just relaunch.** Corrected 2026-09-17: I said
"cured by a relaunch", which is wrong for the card. The unfixed driver's boot self-test blocks its own
message loop for up to **~350 s**, so `/dev/audio0` is unopenable for that whole time and an immediate
relaunch hangs exactly the same way. It clears by itself after ~6 minutes, or instantly on a reboot.
(On the netboot build this is already bounded to ~10 s.) Symptom: the app stops right after
`SDL audio driver is "phoenix"`.
↩ **If the card ever misbehaves, the fallbacks are real:** `da752ac1` (pre-merge) and `16ad56f9` in
`artifacts/rpi4b/` both **verified intact 2026-09-17** — sha256 matches the name each is filed under, so
they are usable, not merely listed.
🧹 Housekeeping, your call: that directory holds **20 GB across 13 images**, 9 of them superseded. Disk
is not tight (137 GB free) and they are your build artifacts, so I have deleted nothing.

## 2b. ✅ REEL — settled per your notes (2026-09-16)

`artifacts/hdmi-video/20260916-160506-phoenix-rtos-rpi4-showcase.mp4` — 239 s, 11 segments, 11/11 on
`verify-demo-reel.py`, bt709-tagged.
✅ **Checked 2026-09-17 — upload it as-is, no remux needed:** H.264 **High @ L4.0**, 1920×1080p30, `yuv420p`, 4.9 Mbit/s, bt709 primaries+transfer, and **faststart is already set** (`moov` at byte 36, ahead of `mdat`) — so it starts playing before the whole 147 MB has downloaded. That is the property that usually needs a remux before a video is worth putting on a web page.

- **Caption raised** off the bottom edge (136–200 px), clear of the player's scrub bar and the games' HUDs.
- **Order unchanged**, **H.265 kept with your phone footage**, **vkQuake included** — your calls. The
  self-referential H.265 "alternative" I had built is deleted; it read as a game segment.
- **Every caption now carries a frame rate**, measured at the page flip with the winsys counter rather
  than read off a HUD — vkQuake **43 fps average**, SuperTuxKart **7–8**.
- ⚠ **vkQuake's number was re-derived after it disagreed with the screen.** The first "42" was a median
  over the whole capture, but vkQuake plays one demo then sits on a static console at a dead-flat
  42.2 fps — so that median measured the *console*. It was within 1 fps of right by luck. The demo
  itself is 2140 frames / 50.1 s = **42.7**; the caption says "average" because the engine's own counter
  is a ~1 s estimate swinging 23–65. STK checked the same way and is clean (race 7.95). Method is now
  documented above the segment table in `make-demo-reel.sh`.



## Weekly-log §3 as it stood 2026-09-17 16:45 (moved here to keep the log short)

## 3. ✅ This week — fixes, verification, publication hygiene

Full detail: [`docs/done/2026-09-17-w38-sd-lane-and-fixes.md`](../done/2026-09-17-w38-sd-lane-and-fixes.md)
· [`2026-09-16-w38-verification-detail.md`](../done/2026-09-16-w38-verification-detail.md)

**Fixed:** `vkq-tile-flicker` · vkQuake now **loops its demos** · **every rebuild was silently wiping the
shader cache** (`prepare-buildroot` deleted the driver fingerprint) · **`q2-sdl-openaudio-hang`** —
Quake II hanging at startup, root-caused to `rpi4-audio`'s boot self-test running *before* its message
loop; bounded ~350 s → ~10 s and now announces itself — and since 2026-09-17 that announcement carries `DMA_CS` + the ring cursors as well as `PWM_STA`, so one occurrence decides which half stalled (devices `d13778d`, re-gated **6/6**, 0 faults) · upstream merge adopted (17 commits, two
regressions caught).

**Verified on the current build:** libc **20 suites / 1 131 tests / 0 failures** · six-app gate **6/6**,
0 faults, torches present · X desktop **31.7 min** netboot and **~33 min on the card** · STK soak 10/10
clean. ⚠ The GL window is bounded at 20 000 frames (~27–33 min) and exits normally — relaunch it.

**★ HEAP GUARD NOW NAMES THE FAILING TEST** (libphoenix `8659311`, 2026-09-17; manifest
`2026-09-17-w38-malloc-why-gate.md`). `free()`/`realloc()`
printed "corrupt chunk header — leaking the block" without saying **which** of eight checks rejected
it — and they mean different things: a pointer into an already-RELEASED heap is a use-after-free of a
whole heap, a bad size is a smashed header in a live one. Now prints `why=1..8`. No behaviour change;
core+ports rebuilt, libc string/stdlib/stdio **383 tests / 0 failures**, stale census **0 of 354**,
six-app gate **6/6 rc 0** with real flipstat frames on every game (9635 / 10918 / 10623 / 8025 / 1920)
and torches present. All 8 codes covered host-side in `tools/malloc-harness` (13/13).
🔧 Both test-cycle scripts now **grade their own log** at the end (fault count + `ends_mid_line`) —
that is the actual reason those 4 fires sat unnoticed: nothing ran `uart-summary.sh` on them.
⏳ Running: 6 boots × 3 STK races (`whyhunt`), trying to catch a `why=` in the act.
🔎 Where it is **not**: the host harness compiles the real `malloc_dl.c`, and **8 seeds × 300 k ops
(~2.4 M operations, ~75 k heap mmap/munmap cycles)** came back with **0** invariant violations — so
the single-threaded bookkeeping is clean and the untested axis is **concurrency**, which is what both
apps that fired have in common. New `bin/mtstress` (N threads, tagged blocks, heap-churning size mix)
is built and staged for the next free Pi slot.
Why it matters: the guards fired 4× since 2026-09-12 with **no fault at all**, so those runs graded
clean (§4).

**★ GRADER AUDIT (2026-09-17)** — a read-only sweep of every grading script for the "cannot fail"
defect class found **eleven more** after yesterday's two. ⚠ **The six-app gate was the worst of
them:** its `frames` column counted HDMI *snapshots* (the capture card grabs one every ~25 s whether
or not the Pi drew anything), so the one column its own comment called "the real evidence for a GPU
app, 0 for a hung app" could never be 0; its fault regex was a private copy frozen before the
allocator patterns existed; and its log pickup had no lower time bound. Now: two columns (`frames`
= winsys page flips, `snaps` = grabs), the fault set comes from `uart-summary.sh`, a GPU app that
flips no frame FAILS, and the #67 torch verdict is folded into the exit status. Re-validated on HARDWARE, both lanes: card/qspasm **frames 9471, snaps 25, faults 0, rc 0** (and the
torch check correctly skipped, since vkQuake was not in that run) · netboot/vkq **frames 8153,
snaps 21, faults 0, torches PRESENT, rc 0**. Also time-bounded `check-torch-rois` (an earlier
run's frames could satisfy this run's verdict) and `test-nfs-recovery`'s post-restart criterion. All fixed, each with a negative control
where one exists: `grade-app-capture` passed on 10/20/30/100 faults (`endswith("0")`) ·
`check-stale-binaries` called a **zero-binary census** a PASS and never scanned `/sbin` (census
333 → 354 binaries) · `verify-demo-reel` graded an **empty frame window** "ok" (a 40 s cut of the
reel now FAILs; the real reel still 11/11) · `verify-sd-image-contents` ended in "safe to flash"
after SKIPping the **whole FAT partition** (now UNVERIFIED, rc 3) · `qemu-boot-sdimage`'s
wrong-variant warning never touched rc · the rebuild's "DO NOT SHIP THIS BUILD" was a warning the
build ignored · `verify-rpi4b-sdimg` ignored `$1` and verified a different image.
✅ **No published claim changes:** all **83 archived gate logs** re-graded with the full fault set —
the gate's narrower regex missed nothing (the only 2 differences are the benign `vm: page init done`
banner).

**Two graders that could not fail**, both fixed: `uart-summary`'s "netif has IP" passed with *no*
address; the gate's `launched` column passed on psh's own echo. ⚠ Grade GPU apps by flipstat frames.

**Publication hygiene:** `q3-qvm-recipe` **closed** — the QVM pak rebuilds from source, verified
byte-wise (2 of 3 QVMs identical, the third differs only in `__DATE__`) · tech-debt register reconciled
(6 stale entries, incl. TD-19's retracted `dsb; isb` claim) · `KNOWN-ISSUES` audited (SD row was false,
two rows weren't rendering, three over-claims in the binner-wedge cell) · hevc testdata README
understated 13.3 MB of tracked video — **your personal footage is still not committed**.



## Weekly-log §3 as it stood 2026-09-17 18:00 (moved here to keep the log short)

## 3. ✅ This week — fixes, verification, publication hygiene

Full detail: [`docs/done/2026-09-17-w38-sd-lane-and-fixes.md`](../done/2026-09-17-w38-sd-lane-and-fixes.md)
· [`2026-09-16-w38-verification-detail.md`](../done/2026-09-16-w38-verification-detail.md)

**Fixed:** `vkq-tile-flicker` · vkQuake **loops its demos** · **every rebuild silently wiped the shader
cache** · **`q2-sdl-openaudio-hang`** bounded ~350 s → ~10 s and now announces itself with `DMA_CS` +
ring cursors (devices `d13778d`) · upstream merge adopted (17 commits, two regressions caught).

**Verified on the current build:** libc **20 suites / 1 131 tests / 0 failures** · six-app gate **6/6**,
0 faults, torches present, real flipstat frames on every game · X desktop **31.7 min** netboot and
**~33 min** on the card · STK soak 10/10. ⚠ The GL window is bounded at 20 000 frames (~27–33 min) and
exits normally — relaunch it.

**★ HEAP GUARD NOW NAMES THE FAILING TEST** (libphoenix `8659311`; manifest
`2026-09-17-w38-malloc-why-gate.md`). "corrupt chunk header" never said **which** of eight checks
rejected the block — a pointer into a RELEASED heap and a smashed size want opposite hunts. Now
`why=1..8`, every code covered host-side (13/13). Re-gated **6/6 rc 0**, libc **383/0**, stale census
**0 of 354**.
🔎 **Not** the allocator's own logic, single- or multi-threaded: the host harness (the real
`malloc_dl.c`) ran **2.4 M single-threaded ops / 75 k heap cycles** and then — after its mutex stub
was made a **real** pthread lock — **3.2 M concurrent ops across 8 threads** plus an ASan run, all
with **0 violations and 0 tag mismatches**. What that leaves is Phoenix-specific: the kernel mutex
under SMP, `munmap` of a released heap, or a writer outside the allocator. ⊕ **On target now:** two new libc tests (`tests f8e0a39`) — the suite had a mutual-exclusion test for
**spinlocks only** — show the pthread mutex **and** the raw kernel mutex the allocator uses both
serialize 4×20 000 non-atomic increments: **PASS, 8 tests 0 failures**. `bin/mtstress` passed in the
same boot and then under load: **8×200 k, 16×150 k and 4×400 k ops on hardware — 2.5 M allocs/frees,
671 k reallocs, 0 tag mismatches, 0 fault lines**. ⇒ **the mutex is not the explanation.**
What is left: a writer **outside** the allocator, or something Phoenix-specific about `munmap` of a
released heap. ⓘ Every field fire came from a run doing heavy **GPU** work, which mtstress does not
touch — and in the 71-fire run **no traced BO mapping landed anywhere near the affected heap**.
🔧 So the next fire is made self-diagnosing instead: the report now also prints **`lheap?` / `freed?`**
(libphoenix `e4f7c65`), which separates "a block outlived its heap" from "stale pointer, address
reused" from "smashed `->heap`". Verified on HW: stdlib **93/0**, pthread newlocks **8/0**, mtstress
8×100 k clean, quakespasm 4 269 frames, 0 fault lines; manifest
`2026-09-17-w38-malloc-heap-provenance.md`; six-app gate running.
🔧 Both test-cycle scripts now **grade their own log** — nothing ever ran `uart-summary.sh` on the four
runs whose guards fired, which is the actual reason they sat unnoticed for two days.
⏳ Running: 6 boots × 3 STK races (`whyhunt`), trying to catch a `why=` in the act.

**★ GRADER AUDIT (2026-09-17)** — **13 graders that could not fail**, all fixed, each with a negative
control where one exists. Worst was the six-app gate itself: its `frames` column counted HDMI
*snapshots* (the capture card grabs one every ~25 s regardless), so the one column meant to catch a
hung app could never be 0; its fault regex was a private copy frozen before the allocator patterns
existed; its log pickup had no lower time bound. Now `frames` = winsys page flips, `snaps` = grabs,
the fault set comes from `uart-summary.sh`, a GPU app that flips no frame FAILS, and the #67 torch
verdict is folded into the exit status — re-validated on hardware on both lanes.
✅ **No published claim changes:** all **83 archived gate logs** re-graded with the full fault set; the
narrower regex had missed nothing. Full list of the thirteen: the done file above.

**Publication hygiene:** `q3-qvm-recipe` **closed** — the QVM pak rebuilds from source, verified
byte-wise · tech-debt register reconciled (6 stale entries, incl. TD-19's retracted `dsb; isb` claim)
· `KNOWN-ISSUES` audited · hevc testdata README understated 13.3 MB of tracked video — **your personal
footage is still not committed**.



## Weekly-log §1 as it stood 2026-09-17 23:00 (the overnight SD-lane table)

## 1. OVERNIGHT (2026-09-16 → 17) — nothing blocking, two calls are yours

You flashed the card via me and left it in the Pi. **The SD lane — the one thing this project had never
exercised — is now verified on every axis I can measure without you:**

| check | result |
|---|---|
| SD boot reliability | **6/6**, 0 faults |
| Six-app showcase **on the card** | **17/18** across THREE passes. ⚠ *Corrected 2026-09-17:* I reported 12/12 for the first two — one Quake II run (`sdgate2`) hit `q2-sdl-openaudio-hang` and drew **nothing** (26 black frames, 0 flipstat), and the gate's columns could not see it. The third pass is 6/6 with frames on every app. |
| #67 vkQuake torches **on the card** | **8/8** (6-trial rate + 2 gate runs) |
| Shader cache across an SD reboot | **persists** — 27 blobs after a boot that ran no GPU app |
| X desktop endurance **on the card** | **~33 min**, 0 faults, Life still ticking at the end |
| vkQuake endurance **on the card** | **30 min, 51 778 frames, 0 faults** — 26.1 fps at the start, **28.2 at the end** (no decay), torches still lit in the final frame |
| SuperTuxKart endurance **on the card** | **~24 min racing, 11 262 frames, 0 faults and 0 allocator/guard events** — 8.9 → 8.7 fps. The app with the crash-family history, sustained. ⓘ Cut by my window, not finished: a 15-lap profile, so no lap summary |
| Netboot tree (carries tonight's fixes) | gated **6/6**, 0 faults |
| libc suites after the core rebuild | **ALL 20 suites, 1 131 tests, 0 failures**, 0 faults (`libcverify` + `libcA` + `libcB`) |

⏸ **Your two calls, neither urgent:**
1. **Re-flash the card?** It carries `b95e983a`, which predates the vkQuake loop fix and the audio bound.
   Neither is needed for the demo as configured, so I left your verified card alone.
2. **TD-19 — add the `isb` to the TLBI helpers?** The doc's `dsb; isb` claim was false and is retracted;
   per ARM ARM the sequence is arguably incomplete. I deliberately did **not** touch it: a barrier on
   every TLBI is a global change to a correctness-sensitive path and wants you watching.

ⓘ **Leave the card in.** Bench is on the **netboot** default; both lanes are one command apart
(`netboot-server-down.sh` ⇒ SD boot, `-up` ⇒ netboot). The SD-lane facts and harness are in durable
memory, so the next session starts knowing them.



## Grader + doc audit, full list (moved from the weekly log 2026-09-18)

**★ GRADER + DOC AUDIT (2026-09-17)** — **13 graders that could not fail**, all fixed with negative
controls. Worst was the six-app gate: its `frames` column counted HDMI *snapshots*, so the column
meant to catch a hung app could never be 0; its fault regex was a frozen private copy; its log pickup
had no time bound. Now `frames` = page flips, `snaps` = grabs, fault set from `uart-summary.sh`, a
no-frame GPU app FAILS, torch verdict in the exit status. ✅ All **83 archived gate logs** re-graded —
no published claim changed. Then the same sweep over the **public docs** found **14** stale claims
(README asserting the *opposite* of a measurement in one paragraph and the correction in another; the
"authoritative" hardware matrix still carrying #67 as open, missing torches, the closed QVM gap and
"HW-blocked (no card)"; `CHANGES` still calling SD boot unverified) — all fixed.

**Publication hygiene:** `q3-qvm-recipe` **closed** (pak reproducible from source, verified byte-wise)
· tech-debt register reconciled · hevc testdata README understated 13.3 MB — **your personal footage
is still not committed**.
