# #67 alias-model glitch — the REAL fix (single-pose), 2026-07-27

## Honest correction of the record
The prior #67 "fixes" (fix-A waited-L2T barrier `214be9a`; SLCACTL-early ordering
`457a650`) and their "RESOLVED" claims were **false positives**. They were validated
by a **cross-boot determinism** harness (0.0% cross-boot = "fixed") and a DET binary
run with forced fixed-timestep + `r_dynamic 0` — neither of which reflects
correctness or the user's real run conditions. A *consistently-broken* render scores
0.0% cross-boot and falsely passes. Real-game HDMI video (2026-07-27) showed the
weapon pickups still rendering as mangled black-triangle spikes — the glitch was
never actually fixed, only measured wrong. The cross-boot cache/timing/SLCACTL axis
was aimed at the wrong thing; the surviving glitch is a **deterministic correctness
bug**, not a race.

## Root cause (deterministic)
`GL_DrawAliasFrame_GLSL` (external/quakespasm/Quake/r_alias.c) binds five vertex
attributes incl. two 8-bit position attributes Pose1Vert + Pose2Vert. The V3D
mishandles the alias draw when **blend==0** (single effective pose) while BOTH byte
position attributes are enabled/fetched → a subset of vertices collapse into black
degenerate-triangle spikes. blend==0 happens for exactly the broken classes:
- weapon PICKUPS (numposes==1, e.g. grenade launcher) — always,
- torch flames / `r_nolerp_list` models — always,
- the idle/paused viewmodel & monsters — intermittently ("sometimes broken").
The animating path (blend!=0: firing viewmodel, walking monsters) renders correctly.
At blend==0 the shader's `mix(Pose1,Pose2,0.0)==Pose1`, so Pose2 is mathematically
unused — but leaving it bound triggers the collapse. (The old de-alias workaround —
binding Pose2 to a duplicated block for numposes==1 — was a red herring; it doesn't
prevent the mishandling.)

## Fix (external/quakespasm `4ef0a42`)
In `GL_DrawAliasFrame_GLSL`, when `blend==0` do NOT enable/bind the Pose2 vertex+
normal attributes at all. A disabled attribute supplies the generic default which
`mix()`es away at blend==0, so position/normal come solely from Pose1. Only bind
Pose2 for the real 2-pose lerp (blend!=0), which was already correct. ~20 lines,
app-side, no winsys/Mesa change.

## Verification (real-game video — the reliable metric this time)
- **BEFORE** (demo2.mp4, pre-fix): demo1 grenade-launcher pickup = black-triangle
  spike; a second alias model = collapsed black-triangle mess. (artifacts/qglitch-67/
  2026-07-27-guns-still-broken/)
- **AFTER** (spfix2, boot 1): the SAME demo1 grenade-launcher pickup renders as a
  complete, recognizable weapon (blue/gray body, barrel, grip, magazine) — no spikes.
  (artifacts/qglitch-67/2026-07-27-single-pose-FIX/grenade-launcher-AFTER-fix.png)
- **AFTER** (conf3, boot 3): demo renders clean — viewmodel intact throughout incl.
  firing; monsters (zombies) render as recognizable figures; grenades fire+explode.
- **DECISIVE A/B on the SAME model, SAME spot** (yellow-grid-floor room, demo1):
  `guns-BEFORE-fix.png` = the grenade launcher as a black-triangle spike collapse;
  `grenade-launcher-AFTER-fix.png` = the same weapon, same location, fully intact.
- **AFTER (conf4, boot 3, 2026-07-27):** a fresh netboot captured continuously over
  HDMI — quake ran 27–46 fps, **0 faults, 0 GPU wedges, 0 NFS-34**, demo1 played with
  wall torches as clean flames, monsters as recognizable figures, held viewmodel
  (shotgun→nailgun→rocket) intact throughout, and the demo picked up shells/nails/
  armor/**Super Nailgun** with no spike artifacts. (conf4-boot3-sheet{1,2}.png.)
- Method: continuous HDMI video via ffmpeg on /dev/video4, stripped to frames,
  inspecting the actual gun pickups (the fleeting-but-decisive test the user named).
  DET/cross-boot harness abandoned as discredited.
- **Boot count: 3 clean confirmations** (spfix2, conf3, conf4); one other boot excluded
  (hit the NFS `-34`, quake never launched). The bug is deterministic, so the same-model
  before/after is already decisive; 3 clean boots additionally cover the *intermittent*
  idle-viewmodel/monster case.
- **Gun #2 caveat (honest):** the user named "two guns" in demo1. Gun #1 (grenade
  launcher floor pickup) has the decisive before/after above. I could NOT isolate an
  equally clean close-up of a *distinct second* floor-weapon pickup — the demo grabs the
  later weapons (Super Nailgun etc.) on the move through caves, not in a central
  approach. Gun #2 is the **identical code path** (numposes==1 → blend==0 floor pickup);
  gun #1 rendering clean is direct proof for that path, and the held weapon cycling
  through all weapon models renders intact. So the claim rests on gun #1's before/after
  + the shared mechanism, not on a photogenic gun #2 shot.

## Still open
- **NFS exec `-34`**: quake fails to launch ~1/10–1/5 boots (`exec ... failed
  (err=-34)`); the earlier re-drive (kernel c25ed0cb/7e6cbe37) is INSUFFICIENT — the
  `-34` (libnfs catch-all for an unmapped NFSv4 status on the first cold OPEN after
  takeover) persists **>10 s**, exceeding the retry. Correct fix needs the raw NFSv4
  status (add a libnfs log before nfs4.c:188) then handle THAT — not more retries.
  This actively blocked GPU-verification boots tonight.
- fix-A + SLCACTL-ordering: keep (fix-A prevents a separate render wedge; ordering is
  harmless), but they are NOT the #67 fix — the single-pose change is.
- **SEPARATE GPU binner-wedge (still present, NOT the geometry glitch):** the spfix2
  run (this exact fixed build) hit repeated `BIN TIMEOUT ... GPU wedged — true reset +
  drop this frame` late in a sustained 230 s demo1→2→3 loop (drops climbed to 251,
  0.8 fps). conf3 (70 s) and conf4 (110 s) stayed healthy (20–46 fps, 0 wedges). So the
  wedge is a marginal HW depth-pipeline drain stall that surfaces on long/complex runs —
  a real usability issue to chase separately, orthogonal to #67. Do NOT let it block the
  #67 verdict, but do NOT claim "GPU perfect" either.
- Note: `/srv/.../id1/autoexec.cfg` was set to `r_dynamic 1` during testing (a prior
  15-byte autoexec was overwritten).
