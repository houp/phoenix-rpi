/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2026 Phoenix Systems. Author: Witold Bołt.
 *
 * Libc / Mesa gap-fillers for the quake3e Phoenix port: the two symbols the
 * engine + ported Mesa V3D GL stack reference but Phoenix's libc/libm and the
 * GPU archives (libGL-phoenix.a / libv3d-phoenix.a) do not provide. Kept out
 * of the archives; linked directly into the quake3e ELF.
 */
#include <math.h>
#include <pthread.h>
#include <time.h>

/* Phoenix's libm subset lacks rint(). quake3e's common.c uses it for simple
 * round-to-nearest and does not depend on the standard's round-half-to-even
 * tie rule, so round-half-away is both correct enough and obvious. */
double
rint(double x)
{
	return (x < 0.0) ? ceil(x - 0.5) : floor(x + 0.5);
}

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
