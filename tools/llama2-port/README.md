# llama2.c on Phoenix-RTOS / Raspberry Pi 4 — CPU LLM inference (phase 1)

A pure-C Llama-2 inference engine running a small language model on Phoenix-RTOS
on the Pi 4. This is **phase 1 (CPU)** of the owner's "ML inference on Pi4 GPU"
task — a working, deterministically-verified inference pipeline that the GPU
(V3D) matmul acceleration will build on.

## Result (HW-verified 2026-08-13, netboot, 0 faults)

`run-llama2 stories260K.bin -z tok512.bin -t 0.0 -n 80` on the Pi 4 produces output
**bit-identical** to the x86 reference build (temperature 0 = greedy = deterministic):

> Once upon a time, there was a little girl named Lily. She loved to play outside
> in the park. One day, she saw a big, red ball. She wanted to play with it, but
> it was too high. Lily's mom said, "Lily, let's go to

~370 tok/s (fp32, single-threaded, stories260K ~260k params). 0 Data Aborts, 0
`not implemented`. libphoenix's libm already covers the full math surface
(expf/exp/sqrtf/sinf/cosf/powf) — no libphoenix changes were needed.

## Source & license

`run.c` is from Andrej Karpathy's llama2.c (https://github.com/karpathy/llama2.c),
**MIT** — a port like busybox/bash/curl, no GPL concern.

## Phoenix patch (the only change)

Upstream `read_checkpoint()` `mmap()`s the checkpoint file (`MAP_PRIVATE`). Phoenix
is not relied upon to provide a file-backed private mapping (esp. over NFS), so
under `#if defined(__phoenix__)` the loader instead `malloc()`s `file_size` and
`fread()`s the whole checkpoint into RAM (fine within the Pi 4's 4 GB). Upstream
mmap path is retained for other platforms. `free_transformer()` frees the buffer
under the same guard. No other source changes.

## Build

    aarch64-phoenix-gcc -O3 -static -o run-llama2 run.c -lm

Single-threaded on purpose (no OpenMP on Phoenix). See `build.sh`.

## Models (stage into the netboot NFS root)

- Pipeline-proof (tiny, ~1 MB): `stories260K.bin` + `tok512.bin`
  (https://huggingface.co/karpathy/tinyllamas/resolve/main/stories260K/).
- Scale-up (~60 MB): `stories15M.bin` + the default 32000-vocab `tokenizer.bin`.

## Verification recipe

Temperature 0 + no prompt → deterministic greedy generation. Assert the exact
output string over psh (plain-path argv avoids psh's quote/space arg-splitting).
`./scripts/test-cycle-psh-interact.sh --label llama2 -- "/bin/run-llama2 /stories260K.bin -z /tok512.bin -t 0.0 -n 80"`.

## Phase 2 (owner-gated): V3D GPU acceleration

The matmul (`matmul()` in run.c — the dominant cost) is the GPU-acceleration target
for the owner's "on Pi4 GPU". V3D has no Clover/OpenCL on this port, so this means a
compute path via the V3D CSD (compute shader dispatch) — a novel, dedicated effort
that is not autonomously verifiable (needs shader bring-up + numeric validation).
Ship phase 1 (this), then design-doc + owner-gate phase 2.
