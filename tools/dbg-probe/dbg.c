/* Phoenix-RTOS aarch64 in-process debug/backtrace facility.
 *
 * Phoenix delivers no ucontext to signal handlers, but the libphoenix aarch64 signal trampoline
 * (patched) stashes the interrupted cpu_context_t* into the global `_dbg_signal_ctx` on every
 * signal. This facility reads that context (interrupted pc + x29 frame pointer) and walks the
 * frame-pointer chain, so a crash OR a watchdog-interrupted HANG prints a real backtrace naming
 * the actual code — over UART, with the Pi kept booted (only NFS binaries change).
 *
 * Build the target with -fno-omit-frame-pointer so the x29 chain is valid.
 * Host-side symbolize: aarch64-phoenix-addr2line -f -e <elf> <printed ret addresses>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

/* Set by libphoenix's patched _signal_trampoline to the interrupted cpu_context_t* + the
 * interrupted PC (leaf/fault site; signalCtx->pc itself is clobbered with the handler addr). */
extern void *_dbg_signal_ctx;
extern unsigned long _dbg_signal_pc;

/* Mirror of the kernel aarch64 cpu_context_t layout (hal/aarch64/arch/cpu.h). MUST match the
 * toolchain's float ABI (same __SOFTFP__ as the kernel build => same offsets). */
typedef struct {
	unsigned long savesp;
	unsigned long cpacr;
#ifndef __SOFTFP__
	unsigned long fpcr;
	unsigned long fpsr;
	unsigned long freg[2 * 32];
#endif
	unsigned long psr;
	unsigned long pc;
	unsigned long x[31]; /* x[29] = frame pointer, x[30] = link register */
	unsigned long sp;
} dbg_ctx_t;

static void walk_fp(unsigned long fp)
{
	for (int i = 0; i < 40 && fp; i++) {
		unsigned long next_fp = ((unsigned long *)fp)[0];
		unsigned long ret     = ((unsigned long *)fp)[1];
		if (ret == 0)
			break;
		printf("dbg:   #%02d 0x%lx\n", i, ret);
		if (next_fp <= fp || (next_fp - fp) > 0x400000UL)
			break;
		fp = next_fp;
	}
	fflush(stdout);
}

void dbg_backtrace(const char *tag)
{
	dbg_ctx_t *c = (dbg_ctx_t *)_dbg_signal_ctx;
	printf("dbg: ===== backtrace [%s] =====\n", tag ? tag : "");
	if (c != NULL) {
		/* Interrupted/faulting code: _dbg_signal_pc is the leaf/fault instruction (signalCtx->pc
		 * is clobbered with the handler), x[29] the frame pointer, x[30] the leaf's return addr. */
		printf("dbg:   pc=0x%lx sp=0x%lx fp(x29)=0x%lx lr(x30)=0x%lx\n",
		       _dbg_signal_pc, c->sp, c->x[29], c->x[30]);
		printf("dbg:   #LEAF 0x%lx\n", _dbg_signal_pc);
		walk_fp(c->x[29]);
	}
	else {
		/* Not from a signal: unwind the caller of dbg_backtrace. */
		printf("dbg:   (no signal context; unwinding caller)\n");
		walk_fp((unsigned long)__builtin_frame_address(0));
	}
	printf("dbg: ===== end backtrace =====\n");
	fflush(stdout);
}

static void fault_handler(int sig)
{
	printf("\ndbg: *** FAULT signal=%d ***\n", sig);
	fflush(stdout);
	dbg_backtrace("fault");
	_exit(128 + sig);
}

static volatile unsigned g_wd_ticks = 0;
static unsigned int g_wd_secs = 0;

static void watchdog_handler(int sig)
{
	(void)sig;
	g_wd_ticks++;
	printf("\ndbg: *** WATCHDOG tick #%u (possible hang) ***\n", g_wd_ticks);
	fflush(stdout);
	dbg_backtrace("watchdog");
	if (g_wd_secs)
		alarm(g_wd_secs); /* re-arm */
}

static void install(int sig, void (*fn)(int))
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = fn;
	sigaction(sig, &sa, NULL);
}

void dbg_init(void)
{
	install(SIGSEGV, fault_handler);
	install(SIGILL, fault_handler);
	install(SIGBUS, fault_handler);
	install(SIGFPE, fault_handler);
	install(SIGABRT, fault_handler);
	printf("dbg: fault handlers installed (SEGV/ILL/BUS/FPE/ABRT)\n");
	fflush(stdout);
}

void dbg_arm_watchdog(unsigned int secs)
{
	g_wd_secs = secs;
	install(SIGALRM, watchdog_handler);
	alarm(secs);
	printf("dbg: watchdog armed (%u s)\n", secs);
	fflush(stdout);
}
