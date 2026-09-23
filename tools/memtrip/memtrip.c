/*
 * memtrip.c - a large known-pattern region, scanned for stray writes.
 *
 * WHY THIS EXISTS. C1 is only ever *noticed* when the stray write happens to
 * land on a malloc heap header -- 16 bytes per heap, a few hundred heaps, so a
 * few KB of sensitive target inside hundreds of MB of address space. Every
 * other landing is invisible. That is very likely why the observed rate is only
 * ~2.5% per SuperTuxKart run while the underlying event may be far more common:
 * we are watching a tiny fraction of the dartboard.
 *
 * So: fill a big region with a known pattern and watch it. 64 MiB of tripwire is
 * thousands of times the cross-section of the heap headers, which should turn a
 * ~2.5%-per-run event into something that fires in most runs -- and it reports
 * the exact address and the surrounding words, which settles the question the
 * allocator's own instrument cannot: is this an isolated 4-byte store, or a
 * foreign structure written over a range?
 *
 * ⚠ A negative result here is informative too. If the tripwire stays clean
 * across runs in which the allocator DOES fire, the write is not uniformly
 * distributed -- it is aimed at something about heap headers (page+4), and that
 * is a completely different search.
 *
 * Run it alongside the workload:   memtrip &   then   stk ...
 *
 * Copyright 2026 Phoenix Systems  %LICENSE%
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>

/* Not 0 and not 0x80000001, and not a plausible pointer or length, so anything
 * that is not this word is unambiguously somebody else's write. */
#define TRIP_PATTERN 0xa5a5a5a5u

#define TRIP_MAX_REPORTS 12


static void report(const uint32_t *base, size_t words, size_t i)
{
	size_t lo = (i >= 4u) ? (i - 4u) : 0u;
	size_t hi = ((i + 5u) < words) ? (i + 5u) : words;
	size_t k;

	printf("memtrip: STRAY WRITE at %p (word %zu, byte offset 0x%zx)\n",
		(const void *)&base[i], i, i * sizeof(uint32_t));
	printf("memtrip:   value = 0x%08x (expected 0x%08x)\n", base[i], TRIP_PATTERN);
	printf("memtrip:   page-aligned? %s ; offset-in-page = 0x%lx\n",
		((((uintptr_t)&base[i]) & 0xfffu) == 0u) ? "yes" : "no",
		(unsigned long)(((uintptr_t)&base[i]) & 0xfffu));

	/* The discriminator: isolated store, or a run of foreign words? */
	for (k = lo; k < hi; k++) {
		printf("memtrip:   w[%+d] = 0x%08x%s\n", (int)(k - i), base[k],
			(base[k] == TRIP_PATTERN) ? "" : "   <-- not pattern");
	}
	fflush(stdout);
}


int main(int argc, char **argv)
{
	size_t mib = (argc > 1) ? (size_t)strtoul(argv[1], NULL, 0) : 64u;
	size_t bytes = mib * 1024u * 1024u;
	size_t words = bytes / sizeof(uint32_t);
	unsigned reports = 0;
	unsigned long pass = 0;
	uint32_t *base;
	size_t i;

	/* Detach like rpi4-wifi does. psh implements NO `&` -- there is no background
	 * handling in it at all -- so a long-running foreground tool simply owns the
	 * shell and every later command is accepted and never run. Measured: after
	 * `memtrip 8 &`, neither `/bin/ls /dev` nor `echo` produced a byte. A resident
	 * tool has to fork itself, exactly as the wifi daemon does. */
	{
		pid_t pid = fork();

		if (pid < 0) {
			printf("memtrip: fork failed -- cannot detach\n");
			fflush(stdout);
			return 1;
		}
		if (pid > 0) {
			return 0; /* parent returns the prompt to psh */
		}
		(void)setsid();
	}

	base = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (base == MAP_FAILED) {
		printf("memtrip: mmap of %zu MiB FAILED -- nothing below is meaningful\n", mib);
		fflush(stdout);
		return 1;
	}

	/* Writing every word also faults every page in, so the region is resident and
	 * a stray physical write has somewhere real to land. */
	for (i = 0; i < words; i++) {
		base[i] = TRIP_PATTERN;
	}

	printf("memtrip: armed %zu MiB at %p .. %p, pattern 0x%08x\n",
		mib, (void *)base, (void *)(base + words), TRIP_PATTERN);
	fflush(stdout);

	/* `memtrip <mib> selftest` plants the exact C1 word and lets the normal scan
	 * find it. An instrument that has never been seen to fire is not evidence of
	 * anything when it stays quiet, and this one's whole value is in its silence
	 * being meaningful -- so prove the detector on the target, not just on paper. */
	if ((argc > 2) && (strcmp(argv[2], "selftest") == 0)) {
		base[words / 3u] = 0x80000001u;
		printf("memtrip: selftest -- planted 0x80000001 at word %zu; the next pass must report it\n",
			words / 3u);
		fflush(stdout);
	}

	for (;;) {
		unsigned hits = 0;

		for (i = 0; i < words; i++) {
			if (base[i] != TRIP_PATTERN) {
				hits++;
				if (reports < TRIP_MAX_REPORTS) {
					reports++;
					report(base, words, i);
				}
				/* Restore, so the next pass reports only NEW writes and a single
				 * landing cannot be counted forever. */
				base[i] = TRIP_PATTERN;
			}
		}

		pass++;
		/* Near-silent by design: the harness ends a command's capture on UART
		 * IDLE, so a chatty background scanner would stop the cycle ever reaching
		 * the workload. Heartbeat rarely, just often enough to prove it is alive. */
		if ((hits != 0u) || ((pass % 150u) == 0u)) {
			printf("memtrip: pass %lu done, %u stray word(s) this pass, %u reported\n",
				pass, hits, reports);
			fflush(stdout);
		}

		/* Leave the CPU to the workload; the point is to be present, not fast. */
		usleep(200 * 1000);
	}

	return 0;
}
