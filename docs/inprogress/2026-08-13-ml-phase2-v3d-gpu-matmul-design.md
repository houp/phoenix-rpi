# ML phase 2 — V3D GPU matmul acceleration for llama2 (design + feasibility)

Phase 1 (CPU) shipped: llama2.c runs on Phoenix/RPi4, deterministic, HW-verified at 260K + 15M
([[project_ml_inference_llama2]], tools/llama2-port/). Phase 1 tok/s: stories15M ≈ **5.8 tok/s** (fp32, single
CPU) — the motivation for GPU offload. This doc scopes phase 2: offloading llama2's `matmul()` (the dominant cost)
to the Pi 4's V3D 4.2 GPU via **compute (CSD)**.

## Feasibility — ESTABLISHED

- **HW:** BCM2711 V3D 4.2 supports compute-shader dispatch (CSD). The Linux v3d driver implements it
  (external/linux/.../v3d/v3d_submit.c) and exposes `DRM_V3D_PARAM_SUPPORTS_CSD`.
- **uAPI:** the port's `tools/v3d-driver-port/v3d_drm.h` already has `DRM_V3D_SUBMIT_CSD` (0x07) +
  `struct drm_v3d_submit_csd { __u32 cfg[7]; __u32 coef[4]; ... }` — the standard CSD submit (cfg = workgroup
  dims + shader/uniforms addresses; coef = the dispatch grid/wg config).
- **Mesa:** upstream Mesa v3d gallium supports GL/PIPE compute shaders. BUT the port's Mesa build list
  (v3d-core-sources.txt) does **not** appear to include the v3d compute-compiler paths — so using Mesa to compile
  the kernel needs those sources added (non-trivial), OR hand-write the QPU kernel.
- **Port submit path:** `tools/v3d-driver-port/v3d_libdrm_shim.c` is **render-only** today — `SUBMIT_CL` is
  synchronous (blocks on FLDONE/FRDONE), programming CT0CA/CT1CA. There is **no CSD handler yet**. Adding one is
  **additive** (a new ioctl path), so it does NOT touch the load-bearing GL render path → **low regression risk** to
  Quake/vkQuake/X11 (all of which use SUBMIT_CL rendering).

## The key enabler — matmul is NUMERICALLY verifiable (not HDMI-bound)

Unlike DRI/DRM or a DE (which need HDMI/visual verification, poor for autonomous mode), a compute matmul writes its
result to a buffer that we **read back and diff against the CPU matmul** — deterministic, exact (or ULP-bounded for
fp), no display involved. This upgrades phase 2 from "owner-gate / not autonomously verifiable" (the earlier
assumption) toward **autonomously attemptable**, gated only by CSD bring-up difficulty.

## Design

`matmul(xout, x, w, n, d)` computes `xout[i] = sum_j w[i*n+j]*x[j]` for i in [0,d) — a matrix(d×n)·vector(n).
Called per layer for the big weight matrices (dim×dim attention proj, dim×hidden FFN). Offload the large ones to
V3D; keep small ops on CPU.

- **Approach A — Mesa compute shader:** add the v3d compute-compiler sources to the port build; write a GLSL/SPIR-V
  matmul compute shader; dispatch through Mesa's compute pipe. Pro: reuse Mesa's QPU codegen. Con: the port's Mesa
  compute path is currently unbuilt (real integration work); heavier.
- **Approach B — hand-authored QPU kernel + direct CSD submit:** write one matmul kernel in V3D QPU (via
  tools/v3d-shader-tool) and submit it through a new `SUBMIT_CSD` shim handler. Pro: minimal, targeted, no
  Mesa-compute dependency. Con: hand-QPU (VPM/TMU loads, the V3D 4.2 ISA) is hard.
- **Recommendation:** start with **B's submit-path bring-up** using a *trivial* kernel (below); pick A vs B for the
  real matmul kernel after the dispatch path works.

## Build plan (multi-cycle, each step numerically verified)

1. **CSD dispatch bring-up (milestone).** Add `SUBMIT_CSD` to v3d_libdrm_shim.c: program CSD_QUEUED_CFG0..7 from
   `cfg[]`/`coef[]`, kick, block on CSDDONE (model on the existing synchronous SUBMIT_CL). Run a trivial compute
   kernel (e.g. write threadIdx to an output buffer, or vector-add) → **read back + numeric-verify**. This proves
   compute works on the port. Autonomously verifiable.
2. **matmul compute kernel** (A or B) → numeric-diff vs CPU matmul on random inputs (ULP-bounded).
3. **Wire into llama2:** V3D path for the big matmuls, CPU fallback; verify **end-to-end deterministic output
   bit-identical to phase-1 CPU** (the existing 260K/15M references) + measure tok/s speedup.

## Risk / autonomy

- Regression: LOW — additive CSD path, GL render untouched. Git-disciplined (commit each step; the shim change is
  isolated).
- Autonomy: steps 1–3 are numerically verifiable over psh/diag (no HDMI). **Attempt autonomously**, starting at the
  CSD bring-up milestone; escalate to owner-gate only if QPU codegen (kernel authoring) proves intractable without a
  display/owner in the loop.
- Perf expectation: V3D is a modest GPU; a well-mapped fp32 matmul could give a several-× speedup over 5.8 tok/s,
  but memory-bandwidth-bound — measure, don't assume.

## NEXT ACTION when resumed
Implement step 1 (CSD dispatch bring-up in v3d_libdrm_shim.c + trivial kernel + numeric read-back). That single
milestone decides Approach A vs B and whether the whole phase is autonomously tractable.
