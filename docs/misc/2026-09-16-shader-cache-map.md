# The V3D shader disk cache: what it is, what it is worth, and the bug that made it useless

*2026-09-16. Written after a defect hunt that started from a line in the weekly log telling the owner
to "warm each GPU app twice before presenting". The advice was a ritual with no mechanism behind it;
the mechanism turned out to be broken. Companion to
[`2026-09-08-stk-time-to-race.md`](2026-09-08-stk-time-to-race.md) and the `shader-cache-speckle` row
in [`../KNOWN-ISSUES.md`](../KNOWN-ISSUES.md).*

## 1. What exists

`sources/phoenix-rtos-devices/gpu/rpi4-v3d/mesa/v3d_phoenix_stubs.c:24-223` is a **real
implementation**, not a stub (the file header still says "stubs" because everything else in it is a
no-op). One file per cache key, named by the 64-char hex of a 32-byte BLAKE3 digest; `disk_cache_put`
publishes atomically via a temp file + `rename()`, with pid-tagged temp names so concurrent writers
(e.g. glamor-X) do not collide.

It is compiled into **`libv3d-phoenix.a` only**. The Vulkan archive has no `disk_cache_*` of its own
and resolves these symbols at link, so **GL and Vulkan share one cache directory** — QuakeSpasm,
Quake II, Quake III, SuperTuxKart and glamor-X on the GL side, vkQuake on the V3DV side. On a hit,
V3DV skips the whole SPIR-V→NIR→QPU chain before any `spirv_to_nir` work happens.

Cache directory: `$MESA_SHADER_CACHE_DIR`, else `$MESA_GLSL_CACHE_DIR`, else cwd-relative
`./.mesa-shader-cache`, plus a `/v<N>` segment from `V3D_PHX_CACHE_VERSION` (`:44`, currently **1**,
never bumped). ⚠ The two environment overrides are effectively dead — psh cannot set environment
variables — so the cwd-relative default is what actually runs everywhere.

Per boot variant, given cwd `/`:

| variant | `/` is | cache lands at | survives reboot |
|---|---|---|---|
| `nfsroot` (the bench lane) | NFS export | `<export>/.mesa-shader-cache/v1` | **yes, proven** |
| `netboot` | dummyfs (RAM) | RAM | no |
| `sd` (the shipped image) | ext2 on the card | `/.mesa-shader-cache/v1` | **should, never observed** |

The nfsroot case is proven: a cold boot wrote 52 blobs and a warm boot took 52/52 hits with mtimes
unchanged (2026-09-01). The SD case is *inferred* — this bench has no card reader, so SD boot has
never been exercised here at all.

## 2. What it is actually worth — and the number that is wrong

**vkQuake is the app the cache rescues: ~67 s of black screen on a cold cache**, building 67 shader
modules before the first frame (measured 2026-09-04). That is the number that matters in front of an
audience, because a black screen that long reads as a hang — and has been misread as one twice
(2026-09-09 cost three cycles; 2026-09-13 left a trial with 3 frames).

⚠ **SuperTuxKart's oft-quoted "~35 s of pure recompilation" is a misattribution.** The claim came from
first-run-of-boot 55.5 s vs 20.0 s warm, with the whole delta charged to shader compilation. A
controlled probe refutes it ([`2026-09-08-stk-time-to-race.md`](2026-09-08-stk-time-to-race.md):84-102):
across one full-deferred first run the cache went **200 entries → 200** while 50 shaders "compiled" —
every one a *hit* — and the run still took 49.8 s. So the cache is worth only the **~5.5 s** it already
saves on STK, and **~27 s of STK's first-run cost is something else**; the leading hypothesis is
contiguous-BO allocator warm-up, since it is per-boot rather than per-process and specific to the
deferred pipeline's larger render targets.

That wrong attribution was sitting in `sync-netboot-tree.sh`'s own comment, which is where it kept
getting copied from. It is corrected there now.

## 3. Invalidation: no build-id, one hand-edited integer, one host-side fingerprint

Phoenix emits no ELF build-id, so `build_id_*` return NULL and the driver-identity component of the
cache key is **empty**. Upstream Mesa passes the build SHA into `disk_cache_create`; our
implementation discards that argument. **The key is therefore purely shader-derived** — a driver
rebuild that changes QPU codegen does not change any key, and stale blobs get executed. That is the
`shader-cache-speckle` failure: coloured speckle *over an otherwise valid frame* (as opposed to a real
GPU wedge, which drops whole frames and logs one).

Two controls guard it, and both are real:

1. **`V3D_PHX_CACHE_VERSION`** — bumping it changes the `/v<N>` path segment and invalidates
   everything. Appears in exactly one commit; never bumped.
2. **A host-side driver fingerprint** (`sync-netboot-tree.sh`, since 2026-09-08): sha256 of
   `libv3d-phoenix.a` + `libGL-phoenix.a` + `libv3dv-phoenix.a`, stored at
   `${buildroot}/.mesa-shader-cache.driver-id`. Equal ⇒ keep the cache; otherwise `rm -rf` it.

⚠ The SD lane has **neither** — the fingerprint logic is host/NFS-only. Its saving grace is that the
only update path for a card is a reflash, which wipes the root anyway. Updating binaries in place on a
mounted card would leave stale blobs completely unguarded.

## 4. 🐞 The fingerprint was deleted on every rebuild (fixed 2026-09-16)

`prepare-buildroot.sh` rsyncs the project source into the buildroot with `--delete`. The fingerprint
file lives at the buildroot *root* and does not exist in the source, so **every prepare deleted it** —
and prepare runs on every rebuild by default. The next sync then found no fingerprint, took its
"(or first run)" branch, and wiped the shader cache **even though the GPU driver had not changed**.

So the 2026-09-08 conditional-keep optimisation was inert from the day it landed: a cold cache was the
norm after any rebuild, which is exactly why cold-cache runs kept being misread as hangs.

Proven with both controls in one session:

* a cycle after a prepare-running rebuild logged `cleared — GPU driver changed (or first run)` with an
  **unchanged** driver, while the next cycle — after `build-port.sh`, which does not prepare — logged
  `KEPT — 127 entries`;
* directly: with the exclude the fingerprint survives prepare byte-identical; with it removed, prepare
  deletes it;
* end-to-end afterwards: `prepare-buildroot.sh` then `sync-netboot-tree.sh` ⇒ `KEPT — 127 entries`.

Fix: exclude `/.mesa-shader-cache.driver-id` from that rsync (coord `4bfdfb135`).

👁 **Watch item this creates.** The cache now genuinely survives rebuilds, where the bug was wiping it
— accidentally safe. Invalidation now rests entirely on the fingerprint. If green speckle over a valid
frame ever appears after a GPU-driver change, suspect that logic first;
`sudo rm -rf /srv/phoenix-rpi4-nfs-gcc16/.mesa-shader-cache` is the escape hatch.

## 5. Two traps worth keeping

**The cache can be silently disabled.** In `disk_cache_create`, `cold` is computed, then `mkdir_p`
runs; if the mkdir fails the function returns NULL **before** the "cache COLD" printf. On a read-only
or full root you get no caching *and no message*. So: a first GL/VK run on a fresh root **must** print
`v3d: shader cache COLD (...)`. Absence of that line does not mean "warm" — it can mean "caching is
off". This matters most on the SD image, the one lane nobody has watched.

**Do not conclude a symbol is absent from a repo-root grep.** `grep` in this environment is ugrep run
with `--ignore-files`, so it honours `.gitignore`, and the coordination repo ignores `sources/` and
`external/`. A recursive grep from the repo root silently skips every sibling source tree and every
fork, returning rc=0 and no output. That is how `V3D_PHX_CACHE_VERSION` was briefly and wrongly
declared nonexistent. Scope the search (`grep -rn X sources/`) or grep the file.

## 6. Not done

Nothing pre-seeds the cache into the SD image; "ship a warm shader cache with the image" was proposed
on 2026-09-05 and never implemented (only the COLD printf, the other half of that proposal, exists).
So the first boot of a freshly flashed card is guaranteed cold for every GPU app. Whether that is
worth fixing depends on a fact nobody has measured — whether the cache persists across SD reboots at
all — and that needs a card in the Pi.
