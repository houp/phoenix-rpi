# W38, 2026-09-14 → 09-17 — fixes, verification, grader + doc audit

Archived from the weekly log 2026-09-18 to keep it readable. The current state lives in
[`docs/inprogress/WEEK-2026-W38.md`](../inprogress/WEEK-2026-W38.md).

**Fixed:** `vkq-tile-flicker` · vkQuake **loops its demos** · **every rebuild silently wiped the shader
cache** · **`q2-sdl-openaudio-hang`** bounded ~350 s → ~10 s and now names which half stalled
(devices `d13778d`) · upstream merge adopted (17 commits, two regressions caught).

**Verified on the current build:** libc **20 suites / 1 131 tests / 0 failures** · six-app gate **6/6**,
0 faults, torches present, real flipstat frames on every game · X desktop **31.7 min** netboot and
**~33 min** on the card · STK soak 10/10. ⚠ The GL window is bounded at 20 000 frames (~27–33 min) and
exits normally — relaunch it.

**★ THE HEAP GUARD NOW DIAGNOSES ITSELF** (libphoenix `8659311` + `e4f7c65`): "corrupt chunk header"
now prints `why=1..8` plus `lheap?`/`freed?`. All 8 codes covered host-side; re-gated 6/6. And the
bug is **not** in the allocator's logic or its locking — ~8 M operations (2.4 M single-thread, 3.2 M
concurrent over 8 threads against a real lock + ASan, 2.5 M on hardware) found **0 violations**. See
§ *the residue* below for what the archive says instead.
🔧 Both test-cycle scripts now **grade their own log** — nothing ever ran `uart-summary.sh` on the four
runs whose guards fired, which is the real reason they sat unnoticed for two days.

**★ GRADER + DOC AUDIT (2026-09-17):** **13 graders that could not fail** and **14 stale public-doc
claims**, all fixed. Worst was the six-app gate's `frames` column counting HDMI *snapshots*, so the
one column meant to catch a hung app could never be 0; and the README asserting the *opposite* of a
measurement in one paragraph while carrying the correction in another. All 83 archived gate logs
re-graded — no published claim changed. Full list: the done file above.

