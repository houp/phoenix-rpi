/*
 * pwmwrite — does the BCM2711 PWM block ever DROP a back-to-back register write?
 *
 * Why this exists (2026-09-18). `rpi4-audio`'s DMA stall was captured three times
 * and every register the driver programs reads back correct at the abort: the PWM
 * clock is BUSY, CM_PWMDIV/PWM_CTL/PWM_DMAC are exactly as written, the ring is
 * full and the FIFO non-empty — yet the PWM consumes nothing. The one value none
 * of those prints could show is the PERIOD (RNG1): a channel with RNG1 == 0 is
 * enabled, clocked and fed while consuming nothing, which explains every other
 * register at once.
 *
 * `audio_pwmInit()` writes CTL=0, RNG1, RNG2, CTL=CLRF1, CTL=enable back to back
 * with no delay or read-back. The PWM sits in its own ~9.6 MHz clock domain, and
 * dropped back-to-back writes to it are a documented BCM hazard — exactly the
 * shape of a 1-in-14 boot failure. Waiting for boot aborts samples that at one
 * per ~2.5 minutes; this samples it ~100k times in one run, and it does so on
 * **PWM0** (0xfe20c000), which this port does not use and whose pins are not
 * routed, so nothing audible or shared is disturbed.
 *
 * Three spacings, same write pattern, so the remedy is measured and not guessed:
 *   back-to-back   — what the driver does today
 *   + barrier      — a read-back of the same register between writes
 *   + 10 us delay  — the pacing the BCM docs suggest for this peripheral
 *
 * ── --start-test: the PIO-only reproducer for "enabled but never transmits" ──
 *
 * The stall's signature is an enabled, clocked, FIFO-fed channel that consumes
 * nothing (STA=0x100, DMA_CS=ACTIVE|DREQ_STOPPED). Everything above tests the
 * WRITE path and came back clean, so what is left is the channel's own start:
 * does `audio_pwmInit()`'s exact configure-then-feed sequence sometimes leave a
 * channel that never begins transmitting? `--start-test` answers that with DMA
 * removed from the picture entirely — it configures PWM0 exactly as the driver
 * configures PWM1, pushes a handful of duty words by PIO, and asks whether the
 * channel drained them. A few thousand trials sample a per-init race that boots
 * sample once per 2.5 minutes.
 *
 * What the verdict discriminates:
 *   NOT REPRODUCED  the channel always starts. The stall needs DMA (DREQ
 *                   routing / threshold) or something outside PWM init.
 *   REPRODUCED      PWM init alone can leave a dead channel — a PIO-visible
 *                   defect, no DMA required, reproducible in seconds.
 * And if it reproduces, the consecutive-run counters say whether the drafted
 * "re-arm the PWM" fix can work: isolated stuck trials are a per-init race a
 * re-arm clears, while a run of stuck trials that survives the next trial's
 * CTL=0 + CLRF1 is state a CTL re-init cannot reach.
 *
 * Usage: pwmwrite [iterations]        (default 100000) — the write-drop tests
 *        pwmwrite --start-test [N] [GAP_US]  (default 2000, gap 0) — the start
 *            reproducer; GAP_US leaves the channel enabled with an empty FIFO for
 *            that long before feeding it, which is the shape the driver actually has
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

#define PWM0_PAGE 0xfe20c000u   /* PWM0: unused by this port (audio is PWM1 @ +0x800) */
#define PWM_CTL   (0x00u / 4u)
#define PWM_STA   (0x04u / 4u)
#define PWM_RNG1  (0x10u / 4u)
#define PWM_DAT1  (0x14u / 4u)
#define PWM_FIF1  (0x18u / 4u)
#define PWM_RNG2  (0x20u / 4u)

/* PWM_CTL bits, named to match rpi4-audio.c. */
#define CTL_PWEN1 (1u << 0)
#define CTL_USEF1 (1u << 5)
#define CTL_CLRF1 (1u << 6)
#define CTL_MSEN1 (1u << 7)
#define CTL_PWEN2 (1u << 8)
#define CTL_USEF2 (1u << 13)
#define CTL_MSEN2 (1u << 15)

/* The exact enable word audio_pwmInit() writes: both channels FIFO-fed (USEF),
 * mark/space (MSEN), enabled (PWEN). */
#define CTL_AUDIO_ENABLE \
	(CTL_USEF1 | CTL_MSEN1 | CTL_PWEN1 | CTL_USEF2 | CTL_MSEN2 | CTL_PWEN2)

/* CPRMAN PWM clock — PWM0 and PWM1 SHARE it, so the start test cannot run
 * without it and must never reconfigure it out from under a running audio
 * driver. Offsets and the password from rpi4-audio.c. */
#define CPRMAN_PAGE  0xfe101000u
#define CM_PWMCTL    (0xa0u / 4u)
#define CM_PWMDIV    (0xa4u / 4u)
#define CM_PASSWD    0x5a000000u
#define CM_CTL_ENAB  (1u << 4)
#define CM_CTL_BUSY  (1u << 7)
#define CM_SRC_OSC   1u          /* BCM2711 crystal oscillator */
#define CM_OSC_HZ    54000000u
#define PWM_CLK_DIVI 2u          /* what audio_clockInit() programs -> 27 MHz */

/* PWM_STA bits, from the BCM2835 peripherals doc. ⚠ Bit 8 is BERR — a BUS ERROR
 * latched when a register write does not take — and bit 9 is STA1 (channel 1
 * transmitting). Getting these two the wrong way round cost an hour on
 * 2026-09-18: `STA=0x100` reads as "transmitting" if you assume bit 8 is STA1,
 * when it actually says "a write failed and the FIFO is not empty". */
#define STA_FULL1 (1u << 0)
#define STA_EMPT1 (1u << 1)
#define STA_BERR  (1u << 8)
#define STA_STA1  (1u << 9)
/* Every latched error bit in one mask: WERR1, RERR1, GAPO1..4, BERR (bits 2..8).
 * All write-1-to-clear. FULL1/EMPT1/STA1 are LIVE state and must not be poked. */
#define STA_ERRS  0x000001fcu

#define PAT_A 612u
#define PAT_B 613u

/* Start-test tuning. RANGE is the driver's period; DUTY is mid-scale so the
 * channel actually toggles. WORDS fits inside the 16-word FIFO with room to
 * spare, and is enough that a healthy channel takes several periods to drain it
 * (so "drained" is evidence of transmission, not of a stale EMPT1). */
#define START_TRIALS  2000ul
#define START_RANGE   612u
#define START_DUTY    (START_RANGE / 2u)
#define START_WORDS   8u
/* Uncached device reads, so the tight spin is the FAST sampler: it catches STA1
 * while the channel is mid-transmit, which a usleep()-paced loop can miss
 * entirely if Phoenix's sleep granularity is coarser than the ~182 us the FIFO
 * takes to drain. The paced loop after it is the SLOW backstop. */
#define START_SPIN    20000u
#define START_POLLS   100u
#define START_POLL_US 20u
#define START_HIST    12u


static volatile uint32_t *pwm;
static volatile uint32_t *cprman;


/* One trial: disable, set the period, read it back. Returns 1 on a DROPPED write. */
static int trial_backToBack(uint32_t want)
{
	pwm[PWM_CTL] = 0u;
	pwm[PWM_RNG1] = want;
	return (pwm[PWM_RNG1] != want) ? 1 : 0;
}


static int trial_barrier(uint32_t want)
{
	pwm[PWM_CTL] = 0u;
	(void)pwm[PWM_CTL];        /* read-back: forces the write to retire first */
	pwm[PWM_RNG1] = want;
	(void)pwm[PWM_RNG1];
	return (pwm[PWM_RNG1] != want) ? 1 : 0;
}


static int trial_delay(uint32_t want)
{
	pwm[PWM_CTL] = 0u;
	usleep(10);
	pwm[PWM_RNG1] = want;
	usleep(10);
	return (pwm[PWM_RNG1] != want) ? 1 : 0;
}


/* Does the write pattern itself raise a BUS ERROR? The audio driver's ready line
 * reports STA=0x102 on EVERY boot -- EMPT1|BERR -- so `audio_pwmInit()` latches a
 * bus error every time it runs, and nothing in the driver decodes bit 8 to say so.
 * BERR is write-1-to-clear, so each trial starts from a clean slate. */
static unsigned long runBerr(const char *name, int (*fn)(uint32_t), unsigned long iters)
{
	unsigned long i, berr = 0;

	for (i = 0; i < iters; i++) {
		pwm[PWM_STA] = STA_BERR;            /* W1C */
		(void)fn(((i & 1u) != 0u) ? PAT_A : PAT_B);
		if ((pwm[PWM_STA] & STA_BERR) != 0u) {
			berr++;
		}
	}
	printf("pwmwrite: BERR %-12s %lu/%lu writes raised a bus error\n", name, berr, iters);
	return berr;
}


/* ★ The case that matters, added after the first run came back clean: the BCM2835
 * doc says BERR is set "if the bus tries to write successive cycles to the same
 * set of registers". The first probe wrote CTL then RNG1 -- DIFFERENT registers,
 * hence 0 bus errors. `audio_pwmInit()` writes **PWM_CTL three times in a row**
 * (0, CLRF1, enable), which is precisely the documented hazard, and the driver
 * latches BERR on every single boot (entry STA=0x2, ready STA=0x102).
 * These two trials reproduce that pattern unpaced and paced. */
static int trial_sameRegUnpaced(uint32_t want)
{
	(void)want;
	pwm[PWM_CTL] = 0u;
	pwm[PWM_CTL] = (1u << 6);      /* CLRF1 */
	pwm[PWM_CTL] = (1u << 0) | (1u << 5) | (1u << 7);  /* PWEN1|USEF1|MSEN1 */
	return 0;
}


static int trial_sameRegPaced(uint32_t want)
{
	(void)want;
	pwm[PWM_CTL] = 0u;
	(void)pwm[PWM_CTL];
	usleep(10);
	pwm[PWM_CTL] = (1u << 6);
	(void)pwm[PWM_CTL];
	usleep(10);
	pwm[PWM_CTL] = (1u << 0) | (1u << 5) | (1u << 7);
	(void)pwm[PWM_CTL];
	return 0;
}


static unsigned long run(const char *name, int (*fn)(uint32_t), unsigned long iters)
{
	unsigned long i, bad = 0;

	for (i = 0; i < iters; i++) {
		if (fn(((i & 1u) != 0u) ? PAT_A : PAT_B) != 0) {
			bad++;
			if (bad <= 3u) {
				printf("pwmwrite: %s: DROPPED at iteration %lu (RNG1 reads %u, wanted %u)\n",
					name, i, pwm[PWM_RNG1], ((i & 1u) != 0u) ? PAT_A : PAT_B);
			}
		}
	}
	printf("pwmwrite: %-14s %lu/%lu dropped\n", name, bad, iters);
	return bad;
}


/* ------------------------------------------------------------------------- */
/* --start-test: does the driver's own init sequence ever leave a channel that */
/* is enabled, clocked and FIFO-fed yet never transmits? PIO only, no DMA.     */
/* ------------------------------------------------------------------------- */

typedef struct {
	uint32_t sta_final;    /* PWM_STA after the trial finished */
	uint32_t ctl_live;     /* PWM_CTL as it stood BEFORE the trial parked it */
	unsigned int pushed;   /* duty words actually accepted by the FIFO */
	int sta1_seen;         /* STA1 observed set at least once */
	int drained;           /* EMPT1 observed set -> the channel consumed them */
	int berr;              /* a bus error was latched during this trial */
} start_result_t;


/* The PWM clock is SHARED with PWM1, i.e. with rpi4-audio. Returns 1 if it was
 * already running (left strictly untouched), 0 if we started it here, -1 if it
 * refuses to run. audio_clockInit()'s sequence, verbatim, on the start path. */
static int startClockEnsure(void)
{
	uint32_t spin;

	if ((cprman[CM_PWMCTL] & CM_CTL_BUSY) != 0u) {
		return 1;
	}

	cprman[CM_PWMCTL] = CM_PASSWD | (cprman[CM_PWMCTL] & ~CM_CTL_ENAB);
	for (spin = 1000000u; (spin != 0u) && ((cprman[CM_PWMCTL] & CM_CTL_BUSY) != 0u); spin--) {
	}

	cprman[CM_PWMDIV] = CM_PASSWD | (PWM_CLK_DIVI << 12);
	cprman[CM_PWMCTL] = CM_PASSWD | CM_CTL_ENAB | CM_SRC_OSC;

	for (spin = 1000000u; (spin != 0u) && ((cprman[CM_PWMCTL] & CM_CTL_BUSY) == 0u); spin--) {
	}
	return ((cprman[CM_PWMCTL] & CM_CTL_BUSY) != 0u) ? 0 : -1;
}


/* One period in microseconds, from the divisor the clock is ACTUALLY running at
 * — the firmware may have left a different DIVI than the driver programs, and
 * then the familiar "22.7 us" would simply be wrong. */
static unsigned long startPeriodUs(void)
{
	uint32_t divi = (cprman[CM_PWMDIV] >> 12) & 0xfffu;

	if (divi == 0u) {
		return 0ul;
	}
	return ((unsigned long)START_RANGE * (unsigned long)divi * 1000000ul) / (unsigned long)CM_OSC_HZ;
}


/* One trial. Configure PWM0 exactly the way audio_pwmInit() configures PWM1,
 * feed it by PIO, and find out whether it transmitted. */
/* Microseconds to leave the channel ENABLED with an EMPTY FIFO before feeding it.
 * 0 reproduces the probe's original back-to-back shape; the driver's real shape is a
 * gap of milliseconds, because audio_pwmInit() enables the channel and only then does
 * portCreate/create_dev and two printfs before audio_dmaStart() arms the DMA. If the
 * dead channel needs that idle-enabled window, a zero-gap probe can never see it. */
static unsigned long startGapUs = 0ul;


static void startTrial(start_result_t *r)
{
	unsigned int i;
	uint32_t sta;

	r->sta_final = 0u;
	r->ctl_live = 0u;
	r->pushed = 0u;
	r->sta1_seen = 0;
	r->drained = 0;
	r->berr = 0;

	/* 1. disable while configuring, paced with a read-back like the driver. */
	pwm[PWM_CTL] = 0u;
	(void)pwm[PWM_CTL];
	usleep(10);

	/* Latched errors are write-1-to-clear: start clean so this trial's counters
	 * describe this trial and not the run's history. */
	pwm[PWM_STA] = STA_ERRS;

	/* 2. clear the FIFO. */
	pwm[PWM_CTL] = CTL_CLRF1;
	(void)pwm[PWM_CTL];
	usleep(10);

	/* 3. the driver's period on both channels. */
	pwm[PWM_RNG1] = START_RANGE;
	(void)pwm[PWM_RNG1];
	pwm[PWM_RNG2] = START_RANGE;
	(void)pwm[PWM_RNG2];
	usleep(10);

	/* 4. the driver's enable word. */
	pwm[PWM_CTL] = CTL_AUDIO_ENABLE;
	(void)pwm[PWM_CTL];
	usleep(10);

	/* 4b. optionally sit enabled with an empty FIFO, the way the driver does. */
	if (startGapUs != 0ul) {
		usleep((unsigned int)startGapUs);
	}

	/* 5. feed mid-scale duty words by PIO. Nothing here involves DMA. */
	for (i = 0; i < START_WORDS; i++) {
		if ((pwm[PWM_STA] & STA_FULL1) != 0u) {
			break;
		}
		pwm[PWM_FIF1] = START_DUTY;
		r->pushed++;
	}

	/* 6a. fast sampler, no sleeping: catches STA1 mid-transmit. */
	for (i = 0; i < START_SPIN; i++) {
		sta = pwm[PWM_STA];
		if ((sta & STA_STA1) != 0u) {
			r->sta1_seen = 1;
		}
		if ((sta & STA_EMPT1) != 0u) {
			r->drained = 1;
			break;
		}
	}

	/* 6b. slow backstop, well past the couple of periods the FIFO needs. */
	if (r->drained == 0) {
		usleep(100);
		for (i = 0; i < START_POLLS; i++) {
			sta = pwm[PWM_STA];
			if ((sta & STA_STA1) != 0u) {
				r->sta1_seen = 1;
			}
			if ((sta & STA_EMPT1) != 0u) {
				r->drained = 1;
				break;
			}
			usleep(START_POLL_US);
		}
	}

	/* 7. record — CTL included, and BEFORE parking: read after the park below it
	 * would always be 0 and a stuck trial would look like it was never enabled. */
	r->sta_final = pwm[PWM_STA];
	r->ctl_live = pwm[PWM_CTL];
	r->berr = ((r->sta_final & STA_BERR) != 0u) ? 1 : 0;

	pwm[PWM_CTL] = 0u;
	(void)pwm[PWM_CTL];
}


static int runStartTest(unsigned long trials)
{
	unsigned long i, stuck = 0, berr = 0, nofeed = 0, sta1 = 0;
	unsigned long runs = 0, run_len = 0, run_max = 0;
	uint32_t hist_sta[START_HIST];
	unsigned long hist_n[START_HIST];
	unsigned int hist_used = 0, h;
	start_result_t r;
	int clk, first_dumped = 0;
	unsigned long period_us;

	memset(hist_sta, 0, sizeof(hist_sta));
	memset(hist_n, 0, sizeof(hist_n));

	printf("pwmwrite: start-test: CM_PWMCTL=0x%08x (ENAB=%u BUSY=%u) CM_PWMDIV=0x%08x\n",
		cprman[CM_PWMCTL], ((cprman[CM_PWMCTL] & CM_CTL_ENAB) != 0u) ? 1u : 0u,
		((cprman[CM_PWMCTL] & CM_CTL_BUSY) != 0u) ? 1u : 0u, cprman[CM_PWMDIV]);

	clk = startClockEnsure();
	if (clk < 0) {
		printf("pwmwrite: start-test VERDICT: REFUSED (clock) — the shared PWM clock will not "
			"report BUSY, so no channel can transmit and every trial would fail for a reason "
			"that is not the defect. Start rpi4-audio (or fix the clock) and re-run.\n");
		return 2;
	}
	if (clk == 1) {
		printf("pwmwrite: start-test: clock was ALREADY running — using it as-is, not "
			"reconfigured (rpi4-audio owns it).\n");
	}
	else {
		printf("pwmwrite: start-test: clock was NOT running — started it here exactly as "
			"audio_clockInit() does (DIVI=%u from the oscillator), and leaving it running on "
			"exit so a driver that starts meanwhile is not cut off.\n", PWM_CLK_DIVI);
	}

	period_us = startPeriodUs();
	printf("pwmwrite: start-test: %lu trials on PWM0 @ 0x%08x, RNG=%u, %u duty words per trial, "
		"enable->feed gap %lu us, one period ~%lu us (from the live divisor)\n",
		trials, PWM0_PAGE, START_RANGE, START_WORDS, startGapUs, period_us);

	for (i = 0; i < trials; i++) {
		startTrial(&r);

		if (r.pushed == 0u) {
			nofeed++;
		}
		if (r.berr != 0) {
			berr++;
		}
		if (r.sta1_seen != 0) {
			sta1++;
		}

		/* The reproducer's own definition: the channel is enabled, clocked and
		 * holding data, and neither transmitted nor emptied. */
		if ((r.sta1_seen == 0) && (r.drained == 0)) {
			stuck++;
			if (run_len == 0ul) {
				runs++;
			}
			run_len++;
			if (run_len > run_max) {
				run_max = run_len;
			}
			if (first_dumped == 0) {
				first_dumped = 1;
				printf("pwmwrite: start-test: FIRST STUCK trial %lu — CTL=0x%08x STA=0x%08x "
					"RNG1=%u RNG2=%u DAT1=%u pushed=%u CM_PWMCTL=0x%08x CM_PWMDIV=0x%08x\n",
					i, r.ctl_live, r.sta_final, pwm[PWM_RNG1], pwm[PWM_RNG2],
					pwm[PWM_DAT1], r.pushed, cprman[CM_PWMCTL], cprman[CM_PWMDIV]);
			}
		}
		else {
			run_len = 0ul;
		}

		for (h = 0; h < hist_used; h++) {
			if (hist_sta[h] == r.sta_final) {
				break;
			}
		}
		if (h < hist_used) {
			hist_n[h]++;
		}
		else if (hist_used < START_HIST) {
			hist_sta[hist_used] = r.sta_final;
			hist_n[hist_used] = 1ul;
			hist_used++;
		}

		/* A silent UART for minutes reads as a hang; say so every so often. */
		if (((i + 1ul) % 500ul) == 0ul) {
			printf("pwmwrite: start-test: %lu/%lu trials, %lu never transmitted\n",
				i + 1ul, trials, stuck);
		}
	}

	printf("pwmwrite: start-test: final-STA histogram (bit0=FULL1 bit1=EMPT1 bit8=BERR bit9=STA1)\n");
	for (h = 0; h < hist_used; h++) {
		printf("pwmwrite: start-test:   STA=0x%08x  %lu\n", hist_sta[h], hist_n[h]);
	}
	printf("pwmwrite: start-test: trials=%lu never-transmitted=%lu bus-errors=%lu "
		"STA1-seen=%lu fifo-refused-feed=%lu stuck-runs=%lu longest-stuck-run=%lu\n",
		trials, stuck, berr, sta1, nofeed, runs, run_max);

	if (stuck == 0ul) {
		printf("pwmwrite: start-test VERDICT: NOT REPRODUCED — all %lu trials transmitted. "
			"The driver's PWM init alone does not leave a dead channel, so the audio stall "
			"needs DMA (DREQ routing/threshold) or something outside audio_pwmInit().\n",
			trials);
		return 0;
	}
	printf("pwmwrite: start-test VERDICT: REPRODUCED — %lu/%lu trials left the channel enabled, "
		"clocked and FIFO-fed without transmitting. No DMA involved, so this is the PIO half of "
		"the stall, reproducible in seconds instead of one sample per boot.\n", stuck, trials);
	printf("pwmwrite: start-test: longest stuck run = %lu. A run of 1 is a per-init race that a "
		"PWM re-arm would clear; a longer run survived the next trial's CTL=0 + CLRF1, i.e. it is "
		"state a CTL re-init cannot reach and the drafted re-arm fix would NOT cure.\n", run_max);
	return 1;
}


int main(int argc, char **argv)
{
	int startTest = ((argc > 1) && (strcmp(argv[1], "--start-test") == 0)) ? 1 : 0;
	unsigned long iters;
	volatile uint32_t *page;
	unsigned long bad;

	if (startTest != 0) {
		iters = (argc > 2) ? strtoul(argv[2], NULL, 0) : START_TRIALS;
		if (argc > 3) {
			startGapUs = strtoul(argv[3], NULL, 0);
		}
	}
	else {
		iters = (argc > 1) ? strtoul(argv[1], NULL, 0) : 100000ul;
	}
	if (iters == 0ul) {
		printf("pwmwrite: iteration count must be non-zero\n");
		return 2;
	}

	page = mmap(NULL, _PAGE_SIZE, PROT_READ | PROT_WRITE,
		MAP_DEVICE | MAP_UNCACHED | MAP_PHYSMEM | MAP_ANONYMOUS, -1, (off_t)PWM0_PAGE);
	if (page == MAP_FAILED) {
		printf("pwmwrite: mmap of PWM0 failed\n");
		return 2;
	}
	pwm = page;

	if (startTest != 0) {
		page = mmap(NULL, _PAGE_SIZE, PROT_READ | PROT_WRITE,
			MAP_DEVICE | MAP_UNCACHED | MAP_PHYSMEM | MAP_ANONYMOUS, -1, (off_t)CPRMAN_PAGE);
		if (page == MAP_FAILED) {
			printf("pwmwrite: mmap of CPRMAN failed\n");
			return 2;
		}
		cprman = page;
		return runStartTest(iters);
	}

	printf("pwmwrite: PWM0 @ 0x%08x, %lu iterations per spacing\n", PWM0_PAGE, iters);
	printf("pwmwrite: CTL=0x%08x RNG1=%u RNG2=%u before we touch anything\n",
		pwm[PWM_CTL], pwm[PWM_RNG1], pwm[PWM_RNG2]);

	bad = run("back-to-back", trial_backToBack, iters);
	bad += run("read-barrier", trial_barrier, iters);
	bad += run("10us-delay", trial_delay, iters / 20u);  /* 20x slower per trial */

	printf("pwmwrite: --- bus errors per spacing (the driver latches one every boot) ---\n");
	{
		unsigned long b1 = runBerr("back-to-back", trial_backToBack, iters / 10u);
		unsigned long b2 = runBerr("read-barrier", trial_barrier, iters / 10u);
		unsigned long b3 = runBerr("10us-delay", trial_delay, iters / 100u);

		unsigned long b4 = runBerr("CTLx3-unpaced", trial_sameRegUnpaced, iters / 10u);
		unsigned long b5 = runBerr("CTLx3-paced", trial_sameRegPaced, iters / 100u);

		printf("pwmwrite: BERR summary back-to-back=%lu read-barrier=%lu 10us=%lu "
			"CTLx3-unpaced=%lu CTLx3-paced=%lu\n", b1, b2, b3, b4, b5);
		if ((b4 > 0u) && (b5 == 0u)) {
			printf("pwmwrite: ⇒ REPRODUCED: successive writes to the SAME register raise "
				"bus errors and pacing removes them. audio_pwmInit() writes PWM_CTL 3x "
				"unpaced.\n");
		}
		if ((b1 > 0u) && (b2 == 0u) && (b3 == 0u)) {
			printf("pwmwrite: ⇒ PACING FIXES IT: unpaced writes raise bus errors, paced ones "
				"do not. audio_pwmInit() writes five registers unpaced.\n");
		}
	}

	if (bad == 0u) {
		printf("pwmwrite: PASS — no dropped write in any spacing. The PWM does not lose "
			"back-to-back register writes, so a lost RNG1 is NOT the audio stall's cause.\n");
		return 0;
	}
	printf("pwmwrite: FAIL — %lu dropped write(s); the spacing that dropped them is the one "
		"audio_pwmInit() uses today.\n", bad);
	return 1;
}
