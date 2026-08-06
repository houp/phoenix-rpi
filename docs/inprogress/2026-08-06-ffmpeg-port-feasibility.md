# E4 — ffmpeg / libavcodec port feasibility (Phoenix-RTOS, Pi 4 aarch64)

Date: 2026-08-06
Scope: **bounded feasibility assessment**, host-side only. No source repo modified, nothing
committed, Pi/netboot untouched. ffmpeg **n6.1** shallow-cloned to
`/home/houp/.claude/jobs/c8f1289c/tmp/ffmpeg` (temp, not added to `external/`).
Toolchain probed: `.toolchain/aarch64-phoenix/bin/aarch64-phoenix-gcc` (GCC 14.2.0).

---

## Verdict (two-tier — pick the question)

- **Core library port (build libavutil/libavcodec.a for a small decoder set): TRACTABLE.**
  Every cheap build probe came back green. This is the question the probes actually answer.
- **End-to-end "video decode on the Pi", unattended, HW-validated: HARD-BUT-POSSIBLE.**
  Gated on things *not* probed here: sw-decode perf at resolution, getting video files onto
  the Pi over the flaky/slow NFS root, and the absence of any HW-decode path. None is a
  toolchain blocker; together they make an unattended end-to-end demo the hard part, not the port.

**INFEASIBLE-UNATTENDED** applies only to Pi **hardware** video decode (VideoCore) — see §5.

---

## What was actually observed (evidence)

### 1. `./configure` works for this cross target — first pass, exit 0
```
./configure --enable-cross-compile --arch=aarch64 --target-os=none \
  --cc=aarch64-phoenix-gcc --cross-prefix=aarch64-phoenix- \
  --disable-everything --enable-decoder=mjpeg,rawvideo --disable-asm \
  --disable-doc --disable-programs --disable-network --disable-pthreads \
  --disable-shared --enable-static
  → CONFIGEXIT=0
```
`--target-os=none` was accepted directly; no fallback to `linux` needed, no autotools involved
(ffmpeg uses its own hand-rolled `configure` shell script — no libtool/automake/hosted-POSIX
assumptions to fight). The only warning is a benign `pkg-config not found` (irrelevant: we
`--disable-everything` for external libs). `config.log` probe failures are **all expected
optional-feature probes** that fail harmlessly: `windows.h`, `linux/videodev2.h`, `X11/Xlib.h`,
`vdpau`, Objective-C, `_mingw.h`, `dlfcn.h`, etc. — none gate the requested build.

### 2. NEON / aarch64 hand-written asm ASSEMBLES — keep asm ON
Second configure with `--enable-asm --enable-neon` → `HAVE_NEON=1`, `HAVE_ARMV8=1`, exit 0.
Assembled three representative `.S` files with the GNU `aarch64-phoenix-as`, all produced `.o`:
```
AS libavutil/aarch64/float_dsp_neon.o
AS libavcodec/aarch64/h264idct_neon.o
AS libavcodec/aarch64/hpeldsp_neon.o   → MAKEEXIT=0
```
Blocker (d) does **not** materialize: the hand-written aarch64 SIMD builds with this toolchain.
`--disable-asm` is *not* required (and would badly hurt decode perf — keep it on).

### 3. Threading — libphoenix pthreads satisfy the configure probe
With `--enable-pthreads`: `HAVE_PTHREADS=1`, `HAVE_THREADS=1`, `HAVE_PTHREAD_CANCEL=1`.
So ffmpeg's frame-/slice-threading API layer is available at build time.
**Caveat:** this proves the *API* satisfies configure; it does **not** prove frame/slice
threading is robust under load on Phoenix (detected, unproven-at-runtime). A conservative
first bring-up can run single-threaded (`-threads 1`) and add threads later.

### 4. Compile surface — one real blocker, and it is the known libm gap
Sampled ~14 TUs across libavutil + the libavcodec decode core. Cleanly compiled:
`mem`, `mathematics`, `buffer`, `log`, `avstring`, `time`, and (after the fix below)
`eval`, `rational`, `avpacket`, `codec_desc`, `decode`.

The **only** compile blocker hit is a declaration clash in `libavutil/libm.h`:
```
libavutil/libm.h:121: error: static declaration of 'erf' follows non-static declaration
  .toolchain/.../include/math.h:100: note: previous declaration of 'erf'
```
Root cause = the documented libphoenix pattern (MEMORY: *libphoenix math.h declares full C99
but only a subset is defined*). configure's **link** probe for `erf/exp2/exp2f/log2f` failed
(declared-but-undefined → link error) → set `HAVE_{ERF,EXP2,EXP2F,LOG2F}=0` → ffmpeg emits its
own `static inline` fallback → clashes with libphoenix's non-static prototype. Exactly 4
functions clash; every other math fn probed present (`cbrt, copysign, hypot, log2, lrint, rint,
round, trunc, isnan, isinf` all `HAVE_*=1`), so libphoenix libm is otherwise sufficient.

**Fix is two steps (a compile fix is NOT the whole fix):**
1. Compile: set `HAVE_{ERF,EXP2,EXP2F,LOG2F}=1` in `config.h` (or via a compat header /
   config patch) so ffmpeg stops emitting its inline fallback. **Verified:** doing this made
   `decode.o`, `avpacket.o`, `codec_desc.o`, `eval.o`, `rational.o` all compile (MAKEEXIT=0).
2. Link: because those symbols are now deferred to libphoenix and libphoenix *doesn't define
   them*, any enabled decoder that actually calls `erf/exp2/exp2f/log2f` will be an **undefined
   reference at link**. Supply the 4 definitions — trivially, ffmpeg's own `static inline`
   bodies already live in `libavutil/libm.h`; lift them into a shim `.c`, or implement in
   libphoenix per the standing "implement missing libc" rule. Simple decoders (mjpeg/rawvideo/
   h264 video path) are unlikely to pull these in at all, so the real closure is tiny.

### 5. Other libc gaps (projected from config.log link-probes, not a measured link)
No final link was attempted (per task). Projected closure from probes + compile sampling:
- `HAVE_MEMALIGN=0`, `HAVE_POSIX_MEMALIGN=0` — **benign**: `av_malloc` falls back to
  `malloc` + manual over-allocation/alignment. No action needed.
- `sysctl`, `sched_getaffinity`, `gethrtime` link-probes failed — **gated off** on this target
  (used only for CPU-count / hrtime paths that `--target-os=none` doesn't enable).
- The 4 libm symbols in §4 are the only *real* projected undefined refs.
Treat this as a **projected** surface, not a link-verified one; a minimal `libavutil.a` /
`libavcodec.a` link is the natural next step if pursued.

---

## No-dynamic-linking implication
Not a problem for a decode-only build. ffmpeg only needs `dlopen` for *external* codec libs
(x264/x265/etc.) and some hwaccel loaders — all excluded by `--disable-everything` +
`--enable-decoder=<builtin>`. Built-in decoders are compiled into `libavcodec.a` and linked
**statically** into one ELF, matching the proven Q2/Q3/quakespasm single-ELF pattern.
`--disable-shared --enable-static` already forces this; `config.log` shows no `dlfcn`/`dlopen`
dependency in the requested config.

---

## Pi 4 HW decode (VideoCore) vs software libavcodec — (e)
**Software decode is the tractable path; HW decode is a separate, much larger project.**
- The Pi 4's VideoCore H.264 decoder is reached on Linux either via the deprecated **MMAL**
  firmware interface or the **V4L2 stateful M2M** driver (`bcm2835-codec`). **Phoenix has
  neither** — no V4L2 subsystem, no kernel codec driver. ffmpeg's `h264_v4l2m2m` / `h264_mmal`
  decoders would have nothing to bind to.
- Phoenix *does* have a userspace VideoCore mailbox pattern (thermal driver, `libvcmbox`), but
  the codec block is a different firmware channel that is not wired up. Bringing up HW decode =
  writing a from-scratch mailbox/V4L2-style codec driver + firmware protocol — **far larger**
  than the sw port and squarely **INFEASIBLE-UNATTENDED**.
- Software H.264 on Cortex-A72 @1.5 GHz with NEON: realistic for SD / ~720p; **1080p is
  marginal**. mjpeg/rawvideo are cheap. Recommend starting sw-only.

---

## Key risks
1. **NFS runtime-read limit (the headline runtime risk).** The netboot NFS root is ~100 Mbps
   with occasional read failures — the same limit that gated large-asset apps (games).
   Multi-MB/GB video files streamed off NFS during decode will hit exactly this. Mitigation:
   test with a tiny clip staged on SD/tmpfs, not a large NFS file; treat NFS video streaming as
   out of scope for a first demo.
2. **libm undefined-reference at link** (§4 step 2) — low effort but must not be skipped; the
   compile-only fix hides it until link time.
3. **Threading robustness under load** — API present, runtime unproven; de-risk with
   `-threads 1` first.
4. Perf ceiling at 1080p sw-decode (§5).

---

## Recommended approach if pursued
- **Codecs:** start `--enable-decoder=mjpeg,rawvideo` (near-zero libc surface, no threading), then
  add `h264` (with the NEON asm on) as the real target. `--enable-demuxer` only as needed
  (e.g. `mov,matroska` or just rawvideo) — keep the closure small.
- **asm:** **ON** (`--enable-asm --enable-neon`) — it assembles and is needed for perf.
- **threads:** build with pthreads, run `-threads 1` for first bring-up.
- **Build driver:** mirror the existing `tools/*-port` pattern — a `phoenix_ffmpeg_compat.h`
  force-include for the libm flag/decl reconciliation, plus a py driver that compiles each TU
  and link-drives to enumerate the undefined-symbol closure. Feed the config via a patched
  `config.h` (flip the 4 `HAVE_*` flags) rather than editing ffmpeg C.
- **Decode-only** (no encode, no network, no external libs, static single ELF).

## Rough effort estimate
- **Core-library port to a linking single ELF (mjpeg + h264 sw decode, asm on):
  ~2–4 focused sessions.** configure is free (done), asm free (done), the libm gap is the one
  real blocker and it's a known ~1-session pattern; the rest is enumerating a modest
  undefined-symbol closure and wiring a compat header + build driver.
- **On-Pi runtime bring-up + a playing/decoding demo:** add ~2–4 sessions, dominated by the
  file-delivery (NFS) problem and threading/perf tuning, plus a sink for decoded frames
  (`/dev/fb0` scanout already exists — a raw-frame-to-fb0 sink is plausible).
- **VideoCore HW decode:** not estimated — separate large driver project, out of scope.

## Go / no-go
**GO for a bounded software-decode core port** (mjpeg first, then h264, asm on, static ELF) —
the toolchain, asm, threads, and libc surface are all favorable and the single blocker is a
known-tractable libm gap. **NO-GO (defer) for VideoCore HW decode** and for any demo that
relies on streaming large video off the NFS root.
