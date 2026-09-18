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

⚠ **All five fixes are reasoning, not measurement.** None is attributed to an observed defect; they
are landed because the port's own standard says an incidental barrier is a latent defect. They ship
together and are gated together.
