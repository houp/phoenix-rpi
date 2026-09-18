/*
 * pwmdma — does the DMA→DREQ→FIFO handshake ever park instead of streaming?
 *
 * Why this exists (2026-09-18). `rpi4-audio`'s DMA stall is captured, contained and
 * still unexplained (docs/misc/2026-09-18-audio-dma-stall-captures.md). Its rate is
 * ~7 boots in 100 and one boot costs ~2.5 minutes, so every hypothesis about it has
 * been settled — or not settled — at one sample per two and a half minutes.
 *
 * `bin/pwmwrite --start-test` retired the PWM half: 7 000 PIO starts of the driver's
 * exact init sequence, 0 failures ("Fifth hypothesis retired: the PWM starts fine —
 * it is the DMA path"). What that null explicitly did NOT cover is the DMA path
 * itself: the DREQ handshake, the PANIC/DREQ thresholds, and the first burst arriving
 * while the channel is coming up. This probe is that branch. It performs exactly the
 * shape of `audio_dmaArm()` — PWM init, `PWM_DMAC`, channel RESET, `CONBLK_AD`,
 * `ACTIVE`, settle, measure `DMA_SOURCE_AD` progress — thousands of times per boot,
 * on hardware the audio driver does not own:
 *
 *   - PWM **0** @ 0xfe20c000 (the driver owns PWM1 @ +0x800). PWM0's pins are not
 *     muxed on this port, so nothing is audible and no GPIO is touched.
 *   - legacy DMA channel **6** (the driver owns 5).
 *   - DREQ/PERMAP **5**, the legacy PWM0 DREQ (the driver uses 1 for PWM1).
 *
 * A trial FAILS when the channel advances fewer than DMA_START_MIN_WORDS ring words
 * across the settle — the same progress test, and the same threshold, the driver
 * applies on every arm. On the first failing trial the probe dumps every register the
 * driver would, then tries ONE re-arm and says whether it recovered: that is the
 * discrimination the driver's re-arm print was built for, answered thousands of times
 * faster than a boot hunt can answer it.
 *
 * ── The `--cycle-clock` mode (2026-09-18, the last surviving hypothesis) ───────────
 *
 * That default mode, and pwmwrite's, both ran 8 000 trials with ZERO failures — and
 * every one of those trials ran on a CPRMAN PWM generator that had been running for
 * MINUTES, because both probes refuse to reconfigure a generator `rpi4-audio` owns.
 * On a real boot the generator is started MICROSECONDS before PWEN. That proximity is
 * the only structural difference left between a probe trial that never fails and a
 * boot that fails ~7% of the time (capture record, "Sixth hypothesis retired").
 *
 * `--cycle-clock` tests exactly that and nothing else: each trial STOPS and RESTARTS
 * the CPRMAN PWM generator with the identical sequence `audio_clockInit()` uses
 * (rpi4-audio.c:245-271) — stop → wait !BUSY → SRC while disabled, MASH masked → DIV
 * → ENAB|GATE → wait BUSY — and then immediately runs the same per-trial work. The
 * `--clock-gap-us` knob inserts a delay between "the generator reports BUSY" and the
 * PWM init that ends in PWEN, so the proximity can be SWEPT rather than assumed:
 *
 *   pwmdma 500 5000 0 --cycle-clock --clock-gap-us 0
 *   pwmdma 500 5000 0 --cycle-clock --clock-gap-us 10
 *   pwmdma 500 5000 0 --cycle-clock --clock-gap-us 100
 *   pwmdma 500 5000 0 --cycle-clock --clock-gap-us 1000
 *
 * (Four command lines rather than a built-in sweep loop, because psh cannot chain
 * commands and a per-invocation verdict line is what a boot log can be grepped for.)
 *
 * ⚠⚠ `--cycle-clock` IS INTRUSIVE AND MUST BE ASKED FOR. The CPRMAN PWM generator is
 * SHARED between PWM0 and PWM1, and `rpi4-audio` streams silence over PWM1 + DMA
 * channel 5 forever. Stopping that generator WILL disturb the audio driver's stream
 * and it may not recover — the driver is not told, and nothing re-arms it. Run this
 * mode only on a dedicated probe boot with nothing playing audio. The mode is
 * unreachable by default and unreachable from a bare trial count: only the explicit
 * flag turns it on, and `--clock-gap-us` without it is REFUSED rather than ignored.
 * The probe never writes the PWM1 half of the page and never writes channel 5; it
 * only READS them, before and after, and reports what the collateral looks like.
 *
 * ── The `--cb-race` mode (2026-09-18) ─────────────────────────────────────────────
 *
 * Everything above measures whether the channel STREAMS. This mode measures something
 * narrower and much more direct: does the engine ever fetch a control block whose CPU
 * stores have NOT LANDED YET?
 *
 * The hypothesis, stated so it can be wrong. The control block lives in memory this
 * port maps Normal Inner/Outer Non-cacheable: `MAP_UNCACHED` alone sets `PGHD_NOT_CACHED`
 * (kernel vm/map.c:539-541), which selects MAIR index 1 (hal/aarch64/pmap.c:64,488-490),
 * and `MAIR_EL1_VALUE = 0x000444FF` (hal/aarch64/_init.S:137) puts 0x44 in that byte —
 * Normal, inner and outer non-cacheable. The DMA registers are mapped
 * `MAP_DEVICE | MAP_UNCACHED`, i.e. both attribute bits, which selects MAIR index 3
 * (pmap.c:65,482-486) whose byte is 0x00 — Device-nGnRnE. aarch64 orders Device-nGnRnE
 * accesses against OTHER Device accesses; it does NOT order them against Normal-NC
 * stores. So a driver that writes a control block and then immediately writes
 * `CONBLK_AD` + `CS=ACTIVE` with no `dsb` can have the engine — a non-coherent external
 * master reading DRAM directly — fetch a CB whose stores are still in flight. That is
 * the reasoning behind the `dsb sy` now in `audio_dmaArm()` (rpi4-audio.c:628-649), and
 * until this mode existed it was reasoning and not measurement.
 *
 * How a fetch of the wrong bytes is made VISIBLE. Two control blocks, A at the top of
 * the uncached page and B 64 bytes in (far enough apart that no write-buffer merge can
 * span both), identical except for the two fields the channel loads into registers the
 * CPU can read back: `SOURCE_AD` (0x0c) and `TXFR_LEN` (0x14) — Linux's
 * `BCM2835_DMA_SOURCE_AD` / `BCM2835_DMA_LEN` (bcm2835-dma.c:123,125). Each trial picks
 * the CB the previous trial did not use, writes those two fields FRESH, and then — with
 * NO barrier unless `--barrier` was passed — writes `CONBLK_AD` and `CS=ACTIVE`
 * back-to-back and immediately reads the two live registers back.
 *
 * The values cycle through THREE variants while the CB alternates between TWO blocks.
 * 2 and 3 are coprime, so on every trial the three candidate sources of the engine's
 * bytes are three DIFFERENT variants:
 *   fresh          = variant i % 3      what we just wrote, what a correct fetch reads
 *   stale (own CB) = variant (i+1) % 3  what that CB held before this trial's stores
 *   the other CB   = variant (i+2) % 3  what the block we did NOT name holds right now
 * so the register readback alone names which one the engine followed. A/B seeded at
 * setup (CB A as "trial -2", CB B as "trial -1") and drained with a `dsb sy` in BOTH
 * arms, so trial 0's stale baseline is known-landed rather than assumed.
 *
 *   variant  SOURCE offset in the ring   TXFR_LEN
 *   0        0                           32768
 *   1        16384                       24576
 *   2        32768                       16384
 *
 * ⚠ Those lengths are deliberately NOT the whole ring. The two fields are two separate
 * stores and the race can land one without the other, so every CROSS combination has to
 * be a legal transfer too: max offset (32768) + max length (32768) = 65536 = exactly the
 * ring. The engine can therefore never be pointed outside the ring, whatever it reads,
 * and `dest_ad` is PWM0's FIFO in every variant.
 *
 * What a mismatch can mean OTHER than the race, and how each is excluded:
 *   - the engine consumed part of the transfer before the readback. Variants are
 *     separated by 16384 bytes of source and 8192 of length; a match window is only
 *     CB_RACE_DRIFT (2048) bytes wide, so consumption cannot carry one variant into
 *     another's window. Anything outside every window is counted UNCLASSIFIED, never as
 *     the race, and the run prints the largest drift it actually saw.
 *   - the engine had not fetched the CB yet. `CS=RESET` (bcm2835-dma.c:147) zeroes the
 *     channel's live registers, so "not fetched" is its own bucket, and the readback
 *     polls (bounded) for the first non-zero sample. Trials 0 AND 1 REFUSE outright if
 *     the post-reset registers are not zero, because then the sentinel is worthless.
 *     ⚠ MEASURED 2026-09-18 on BCM2711: RESET clears TXFR_LEN and CONBLK_AD but NOT
 *     SOURCE_AD, which keeps the previous trial's value. So the sentinel and the
 *     "not fetched" bucket both rest on TXFR_LEN alone — it is loaded from the control
 *     block, and the three variants have distinct lengths, so it identifies the fetched
 *     block by itself. SOURCE_AD is corroborating evidence, never the discriminator.
 *   - a self-chain reloaded the CB under us. `nextconbk = 0` here: these CBs are
 *     ONE-SHOT, unlike the streaming CB the other modes use.
 *   - the compiler moved the stores. The CB is `volatile`, as are the MMIO windows, and
 *     a compiler may not reorder volatile accesses against each other. What is under
 *     test is the ARCHITECTURAL ordering, not the compiler's.
 *   - a torn fetch (one field fresh, the other stale) is the strongest evidence of all,
 *     so it is held to a stricter test: both fields must land inside SOME variant's
 *     window AND the two implied "bytes consumed" figures must agree to within
 *     CB_RACE_SLACK. Without that, a preemption between ACTIVE and the readback could
 *     drift one field into a neighbour's window and manufacture a torn reading.
 *   - the channel was stopped badly by the PREVIOUS trial. This mode halts a channel
 *     microseconds after ACTIVE, mid-burst, with WAIT_RESP set — so it stops the way
 *     Linux's `bcm2835_dma_abort()` does (bcm2835-dma.c:709-745): NEXTCONBK=0, then
 *     `ABORT|ACTIVE`, wait for ABORT to clear, clear ACTIVE, then RESET. A bare RESET
 *     with a write outstanding can latch DEBUG errors that would read as UNCLASSIFIED.
 *
 * `--preload N` writes N silence words into the ring immediately before the CB stores,
 * in BOTH arms, so the CB stores queue behind a burst of Normal-NC traffic. It widens
 * the window rather than changing what is being tested; sweep it before believing a null.
 *
 * A/B: run it twice, once without `--barrier` and once with. `--barrier` is the control
 * arm — it is the identical loop with a `dsb sy` before the MMIO writes, so it must come
 * back with zero mismatches; a REPRODUCED without its control arm is not a finding.
 * (Two invocations, not one interleaved run: the barrier is a compile-shaped decision in
 * the tight sequence, and keeping the two loops textually identical matters more here
 * than sharing a boot. A run-to-run confounder is possible and is the known weakness.)
 *
 * Verdicts (grep-able, one line, and the exit status matches; the text names the mode
 * and the clock gap so a log says which experiment produced it):
 *   REPRODUCED      k/n trials parked. The DMA→DREQ→FIFO handshake can fail with the
 *                   PWM configured correctly — reproducible in seconds, and the
 *                   re-arm result says whether the driver's containment can clear it.
 *   NOT REPRODUCED  every trial streamed.
 *   REFUSED         a precondition is not met (clock, DMA channel enable, buffer
 *                   placement, an inert knob). Says so loudly rather than producing
 *                   failures that are not the defect.
 *
 * Usage: pwmdma [trials] [settle_us] [gap_us] [--cycle-clock] [--clock-gap-us N]
 *        pwmdma --cb-race [trials] [fifo_gap_us] [--barrier] [--preload N]
 *        trials     default 500
 *        settle_us  default 5000 — at ~44.1 kHz a healthy channel advances ~440 ring
 *                   words in 5 ms; a parked one moves at most the 16-word FIFO depth
 *        gap_us     default 0 — microseconds to sit PWM-enabled before arming the
 *                   DMA. The driver's real shape has MILLISECONDS here (portCreate,
 *                   create_dev and two printfs run between audio_pwmInit() and
 *                   audio_dmaStart()), and pwmwrite's start-test had to grow exactly
 *                   this knob before its second row meant anything.
 *        --cycle-clock      stop + restart the SHARED CPRMAN PWM generator before
 *                           every trial (see the warning above). Off by default.
 *        --clock-gap-us N   microseconds between "generator reports BUSY" and the PWM
 *                           init that ends in PWEN. Default 0. Requires --cycle-clock.
 *
 *        --cb-race          the control-block visibility race described above. In THIS
 *                           mode the positionals mean [trials] [fifo_gap_us] — a third
 *                           is refused, because `settle_us` has no meaning here and an
 *                           inert knob is exactly the trap this project keeps paying for.
 *                           trials default 5000, fifo_gap_us default 500 (time for the
 *                           16-word PWM FIFO to drain between trials, so every trial's
 *                           DREQ is asserted).
 *        --barrier          --cb-race only: `dsb sy` before the MMIO writes. The CONTROL
 *                           arm; expected count is zero.
 *        --preload N        --cb-race only: silence words written to the ring immediately
 *                           before the CB stores, to widen the window. Default 256.
 *
 * Copyright 2026 Phoenix Systems
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

/* ---------------------------------------------------------------------------
 * Register map. Every layout below is cited: this file's sibling probe already
 * cost the project an hour from a bit decoded from memory (PWM_STA's BERR/STA1
 * swap), so nothing here is recalled.
 * ------------------------------------------------------------------------- */

/* PWM0: unused by this port. bcm283x.dtsi:413-415 gives `pwm@7e20c000` with
 * `reg = <0x7e20c000 0x28>`; bcm2711.dtsi:275-277 gives PWM1 at 0x7e20c800, which
 * is rpi4-audio's. Bus 0x7e... maps to ARM low-peripheral 0xfe... */
#define PWM0_PAGE 0xfe20c000u

/* PWM1 lives at +0x800, i.e. INSIDE the same 4 kB page this probe maps for PWM0.
 * That is convenient and dangerous in equal measure: it is how the collateral report
 * can read rpi4-audio's own channel, and it is why every write in this file indexes
 * PWM0's half only. Nothing below ever writes through PWM1_OFF. */
#define PWM1_OFF (0x800u / 4u)

/* PWM register offsets (word index) — rpi4-audio.c:63-72. */
#define PWM_CTL  (0x00u / 4u)
#define PWM_STA  (0x04u / 4u)
#define PWM_DMAC (0x08u / 4u)
#define PWM_RNG1 (0x10u / 4u)
#define PWM_DAT1 (0x14u / 4u)
#define PWM_RNG2 (0x20u / 4u)

/* PWM_CTL bits — rpi4-audio.c:75-83. */
#define CTL_PWEN1 (1u << 0)
#define CTL_USEF1 (1u << 5)
#define CTL_CLRF1 (1u << 6)
#define CTL_MSEN1 (1u << 7)
#define CTL_PWEN2 (1u << 8)
#define CTL_USEF2 (1u << 13)
#define CTL_MSEN2 (1u << 15)

/* The exact enable word audio_pwmInit() writes (rpi4-audio.c:306): both channels
 * FIFO-fed (USEF), mark/space (MSEN), enabled (PWEN). */
#define CTL_AUDIO_ENABLE \
	(CTL_USEF1 | CTL_MSEN1 | CTL_PWEN1 | CTL_USEF2 | CTL_MSEN2 | CTL_PWEN2)

/* PWM_STA bits — rpi4-audio.c:86-89 and pwmwrite.c:97-108. ⚠ bit 8 is BERR (a
 * latched BUS ERROR) and bit 9 is STA1 (channel 1 transmitting); getting these two
 * the wrong way round is the mistake this project already paid for once. */
#define STA_FULL1 (1u << 0)
#define STA_EMPT1 (1u << 1)
#define STA_BERR  (1u << 8)
#define STA_STA1  (1u << 9)

/* PWM_DMAC (0x08): ENAB(31) | PANIC[15:8] | DREQ[7:0] — rpi4-audio.c:170-171. The
 * thresholds below are the driver's (PANIC=8, DREQ=4); there is no upstream PWM-FIFO
 * DMA driver anywhere to compare them against (capture record, "What upstream says
 * about the rest of our sequence"), so they are copied, not chosen. */
#define PWM_DMAC_ENAB  (1u << 31)
#define PWM_DMAC_AUDIO (PWM_DMAC_ENAB | (8u << 8) | (4u << 0))

/* CPRMAN PWM clock — SHARED between PWM0 and PWM1, i.e. with rpi4-audio. Offsets,
 * password and bit meanings from rpi4-audio.c:92-121. */
#define CPRMAN_PAGE  0xfe101000u
#define CM_PWMCTL    (0xa0u / 4u)
#define CM_PWMDIV    (0xa4u / 4u)
#define CM_PASSWD    0x5a000000u
#define CM_CTL_ENAB  (1u << 4)
#define CM_CTL_GATE  (1u << 6)
#define CM_CTL_BUSY  (1u << 7)
#define CM_CTL_MASH  (3u << 9)
#define CM_SRC_MASK  0xfu
#define CM_SRC_OSC   1u          /* BCM2711 crystal oscillator, 54 MHz */
#define CM_OSC_HZ    54000000u
#define PWM_CLK_DIVI 2u          /* what audio_clockInit() programs -> 27 MHz */

/* Period + rate, the driver's (rpi4-audio.c:131-133). */
#define PWM_RANGE  612u
#define AUDIO_RATE (27000000u / PWM_RANGE)   /* ~44117 Hz */

#define SPIN_MAX 1000000u

/* ---------------------------------------------------------------------------
 * Legacy DMA controller. Channel registers at DMA_BASE + chan * 0x100.
 *
 * CHANNEL 6, and here is the whole argument: bcm2711.dtsi:106 declares
 * `brcm,dma-channel-mask = <0x07f5>`, i.e. bits {0,2,4,5,6,7,8,9,10} are the
 * channels the firmware leaves to the OS. 6 is in that set, and 5 — which is
 * rpi4-audio's (rpi4-audio.c:143) — is the one channel this probe must not touch.
 * ------------------------------------------------------------------------- */
#define DMA_BASE  0xfe007000u
#define DMA_CHAN  6u
#define DMA_AUDIO 5u                   /* rpi4-audio.c:143 — READ-ONLY here, never armed */
#define DMA_CHANNEL_MASK_DTS 0x07f5u   /* bcm2711.dtsi:106, for the entry print */

#define DMA_CS         (0x00u / 4u)
#define DMA_CONBLK_AD  (0x04u / 4u)
#define DMA_SOURCE_AD  (0x0cu / 4u)
#define DMA_DEST_AD    (0x10u / 4u)
#define DMA_TXFR_LEN_R (0x14u / 4u)
#define DMA_NEXTCONBK  (0x1cu / 4u)
#define DMA_DEBUG      (0x20u / 4u)

/* Shared across all channels: a per-channel enable bitmap at 0xff0
 * (external/linux/drivers/dma/bcm2835-dma.c:206-208, "shared registers for all dma
 * channels"). Linux only defines it — the firmware normally leaves the mask's
 * channels enabled — but a channel whose bit is clear will sit at ACTIVE and never
 * fetch, which would look exactly like the defect. Checked at entry, never trusted. */
#define DMA_ENABLE (0xff0u / 4u)

/* DMA_CS / DMA_DEBUG bits — rpi4-audio.c:151-162, cross-checked against
 * bcm2835-dma.c. CS bit 5 is ISHELD ("Is held by DREQ flow control",
 * bcm2835-dma.c:136) and bit 3 the live DREQ state (:134): CS=0x21 is the HEALTHY
 * steady state of a DREQ-paced channel, not a stall signature. */
#define DMA_CS_ACTIVE (1u << 0)
#define DMA_CS_END    (1u << 1)
#define DMA_CS_ERROR  (1u << 8)
/* BIT(30), "Stop current CB, go to next, WO" — bcm2835-dma.c:146. Used only by the
 * --cb-race stop path, which mirrors bcm2835_dma_abort() (bcm2835-dma.c:709-745). */
#define DMA_CS_ABORT  (1u << 30)
#define DMA_CS_RESET  (1u << 31)   /* "WO, self clearing" — bcm2835-dma.c:147 */
#define DMA_DBG_LAST_NOT_SET (1u << 0)
#define DMA_DBG_FIFO_ERR     (1u << 1)
#define DMA_DBG_READ_ERR     (1u << 2)
#define DMA_DBG_ERRORS       (DMA_DBG_LAST_NOT_SET | DMA_DBG_FIFO_ERR | DMA_DBG_READ_ERR)

/* Transfer-info (CB word 0) — rpi4-audio.c:164-167. PERMAP is bits [20:16]:
 * `BCM2835_DMA_PER_MAP(x) ((x & 31) << 16)` (bcm2835-dma.c:167). */
#define TI_WAIT_RESP  (1u << 3)
#define TI_DEST_DREQ  (1u << 6)
#define TI_SRC_INC    (1u << 8)
#define TI_PERMAP(x)  (((x) & 31u) << 16)

/* DREQ 5 = the legacy PWM (PWM0). ⚠ EVIDENCE, stated exactly, because this one is
 * INFERRED and not a direct citation: there is no PWM `dmas` property anywhere in
 * the Linux tree (the analogue jack is driven by VideoCore firmware over VCHI), so
 * no upstream node names the PWM DREQ. What does exist is every OTHER node's DREQ
 * number, and they match the BCM2835 peripherals §4.2.1.3 DREQ table slot for slot:
 *   i2s/PCM TX+RX = 2,3   (bcm2835-common.dtsi:196)
 *   SMI           = 4     (bcm270x.dtsi:83)
 *   SPI TX+RX     = 6,7   (bcm2835-common.dtsi:206)
 *   e.MMC         = 11    (bcm270x.dtsi:43)
 *   SD HOST       = 13    (bcm2835-common.dtsi:201)
 *   HDMI          = 17    (bcm2835-common.dtsi:133)
 * Those are the same table, and slot 5 of that table is PWM. bcm2711.dtsi EXTENDS
 * that numbering rather than replacing it -- hdmi1's audio-rx keeps the 2835 HDMI
 * slot 17 (bcm2711.dtsi:398) while the second controller, hdmi0, takes 10
 * (:357), a slot the 2835 table leaves unused -- and it is SILENT on 1 and 5: no
 * node in it uses either number. Independently, rpi4-audio's PERMAP 1
 * for the BCM2711 PWM1 instance (rpi4-audio.c:167) is empirically live — it streams
 * 1784-2352 ring words per 20 ms on healthy boots — which is what re-points 1 at
 * PWM1 and leaves 5 as the legacy PWM0 slot. If EVERY trial here parks, suspect this
 * number first; the verdict says so. */
#define DREQ_PWM0 5u

/* PWM0's FIFO, peripheral bus address: PWM0 base 0x7e20c000 (bcm283x.dtsi:415) +
 * FIF1 at 0x18. The driver's PWM1 equivalent is 0x7e20c818 (rpi4-audio.c:168). */
#define PWM0_FIF1_BUS 0x7e20c018u

/* DRAM through the 0xC0000000 legacy uncached alias, which only reaches the low
 * 1 GB — rpi4-audio.c:169. */
#define DRAM_BUS(pa) (0xc0000000u | ((uint32_t)(pa) & 0x3fffffffu))

/* Ring + threshold: the driver's, unchanged (rpi4-audio.c:176-186). */
#define RING_WORDS 16384u
#define RING_BYTES (RING_WORDS * 4u)
#define DMA_START_MIN_WORDS 64u

/* Advance histogram: log2-ish buckets around the ~440 words a healthy channel
 * covers in the default 5 ms settle. */
#define HIST_BUCKETS 10u

/* --cb-race geometry. See the header comment for why the lengths stop short of the
 * ring: every CROSS combination of a source offset with a length must also be a legal
 * transfer, because a torn fetch can pair any offset with any length. */
#define CB_RACE_VARIANTS 3u
#define CB_RACE_B_OFF    64u      /* byte offset of control block B inside the page */
#define CB_RACE_DRIFT    2048u    /* bytes the engine may consume before the readback */
#define CB_RACE_SLACK    256u     /* allowed disagreement between the two consumed figures */
#define CB_RACE_POLL     20000u   /* readback polls before giving up on a fetch */
#define CB_RACE_SPINS    10000u   /* abort/reset handshake spins */
#define CB_RACE_TRIALS   5000ul   /* default trial count for the mode */
#define CB_RACE_FIFO_US  500ul    /* default inter-trial gap: lets the PWM FIFO drain */
#define CB_RACE_PRELOAD  256ul    /* default window amplifier, in ring words */

/* How long the collateral report watches rpi4-audio's channel before deciding
 * whether it is still moving. A healthy ch5 covers ~1 800 ring words per 20 ms
 * (rpi4-audio.c:552-553), so 20 ms is far more than enough to tell moving from
 * parked. */
#define AUDIO_WATCH_US 20000u


/* DMA control block (32 bytes, 256-bit aligned) — rpi4-audio.c:190-198. */
typedef struct {
	uint32_t ti;
	uint32_t source_ad;
	uint32_t dest_ad;
	uint32_t txfr_len;
	uint32_t stride;
	uint32_t nextconbk;
	uint32_t pad[2];
} dma_cb_t;


/* A read-only look at the hardware rpi4-audio owns: PWM1's half of the PWM page and
 * legacy DMA channel 5. Never written, only sampled. */
typedef struct {
	uint32_t cs;
	uint32_t debug;
	uint32_t source_ad;
	uint32_t ctl;
	uint32_t sta;
} audio_snap_t;


static volatile uint32_t *pwm;       /* PWM0 (and, at PWM1_OFF, the audio driver's — read-only) */
static volatile uint32_t *cprman;
static volatile uint32_t *dmapage;   /* whole DMA page, for the shared ENABLE word */
static volatile uint32_t *dma;       /* our channel's register block */
static volatile uint32_t *dmaAudio;  /* channel 5's register block — read-only */
static volatile uint32_t *ring;
static uintptr_t ring_pa;
static uintptr_t cb_pa;
static unsigned long gapUs;

/* --cb-race state. `cbRace` is the only way into that mode; `cbBarrier` selects the
 * control arm. The variant tables are the two fields the engine loads into registers
 * the CPU can read back — see the header comment for the geometry argument. */
static int cbRace;
static int cbBarrier;
static unsigned long preloadWords = CB_RACE_PRELOAD;
static unsigned long fifoGapUs = CB_RACE_FIFO_US;
static const uint32_t cbRaceOff[CB_RACE_VARIANTS] = { 0u, 16384u, 32768u };
static const uint32_t cbRaceLen[CB_RACE_VARIANTS] = { 32768u, 24576u, 16384u };

/* --cycle-clock state. `cycleClock` is the ONLY way into the intrusive path. */
static int cycleClock;
static unsigned long clockGapUs;
static unsigned long clkStillBusy;   /* generator did not stop before reprogramming */
static uint32_t postClkCtl, postClkDiv;  /* CM_PWMCTL/DIV right after the restart */

/* Gap calibration, trial 0 only: usleep()'s real floor is not documented for this
 * port, and a sweep whose 10 us and 100 us points collapse onto the same achieved
 * delay is a null nobody can read. Only trial 0 is timestamped, so trials 1..N-1 are
 * not perturbed by the two clock_gettime() syscalls. */
static int gapMeasure;
static struct timespec tsBusy, tsPwen;


static long usSince(const struct timespec *a, const struct timespec *b)
{
	return (long)(b->tv_sec - a->tv_sec) * 1000000l + (b->tv_nsec - a->tv_nsec) / 1000l;
}


/* The CPRMAN PWM generator bring-up, EXACTLY as audio_clockInit() does it today
 * (rpi4-audio.c:245-271): stop the generator, then SRC while disabled, then DIV,
 * then ENAB — three separate writes, MASH masked — which is Linux's order and not
 * the obvious one. (pwmwrite.c still carries the older single-write shape; it is
 * deliberately not copied.)
 *
 * This is the ONE copy of that sequence in this file: clockEnsure() guards it and
 * --cycle-clock calls it directly, so "the driver's real shape" stays a single
 * audited body rather than two that can drift apart.
 *
 * Returns 0 if the generator reports BUSY afterwards, -1 if it will not come up. */
static int clockProgram(void)
{
	uint32_t spin, ctl;

	/* Stop the generator before touching SRC or DIV (keep KILL low), wait for !BUSY. */
	ctl = cprman[CM_PWMCTL] & ~(CM_PASSWD | CM_CTL_ENAB);
	cprman[CM_PWMCTL] = CM_PASSWD | ctl;
	for (spin = SPIN_MAX; (spin != 0u) && ((cprman[CM_PWMCTL] & CM_CTL_BUSY) != 0u); spin--) {
	}
	if ((cprman[CM_PWMCTL] & CM_CTL_BUSY) != 0u) {
		/* The driver prints here; a probe doing this thousands of times counts instead. */
		clkStillBusy++;
	}

	/* Source while disabled, then the divider, then enable — three separate writes. */
	ctl = (cprman[CM_PWMCTL] & ~(CM_PASSWD | CM_CTL_ENAB | CM_CTL_MASH | CM_SRC_MASK)) | CM_SRC_OSC;
	cprman[CM_PWMCTL] = CM_PASSWD | ctl;
	cprman[CM_PWMDIV] = CM_PASSWD | (PWM_CLK_DIVI << 12);
	cprman[CM_PWMCTL] = CM_PASSWD | ctl | CM_CTL_ENAB | CM_CTL_GATE;

	for (spin = SPIN_MAX; (spin != 0u) && ((cprman[CM_PWMCTL] & CM_CTL_BUSY) == 0u); spin--) {
	}
	return ((cprman[CM_PWMCTL] & CM_CTL_BUSY) != 0u) ? 0 : -1;
}


/* Default-mode policy, pwmwrite's, and it matters: rpi4-audio OWNS this generator.
 * Returns 1 if it was already running (left strictly untouched), 0 if started here,
 * -1 if it will not come up. Never reconfigures a running clock. */
static int clockEnsure(void)
{
	if ((cprman[CM_PWMCTL] & CM_CTL_BUSY) != 0u) {
		return 1;
	}
	return clockProgram();
}


/* One --cycle-clock restart: the driver's sequence, plus the snapshot the first
 * parked trial prints and the trial-0 timestamp the gap calibration needs. */
static int clockCycle(void)
{
	int rc = clockProgram();

	if (gapMeasure != 0) {
		(void)clock_gettime(CLOCK_MONOTONIC, &tsBusy);
	}
	postClkCtl = cprman[CM_PWMCTL];
	postClkDiv = cprman[CM_PWMDIV];
	return rc;
}


/* Leave the shared generator RUNNING, always. A probe that exits with the PWM clock
 * stopped would take rpi4-audio down for the rest of the boot with no way back. */
static void clockLeaveRunning(void)
{
	if ((cprman[CM_PWMCTL] & CM_CTL_BUSY) == 0u) {
		printf("pwmdma: shared PWM clock is STOPPED at exit — restarting it so rpi4-audio's "
			"generator is left running.\n");
		if (clockProgram() != 0) {
			printf("pwmdma: ⚠ the shared PWM clock will NOT report BUSY again. PWM1 cannot "
				"transmit until something reprograms CM_PWMCTL; reboot before trusting audio.\n");
		}
	}
}


/* audio_pwmInit() (rpi4-audio.c:275-325) on PWM0, pacing and BERR clear included.
 * The pacing is not cosmetic: unpaced back-to-back writes to PWM_CTL latch a bus
 * error, which is the one defect the 2026-09-18 night actually fixed.
 *
 * ⓘ That pacing means PWEN lands ~30 us after this function is entered — in the
 * DRIVER too, which runs the identical body — so --clock-gap-us 0 is the driver's
 * own clock→PWEN distance, and the knob adds to it rather than defining it. The
 * trial-0 calibration print reports the distance actually achieved. */
static void pwmInit0(void)
{
	pwm[PWM_CTL] = 0u;
	(void)pwm[PWM_CTL];
	usleep(10);

	pwm[PWM_RNG1] = PWM_RANGE;
	(void)pwm[PWM_RNG1];
	pwm[PWM_RNG2] = PWM_RANGE;
	(void)pwm[PWM_RNG2];
	usleep(10);

	pwm[PWM_CTL] = CTL_CLRF1;
	(void)pwm[PWM_CTL];
	usleep(10);

	if (gapMeasure != 0) {
		(void)clock_gettime(CLOCK_MONOTONIC, &tsPwen);
	}
	pwm[PWM_CTL] = CTL_AUDIO_ENABLE;
	(void)pwm[PWM_CTL];
	usleep(10);

	pwm[PWM_STA] = STA_BERR;   /* W1C, so this trial's STA describes this trial */
}


/* Ring words consumed since the control block was (re)loaded.
 *
 * ⚠ Measured as the DISTANCE FROM THE RING BASE, not as a delta against a cursor
 * sampled just after CS=ACTIVE. The CB fixes source_ad at the ring base and every
 * trial rewrites CONBLK_AD, so after the load SOURCE_AD is base + progress — while a
 * pre-ACTIVE sample can still hold the PREVIOUS trial's cursor for as long as the
 * engine takes to fetch the CB, and a delta against that reads a PARKED channel as a
 * huge advance. (The driver's audio_dmaArm() takes its delta the other way and is
 * safe at first arm because the channel starts from RESET; on a re-arm it has the
 * same hazard.) The settle is far shorter than one lap of the ring, so no wrap.
 *
 * A cursor OUTSIDE the ring means the engine never loaded the CB (RESET zeroes
 * SOURCE_AD) — that is zero progress, not a wrapped index. */
static uint32_t ringAdvance(void)
{
	uint32_t src = dma[DMA_SOURCE_AD] & 0x3fffffffu;
	uint32_t base = (uint32_t)ring_pa & 0x3fffffffu;
	uint32_t off = src - base;

	return (off < RING_BYTES) ? (off / 4u) : 0u;
}


/* audio_dmaArm() (rpi4-audio.c:525-550) on PWM0 + our channel: enable the PWM's DMA
 * request, reset the channel, clear its latched errors, point it at the CB, go.
 * Returns the ring words covered across the settle. */
static uint32_t dmaArm(unsigned long settleUs)
{
	uint32_t spins;

	pwm[PWM_DMAC] = PWM_DMAC_AUDIO;

	dma[DMA_CS] = DMA_CS_RESET;
	for (spins = 10000u; (spins != 0u) && ((dma[DMA_CS] & DMA_CS_RESET) != 0u); spins--) {
	}
	dma[DMA_DEBUG] = DMA_DBG_ERRORS;
	dma[DMA_CONBLK_AD] = DRAM_BUS(cb_pa);
	dma[DMA_CS] = DMA_CS_ACTIVE;

	usleep((unsigned int)settleUs);
	return ringAdvance();
}


/* Everything the driver prints at a stall, plus the advance figure. */
static void dumpState(const char *what, unsigned long trial, uint32_t advance)
{
	printf("pwmdma: %s trial %lu — advanced %u ring words (want >= %u)\n"
		"pwmdma:   PWM_STA=0x%08x PWM_CTL=0x%08x PWM_DMAC=0x%08x RNG1=%u DAT1=%u\n"
		"pwmdma:   CM_PWMCTL=0x%08x CM_PWMDIV=0x%08x\n"
		"pwmdma:   DMA_CS=0x%08x DMA_DEBUG=0x%08x CONBLK=0x%08x SRC=0x%08x DEST=0x%08x "
		"LEN=%u NEXT=0x%08x\n",
		what, trial, advance, DMA_START_MIN_WORDS,
		pwm[PWM_STA], pwm[PWM_CTL], pwm[PWM_DMAC], pwm[PWM_RNG1], pwm[PWM_DAT1],
		cprman[CM_PWMCTL], cprman[CM_PWMDIV],
		dma[DMA_CS], dma[DMA_DEBUG], dma[DMA_CONBLK_AD], dma[DMA_SOURCE_AD],
		dma[DMA_DEST_AD], dma[DMA_TXFR_LEN_R], dma[DMA_NEXTCONBK]);

	if (cycleClock != 0) {
		/* The state the generator was in when THIS trial's PWM init started, as opposed
		 * to the live values above, which are read after the settle. */
		printf("pwmdma:   post-restart CM_PWMCTL=0x%08x CM_PWMDIV=0x%08x (sampled immediately "
			"after this trial's restart reported BUSY). 0x91 = SRC(1)|ENAB|BUSY and is the "
			"EXPECTED reading: CM_GATE (bit 6) is written but does NOT read back on this "
			"block (rpi4-audio.c:99-108), so a clear bit 6 is not a lost GATE.\n",
			postClkCtl, postClkDiv);
	}
}


static unsigned int histBucket(uint32_t advance)
{
	if (advance == 0u) {
		return 0u;
	}
	if (advance <= 16u) {
		return 1u;
	}
	if (advance < 64u) {
		return 2u;
	}
	if (advance < 128u) {
		return 3u;
	}
	if (advance < 256u) {
		return 4u;
	}
	if (advance < 512u) {
		return 5u;
	}
	if (advance < 1024u) {
		return 6u;
	}
	if (advance < 2048u) {
		return 7u;
	}
	if (advance < 4096u) {
		return 8u;
	}
	return 9u;
}


static const char *histLabel(unsigned int b)
{
	static const char *const names[HIST_BUCKETS] = {
		"0 (no progress, or CB never fetched)",
		"1..16 (<= FIFO depth: PARKED)",
		"17..63 (below threshold)",
		"64..127",
		"128..255",
		"256..511",
		"512..1023",
		"1024..2047",
		"2048..4095",
		">= 4096",
	};
	return names[b];
}


/* READ-ONLY sample of the hardware rpi4-audio owns. Not one write anywhere in here. */
static void audioSnap(audio_snap_t *s)
{
	s->cs = dmaAudio[DMA_CS];
	s->debug = dmaAudio[DMA_DEBUG];
	s->source_ad = dmaAudio[DMA_SOURCE_AD];
	s->ctl = pwm[PWM1_OFF + PWM_CTL];
	s->sta = pwm[PWM1_OFF + PWM_STA];
}


/* Two samples AUDIO_WATCH_US apart, because a single CS reading cannot answer the
 * question: DMA_CS=0x21 is ACTIVE|ISHELD, the HEALTHY steady state of a DREQ-paced
 * channel (bcm2835-dma.c:134-136), and it is also what a parked channel reads. Only
 * the read cursor moving distinguishes them. A plain != is enough here — unlike the
 * probe's own re-armed channel, ch5 has been streaming since boot and is never
 * re-armed by us, so there is no pre-ACTIVE cursor hazard. */
static void audioWatch(const char *when)
{
	audio_snap_t a, b;

	audioSnap(&a);
	usleep(AUDIO_WATCH_US);
	audioSnap(&b);

	printf("pwmdma: [audio-side, read-only] %s: DMA ch%u CS=0x%08x DEBUG=0x%08x SRC 0x%08x->0x%08x "
		"(%s over %u us); PWM1 CTL=0x%08x STA=0x%08x\n",
		when, DMA_AUDIO, b.cs, b.debug, a.source_ad, b.source_ad,
		(a.source_ad != b.source_ad) ? "MOVING" : "did NOT move", AUDIO_WATCH_US,
		b.ctl, b.sta);
}


/* Park the hardware: channel stopped, PWM0 no longer requesting, PWM0 disabled. */
static void parkHardware(void)
{
	if (dma != NULL) {
		dma[DMA_CS] = 0u;
	}
	if (pwm != NULL) {
		pwm[PWM_DMAC] = 0u;
		pwm[PWM_CTL] = 0u;
		(void)pwm[PWM_CTL];
	}
}


static int refuse(const char *reason)
{
	printf("pwmdma: VERDICT: REFUSED (%s)\n", reason);
	return 2;
}


/* One string that names the experiment, so a boot log's verdict line says which of
 * the sweep points produced it. */
static void modeTag(char *buf, size_t n)
{
	if (cbRace != 0) {
		snprintf(buf, n, "mode=cb-race barrier=%s preload=%lu words fifo-gap=%lu us",
			(cbBarrier != 0) ? "ON (control arm)" : "OFF (test arm)", preloadWords, fifoGapUs);
	}
	else if (cycleClock != 0) {
		snprintf(buf, n, "mode=cycle-clock clock-gap=%lu us", clockGapUs);
	}
	else {
		snprintf(buf, n, "mode=settled-clock (generator untouched)");
	}
}


/* The ring and the control-block page every mode shares, allocated ONCE: the thing
 * under test is never the allocator, and a fresh mapping per trial would also change
 * the physical address the DMA fetches from every time. Returns 0 on success, or the
 * refusal's exit status having already printed the refusal line. */
static int buffersAlloc(dma_cb_t **cbOut)
{
	dma_cb_t *cb;
	unsigned long i;

	cb = mmap(NULL, _PAGE_SIZE, PROT_READ | PROT_WRITE,
		MAP_CONTIGUOUS | MAP_UNCACHED | MAP_ANONYMOUS, -1, 0);
	ring = mmap(NULL, (RING_BYTES + _PAGE_SIZE - 1u) & ~((uint32_t)_PAGE_SIZE - 1u),
		PROT_READ | PROT_WRITE, MAP_CONTIGUOUS | MAP_UNCACHED | MAP_ANONYMOUS, -1, 0);
	if ((cb == MAP_FAILED) || (ring == MAP_FAILED)) {
		return refuse("could not mmap a contiguous uncached ring + control block");
	}

	for (i = 0ul; i < RING_WORDS; i++) {
		ring[i] = PWM_RANGE / 2u;   /* mid-scale = silence */
	}
	ring_pa = (uintptr_t)va2pa((void *)ring);
	cb_pa = (uintptr_t)va2pa(cb);

	/* rpi4-audio.c:585-598, verbatim in intent: the 0xC0000000 alias reaches only the
	 * low 1 GB, so check each buffer's LAST byte — a ring based just under 1 GB can
	 * still straddle the boundary and DMA its tail from the low alias. Refuse rather
	 * than drive a bad DMA. */
	if (((((ring_pa + RING_BYTES - 1u) | (cb_pa + _PAGE_SIZE - 1u)) >> 30) != 0)) {
		printf("pwmdma: ring PA=0x%08x cb PA=0x%08x — at or above 1 GB, and the legacy "
			"0xC0000000 DMA alias cannot reach there; DRAM_BUS() would TRUNCATE the "
			"address and the engine would fetch unrelated DRAM.\n",
			(uint32_t)ring_pa, (uint32_t)cb_pa);
		return refuse("DMA buffer physical address >= 1GB");
	}

	*cbOut = cb;
	return 0;
}


static int run(unsigned long trials, unsigned long settleUs)
{
	unsigned long i, parked = 0ul, clkFail = 0ul, step;
	unsigned long hist[HIST_BUCKETS];
	unsigned int b;
	uint32_t advance, reArm;
	int firstDumped = 0, reArmOk = -1, calibrated = 0, rc;
	char tag[128];
	dma_cb_t *cb;

	memset(hist, 0, sizeof(hist));
	modeTag(tag, sizeof(tag));

	rc = buffersAlloc(&cb);
	if (rc != 0) {
		return rc;
	}

	cb->ti = TI_WAIT_RESP | TI_DEST_DREQ | TI_SRC_INC | TI_PERMAP(DREQ_PWM0);
	cb->source_ad = DRAM_BUS(ring_pa);
	cb->dest_ad = PWM0_FIF1_BUS;
	cb->txfr_len = RING_BYTES;
	cb->stride = 0u;
	cb->nextconbk = DRAM_BUS(cb_pa);   /* self-chain -> loop the ring forever */
	cb->pad[0] = cb->pad[1] = 0u;

	printf("pwmdma: ring PA=0x%08x (%u words) cb PA=0x%08x ti=0x%08x dest=0x%08x "
		"(PWM0 FIF1) permap=%u\n",
		(uint32_t)ring_pa, RING_WORDS, (uint32_t)cb_pa, cb->ti, cb->dest_ad, DREQ_PWM0);
	printf("pwmdma: %lu trials, settle %lu us, enable->arm gap %lu us, threshold %u words "
		"(a healthy channel covers ~%lu at %u Hz; a parked one <= the 16-word FIFO) [%s]\n",
		trials, settleUs, gapUs, DMA_START_MIN_WORDS,
		((unsigned long)AUDIO_RATE * settleUs) / 1000000ul, AUDIO_RATE, tag);

	/* The threshold is the driver's fixed 64 words, so a settle too short to cover
	 * comfortably more than that parks 100%% of trials for a reason that is the CLI,
	 * not the defect. Refuse rather than print a REPRODUCED verdict nobody can trust. */
	if ((((unsigned long)AUDIO_RATE * settleUs) / 1000000ul) < (2ul * DMA_START_MIN_WORDS)) {
		printf("pwmdma: a %lu us settle covers only ~%lu ring words at %u Hz, against a fixed "
			"%u-word threshold.\n", settleUs,
			((unsigned long)AUDIO_RATE * settleUs) / 1000000ul, AUDIO_RATE,
			DMA_START_MIN_WORDS);
		return refuse("settle too short for the progress threshold — use >= 3000 us");
	}

	if (cycleClock != 0) {
		audioWatch("BEFORE any clock cycling");
	}

	step = trials / 10ul;
	if (step == 0ul) {
		step = 1ul;
	}

	for (i = 0ul; i < trials; i++) {
		if (cycleClock != 0) {
			/* Calibrate on the first SUCCESSFUL cycle, not on trial 0: if trial 0 is the
			 * one clock failure, a sweep point would come back with no calibration line
			 * and its achieved gap would be unknown. */
			gapMeasure = (calibrated == 0) ? 1 : 0;

			/* Disable PWM0 BEFORE stopping the generator, so every trial cycles the clock
			 * from the same PWM state a real boot does: on a boot the generator is
			 * (re)programmed with the PWM block in the firmware's idle state, not with a
			 * channel this probe left enabled by the previous trial. This write is outside
			 * the clock sequence and before it, so the "clock BUSY -> PWEN" distance the
			 * knob sweeps is unaffected. */
			pwm[PWM_CTL] = 0u;
			(void)pwm[PWM_CTL];
			usleep(10);

			if (clockCycle() != 0) {
				/* Its own bucket, never folded into `parked`: a trial that never had a
				 * running clock did not test the handshake, and counting it as the defect
				 * would manufacture a REPRODUCED verdict nobody can trust. */
				clkFail++;
				if (clkFail == 1ul) {
					printf("pwmdma: trial %lu — the generator did NOT report BUSY after the "
						"restart (CM_PWMCTL=0x%08x CM_PWMDIV=0x%08x). Not armed, not counted "
						"as parked.\n", i, cprman[CM_PWMCTL], cprman[CM_PWMDIV]);
				}
				continue;
			}
			if (clockGapUs != 0ul) {
				usleep((unsigned int)clockGapUs);
			}
		}

		pwmInit0();
		if (gapMeasure != 0) {
			gapMeasure = 0;
			calibrated = 1;
			printf("pwmdma: gap calibration (trial 0 only, so later trials stay unperturbed): "
				"clock BUSY -> PWEN measured %ld us at --clock-gap-us %lu. The figure includes "
				"pwmInit0()'s own 3 x usleep(10) pacing — which the DRIVER also has — and two "
				"clock_gettime() syscalls, so read it as an upper bound. If two sweep points "
				"report the same figure, usleep()'s floor collapsed them and the difference "
				"between them is not evidence.\n",
				usSince(&tsBusy, &tsPwen), clockGapUs);
		}
		if (gapUs != 0ul) {
			usleep((unsigned int)gapUs);
		}
		advance = dmaArm(settleUs);
		hist[histBucket(advance)]++;

		if (advance < DMA_START_MIN_WORDS) {
			parked++;
			if (firstDumped == 0) {
				firstDumped = 1;
				dumpState("FIRST PARKED", i, advance);

				/* One re-arm, the driver's own containment move (rpi4-audio.c:612-627):
				 * a paced PWM re-init plus a fresh DMA arm. Whether it un-sticks the
				 * channel is the discrimination the driver's re-arm print was built for
				 * — a state a CTL re-init can reach, versus one it cannot. */
				dma[DMA_CS] = 0u;
				pwmInit0();
				reArm = dmaArm(settleUs);
				reArmOk = (reArm >= DMA_START_MIN_WORDS) ? 1 : 0;
				printf("pwmdma: RE-ARM after the first parked trial: advanced %u words — %s\n",
					reArm,
					(reArmOk == 1) ?
						"RECOVERED (the parked channel IS re-armable, so the driver's "
						"containment can clear this state)" :
						"did NOT recover (a paced CTL=0 + CLRF1 + re-enable does not reach "
						"whatever state this is)");
			}
		}

		/* Stop the channel AND drop the PWM's DMA request between trials: on a real
		 * boot PWM_DMAC is 0 when audio_pwmInit() runs, so leaving ENAB set would make
		 * trials 2..N a different shape from the one being reproduced. */
		dma[DMA_CS] = 0u;
		pwm[PWM_DMAC] = 0u;

		if (((i + 1ul) % step) == 0ul) {
			printf("pwmdma: %lu/%lu trials, %lu parked, %lu clock restarts failed\n",
				i + 1ul, trials, parked, clkFail);
		}
	}

	parkHardware();
	if (cycleClock != 0) {
		clockLeaveRunning();
	}

	printf("pwmdma: advance histogram (ring words covered in the %lu us settle)\n", settleUs);
	for (b = 0u; b < HIST_BUCKETS; b++) {
		if (hist[b] != 0ul) {
			printf("pwmdma:   %-30s %lu\n", histLabel(b), hist[b]);
		}
	}
	printf("pwmdma: trials=%lu parked=%lu clock-restart-failures=%lu threshold=%u words [%s]\n",
		trials, parked, clkFail, DMA_START_MIN_WORDS, tag);
	if (cycleClock != 0) {
		printf("pwmdma: clock restarts: %lu attempted, %lu failed, %lu found the generator still "
			"BUSY after the disable write (the case audio_clockInit() warns about)\n",
			trials, clkFail, clkStillBusy);

		/* Collateral, reported as an OBSERVATION and never as this probe's verdict: the
		 * question asked here is "did the audio stall reproduce on PWM0", not "is the
		 * audio driver still alive". A parked ch5 is a consequence of the mode, expected
		 * and warned about up front — worth printing because it is evidence about what
		 * losing the shared clock does to a streaming channel, not because it grades the
		 * run. */
		audioWatch("AFTER the run (collateral observation, NOT the verdict)");
		printf("pwmdma: ⓘ if ch%u stopped moving, THIS PROBE did that by stopping the shared "
			"generator under a driver that was told nothing — the expected cost of the mode, "
			"and evidence that losing the clock mid-stream parks a DREQ-paced channel. It is "
			"NOT a reproduction of the boot-time stall and must not be reported as one.\n",
			DMA_AUDIO);
	}

	/* ⚠ A generator that never came back would otherwise produce the worst verdict this
	 * tool can emit: every trial takes the `continue` path, parked stays 0, and the run
	 * prints "NOT REPRODUCED — 0/N trials streamed" and exits 0 having armed NOTHING.
	 * That is a cannot-fail check, the exact class of bug this project keeps paying for.
	 * Refuse instead, loudly and with the refusal's exit status. */
	if ((cycleClock != 0) && (clkFail == trials)) {
		return refuse("the shared PWM generator did not report BUSY on ANY trial — nothing was "
			"armed and nothing was tested, so this run says nothing about the defect");
	}

	if (parked == 0ul) {
		if (cycleClock != 0) {
			printf("pwmdma: VERDICT: NOT REPRODUCED — %lu/%lu trials streamed (>= %u ring words "
				"each in %lu us) across %lu stop->restart->PWEN->arm cycles of the SHARED PWM "
				"generator at [%s]. At this clock->enable distance the proximity does NOT park "
				"PWM0 / channel %u / DREQ %u. ⚠ Scope: this says nothing about a mechanism "
				"specific to PWM1 or to DREQ 1, and nothing about gaps other than this one — "
				"sweep 0/10/100/1000 us before calling the hypothesis dead.\n",
				trials - clkFail, trials, DMA_START_MIN_WORDS, settleUs, trials - clkFail,
				tag, DMA_CHAN, DREQ_PWM0);
		}
		else {
			printf("pwmdma: VERDICT: NOT REPRODUCED — all %lu trials streamed (>= %u ring words "
				"each in %lu us) [%s]. The DMA->DREQ->FIFO handshake starts reliably on a "
				"settled clock, so with the PWM half already retired by pwmwrite --start-test "
				"the audio stall needs something this mode does not reach: the clock-start -> "
				"PWM-enable proximity is the named survivor, and --cycle-clock is the mode that "
				"tests it.\n",
				trials, DMA_START_MIN_WORDS, settleUs, tag);
		}
		return 0;
	}

	printf("pwmdma: VERDICT: REPRODUCED — %lu/%lu trials parked (advanced < %u ring words in "
		"%lu us with the PWM enabled, clocked and DMA-requesting) [%s]. This is the audio "
		"stall's own handshake, sampled thousands of times per boot instead of ~7 times in "
		"100; the re-arm line above says whether the driver's containment clears it.\n",
		parked, trials, DMA_START_MIN_WORDS, settleUs, tag);
	if (cycleClock != 0) {
		printf("pwmdma: ⚠ how to read this in cycle-clock mode: the discriminating signature of "
			"the PROXIMITY hypothesis is a parked rate that is HIGH at --clock-gap-us 0 and "
			"FALLS as the gap grows. A rate that is the same at every gap is the clock RESTART "
			"itself (or this probe's wiring), not the proximity — sweep before concluding.\n");
	}
	if (parked == trials) {
		printf("pwmdma: ⚠ ALL trials parked, which is far likelier to be a WIRING mistake in "
			"this probe than a ~7%% defect — check PERMAP=%u (PWM0's DREQ, inferred not cited), "
			"the shared DMA ENABLE bit %u, and dest=0x%08x before believing this is the "
			"defect.%s\n", DREQ_PWM0, DMA_CHAN, PWM0_FIF1_BUS,
			(cycleClock != 0) ?
				" In this mode, also check that the generator is coming back at all: the "
				"clock-restart-failure count above must be 0." : "");
	}
	if (reArmOk == 0) {
		printf("pwmdma: the parked channel did NOT re-arm — same conclusion the driver draws "
			"when it degrades to a paced null sink.\n");
	}
	return 1;
}


/* ---------------------------------------------------------------------------
 * --cb-race: can the engine fetch a control block whose CPU stores have not landed?
 * ------------------------------------------------------------------------- */

/* What one trial's readback saw, sampled once and then classified, so the classifier
 * never re-reads a moving register and compares two different instants. */
typedef struct {
	uint32_t cs;
	uint32_t conblk;
	uint32_t src;
	uint32_t dest;
	uint32_t len;
	uint32_t next;
	uint32_t debug;
	uint32_t spins;
} cb_snap_t;


static unsigned long cbAbortTimeouts;


/* Stop the channel the way Linux's bcm2835_dma_abort() does (bcm2835-dma.c:709-745):
 * clear NEXTCONBK, set ABORT|ACTIVE, wait for ABORT to clear, drop ACTIVE, then RESET.
 *
 * Why not the bare CS=RESET the other modes use: --cb-race halts a channel a few
 * microseconds after ACTIVE, mid-burst, with WAIT_RESP set — and a reset issued with a
 * write still outstanding can latch DEBUG errors, which would land in the UNCLASSIFIED
 * bucket for a reason that is this probe rather than the race. Upstream also notes the
 * abort handshake can legitimately fail to complete "when dreqs are enabled but not
 * asserted" (bcm2835-dma.c:732-738), so a timeout here is COUNTED, not complained about. */
static void dmaAbortReset(void)
{
	uint32_t spins;

	if (dma[DMA_CONBLK_AD] != 0u) {
		dma[DMA_NEXTCONBK] = 0u;
		dma[DMA_CS] = dma[DMA_CS] | DMA_CS_ABORT | DMA_CS_ACTIVE;
		for (spins = CB_RACE_SPINS; (spins != 0u) && ((dma[DMA_CS] & DMA_CS_ABORT) != 0u); spins--) {
		}
		if ((dma[DMA_CS] & DMA_CS_ABORT) != 0u) {
			cbAbortTimeouts++;
		}
		dma[DMA_CS] = dma[DMA_CS] & ~DMA_CS_ACTIVE;
	}

	dma[DMA_CS] = DMA_CS_RESET;
	for (spins = CB_RACE_SPINS; (spins != 0u) && ((dma[DMA_CS] & DMA_CS_RESET) != 0u); spins--) {
	}
}


/* The two fields that distinguish the variants. Nothing else in the CB ever changes
 * after setup, so a trial genuinely depends on exactly these two stores landing. */
static void cbRaceSet(volatile dma_cb_t *cb, unsigned v)
{
	cb->source_ad = DRAM_BUS(ring_pa + cbRaceOff[v]);
	cb->txfr_len = cbRaceLen[v];
}


/* Which variant's SOURCE_AD window does the live cursor sit in? The engine only ever
 * advances the source, so the window is [offset, offset + CB_RACE_DRIFT] and the
 * unsigned subtraction rejects everything below it as well as everything above.
 * *consumed receives the bytes this reading says have been read. */
static int srcVariant(uint32_t live, uint32_t *consumed)
{
	uint32_t k, base = (uint32_t)ring_pa & 0x3fffffffu, d;

	for (k = 0u; k < CB_RACE_VARIANTS; k++) {
		d = (live & 0x3fffffffu) - (base + cbRaceOff[k]);
		if (d <= CB_RACE_DRIFT) {
			*consumed = d;
			return (int)k;
		}
	}
	return -1;
}


/* The same for the live remaining length, which only ever falls. */
static int lenVariant(uint32_t live, uint32_t *consumed)
{
	uint32_t k, d;

	for (k = 0u; k < CB_RACE_VARIANTS; k++) {
		d = cbRaceLen[k] - live;
		if (d <= CB_RACE_DRIFT) {
			*consumed = d;
			return (int)k;
		}
	}
	return -1;
}


/* Everything the first mismatch needs in order to be argued about later: both control
 * blocks exactly as the CPU reads them back, the channel's own view, and all three
 * candidate variants spelled out so the reader does not have to recompute them. */
static void cbRaceDump(const char *what, unsigned long trial, unsigned used, unsigned fresh,
	unsigned ownStale, unsigned otherVar, const cb_snap_t *s, volatile dma_cb_t *const *cbs,
	const uint32_t *cbBus, int sv, int lv)
{
	unsigned k;

	printf("pwmdma: [cb-race] FIRST %s at trial %lu — armed control block %c (bus 0x%08x)\n"
		"pwmdma:   expected FRESH variant %u:   SOURCE=0x%08x LEN=%u\n"
		"pwmdma:   this CB's STALE variant %u:  SOURCE=0x%08x LEN=%u\n"
		"pwmdma:   the OTHER CB holds variant %u: SOURCE=0x%08x LEN=%u\n"
		"pwmdma:   channel read back: CONBLK=0x%08x SRC=0x%08x DEST=0x%08x LEN=%u "
		"NEXT=0x%08x CS=0x%08x DEBUG=0x%08x (after %u readback polls)\n"
		"pwmdma:   classified: the source field matched variant %d, the length field %d "
		"(-1 = no variant's window)\n",
		what, trial, (used == 0u) ? 'A' : 'B', cbBus[used],
		fresh, DRAM_BUS(ring_pa + cbRaceOff[fresh]), cbRaceLen[fresh],
		ownStale, DRAM_BUS(ring_pa + cbRaceOff[ownStale]), cbRaceLen[ownStale],
		otherVar, DRAM_BUS(ring_pa + cbRaceOff[otherVar]), cbRaceLen[otherVar],
		s->conblk, s->src, s->dest, s->len, s->next, s->cs, s->debug, s->spins,
		sv, lv);

	for (k = 0u; k < 2u; k++) {
		printf("pwmdma:   CB %c as the CPU reads it now (bus 0x%08x): ti=0x%08x source=0x%08x "
			"dest=0x%08x len=%u stride=0x%08x next=0x%08x\n",
			(k == 0u) ? 'A' : 'B', cbBus[k], cbs[k]->ti, cbs[k]->source_ad,
			cbs[k]->dest_ad, cbs[k]->txfr_len, cbs[k]->stride, cbs[k]->nextconbk);
	}
}


static int runCbRace(unsigned long trials)
{
	volatile dma_cb_t *cbs[2];
	volatile dma_cb_t *cb;
	uint32_t cbBus[2], bus;
	unsigned long i, j, step;
	unsigned long okFresh = 0ul, staleOwn = 0ul, staleOther = 0ul, tornCnt = 0ul;
	unsigned long notFetched = 0ul, unclassified = 0ul, engineErr = 0ul, dirtyReset = 0ul;
	unsigned long conblkOdd = 0ul;
	uint32_t maxDrift = 0u, maxSpins = 0u, maxDisagree = 0u, dSrc = 0u, dLen = 0u, dis;
	unsigned fresh, ownStale, otherVar;
	int firstDumped = 0, sv = -1, lv = -1, rc;
	const char *kind;
	cb_snap_t s;
	dma_cb_t *page;
	char tag[128];

	modeTag(tag, sizeof(tag));

	rc = buffersAlloc(&page);
	if (rc != 0) {
		return rc;
	}

	/* B is 64 bytes in, not 32: two adjacent 32-byte blocks could share a write-buffer
	 * merge window, and this mode's whole argument is about what reaches DRAM when. */
	cbs[0] = (volatile dma_cb_t *)page;
	cbs[1] = (volatile dma_cb_t *)((volatile char *)page + CB_RACE_B_OFF);
	cbBus[0] = DRAM_BUS(cb_pa);
	cbBus[1] = DRAM_BUS(cb_pa + CB_RACE_B_OFF);

	for (j = 0ul; j < 2ul; j++) {
		cbs[j]->ti = TI_WAIT_RESP | TI_DEST_DREQ | TI_SRC_INC | TI_PERMAP(DREQ_PWM0);
		cbs[j]->dest_ad = PWM0_FIF1_BUS;
		cbs[j]->stride = 0u;
		cbs[j]->nextconbk = 0u;   /* ONE-SHOT — nothing can reload a CB under the readback */
		cbs[j]->pad[0] = 0u;
		cbs[j]->pad[1] = 0u;
	}

	/* Seed the stale baseline trials 0 and 1 will be compared against: CB A gets what
	 * "trial -2" would have written and CB B what "trial -1" would have. MAP_CONTIGUOUS
	 * memory is NOT zeroed on this port, so without this the first two trials would be
	 * comparing against whatever the previous owner of that DRAM left. The dsb runs in
	 * BOTH arms: an unknown-landed baseline is not the thing under test. */
	cbRaceSet(cbs[0], 1u);
	cbRaceSet(cbs[1], 2u);
	__asm__ volatile("dsb sy" ::: "memory");

	printf("pwmdma: [cb-race] ring PA=0x%08x (%u words), CB A bus=0x%08x, CB B bus=0x%08x "
		"(one-shot, nextconbk=0), dest=0x%08x (PWM0 FIF1) permap=%u\n",
		(uint32_t)ring_pa, RING_WORDS, cbBus[0], cbBus[1], PWM0_FIF1_BUS, DREQ_PWM0);
	printf("pwmdma: [cb-race] positionals in THIS mode are [trials] [fifo_gap_us]: %lu trials, "
		"%lu us inter-trial gap so the 16-word PWM FIFO drains and every trial's DREQ is "
		"asserted, %lu-word preload [%s]\n", trials, fifoGapUs, preloadWords, tag);
	printf("pwmdma: [cb-race] variants (source offset / txfr_len): 0=%u/%u 1=%u/%u 2=%u/%u. "
		"Match window %u bytes; the variants are %u bytes apart in source and %u in length, "
		"so consumption before the readback cannot carry one into another's window. Worst "
		"CROSS combination (offset %u + length %u) ends exactly at the ring end, so no "
		"fetch — fresh, stale or torn — can point the engine outside the ring.\n",
		cbRaceOff[0], cbRaceLen[0], cbRaceOff[1], cbRaceLen[1], cbRaceOff[2], cbRaceLen[2],
		CB_RACE_DRIFT, cbRaceOff[1] - cbRaceOff[0], cbRaceLen[0] - cbRaceLen[1],
		cbRaceOff[2], cbRaceLen[0]);

	pwmInit0();
	pwm[PWM_DMAC] = PWM_DMAC_AUDIO;
	dmaAbortReset();

	step = trials / 10ul;
	if (step == 0ul) {
		step = 1ul;
	}

	for (i = 0ul; i < trials; i++) {
		fresh = (unsigned)(i % CB_RACE_VARIANTS);
		ownStale = (unsigned)((i + 1ul) % CB_RACE_VARIANTS);
		otherVar = (unsigned)((i + 2ul) % CB_RACE_VARIANTS);
		cb = cbs[i & 1ul];
		bus = cbBus[i & 1ul];
		kind = NULL;

		dma[DMA_DEBUG] = DMA_DBG_ERRORS;   /* W1C anything the previous trial latched */

		/* The sentinel that keeps "the engine has not fetched yet" a DISTINCT reading
		 * rather than a mismatch: after RESET the channel's live registers read zero, so
		 * a too-early readback is not some other CB's values. If a reset did not take,
		 * this trial cannot tell the two apart — skip it instead of classifying it.
		 *
		 * ⚠ MEASURED 2026-09-18 on BCM2711: abort+RESET clears TXFR_LEN and CONBLK_AD
		 * but does NOT clear SOURCE_AD — it keeps the previous trial's value (observed
		 * `LEN=0 SRC=0xefba0018 CONBLK=0x00000000`). Including SOURCE_AD here refused
		 * every run on real hardware. TXFR_LEN carries the sentinel on its own: it is
		 * loaded from the control block, so non-zero means a fetch happened, and the
		 * three variants have distinct lengths (32768/24576/16384), so it also
		 * identifies WHICH block was fetched without help from SOURCE_AD. */
		if ((dma[DMA_TXFR_LEN_R] | dma[DMA_CONBLK_AD]) != 0u) {
			/* ⚠ Trials 0 AND 1, not just 0. Trial 0 passes this test for free — a channel
			 * nothing has armed since boot reads zero whether or not RESET clears
			 * anything — so trial 1, the first one that runs after a real load and a real
			 * abort+RESET, is where the sentinel is actually proved. Refusing only at
			 * trial 0 would let a port where RESET does not clear the registers skip
			 * trials 1..N-1 and still print a null off the single trial that ran. */
			if (i <= 1ul) {
				printf("pwmdma: [cb-race] the channel's live registers are NOT zero after "
					"abort+RESET (LEN=%u SRC=0x%08x CONBLK=0x%08x) at trial %lu. This mode's "
					"whole discrimination between 'loaded the wrong control block' and 'has "
					"not loaded one yet' rests on that zero, and trial 1 is the first that "
					"tests it after a real load, so it cannot run here.\n",
					dma[DMA_TXFR_LEN_R], dma[DMA_SOURCE_AD], dma[DMA_CONBLK_AD], i);
				parkHardware();
				return refuse("cb-race: post-RESET channel registers are not zero, so the "
					"not-yet-fetched sentinel does not hold on this hardware");
			}
			dirtyReset++;
			dmaAbortReset();
			continue;
		}

		/* Window amplifier: a burst of Normal-NC stores for the two stores that matter
		 * to queue behind. The value is the silence the ring already holds, so the DATA
		 * does not change — only the depth of write traffic in front of the CB. Present
		 * in both arms; --preload sweeps it. */
		for (j = 0ul; j < preloadWords; j++) {
			ring[j] = PWM_RANGE / 2u;
		}

		/* ── THE SEQUENCE UNDER TEST ────────────────────────────────────────────────
		 * Two Normal-NC stores, then — with nothing whatsoever in between unless
		 * --barrier put a dsb there — the two Device stores that make the engine go and
		 * fetch them. Every pointer here is volatile, so the compiler may not reorder
		 * these four accesses against each other; what is being tested is the
		 * ARCHITECTURAL ordering of Normal-NC against Device, not the compiler's. */
		cb->source_ad = DRAM_BUS(ring_pa + cbRaceOff[fresh]);
		cb->txfr_len = cbRaceLen[fresh];

		if (cbBarrier != 0) {
			__asm__ volatile("dsb sy" ::: "memory");
		}

		dma[DMA_CONBLK_AD] = bus;
		dma[DMA_CS] = DMA_CS_ACTIVE;

		/* Poll on TXFR_LEN alone. SOURCE_AD survives abort+RESET on BCM2711, so an
		 * `s.src == 0` term is false on entry and the loop exits before the engine has
		 * fetched anything -- which is not a timeout but a poll that never ran, and it
		 * scores as "not fetched". Measured: 4969 of 5000 trials, i.e. the run could
		 * not be graded at all. */
		s.len = dma[DMA_TXFR_LEN_R];
		s.src = dma[DMA_SOURCE_AD];
		for (s.spins = 0u; (s.spins < CB_RACE_POLL) && (s.len == 0u); s.spins++) {
			s.len = dma[DMA_TXFR_LEN_R];
			s.src = dma[DMA_SOURCE_AD];
		}
		s.conblk = dma[DMA_CONBLK_AD];
		s.cs = dma[DMA_CS];
		s.debug = dma[DMA_DEBUG];
		s.dest = dma[DMA_DEST_AD];
		s.next = dma[DMA_NEXTCONBK];
		/* ── end of the sequence under test ─────────────────────────────────────────*/

		dmaAbortReset();

		if (s.spins > maxSpins) {
			maxSpins = s.spins;
		}
		if (((s.cs & DMA_CS_ERROR) != 0u) || ((s.debug & DMA_DBG_ERRORS) != 0u)) {
			engineErr++;
		}
		/* CONBLK_AD goes to NEXTCONBK (0 here) once the block is consumed, so only a
		 * NON-ZERO reading that is not the block we named is odd. Reported as an
		 * observation; classification stays content-based, because a lost Device store
		 * to CONBLK_AD would be a different defect from a stale CB fetch. */
		if ((s.conblk != 0u) && (s.conblk != bus)) {
			conblkOdd++;
		}

		/* TXFR_LEN alone, deliberately: SOURCE_AD survives abort+RESET on BCM2711 (see
		 * the sentinel above), so a not-yet-fetched trial reads len=0 with a STALE src.
		 * Requiring src==0 too would drop that trial into the classifier below, where
		 * the retained address matches a variant window and scores as a stale fetch —
		 * manufacturing the very result this probe exists to test for. */
		if (s.len == 0u) {
			notFetched++;
		}
		else {
			sv = srcVariant(s.src, &dSrc);
			lv = lenVariant(s.len, &dLen);

			if ((sv >= 0) && (dSrc > maxDrift)) {
				maxDrift = dSrc;
			}
			if ((lv >= 0) && (dLen > maxDrift)) {
				maxDrift = dLen;
			}
			if ((sv >= 0) && (lv >= 0)) {
				dis = (dSrc > dLen) ? (dSrc - dLen) : (dLen - dSrc);
				if (dis > maxDisagree) {
					maxDisagree = dis;
				}
			}
			else {
				dis = 0u;
			}

			if ((sv < 0) || (lv < 0)) {
				unclassified++;
				kind = "UNCLASSIFIED readback (neither field landed in a variant window)";
			}
			else if (sv == lv) {
				if ((unsigned)sv == fresh) {
					okFresh++;
				}
				else if ((unsigned)sv == ownStale) {
					staleOwn++;
					kind = "STALE fetch — the engine read this control block's PREVIOUS "
						"contents, i.e. our stores had not landed";
				}
				else {
					staleOther++;
					kind = "OTHER-BLOCK fetch — the engine followed the control block we "
						"did NOT name (a lost CONBLK_AD write, not the store race)";
				}
			}
			else if (dis <= CB_RACE_SLACK) {
				/* One field fresh and the other not, with both implied consumption
				 * figures agreeing: a genuinely TORN fetch, the strongest evidence
				 * this probe can produce. */
				tornCnt++;
				kind = "TORN fetch — the two fields came from DIFFERENT variants, so one "
					"store had landed and the other had not";
			}
			else {
				/* The fields disagree about how much was consumed, so the reading is
				 * more likely a long preemption between ACTIVE and the readback than a
				 * torn fetch. Never counted as the race. */
				unclassified++;
				kind = "UNCLASSIFIED readback (fields matched different variants but their "
					"consumed-byte figures disagree — a stretched readback, not a tear)";
			}
		}

		if ((kind != NULL) && (firstDumped == 0)) {
			firstDumped = 1;
			cbRaceDump(kind, i, (unsigned)(i & 1ul), fresh, ownStale, otherVar, &s, cbs,
				cbBus, sv, lv);
		}

		if (fifoGapUs != 0ul) {
			usleep((unsigned int)fifoGapUs);
		}

		if (((i + 1ul) % step) == 0ul) {
			printf("pwmdma: [cb-race] %lu/%lu trials, %lu correct, %lu stale, %lu torn, "
				"%lu unclassified, %lu not fetched\n",
				i + 1ul, trials, okFresh, staleOwn, tornCnt, unclassified, notFetched);
		}
	}

	parkHardware();

	printf("pwmdma: [cb-race] trials=%lu correct=%lu STALE-own=%lu stale-other=%lu TORN=%lu "
		"unclassified=%lu not-fetched=%lu dirty-reset-skipped=%lu engine-error=%lu "
		"odd-CONBLK=%lu [%s]\n",
		trials, okFresh, staleOwn, staleOther, tornCnt, unclassified, notFetched,
		dirtyReset, engineErr, conblkOdd, tag);
	printf("pwmdma: [cb-race] worst drift %u bytes against a %u-byte match window; worst "
		"source/length consumed-byte disagreement %u against a %u-byte tear slack; worst "
		"readback poll %u of %u; abort handshake timed out %lu times (a pause can "
		"legitimately fail while DREQs are enabled but not asserted — bcm2835-dma.c:732-738 "
		"— so it is counted, not treated as a fault). If the drift figure ever approaches "
		"the window, widen the variant separation before trusting this run.\n",
		maxDrift, CB_RACE_DRIFT, maxDisagree, CB_RACE_SLACK, maxSpins, CB_RACE_POLL,
		cbAbortTimeouts);

	/* The cannot-fail guard this tool already carries once, and a FRACTION rather than a
	 * zero test: a run where one trial classified and 4 999 were skipped would otherwise
	 * print a clean "NOT REPRODUCED" off a single sample. Half the trials have to have
	 * produced a readback this mode can actually grade. */
	if (((okFresh + staleOwn + staleOther + tornCnt) * 2ul) < trials) {
		printf("pwmdma: [cb-race] only %lu of %lu trials produced a classifiable readback "
			"(%lu not fetched, %lu unclassified, %lu skipped for a dirty reset).\n",
			okFresh + staleOwn + staleOther + tornCnt, trials, notFetched, unclassified,
			dirtyReset);
		return refuse("cb-race: fewer than half the trials produced a classifiable readback, so "
			"this run cannot grade the race — nothing here is a null");
	}

	if ((staleOwn + tornCnt) == 0ul) {
		printf("pwmdma: VERDICT: NOT REPRODUCED — %lu trials read back exactly the control "
			"block that trial had just written; 0 stale and 0 torn fetches (%lu "
			"unclassified, %lu not fetched, %lu skipped for a dirty reset) [%s]. At this "
			"preload depth the Normal-NC control-block stores were always visible to the "
			"engine by the time it fetched, with %s. ⚠ Scope: a null here bounds the rate, "
			"it does not prove the ordering is architecturally guaranteed — sweep "
			"--preload and compare against the --barrier arm before calling the barrier "
			"hypothesis dead.\n",
			okFresh + staleOther, unclassified, notFetched, dirtyReset, tag,
			(cbBarrier != 0) ? "the dsb sy in place (this IS the control arm)" :
				"NO barrier at all");
		if ((cbBarrier == 0) && ((unclassified + staleOther + conblkOdd) != 0ul)) {
			printf("pwmdma: ⓘ this run had %lu unclassified, %lu other-block and %lu odd-CONBLK "
				"readings. None of them is the race, but a large count means the readback "
				"window is not as tight as the classification assumes — read the first dump "
				"above before quoting the null.\n", unclassified, staleOther, conblkOdd);
		}
		return 0;
	}

	printf("pwmdma: VERDICT: REPRODUCED — %lu stale and %lu torn control-block fetches in %lu "
		"trials [%s]. The engine followed bytes the CPU had already overwritten, which is "
		"the Normal-NC -> Device store-ordering race rpi4-audio's dsb sy "
		"(rpi4-audio.c:628-649) was added against — measured here rather than argued.\n",
		staleOwn, tornCnt, trials, tag);
	if (cbBarrier != 0) {
		printf("pwmdma: ⚠⚠ this is the --barrier CONTROL arm, whose expected count is ZERO. A "
			"mismatch WITH a dsb sy before the MMIO writes is far likelier to be a defect in "
			"this probe than a broken barrier — do not report it as the race until the "
			"first dump above has been read and explained.\n");
	}
	else {
		printf("pwmdma: ⓘ this is the no-barrier TEST arm. It is not a finding until the "
			"control arm has been run on the same build and came back with zero: "
			"`pwmdma --cb-race %lu %lu --barrier --preload %lu`\n",
			trials, fifoGapUs, preloadWords);
	}
	if (staleOther != 0ul) {
		printf("pwmdma: ⚠ %lu trials read the control block this probe did NOT name. That needs "
			"a Device store to CONBLK_AD to have been lost, which the architecture does not "
			"allow — suspect this probe's wiring (the CB addresses, the 64-byte spacing) "
			"before suspecting the hardware.\n", staleOther);
	}
	return 1;
}


/* The banner is printed before ANY mapping or register write, so a log that shows it
 * also shows that the operator asked for it before anything was disturbed. */
static void cycleClockBanner(void)
{
	printf("pwmdma: "
		"================================================================\n"
		"pwmdma: ⚠⚠ --cycle-clock: THIS RUN STOPS AND RESTARTS THE SHARED PWM CLOCK ⚠⚠\n"
		"pwmdma:   The CPRMAN PWM generator (CM_PWMCTL @ 0xfe101000+0xa0) feeds BOTH PWM0\n"
		"pwmdma:   (this probe's) and PWM1 (rpi4-audio's). rpi4-audio is streaming silence\n"
		"pwmdma:   over PWM1 + legacy DMA channel %u right now, and it will NOT be told.\n"
		"pwmdma:   Stopping the generator WILL disturb that stream and it may never recover\n"
		"pwmdma:   for the rest of this boot. Acceptable ONLY on a dedicated probe boot with\n"
		"pwmdma:   nothing playing audio.\n"
		"pwmdma:   What this probe will NOT do: write the PWM1 half of the page, write any\n"
		"pwmdma:   channel-%u register, or exit with the generator stopped (it is always left\n"
		"pwmdma:   running). PWM1/ch%u are READ at entry and exit and reported as collateral.\n"
		"pwmdma: "
		"================================================================\n",
		DMA_AUDIO, DMA_AUDIO, DMA_AUDIO);
}


int main(int argc, char **argv)
{
	unsigned long trials = 500ul, settleUs = 5000ul, pos[3] = { 0ul, 0ul, 0ul };
	volatile uint32_t *page;
	uint32_t enable;
	int i, positional = 0, clockGapGiven = 0, preloadGiven = 0, clk;

	/* Positionals first, flags after — the shape the sibling probe already uses
	 * (`pwmwrite --start-test N`), so both tools read the same way from psh. The
	 * positionals are collected rather than assigned here, because --cb-race gives the
	 * second one a DIFFERENT meaning and the flag may appear after them. */
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--cycle-clock") == 0) {
			cycleClock = 1;
		}
		else if (strcmp(argv[i], "--cb-race") == 0) {
			cbRace = 1;
		}
		else if (strcmp(argv[i], "--barrier") == 0) {
			cbBarrier = 1;
		}
		else if (strcmp(argv[i], "--clock-gap-us") == 0) {
			if ((i + 1) >= argc) {
				return refuse("--clock-gap-us needs a microsecond value");
			}
			i++;
			clockGapUs = strtoul(argv[i], NULL, 0);
			clockGapGiven = 1;
		}
		else if (strcmp(argv[i], "--preload") == 0) {
			if ((i + 1) >= argc) {
				return refuse("--preload needs a ring-word count");
			}
			i++;
			preloadWords = strtoul(argv[i], NULL, 0);
			preloadGiven = 1;
		}
		else if (argv[i][0] == '-') {
			return refuse("unknown option (see the header comment for the usage line)");
		}
		else if (positional < 3) {
			pos[positional] = strtoul(argv[i], NULL, 0);
			positional++;
		}
		else {
			return refuse("too many positional arguments: [trials] [settle_us] [gap_us], or "
				"[trials] [fifo_gap_us] under --cb-race");
		}
	}

	/* An inert knob is the cannot-fail trap this project has already been bitten by: a
	 * sweep of values that silently never reached the code they name would produce a row
	 * of identical nulls and read as a row of experiments. Every knob below is refused
	 * rather than ignored when its mode is absent. */
	if ((clockGapGiven != 0) && (cycleClock == 0)) {
		return refuse("--clock-gap-us has no meaning without --cycle-clock, and would silently "
			"do nothing — pass --cycle-clock or drop the knob");
	}
	if ((cbRace != 0) && (cycleClock != 0)) {
		return refuse("--cb-race and --cycle-clock are different experiments and cannot share a "
			"run — pick one");
	}
	if ((cbRace == 0) && ((cbBarrier != 0) || (preloadGiven != 0))) {
		return refuse("--barrier and --preload have no meaning without --cb-race, and would "
			"silently do nothing — pass --cb-race or drop the knob");
	}

	if (cbRace != 0) {
		trials = (positional > 0) ? pos[0] : CB_RACE_TRIALS;
		if (positional > 1) {
			fifoGapUs = pos[1];
		}
		if (positional > 2) {
			return refuse("--cb-race takes [trials] [fifo_gap_us]; a third positional would be "
				"read as settle_us, which this mode never uses");
		}
		if (trials == 0ul) {
			return refuse("trial count must be non-zero");
		}
		if (preloadWords >= RING_WORDS) {
			return refuse("--preload must stay inside the ring");
		}
	}
	else {
		if (positional > 0) {
			trials = pos[0];
		}
		if (positional > 1) {
			settleUs = pos[1];
		}
		if (positional > 2) {
			gapUs = pos[2];
		}
		if ((trials == 0ul) || (settleUs == 0ul)) {
			return refuse("trial count and settle must both be non-zero");
		}
	}

	if (cycleClock != 0) {
		cycleClockBanner();
	}

	page = mmap(NULL, _PAGE_SIZE, PROT_READ | PROT_WRITE,
		MAP_DEVICE | MAP_UNCACHED | MAP_PHYSMEM | MAP_ANONYMOUS, -1, (off_t)PWM0_PAGE);
	if (page == MAP_FAILED) {
		return refuse("mmap of the PWM0 page failed");
	}
	pwm = page;

	page = mmap(NULL, _PAGE_SIZE, PROT_READ | PROT_WRITE,
		MAP_DEVICE | MAP_UNCACHED | MAP_PHYSMEM | MAP_ANONYMOUS, -1, (off_t)CPRMAN_PAGE);
	if (page == MAP_FAILED) {
		return refuse("mmap of the CPRMAN page failed");
	}
	cprman = page;

	page = mmap(NULL, _PAGE_SIZE, PROT_READ | PROT_WRITE,
		MAP_DEVICE | MAP_UNCACHED | MAP_PHYSMEM | MAP_ANONYMOUS, -1, (off_t)DMA_BASE);
	if (page == MAP_FAILED) {
		return refuse("mmap of the DMA page failed");
	}
	dmapage = page;
	dma = page + (DMA_CHAN * 0x100u) / 4u;
	dmaAudio = page + (DMA_AUDIO * 0x100u) / 4u;

	printf("pwmdma: PWM0 @ 0x%08x (audio owns PWM1 @ 0x%08x — never written), DMA channel %u "
		"@ 0x%08x (audio owns %u — never written), DREQ/PERMAP %u\n",
		PWM0_PAGE, PWM0_PAGE + 0x800u, DMA_CHAN, DMA_BASE + DMA_CHAN * 0x100u, DMA_AUDIO,
		DREQ_PWM0);
	printf("pwmdma: entry PWM0 CTL=0x%08x STA=0x%08x DMAC=0x%08x RNG1=%u; "
		"CM_PWMCTL=0x%08x CM_PWMDIV=0x%08x (before any write of ours)\n",
		pwm[PWM_CTL], pwm[PWM_STA], pwm[PWM_DMAC], pwm[PWM_RNG1],
		cprman[CM_PWMCTL], cprman[CM_PWMDIV]);

	/* The shared per-channel enable. A channel whose bit is clear sits at ACTIVE and
	 * never fetches, which would read as this probe's defect on every trial. Set OUR
	 * bit only (read-modify-write, never clearing anyone else's — the audio driver
	 * does not write this register, so there is no race with it), then re-read. */
	enable = dmapage[DMA_ENABLE];
	printf("pwmdma: DMA ENABLE (0xff0) = 0x%08x, dts channel mask = 0x%04x (bcm2711.dtsi:106; "
		"channel %u %s in the mask)\n",
		enable, DMA_CHANNEL_MASK_DTS, DMA_CHAN,
		((DMA_CHANNEL_MASK_DTS >> DMA_CHAN) & 1u) ? "IS" : "is NOT");
	if ((enable & (1u << DMA_CHAN)) == 0u) {
		dmapage[DMA_ENABLE] = enable | (1u << DMA_CHAN);
		enable = dmapage[DMA_ENABLE];
		printf("pwmdma: channel %u was not enabled; set its bit -> 0x%08x\n", DMA_CHAN, enable);
	}
	if ((enable & (1u << DMA_CHAN)) == 0u) {
		return refuse("the DMA controller will not enable this channel");
	}

	clk = clockEnsure();
	if (clk < 0) {
		return refuse("the shared PWM clock will not report BUSY, so no channel can transmit "
			"and every trial would fail for a reason that is not the defect");
	}
	if (clk == 1) {
		printf("pwmdma: PWM clock was ALREADY running — %s\n",
			(cycleClock != 0) ?
				"and --cycle-clock WILL stop and restart it before every trial (see the "
				"banner); rpi4-audio owns it" :
				"using it as-is, NOT reconfigured (rpi4-audio owns it).");
	}
	else {
		printf("pwmdma: PWM clock was NOT running — started it here exactly as "
			"audio_clockInit() does (DIVI=%u from the %u Hz oscillator), and leaving it "
			"running on exit so a driver that starts meanwhile is not cut off.\n",
			PWM_CLK_DIVI, CM_OSC_HZ);
	}

	if (cbRace != 0) {
		return runCbRace(trials);
	}

	return run(trials, settleUs);
}
