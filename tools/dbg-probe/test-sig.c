/* Tier-0 debug-primitive probe for Phoenix-RTOS aarch64 (Pi4) — v2.
 *
 * Phoenix signal ABI (confirmed from libphoenix/signal/signal.c): handlers are the SIMPLE
 * void(int) form via sa_handler; SA_SIGINFO/sa_sigaction/ucontext are NOT supported. So we
 * register sa_handler and, since there's no ucontext, backtrace via the frame-pointer chain
 * reachable from the handler's own frame (works if the kernel signal frame preserves x29).
 *
 * Questions answered:
 *   (1) SIGSEGV (NULL deref) delivered to a userspace handler?  [crash backtrace feasible]
 *   (2) SIGALRM (async timer) delivered? [interrupt a HANG] — the v1 log already showed signal 14
 *       IS delivered, so this just confirms with a real handler.
 *   (3) Does an fp-walk from inside the handler reach the interrupted code (so a backtrace names
 *       the hang/fault site)? -> the addresses printed are addr2line'd host-side against the ELF.
 *
 * Build: aarch64-phoenix-gcc -O0 -g -fno-omit-frame-pointer -o test-sig test-sig.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

/* Walk the aarch64 frame-pointer chain from `fp`, printing return addresses. Bounded + sanity
 * checked so a garbage chain can't fault/loop. */
static void fp_backtrace(const char *tag, unsigned long fp)
{
	printf("  bt(%s): starting fp=0x%lx\n", tag, fp);
	for (int i = 0; i < 24 && fp; i++) {
		unsigned long next_fp = ((unsigned long *)fp)[0];
		unsigned long ret     = ((unsigned long *)fp)[1];
		printf("  bt(%s): #%d fp=0x%lx ret=0x%lx\n", tag, i, fp, ret);
		if (ret == 0)
			break;
		if (next_fp <= fp || (next_fp - fp) > 0x200000UL)
			break;
		fp = next_fp;
	}
	fflush(stdout);
}

static volatile int g_alrm = 0;

static void on_alrm(int sig)
{
	g_alrm++;
	printf("SIGALRM handler fired #%d (sig=%d)\n", g_alrm, sig);
	fp_backtrace("alrm", (unsigned long)__builtin_frame_address(0));
	alarm(1);
}

static void on_segv(int sig)
{
	printf("SIGSEGV handler fired (sig=%d) — SYNC-FAULT DELIVERY WORKS\n", sig);
	fp_backtrace("segv", (unsigned long)__builtin_frame_address(0));
	_exit(0);
}

static void install(int sig, void (*fn)(int))
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = fn;   /* Phoenix: simple void(int) form */
	int r = sigaction(sig, &sa, NULL);
	printf("sig: sigaction(%d)=%d\n", sig, r); fflush(stdout);
}

int main(int argc, char **argv)
{
	(void)argc; (void)argv;
	printf("sig: probe v2 start\n"); fflush(stdout);
	install(SIGSEGV, on_segv);
	install(SIGALRM, on_alrm);

	/* (2) async delivery: alarm(1) then sleep(3). Expect the handler to fire (~1s) and print a
	 * backtrace that should reach sleep()/main() if the fp-chain links through the signal frame. */
	printf("sig: [A] alarm(1)+sleep(3) — async SIGALRM test...\n"); fflush(stdout);
	alarm(1);
	sleep(3);
	printf("sig: [A] after sleep, g_alrm=%d\n", g_alrm); fflush(stdout);
	alarm(0);

	/* (1)+(3) sync fault: deref NULL -> on_segv should fire + backtrace the fault site. */
	printf("sig: [C] dereferencing NULL to test SIGSEGV...\n"); fflush(stdout);
	volatile int *p = (int *)0;
	int v = *p;
	printf("sig: STILL ALIVE after NULL deref v=%d (unexpected)\n", v); fflush(stdout);
	return 0;
}
