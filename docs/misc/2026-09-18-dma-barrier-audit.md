# DMA barrier discipline across the port — one audit, five gaps

*2026-09-18. Started because `rpi4-audio` turned out to have no memory barrier at all between its
DMA control block and the MMIO kick; the obvious question was who else.*

**The rule, cited not recalled.** `mmap(..., MAP_UNCACHED | MAP_CONTIGUOUS, ...)` yields
`PGHD_NOT_CACHED` (`vm/map.c:539-541`) → MAIR index 1 (`hal/aarch64/pmap.c:64,488-490`) → byte `0x44`
of `MAIR_EL1_VALUE` (`hal/aarch64/_init.S:137`) = **Normal Non-Cacheable**. MMIO with
`MAP_DEVICE | MAP_UNCACHED` lands on MAIR index 3 = **Device-nGnRnE**. aarch64 does **not** order
Normal-NC stores against Device stores, so a driver that builds a descriptor in uncached DRAM and
then kicks an engine through a register **must** put a barrier between them — and `dsb`, not `dmb`,
because completion is the requirement when the consumer is a non-coherent external master reading
DRAM. That is the reasoning the V3D CL path has carried all along (`gpu/rpi4-v3d/v3d_gpu.c:1152-1156`).
The read direction needs the mirror: an MMIO status read says the engine is done, and the payload it
refers to may only be read after that observation is ordered.

⚠ **The read direction is the exception to "`dsb`, not `dmb`".** Ordering a payload load behind a
status load is load-load only — nothing has to *complete*, it only has to not be hoisted — so `dmb`
is sufficient there, and that is why the SDHCI fix reuses the file's `sdio_dataBarrier()` (`dmb sy`)
rather than upgrading it. The `dsb` rule holds for the write direction, where an external master must
actually see the descriptor before the kick lands.

## Fixed this pass

| what | where | why it matters |
|---|---|---|
| **V3D TFU submit — no barrier before the kick** | winsys `ioc_submit_tfu` + the server's copy | **Live on the demo path**: every GL mipmap/blit (`v3d_blit.c`) and every Vulkan image copy (`v3dv_queue.c`) goes through it. Nothing ordered the source texels *or* the fresh PTEs against the register writes. A slip shows as a garbage/zeroed texture level or a TFU fault — **not** a binner wedge. |
| **xHCI event ring — no barrier after the cycle bit** | `usb/xhci/xhci.c` | The cycle bit is the publish flag for the whole TRB; type/parameter/status could be read from the slot's previous owner. `xhci_trb_t` is a plain struct, so the compiler was free to reorder too. Linux carries the same barrier in the same place. One fix covers the IN-data read as well. |
| **SDHCI read — no barrier between completion and payload** | `storage/bcm2711-emmc/sdcard.c` | Every SD read on the SD-boot lane. Load-load only (the staging buffer is uncached), so a reordering hazard rather than a stale-line one. Uses the file's existing `sdio_dataBarrier()`. |
| **V3D direct-mailbox fallback — neither direction** | `mesa/v3d_phoenix_power.c` | Taken only when `/dev/vcmbox` is absent, but this is the known-racy path the `/dev/vcmbox` routing exists to avoid. `rpi4-vcmbox` had both barriers already. |
| **rpi4-audio — no barrier anywhere** | the one that started it | Separate write-up: [`2026-09-18-audio-dma-stall-captures.md`](2026-09-18-audio-dma-stall-captures.md). |

## Already correct — checked, not assumed

V3D **CL** and **CSD** submit (`dsb sy` before the kick) · **genet** TX (`dsb sy`, plus explicit
`dc cvac` because its buffers are *cacheable*) and RX (`dmb ld` after the producer-index read) ·
**xHCI doorbells** (one centralised helper with `dsb sy`, mirroring Linux's
`xhci_ring_*_doorbell()`) · **rpivid/HEVC** (`hevc_dma_fence()` on both sides) · **rpi4-vcmbox** —
the model the others should copy · **bcm2711-pcie**.

## Not fixed, deliberately

- `pcie/server/pcie.c`, `wifi/rpi4-wifi.c`, `bt/rpi4-hci.c` all have uncached bounce buffers plus
  MMIO and **zero** barriers — and none of them is built or staged for `aarch64a72-generic-rpi4b`.
  Worth a `TODO` if they are ever revived; not worth a build cycle now.
- The SDHCI **DMA-write** path is dead code (`useDma` is forced false for writes), so its barriers
  were not touched.

## One scoping correction to KNOWN-ISSUES

The `V3D-binner-wedge` row says *"No unflushed CPU→GPU write exists"*. That is true **of the CL
submit path**, which is what its supporting evidence is about — the TFU path *was* unflushed. ⚠ But
that is **not** a candidate root cause for that row: TFU output is sampled later by CT1's TMU, it is
not fetched by CT0, and the recorded signature is a CT0 front-end stall with `ct0ca` parked inside a
*valid* BCL BO. Do not let the correction read as a match.

## ★ MEASURED 2026-09-18 — the race is real on this hardware

`pwmdma --cb-race`, 5000 trials per arm on the Pi 4, one process per arm:

| arm | correct | **stale** | torn | unclassified | not fetched |
|---|---|---|---|---|---|
| `--barrier` (control, `dsb sy`) | 5000 | **0** | 0 | 0 | 0 |
| no barrier (test) | 4854 | **146** | 0 | 0 | 0 |

A *stale* fetch is the engine reading the control block's **previous** contents — bytes the CPU had
already overwritten. 146 in 5000 (2.9 %) without the barrier, **0 in 5000** with it. The first one
landed on trial 0.

So the Normal-NC → Device store-ordering hazard these five fixes were landed against is not a
deduction from the architecture manual: it is observable on this board, at a rate high enough to hit
in seconds. ⚠ Scope, from the probe's own verdict: the null in the control arm **bounds** the rate,
it does not prove the ordering is architecturally guaranteed — and neither arm covers the first-ever
fetch of freshly `mmap`'d memory, because the probe reuses one control block.

⚠ **The 2.9 % does not transfer to the other four fixes.** It is a *DMA control-block fetch* rate on
one channel at one preload depth. The V3D TFU reads source texels and page-table entries, not a
control block; xHCI and SDHCI are read-direction orderings. The **mechanism class** is shared
(Normal-NC vs Device on this SoC); the number is not. Nobody should read this as "≈3 % of GL blits
were corrupt".

⚠ The five fixes are still **not** attributed to any observed defect in the drivers themselves; what
is now measured is the *mechanism*, not any particular field failure. They ship and are gated
together.
