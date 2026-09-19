# SDMA writes were corrupt because we never applied the emmc2bus address translation

Fixed 2026-09-19. The driver had carried this for months as "a write-DMA quirk on this
controller" with writes forced onto PIO. It was ours.

## Root cause

`bcm2711.dtsi` gives the eMMC2 block its own DMA window:

```
dma-ranges = <0x0 0xc0000000  0x0 0x00000000  0x40000000>;
```

Bus address `0xC0000000 + X` maps CPU-physical `X`, over the low 1 GiB. Linux applies that
translation on every transfer (dma-direct → `sdhci_sdma_address()`), and so does every other DMA
master in this tree — `rpi4-audio` has `DRAM_BUS()`, the kernel has `dtb_armToBus()`.

**The SD driver was the only DMA master handing the engine a raw CPU-physical address.** It
programmed `0x03780000` where the controller wanted `0xC3780000`. The window check in
`sdhost_allocDMA` even quotes the translation in a comment — and then uses it only as a reachability
bound, never applying it.

## Why the old diagnosis was wrong

The code recorded "intermittent first-block silent corruption, firstBadBlk=0, ~1-2/10". Measured
2026-09-19 with `writes=SDMA` confirmed in the init log, it was **whole-transfer and every time**,
with a different checksum each run for identical input. That is not a race; it is reading from the
wrong place.

One log line settled the direction before any fix was tried: the failing runs contained **no**
`sdcard error:` lines, `dd` reported every record, and the card returned to TRAN — so the card
CRC-verified what arrived. The wrong bytes were already wrong **before** the SD bus, which excludes
signal integrity, DDR50 timing and the card, and points squarely at the engine reading the wrong
DRAM.

## The fix, and the evidence

`SDCARD_DRAM_BUS(pa)` applied to `SDHOST_REG_SDMA_ADDRESS`.

⚠ It changes **reads** too, which is what made the test decisive: reads worked with the raw address,
and that was the one fact the hypothesis did not explain. So the run had to show writes fixed *and*
reads still correct.

| check | measured | expected |
|---|---|---|
| write, head 1 MiB | `3044388970` | `3044388970` ✅ |
| write, tail 7 MiB | `2205809865` | `2205809865` ✅ |
| **read control**, card @ 0 | `3890520697` | `3890520697` ✅ |
| write round 2, full 8 MiB | `3921710466` | `3921710466` ✅ |
| write round 3, full 8 MiB | `3921710466` | `3921710466` ✅ |

⚠ **Still unexplained: why reads ever worked with the raw address.** They demonstrably did — the
whole SD-boot lane and every gate ran on them. Both aliases evidently reach the same DRAM for the
write direction of the *engine*, but not for its read direction. Recorded as an open question rather
than papered over.

## Two other defects found on the way

- **`cmdFrame` was never zero-initialised** (`_sdio_cmdSend`). The bitfield union left bit 3 —
  Auto-CMD23 — unwritten, and Auto-CMD23 is *forbidden* with SDMA because `ARGUMENT2` and
  `DMA_ADDRESS` are the same register (0x00); Linux gates it off for SDMA-in-v3 for exactly that
  reason. Two sibling functions already zeroed theirs. Fixed.
- **`sdio_submitBarrier()`** — the barrier before the engine-start write was `dmb sy` (ordering)
  where a non-coherent master reading Normal-NC DRAM needs `dsb` (completion). A real defect, fixed
  earlier the same day; **it was not this bug**, and saying so is the point: it was measured, not
  assumed.

## Hypotheses killed by measurement, not argument

- **Premature completion clobbering a live transfer** (writes poll present-state then issue CMD13,
  where reads wait on an IRQ). The probe shows `dspin` of 14 765–123 376 — the poll really is
  waiting. Dead.
- **SDMA boundary interrupt.** Transfers are ≤128 KiB from a 128 KiB-aligned base; a 512 KiB
  boundary cannot be crossed. Dead on arithmetic.

## Throughput — and a measurement that was wrong

↩ **Retracted:** an earlier version of this file reported PIO at 0.94 MB/s and SDMA at 0.59 MB/s and
concluded DMA was slower. **Both numbers were artefacts.** `test-cycle-psh-interact.sh --idle-secs N`
waits N seconds of UART idle **after each command**, so bracketing a `dd` with `date` measures the
harness's cadence. The tell: three completely unrelated operations — a card write, an NFS file read
to `/dev/null`, and a card read — all "took" **exactly ~312 s**. An NFS read to `/dev/null` cannot
cost the same as an SD write.

⊕ And a second, larger factor was hiding underneath: **`/bin/dd` is busybox and runs at ~0.65 MB/s;
`/usr/bin/dd` is coreutils and runs at ~12.3 MB/s** on the identical transfer. Every timing above had
used busybox.

**Measured properly** — coreutils `dd` prints its own rate, which is immune to the harness:

```
67108864 bytes (67 MB, 64 MiB) copied, 5.43779 s, 12.3 MB/s   [writes=SDMA confirmed in the log]
```

So SDMA writes run at **12.3 MB/s**, matching the driver's own documented DDR50 figure — not 0.59.
The 28-minute card flash was a property of the busybox binary, not the hardware; with coreutils dd a
1.1 GB image is ~2 minutes.

⚠ **Lesson worth more than the number:** never time a transfer through this harness. Read the rate
the tool itself reports.

## ADMA2

Still the better long-term target — it is what Linux runs on this silicon, it removes the uncached
bounce copy entirely (scatter-gather over the caller's own pages) and lifts the 512 KiB request cap.
But the bounce is no longer an obvious bottleneck at 12.3 MB/s, so the case for it is now
architectural rather than urgent.
