/*
 * sigbomb.c - reach hal_cpuPushSignal() with an unmapped user stack.
 *
 * tools/stack-bomb overflows the stack and does NOT take the kernel down, which
 * is what retired the "stack exhaustion is a kernel DoS" claim. Reading the
 * kernel says why it cannot: proc/threads.c _threads_trySignalDeliver() only
 * calls hal_cpuPushSignal() when _threads_checkSignal() returns a signal, and
 * that requires a handler which is neither SIG_IGN nor a default-ignore action.
 * With no handler installed a fatal fault takes the DEFAULT action -- the kernel
 * kills the process and never writes a signal frame to userspace at all.
 *
 * So the frame push is only reachable with a handler INSTALLED. Overflow the
 * stack then, and the kernel must write the signal frame to a stack pointer that
 * is already past the stack VMA: the exact unguarded hal_memcpy in
 * hal/aarch64/cpu.c. That is this program.
 *
 * Modes (argv[1], default "stack"):
 *   stack     handler installed + stack exhausted  -- the case under test
 *   control   handler installed + ordinary NULL deref on a HEALTHY stack --
 *             proves the handler path works at all, so a silent "stack" run
 *             means something rather than a broken harness
 *   deep      handler installed + NULL deref ~740 KiB DOWN a 1 MiB stack, so the
 *             signal is delivered with the SP far into pages that were demand-
 *             paged moments ago. This is the anti-regression case: a guard that
 *             tested page RESIDENCY instead of VMA membership would pass every
 *             other mode here and silently kill this one.
 *   nohandler stack exhausted with NO handler -- today's stack-bomb, for contrast
 *
 * Read the result from the tagged lines, never from the exit code:
 *   SIGBOMB: handler ENTERED      the kernel delivered the frame
 *   SIGBOMB: survived             the process came back alive
 *   (no SIGBOMB line, box alive)  the kernel killed the process -- correct
 *   (box dead / no psh afterwards) the EL1 double-fault, i.e. the bug
 *
 * The handler _exit()s instead of returning: returning from a SIGSEGV handler
 * re-runs the faulting instruction, and a fast failure turned into an endless
 * refault loop is worse than the bug being measured.
 *
 * Copyright 2026 Phoenix Systems  %LICENSE%
 */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* write(2) only inside the handler: printf is not async-signal-safe, and on an
 * exhausted stack its buffering machinery is the last thing we want to invoke. */
static void say(const char *s)
{
	(void)write(STDOUT_FILENO, s, strlen(s));
}


static void onSegv(int sig)
{
	(void)sig;
	say("SIGBOMB: handler ENTERED\n");
	_exit(3);
}


volatile unsigned long g_sink;
volatile unsigned long g_depth;

/* 4 KiB of frame per call, touched at both ends so the page really is used, and
 * the result consumed AFTER the recursive call so it cannot become a tail call. */
static unsigned long recurse(unsigned long depth)
{
	volatile char frame[4096];

	frame[0] = (char)depth;
	frame[4095] = (char)(depth >> 8);
	g_depth = depth;
	g_sink = (unsigned long)frame[0] + (unsigned long)frame[4095];
	g_sink += recurse(depth + 1U);

	return g_sink + (unsigned long)frame[0];
}


/* ~740 KiB of a 1 MiB stack: deep enough that the fault happens in pages the
 * kernel demand-paged moments earlier, with enough headroom left that the signal
 * frame genuinely fits. If this ever starts failing, check SIZE_USTACK first. */
#define DEEP_FRAMES 180U

static unsigned long descend(unsigned long depth)
{
	volatile char frame[4096];

	frame[0] = (char)depth;
	frame[4095] = (char)(depth >> 8);
	g_depth = depth;

	if (depth < DEEP_FRAMES) {
		g_sink += descend(depth + 1U);
	}
	else {
		volatile int *p = NULL;

		*p = 1;
	}

	return g_sink + (unsigned long)frame[0];
}


int main(int argc, char **argv)
{
	const char *mode = (argc > 1) ? argv[1] : "stack";
	struct sigaction sa;

	printf("SIGBOMB: mode=%s\n", mode);
	fflush(stdout);

	if (strcmp(mode, "nohandler") != 0) {
		(void)memset(&sa, 0, sizeof(sa));
		sa.sa_handler = onSegv;
		(void)sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;

		if (sigaction(SIGSEGV, &sa, NULL) != 0) {
			printf("SIGBOMB: sigaction FAILED -- nothing below is meaningful\n");
			fflush(stdout);
			return 1;
		}
		printf("SIGBOMB: handler installed for SIGSEGV\n");
		fflush(stdout);
	}
	else {
		printf("SIGBOMB: no handler installed (stack-bomb equivalent)\n");
		fflush(stdout);
	}

	if (strcmp(mode, "control") == 0) {
		volatile int *p = NULL;

		printf("SIGBOMB: faulting on a HEALTHY stack\n");
		fflush(stdout);
		*p = 1;
		printf("SIGBOMB: survived the NULL write -- handler did not fire\n");
		fflush(stdout);
		return 0;
	}

	if (strcmp(mode, "deep") == 0) {
		printf("SIGBOMB: descending %u frames, then faulting down there\n", DEEP_FRAMES);
		fflush(stdout);
		g_sink = descend(0U);
		printf("SIGBOMB: survived the deep NULL write -- handler did not fire\n");
		fflush(stdout);
		return 0;
	}

	printf("SIGBOMB: recursing 4KiB/frame to exhaust the 1MiB user stack\n");
	fflush(stdout);
	g_sink = recurse(0U);

	printf("SIGBOMB: survived, depth=%lu sink=%lu (NO overflow -- unexpected)\n",
		g_depth, g_sink);
	fflush(stdout);

	return 0;
}
