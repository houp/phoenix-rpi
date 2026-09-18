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
 * Usage: pwmwrite [iterations]        (default 100000)
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
#define PWM_RNG1  (0x10u / 4u)
#define PWM_RNG2  (0x20u / 4u)

#define PAT_A 612u
#define PAT_B 613u


static volatile uint32_t *pwm;


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


int main(int argc, char **argv)
{
	unsigned long iters = (argc > 1) ? strtoul(argv[1], NULL, 0) : 100000ul;
	volatile uint32_t *page;
	unsigned long bad;

	page = mmap(NULL, _PAGE_SIZE, PROT_READ | PROT_WRITE,
		MAP_DEVICE | MAP_UNCACHED | MAP_PHYSMEM | MAP_ANONYMOUS, -1, (off_t)PWM0_PAGE);
	if (page == MAP_FAILED) {
		printf("pwmwrite: mmap of PWM0 failed\n");
		return 2;
	}
	pwm = page;

	printf("pwmwrite: PWM0 @ 0x%08x, %lu iterations per spacing\n", PWM0_PAGE, iters);
	printf("pwmwrite: CTL=0x%08x RNG1=%u RNG2=%u before we touch anything\n",
		pwm[PWM_CTL], pwm[PWM_RNG1], pwm[PWM_RNG2]);

	bad = run("back-to-back", trial_backToBack, iters);
	bad += run("read-barrier", trial_barrier, iters);
	bad += run("10us-delay", trial_delay, iters / 20u);  /* 20x slower per trial */

	if (bad == 0u) {
		printf("pwmwrite: PASS — no dropped write in any spacing. The PWM does not lose "
			"back-to-back register writes, so a lost RNG1 is NOT the audio stall's cause.\n");
		return 0;
	}
	printf("pwmwrite: FAIL — %lu dropped write(s); the spacing that dropped them is the one "
		"audio_pwmInit() uses today.\n", bad);
	return 1;
}
