/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2026 Phoenix Systems. Author: Witold Bołt.
 *
 * Libc / Mesa gap-fillers pulled in by ref_gl1's use of the ported Mesa V3D
 * GL stack (libGL-phoenix.a / libv3d-phoenix.a). These are the symbols the
 * tiny SDL2 GL smoke test never referenced but the full renderer does. Kept
 * out of the archives; linked directly by the yQuake2 ELF.
 */
#include <pthread.h>
#include <time.h>

/* Mesa's util/u_thread.c references pthread_getcpuclockid for per-thread CPU
 * timing; libphoenix lacks it. A monotonic-clock stand-in suffices. */
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

/* Phoenix's libm subset lacks lroundf. */
long
lroundf(float x)
{
	return (long)(x < 0.0f ? x - 0.5f : x + 0.5f);
}

/* NOTE: trace_context_create_threaded (Mesa gallium driver_trace, a
 * GALLIUM_TRACE debug-only symbol referenced by u_threaded_context.c) is
 * already provided by the SDL2 GL-context glue (sdl_phoenix_glctx.c), so it
 * is intentionally NOT defined here. */
