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
 * Verdicts (grep-able, one line, and the exit status matches):
 *   REPRODUCED      k/n trials parked. The DMA→DREQ→FIFO handshake can fail with the
 *                   PWM configured correctly — reproducible in seconds, and the
 *                   re-arm result says whether the driver's containment can clear it.
 *   NOT REPRODUCED  every trial streamed. With the PWM half already retired, the
 *                   stall then needs something neither probe can reach from
 *                   userspace on a settled machine — the clock-start → PWM-enable
 *                   proximity (branch (ii) of the capture record) being the one
 *                   named survivor.
 *   REFUSED         a precondition is not met (clock, DMA channel enable, buffer
 *                   placement). Says so loudly rather than producing failures that
 *                   are not the defect.
 *
 * Usage: pwmdma [trials] [settle_us] [gap_us]
 *        trials     default 500
 *        settle_us  default 5000 — at ~44.1 kHz a healthy channel advances ~440 ring
 *                   words in 5 ms; a parked one moves at most the 16-word FIFO depth
 *        gap_us     default 0 — microseconds to sit PWM-enabled before arming the
 *                   DMA. The driver's real shape has MILLISECONDS here (portCreate,
 *                   create_dev and two printfs run between audio_pwmInit() and
 *                   audio_dmaStart()), and pwmwrite's start-test had to grow exactly
 *                   this knob before its second row meant anything.
 *
 * Copyright 2026 Phoenix Systems
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
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
#define DMA_BASE 0xfe007000u
#define DMA_CHAN 6u
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
#define DMA_CS_RESET  (1u << 31)
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


static volatile uint32_t *pwm;      /* PWM0 */
static volatile uint32_t *cprman;
static volatile uint32_t *dmapage;  /* whole DMA page, for the shared ENABLE word */
static volatile uint32_t *dma;      /* our channel's register block */
static volatile uint32_t *ring;
static uintptr_t ring_pa;
static uintptr_t cb_pa;
static unsigned long gapUs;


/* Bring up the shared CPRMAN PWM clock, EXACTLY as audio_clockInit() does today
 * (rpi4-audio.c:245-271): stop the generator, then SRC while disabled, then DIV,
 * then ENAB — three separate writes, MASH masked — which is Linux's order and not
 * the obvious one. (pwmwrite.c still carries the older single-write shape; it is
 * deliberately not copied.)
 *
 * Policy is pwmwrite's, and it matters: rpi4-audio OWNS this generator. Returns 1 if
 * it was already running (left strictly untouched), 0 if started here, -1 if it will
 * not come up. Never reconfigures a running clock. */
static int clockEnsure(void)
{
	uint32_t spin, ctl;

	if ((cprman[CM_PWMCTL] & CM_CTL_BUSY) != 0u) {
		return 1;
	}

	ctl = cprman[CM_PWMCTL] & ~(CM_PASSWD | CM_CTL_ENAB);
	cprman[CM_PWMCTL] = CM_PASSWD | ctl;
	for (spin = SPIN_MAX; (spin != 0u) && ((cprman[CM_PWMCTL] & CM_CTL_BUSY) != 0u); spin--) {
	}

	ctl = (cprman[CM_PWMCTL] & ~(CM_PASSWD | CM_CTL_ENAB | CM_CTL_MASH | CM_SRC_MASK)) | CM_SRC_OSC;
	cprman[CM_PWMCTL] = CM_PASSWD | ctl;
	cprman[CM_PWMDIV] = CM_PASSWD | (PWM_CLK_DIVI << 12);
	cprman[CM_PWMCTL] = CM_PASSWD | ctl | CM_CTL_ENAB | CM_CTL_GATE;

	for (spin = SPIN_MAX; (spin != 0u) && ((cprman[CM_PWMCTL] & CM_CTL_BUSY) == 0u); spin--) {
	}
	return ((cprman[CM_PWMCTL] & CM_CTL_BUSY) != 0u) ? 0 : -1;
}


/* audio_pwmInit() (rpi4-audio.c:275-325) on PWM0, pacing and BERR clear included.
 * The pacing is not cosmetic: unpaced back-to-back writes to PWM_CTL latch a bus
 * error, which is the one defect the 2026-09-18 night actually fixed. */
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


static int run(unsigned long trials, unsigned long settleUs)
{
	unsigned long i, parked = 0ul, step;
	unsigned long hist[HIST_BUCKETS];
	unsigned int b;
	uint32_t advance, reArm;
	int firstDumped = 0, reArmOk = -1;
	dma_cb_t *cb;

	memset(hist, 0, sizeof(hist));

	/* The ring and the control block are allocated ONCE, outside the trial loop: the
	 * thing under test is the arm, not the allocator, and a fresh mapping per trial
	 * would also change the physical address the DMA fetches from every time. */
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
		"(a healthy channel covers ~%lu at %u Hz; a parked one <= the 16-word FIFO)\n",
		trials, settleUs, gapUs, DMA_START_MIN_WORDS,
		((unsigned long)AUDIO_RATE * settleUs) / 1000000ul, AUDIO_RATE);

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

	step = trials / 10ul;
	if (step == 0ul) {
		step = 1ul;
	}

	for (i = 0ul; i < trials; i++) {
		pwmInit0();
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
			printf("pwmdma: %lu/%lu trials, %lu parked\n", i + 1ul, trials, parked);
		}
	}

	parkHardware();

	printf("pwmdma: advance histogram (ring words covered in the %lu us settle)\n", settleUs);
	for (b = 0u; b < HIST_BUCKETS; b++) {
		if (hist[b] != 0ul) {
			printf("pwmdma:   %-30s %lu\n", histLabel(b), hist[b]);
		}
	}
	printf("pwmdma: trials=%lu parked=%lu threshold=%u words\n",
		trials, parked, DMA_START_MIN_WORDS);

	if (parked == 0ul) {
		printf("pwmdma: VERDICT: NOT REPRODUCED — all %lu trials streamed (>= %u ring words "
			"each in %lu us). The DMA->DREQ->FIFO handshake starts reliably on a settled "
			"clock, so with the PWM half already retired by pwmwrite --start-test the audio "
			"stall needs something neither probe reaches: the clock-start -> PWM-enable "
			"proximity is the named survivor.\n",
			trials, DMA_START_MIN_WORDS, settleUs);
		return 0;
	}

	printf("pwmdma: VERDICT: REPRODUCED — %lu/%lu trials parked (advanced < %u ring words in "
		"%lu us with the PWM enabled, clocked and DMA-requesting). This is the audio stall's "
		"own handshake, sampled thousands of times per boot instead of ~7 times in 100; the "
		"re-arm line above says whether the driver's containment clears it.\n",
		parked, trials, DMA_START_MIN_WORDS, settleUs);
	if (parked == trials) {
		printf("pwmdma: ⚠ ALL trials parked, which is far likelier to be a WIRING mistake in "
			"this probe than a ~7%% defect — check PERMAP=%u (PWM0's DREQ, inferred not cited), "
			"the shared DMA ENABLE bit %u, and dest=0x%08x before believing this is the "
			"defect.\n", DREQ_PWM0, DMA_CHAN, PWM0_FIF1_BUS);
	}
	if (reArmOk == 0) {
		printf("pwmdma: the parked channel did NOT re-arm — same conclusion the driver draws "
			"when it degrades to a paced null sink.\n");
	}
	return 1;
}


int main(int argc, char **argv)
{
	unsigned long trials = 500ul, settleUs = 5000ul;
	volatile uint32_t *page;
	uint32_t enable;
	int clk;

	if (argc > 1) {
		trials = strtoul(argv[1], NULL, 0);
	}
	if (argc > 2) {
		settleUs = strtoul(argv[2], NULL, 0);
	}
	if (argc > 3) {
		gapUs = strtoul(argv[3], NULL, 0);
	}
	if ((trials == 0ul) || (settleUs == 0ul)) {
		return refuse("trial count and settle must both be non-zero");
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

	printf("pwmdma: PWM0 @ 0x%08x (audio owns PWM1 @ 0x%08x — untouched), DMA channel %u "
		"@ 0x%08x (audio owns 5), DREQ/PERMAP %u\n",
		PWM0_PAGE, PWM0_PAGE + 0x800u, DMA_CHAN, DMA_BASE + DMA_CHAN * 0x100u, DREQ_PWM0);
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
		printf("pwmdma: PWM clock was ALREADY running — using it as-is, NOT reconfigured "
			"(rpi4-audio owns it).\n");
	}
	else {
		printf("pwmdma: PWM clock was NOT running — started it here exactly as "
			"audio_clockInit() does (DIVI=%u from the %u Hz oscillator), and leaving it "
			"running on exit so a driver that starts meanwhile is not cut off.\n",
			PWM_CLK_DIVI, CM_OSC_HZ);
	}

	return run(trials, settleUs);
}
