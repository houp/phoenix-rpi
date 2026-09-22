/* nullderef.c - crash with a HEALTHY stack.
 *
 * The control for the exceptions_dumpUserStack() guard. stack-bomb proves the
 * guard fires when sp's page is unmapped; this proves it does NOT fire when the
 * stack is fine, i.e. that ordinary crash dumps still print their stack words.
 * Without this, "no EL1 abort" could equally mean the dump was suppressed for
 * everything, which would be a far worse regression than the bug.
 *
 * Expected: Data Abort (EL0) at far=0, and a "stack:" section containing words
 * -- NOT "stack: not mapped at ...".
 *
 * Copyright 2026 Phoenix Systems  %LICENSE%
 */
#include <stdio.h>

volatile unsigned long *g_p;

int main(void)
{
	printf("nullderef: about to write through a NULL pointer with a healthy stack\n");
	fflush(stdout);
	g_p = (volatile unsigned long *)0;
	*g_p = 0xdeadUL;
	printf("nullderef: UNEXPECTED - the write did not fault\n");
	fflush(stdout);
	return 0;
}
