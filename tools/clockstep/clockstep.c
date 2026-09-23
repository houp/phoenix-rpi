/* Does CLOCK_MONOTONIC survive a wall-clock step on Phoenix?
 *
 * The quake3 fix (C6) rests entirely on that property: psh sets the clock from
 * the network shortly after login, the step is ~56 years on a board with no RTC,
 * and any app that anchors a duration to a pre-step reading gets nonsense.
 * Switching to CLOCK_MONOTONIC only helps if Phoenix's monotonic clock really is
 * unaffected -- "the header defines the constant" is not evidence, and
 * clock_gettime accepts the id either way.
 *
 * So step the wall clock deliberately and measure both.
 *
 *   TAG-mono-delta  should be ~the wall time actually spent (a second or two)
 *   TAG-real-delta  should be ~the size of the step
 *
 * Copyright 2026 Phoenix Systems  %LICENSE%
 */
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define STEP_SECS (56LL * 365LL * 24LL * 3600LL) /* the real-world step size */

static long long ms(const struct timespec *t)
{
	return ((long long)t->tv_sec * 1000LL) + (t->tv_nsec / 1000000L);
}

int main(void)
{
	struct timespec m0, m1, r0, r1, set;

	if (clock_gettime(CLOCK_MONOTONIC, &m0) != 0) {
		printf("TAG-fail clock_gettime(MONOTONIC)\n");
		return 1;
	}
	if (clock_gettime(CLOCK_REALTIME, &r0) != 0) {
		printf("TAG-fail clock_gettime(REALTIME)\n");
		return 1;
	}
	printf("TAG-before mono=%lld real=%lld\n", ms(&m0), ms(&r0));
	fflush(stdout);

	sleep(1);

	/* The step, in the same direction and of the same magnitude as ntpclient's. */
	set.tv_sec = (time_t)(r0.tv_sec + STEP_SECS);
	set.tv_nsec = 0;
	if (clock_settime(CLOCK_REALTIME, &set) != 0) {
		printf("TAG-fail clock_settime -- cannot run the experiment\n");
		return 1;
	}
	printf("TAG-stepped by=%lld s\n", STEP_SECS);
	fflush(stdout);

	if ((clock_gettime(CLOCK_MONOTONIC, &m1) != 0) || (clock_gettime(CLOCK_REALTIME, &r1) != 0)) {
		printf("TAG-fail second read\n");
		return 1;
	}

	printf("TAG-after  mono=%lld real=%lld\n", ms(&m1), ms(&r1));
	printf("TAG-mono-delta %lld ms   (want a few thousand at most)\n", ms(&m1) - ms(&m0));
	printf("TAG-real-delta %lld ms   (want ~%lld)\n", ms(&r1) - ms(&r0), STEP_SECS * 1000LL);
	printf("TAG-verdict %s\n",
		((ms(&m1) - ms(&m0)) < 60000LL) ? "MONOTONIC-SURVIVED-THE-STEP" : "MONOTONIC-MOVED-WITH-THE-WALL-CLOCK");
	return 0;
}
