# Two-front fixes: NFS exec-EIO + GPU torch/monster race (2026-07-26, in progress)

User mandate: fully fix (1) the torch/monster #67 render glitch and (2) the NFS
boot/exec instability — deterministically, correctly (no band-aids), autonomously.

## NFS exec-over-NFS -EIO — FIXED (committed)

- Root cause (kernel `vm/object.c:232`, `object_fetchCluster`): the exec demand-page
  force path opens each ELF-metadata cluster with `proc_open`; that open **fabricated
  `-EIO`** on any transient and was **not retried**, while its sibling `proc_read`
  already retries transients 25× (nfs_ops.c). One transient blip on the cold open
  aborted the whole ~17 MB exec (`exec ... failed (err=-5)`, ~1/10 nfsroot boots).
- Fix (kernel `c25ed0cb`): never fabricate `-EIO` — propagate `proc_open`'s real
  errno (4a) — and give this one open a bounded, backed-off re-drive matching the
  read path (4b), logging each attempt's real errno. Inert on SD.
- Repro learning: the failure is the **first cold exec right after takeover** (nfs-fs
  warming), ~1/10 boots — NOT accelerable per-boot (a 30×-cold-exec/boot synthetic
  loop gave 0/~75 because the pre-exec lookups warm nfs-fs first). So validation is
  **passive**: every subsequent netboot execs quake and exercises the fix; the
  per-attempt log captures the real errno the first time any boot hits the transient.
- Validation so far: 5 clean torch-baseline boots + earlier boots, all quake-OK, 0
  re-drives fired yet (consistent with ~1/10). Ongoing passive validation.

## GPU torch/monster #67 glitch — mechanism CONFIRMED = consumer render race

Two research subagents (2026-07-26) + a HW discriminator settled the mechanism:

- **Linux uses the same cache ops on V3D 4.2** (`v3d_flush_l3`/`v3d_invalidate_l2c`
  are no-ops ≥4.1/3.3; `v3d_invalidate_caches` = bare L2TFLS + SLCACTL 0x0f0f0f0f),
  byte-identical to the Phoenix winsys. So the fix is NOT a missing cache op. And
  Linux's own comment says the L2T flush needs no completion wait (HW self-stalls) —
  so **"fix-A" (the waited-L2T-flush before the CT0 kick) works only as injected
  LATENCY, not a coherency primitive.**
- **Producer path is deterministic** (VBO is Normal-NC, stable-VA, direct uncached
  memcpy, `dsb sy`-drained, memset-init; kernel flushes the cached alias on
  cacheability change). 
- **HW discriminator (5 fresh cold boots, r_dynamic 0, fixed-timestep demo):**
  - VBO **source** CRC (QVBO) byte-identical across all 5 boots, all 41 models.
  - Yet the door/torch frames still **mangle-vary cross-boot**: F0090 3.0%, F0100
    3.5%, F0110 3.1%, F0120 5.1%, the monster frame F0140 5.5%, F0070 3.2% (3–4
    distinct renders / 5 boots; >30/255 per-pixel). (F0000 26.8% = console warmup,
    discounted.)
  - **Identical input → non-deterministic output = a GPU render RACE, not a data/
    producer bug.** Confirms the fire-and-forget SLCACTL slice-cache invalidate
    (TVCCS/TDCCS, no completion bit on V3D 4.2) is raced by the binner's coordinate-
    shader vertex fetch.
- Size/latency clue: fix-A's latency fixed large models (guns) but not tiny ones
  (torches) — a wrong-DRAM producer bug would not be latency-fixable at all, so this
  independently supports the race.

### Open fix question (the deep part)
Linux is glitch-free with the **same** ops → there must be an **ordering** difference
(WHEN Linux invalidates relative to the CT0 kick + what naturally provides settle
time — likely scheduler/IRQ dispatch latency vs Phoenix's synchronous self-powered
inline kick). Studying Linux's submit ordering next; fix = match that ordering, not
add another op. fix-A stays as the latency mitigation until then.

## Harness
- DET quake (`build-quakespasm-det.py`, `external/quakespasm-det`): QVBO source CRC
  (gl_mesh.c), 192×108 full-frame dump every 10th frame (gl_screen.c) → cross-boot
  torch scorer, + `QDET_EXECPROBE` marker-gated exit-at-main for the NFS exec loop
  (compiled only into DET via `-DQDET_EXECPROBE`, never the ship build).
- Torch scorer: 5-boot cross-boot per-frame distinct-count + >30/255 pixel-diff %.
