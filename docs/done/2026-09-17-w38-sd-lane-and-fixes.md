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

Running the SD showcase **twice** (12/12, 0 faults, torches present both times) turned up better
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

