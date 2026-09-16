# 2026-09-16 — W38 verification detail (shipped work, soak, STK fps, vkQuake demo-loop option)

Archived from the weekly log to keep it short. The live log keeps the decisions and the verdicts.

## 3. ✅ Shipped today

- **`vkq-tile-flicker` FIXED** (ports `7ed2949`, port-only). vkQuake had no WSI swapchain: it rendered
  into ONE fb0-backed image and `storeOp=STORE` *was* the present, i.e. it wrote the buffer the display
  was scanning out. V3D is tile-based and stores tiles one at a time, so a refresh landing mid-store
  showed half-new frames — hard-edged, screen-axis-aligned blocks for one frame. **Not a lightmap bug.**
  ★ The winsys already had page-flip and the GL path already used it (which is why QuakeSpasm was the
  clean control); vkQuake just never called `scanout_init`, so `v3d_phoenix_flip()` was a silent no-op.
  HW: `3 buffer(s) TRIPLE-BUFFER+page-flip`, 0 faults, render unchanged. Escape hatch
  `VKQ_SINGLE_BUFFER=1`. ⚠ The flicker **rate is unmeasured** — mechanism gone by construction, but 7+
  detector variants failed, so no before/after percentage is quoted.
  Analysis: [`docs/misc/2026-09-15-vkq-tile-flicker-root-cause.md`](../misc/2026-09-15-vkq-tile-flicker-root-cause.md).
- **The reel**, 11/11 segments re-captured, plus `scripts/verify-demo-reel.py` to gate any future one.
- **`tools/demo-apps/webroot/`** — the page Dillo fetches, which nothing in the tree used to serve.

↩ **Retracted today, so you don't chase them:** `fbcon-freeze` (not a bug — a static screen caused by my
own harness pacing, `--idle-secs` > recording length; fixed with `REC_IDLE_SECS` + a warning), and
"python3/dillo/mc are missing from the image" (they are present; `ls a b` fails if *either* operand is
missing).

## 3a. ✅ The desktop survives a talk-length run — 31.7 min, 0 faults

Every test until now was short: the showcase gate gives each app ~4 minutes and the longest soak on
record was 10. A presentation is 30–60. So: **31.7 minutes of `startx_gpu action`, 0 faults, 0
heap-canary or allocator events**, 126 HDMI ticks.
★ And it is still **animating** at the end, not merely present — tick-to-tick change **4.32** over the
last ten ticks against **5.95** mid-run (0 would be frozen). Checked because "90% non-black" alone
cannot tell a live desktop from a frozen one, which is a mistake I have already made once this week.
ⓘ The colour count falls (11381 → 2138) as Conway's Life converges and the clock stops sweeping — less
on screen changing, not a stall. Manifest `manifests/2026-09-16-xsoak-30min.md`.

## 3c. ⏸ OPTIONAL — vkQuake could loop demos like QuakeSpasm (your call)

vkQuake plays **one** demo (~60 s) then drops to the console; QuakeSpasm loops demo1→2→3→1. Verified
from both UART logs. Now in the README's live-demo notes.

**Why, and the fix.** `Host_Startdemos_f` in our fork has a bring-up patch — an unconditional
`if (1) { cls.demonum = -1; menu_main; return; }` — carrying
`TODO(vkquake-port): restore the demo loop once the world render path is proven`. **That path is now
proven** (6/6 gates, demos render, torches present). But `cl_startdemos` defaults to `1`, so simply
deleting the block would let `quake.rc`'s `startdemos` load a world **during `Host_Init`**, which is
exactly what the patch prevents. The clean version: make the skip conditional on a flag the glue clears
**after** `Host_Init`, then have the glue issue `startdemos <name>` in place of `playdemo <name>`.

**I have not done it.** It changes the boot path of a working showcase app, needs a fork patch regen,
and buys convenience rather than correctness — not a risk worth taking on my own judgement while you
are away. Say the word and it is maybe an hour including a gate.

## 3b. ⓘ STK's ~8.5 fps: measured, and there is no cheap win

You noted STK is "still not smooth". The cheapest possible fix would be a lower render resolution — at
1280×720 it runs **8.43–8.52 fps**, against **8.40–8.53** at 1920×1080. **Identical**, so it is **not
fill-rate bound** and dropping resolution buys nothing. What is left is draw-call/submit-side work with
uncertain payoff, so I have **left it alone**.
[`docs/misc/2026-09-16-stk-fps-not-fill-bound.md`](../misc/2026-09-16-stk-fps-not-fill-bound.md).
