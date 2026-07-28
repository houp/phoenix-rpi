# Model-gallery test environment — plan (2026-07-28)

Goal: a deterministic, comprehensive test that renders EVERY alias model individually and
tells us — with certainty, not eyeballing a live demo — which models render correctly on
the Pi's V3D vs a known-correct reference. Use it to find ALL broken models (weapons,
monsters, torches, items), drive fixes to green, then verify the fixes in the real game.

Motivation: diagnosing #67 bug-2 (nailgun) in the live game failed repeatedly — demos don't
frame pickups, ad-hoc harnesses render through non-game paths and LIE (a viewmodel-swap
collapsed even the known-clean grenade launcher). We need a faithful, repeatable instrument.

## Design (hardened per advisor 2026-07-28)

### Faithfulness is the #1 rule
Render each model through the UNMODIFIED game path: load a real map, spawn ONE entity,
set its `model`, and render via the normal `R_RenderScene` → `R_DrawEntitiesOnList` →
`R_DrawAliasModel` (normal `R_SetupGL` projection + `R_RotateForEntity`, world lighting via
`R_LightPoint`). NO hand-synthesized GL setup, NO viewmodel depth-hack/fov path — those are
how a harness reintroduces lying. Pose comes from the normal `R_SetupAliasFrame`.
- Implementation: gallery mode overrides the client's visible-entity list each frame with a
  single test entity at a fixed origin in front of a fixed camera; the world still renders
  (faithful setup + deterministic CPU lighting), the one test entity is centered.

### Two poses per model
- STATIC pass: entity at a fixed frame → `pose1==pose2` → blend==0 (the bug path).
- ANIMATING pass: two adjacent frames mid-lerp → blend!=0.
Per-model verdict table {static: ok/broken, animating: ok/broken}. This avoids false-positives
(animated monsters only ever draw at blend!=0 in-game; flagging their static pose broken would
be misleading unless labeled) and confirms the blend==0 mechanism.

### Comparison = COVERAGE MASK, not shaded pixels
The confirmed bug is SHAPE (collapsed/missing triangles). llvmpipe vs V3D shade/filter/dither
differently even when both correct → SSIM on lit images has a noise floor. So add a
fullbright-white render style: model = solid white, background dark → threshold → a coverage
mask (which pixels the model covers). Collapsed geometry moves coverage massively; shading
noise doesn't move it at all. Diff = |coverageP i XOR coverage_host| / coverage_host. Auto-flag
models over a threshold; eyeball only the flagged ones.

### Reference = host quakespasm + llvmpipe (headless)
Same quakespasm source + same GLSL alias shader, built for x86 Linux with
`SDL_VIDEODRIVER=offscreen` + Mesa llvmpipe (libEGL + swrast_dri present; no Xvfb needed).
Any Pi(V3D)-vs-host(llvmpipe) coverage diff is a V3D defect. MUST verify `gl_glsl_alias_able`
is true on host (llvmpipe advertises the GL/GLSL version quakespasm's alias path needs) —
else host silently takes a different codepath and the comparison is invalid.

## Sequence (host toolchain NOT on the critical path)
STEP 1 — CALIBRATION GATE (build first, gate everything on it):
  Pi gallery on ~4 models: g_rock, g_nail, one monster (ogre), one torch (flame).
  HARD GATE: the Pi gallery MUST show g_rock CLEAN and g_nail BROKEN (matches user ground
  truth). If g_rock shows broken → harness unfaithful, STOP and fix. If g_nail shows clean →
  the gallery doesn't reproduce the bug, RETHINK before investing. (coherent-weapon vs
  collapsed-spikes is a gross, reliably-judgeable difference.)
STEP 2 — FULL PI SWEEP: all ~62 alias models, static+animating. Read blatant collapses
  directly (no reference needed) → first broken/clean table.
STEP 3 — HOST REFERENCE: headless llvmpipe gallery → reference images; coverage-mask diff for
  the subtle cases + the "100% everything correct" guarantee.
STEP 4 — FIX + ITERATE: with the mechanism finally observable, form + test fixes until the
  gallery is green on Pi (matches host for every model/pose).
STEP 5 — GAME VERIFY: apply the fix to the shipping renderer; confirm in-game (user oracle
  for final sign-off) that nailgun/torches/etc. render correctly and nothing regressed.

## Status
- [x] Step 1: Pi gallery + calibration gate — PASSED 2026-07-28
- [ ] Step 2: full Pi sweep
- [ ] Step 3: host reference + coverage diff
- [ ] Step 4: fix to green
- [ ] Step 5: game verify

## STEP 1 RESULT — CALIBRATION GATE PASSED (2026-07-28)
The faithful gallery (real map + unmodified R_RenderScene path, single test entity, fixed
camera, fullbright, static frame 0, isolated on dark bg, per-model size-normalized) was run
on the Pi over g_rock/g_nail/ogre/flame. Time-ordered cycle = green-weapon → black-spike →
ogre → torch, mapping exactly to g_rock → g_nail → ogre → flame. Result:
- g_rock (grenade launcher): COHERENT weapon — CLEAN. ✓ (matches user)
- g_nail (nailgun): COLLAPSED into a black-triangle SPIKE — BROKEN. ✓ (matches user)
- ogre (326 tris, numframes=147, rendered at frame 0 => blend==0): COHERENT — CLEAN.
- flame (torch): coherent flame — CLEAN here (user saw "some torches" broken in-game; will
  include flame2 + more in the sweep).
=> The harness is FAITHFUL (reproduces the known clean+broken ground truth) and reproduces
the bug deterministically in isolation, every loop. This is the reliable instrument the whole
investigation lacked. Evidence: artifacts/qglitch-67/2026-07-28-gallery/calib_g_nail_BROKEN.png
(collapsed spike) + calib_g_rock_CLEAN.png (coherent). Gallery cvars/cmd: `mg` (arm, loops),
`mg_tga 1` (write files — host only; NOT Pi NFS). Autoexec: `map start` + `mg`.

KEY MECHANISM UPDATE: ogre CLEAN at blend==0/326-tris DISPROVES "blend==0 + high tri count"
as the trigger. The collapse is more specific to g_nail's class. The full sweep (Step 2) will
reveal exactly which models collapse; then the shared property points at the real cause.
