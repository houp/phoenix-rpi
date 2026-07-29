/* Validate the dbg facility: crash mode names the fault call-stack; hang mode's watchdog names
 * the stuck function. Build: aarch64-phoenix-gcc -O0 -g -fno-omit-frame-pointer test-dbg.c dbg.c */
#include "dbg.h"
#include <stdio.h>
#include <unistd.h>

static void level3_fault(void)
{
	printf("test: level3 about to deref NULL\n"); fflush(stdout);
	volatile int *p = (int *)0;
	*p = 42;   /* SIGSEGV here -> backtrace should name level3_fault<-level2<-level1<-main */
}
static void level2(void) { level3_fault(); }
static void level1(void) { level2(); }

static void hang_here(void)
{
	printf("test: entering hang loop (watchdog should name hang_here)\n"); fflush(stdout);
	volatile unsigned long x = 0;
	for (;;) { x++; }   /* tight infinite loop -> watchdog SIGALRM should interrupt + backtrace */
}

int main(int argc, char **argv)
{
	dbg_init();
	if (argc > 1 && argv[1][0] == 'h') {
		dbg_arm_watchdog(3);
		hang_here();   /* never returns; watchdog fires */
	}
	else {
		printf("test: crash-mode\n"); fflush(stdout);
		level1();      /* faults deep in the call chain */
		printf("test: unexpectedly survived\n"); fflush(stdout);
	}
	return 0;
}
