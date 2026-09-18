/*
 * Phoenix-RTOS
 *
 * armtrials - ask rpi4-audio to repeat its own DMA arm sequence N times
 *
 * The stall behind KNOWN-ISSUES q2-sdl-openaudio-hang fires on ~7% of BOOTS: about
 * seven samples per hundred boots at 2.5 minutes each. Two userspace probes already
 * sample the same shape thousands of times per boot (tools/pwm-write-probe,
 * tools/pwm-dma-probe) and found nothing in 10 000 trials -- but both necessarily run
 * on PWM0 / DREQ 5 / DMA channel 6, because the driver owns PWM1 / DREQ 1 / channel 5
 * and a probe that fought it would prove nothing. This tool closes that gap by asking
 * the driver itself to do the repeating, on the instance that actually fails.
 *
 * ⚠ It blocks the driver's message loop for ~20 ms per trial and resets the streaming
 * engine on each one, so /dev/audio0 plays nothing while it runs. Diagnostic only.
 *
 * Copyright 2026 Phoenix Systems
 * Author: Witold Bołt
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

/* SPDX-License-Identifier: BSD-3-Clause */

#include <audio/rpi4-audio/rpi4-audio.h>

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>


int main(int argc, char **argv)
{
	rpi4audio_armtrials_t at;
	unsigned long trials = (argc > 1) ? strtoul(argv[1], NULL, 0) : 500ul;
	int fd;

	if (trials == 0ul) {
		printf("armtrials: trial count must be non-zero\n");
		return 2;
	}

	fd = open("/dev/audio0", O_RDWR);
	if (fd < 0) {
		printf("armtrials: cannot open /dev/audio0\n");
		return 2;
	}

	memset(&at, 0, sizeof(at));
	at.trials = (uint32_t)trials;

	/* ~20 ms per cycle, all of it inside the driver's message loop, so this ioctl() blocks
	 * for trials*20 ms — 5000 trials is ~100 s. A test cycle must give --idle-secs MORE
	 * than that or the harness will cut the run off mid-loop and the count will read low. */
	printf("armtrials: asking rpi4-audio for %lu arm cycles on PWM1 / DREQ 1 / DMA ch5 "
		"(~20 ms each = ~%lu s; the device is silent and the ioctl blocks meanwhile)\n",
		trials, (trials * 20ul) / 1000ul);

	if (ioctl(fd, RPI4AUDIO_ARMTRIALS, &at) < 0) {
		printf("armtrials: RPI4AUDIO_ARMTRIALS failed -- is this a driver build that has it?\n");
		close(fd);
		return 2;
	}
	close(fd);

	if (at.ran == 0u) {
		printf("armtrials: VERDICT: REFUSED (the driver has no streaming engine to arm -- "
			"it fell back to PIO, so there is nothing to sample)\n");
		return 2;
	}

	printf("armtrials: ran=%u parked=%u advance %u..%u ring words\n",
		at.ran, at.parked, at.minWords, at.maxWords);
	if (at.parked != 0u) {
		printf("armtrials: first parked cycle: STA=0x%08x DMA_CS=0x%08x DMA_DEBUG=0x%08x\n",
			at.firstSta, at.firstCs, at.firstDebug);
		printf("armtrials: VERDICT: REPRODUCED -- %u of %u arms of the REAL instance came up "
			"parked. The stall is per-arm, not per-boot, and it is specific to PWM1 / DREQ 1 / "
			"channel 5 (PWM0 / DREQ 5 / channel 6 scored 0 in 3000 arms).\n",
			at.parked, at.ran);
		return 1;
	}

	printf("armtrials: VERDICT: NOT REPRODUCED -- all %u arms of the real instance streamed "
		"(>= the driver's threshold). So the stall is not per-arm on this instance either: what "
		"is left is the BOOT-TIME state the driver arms in -- a generator that has never run "
		"since reset, and lwip/USB/NFS coming up alongside -- neither of which a psh-time run "
		"can reproduce.\n", at.ran);
	return 0;
}
