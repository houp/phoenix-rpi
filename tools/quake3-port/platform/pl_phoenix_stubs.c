/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2026 Phoenix Systems. Author: Witold Bołt.
 *
 * Libc / Mesa gap-fillers for the quake3e Phoenix port: the symbol the ported
 * Mesa V3D GL stack references but Phoenix's libc and the GPU archives
 * (libGL-phoenix.a / libv3d-phoenix.a) do not provide. Kept out of the
 * archives; linked directly into the quake3e ELF.
 *
 * Note: rint() used to be stubbed here too, but libphoenix now provides
 * rint/nearbyint/lrint/lround (phoenix libm), so the stub was removed to avoid
 * a duplicate definition.
 */
#include <pthread.h>
#include <time.h>

/* Mesa's util/u_thread.c references pthread_getcpuclockid for per-thread CPU
 * timing; libphoenix lacks it. A monotonic-clock stand-in suffices (same
 * gap-filler the yQuake2 port carries). */
int
pthread_getcpuclockid(pthread_t thread, clockid_t *clock_id)
{
	(void)thread;
	if (clock_id)
	{
		*clock_id = CLOCK_MONOTONIC;
	}
	return 0;
}
