/*
 * mtstress — multithreaded libphoenix malloc/realloc/free stress for the Pi 4.
 *
 * Why this exists (2026-09-17). The heap containment guards fired in four field
 * runs between 09-15 and 09-17 -- three SuperTuxKart, one vkQuake -- reporting
 * "free() of a corrupt chunk header" up to 71 times in a single run while the
 * process kept running (docs/misc/2026-09-17-stk-rate-and-allocator-guard-fires.md).
 * The host harness (tools/malloc-harness, which compiles the REAL malloc_dl.c)
 * then ran 8 seeds x 300k operations -- ~2.4M ops, ~75k heap mmap/munmap cycles --
 * with ZERO invariant violations. That clears the allocator's single-threaded
 * bookkeeping and leaves the obvious untested axis: the host harness is
 * single-threaded, and both apps that fired are heavily multithreaded.
 *
 * So: many threads, the same allocator, and a size mix chosen to keep heaps
 * being created AND released underneath each other -- the window in which a
 * heap can be handed back to the OS while another thread still holds a block in
 * it. Every block carries a tag pattern that is verified before it is freed, so
 * user-data corruption is caught here rather than inferred later from a crash.
 *
 * The allocator's own guards do the rest of the talking: any "corrupt chunk
 * header", "handed out twice" or "double free" line in the UART log IS the
 * finding, and since 2026-09-17 each carries a `why=` code naming the check
 * that failed.
 *
 * Usage: mtstress [threads] [ops-per-thread]      (default 4 threads, 20000 ops)
 *
 * Copyright 2026 Phoenix Systems
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LIVE_MAX 64u

typedef struct {
	void *ptr;
	size_t size;
	uint8_t tag;
} block_t;

typedef struct {
	unsigned int id;
	unsigned long ops;
	unsigned long allocs;
	unsigned long frees;
	unsigned long reallocs;
	unsigned long bytesPeak;
	unsigned long mismatches;
	unsigned long oom;
	uint64_t rng;
} worker_t;


static uint64_t rnd(uint64_t *s)
{
	/* xorshift64*: no libm, no shared state, reproducible per thread. */
	uint64_t x = *s;

	x ^= x >> 12;
	x ^= x << 25;
	x ^= x >> 27;
	*s = x;
	return x * 0x2545f4914f6cdd1dULL;
}


/* A size mix that keeps the heap moving. Small blocks dominate (that is what a
 * game allocates), but one draw in sixteen is large enough to force a fresh heap
 * mmap, and one in sixty-four is large enough that freeing it can empty a heap
 * and hand it back -- which is the race worth hunting. */
static size_t pickSize(uint64_t *s)
{
	uint64_t r = rnd(s);

	if ((r & 63u) == 0u) {
		return (size_t)(64u * 1024u + (r >> 6) % (192u * 1024u));
	}
	if ((r & 15u) == 0u) {
		return (size_t)(4096u + (r >> 4) % (28u * 1024u));
	}
	return (size_t)(16u + (r >> 4) % 1008u);
}


static void fillTag(block_t *b)
{
	size_t n = (b->size < 64u) ? b->size : 64u;

	memset(b->ptr, (int)b->tag, n);
	/* ...and the tail, so an overrun from the NEXT block is caught too. */
	if (b->size > 128u) {
		memset((char *)b->ptr + b->size - 64u, (int)b->tag, 64u);
	}
}


static int checkTag(const block_t *b)
{
	const unsigned char *p = (const unsigned char *)b->ptr;
	size_t n = (b->size < 64u) ? b->size : 64u;
	size_t i;

	for (i = 0; i < n; i++) {
		if (p[i] != b->tag) {
			return 0;
		}
	}
	if (b->size > 128u) {
		p = (const unsigned char *)b->ptr + b->size - 64u;
		for (i = 0; i < 64u; i++) {
			if (p[i] != b->tag) {
				return 0;
			}
		}
	}
	return 1;
}


static void *worker(void *arg)
{
	worker_t *w = (worker_t *)arg;
	block_t live[LIVE_MAX];
	unsigned int nlive = 0;
	unsigned long i;
	unsigned long bytes = 0;

	memset(live, 0, sizeof(live));

	for (i = 0; i < w->ops; i++) {
		uint64_t r = rnd(&w->rng);
		unsigned int op = (unsigned int)(r % 100u);

		if ((nlive == LIVE_MAX) || ((op < 30u) && (nlive > 0u))) {
			unsigned int k = (unsigned int)((r >> 8) % nlive);

			if (checkTag(&live[k]) == 0) {
				w->mismatches++;
				printf("mtstress[%u]: TAG MISMATCH at %p size %zu tag 0x%02x\n",
					w->id, live[k].ptr, live[k].size, live[k].tag);
			}
			bytes -= live[k].size;
			free(live[k].ptr);
			w->frees++;
			live[k] = live[nlive - 1u];
			nlive--;
		}
		else if ((op < 45u) && (nlive > 0u)) {
			unsigned int k = (unsigned int)((r >> 8) % nlive);
			size_t ns = pickSize(&w->rng);
			void *np;

			if (checkTag(&live[k]) == 0) {
				w->mismatches++;
				printf("mtstress[%u]: TAG MISMATCH (pre-realloc) at %p size %zu\n",
					w->id, live[k].ptr, live[k].size);
			}
			np = realloc(live[k].ptr, ns);
			if (np == NULL) {
				w->oom++;
				continue;
			}
			bytes += ns - live[k].size;
			live[k].ptr = np;
			live[k].size = ns;
			live[k].tag = (uint8_t)(r >> 16);
			fillTag(&live[k]);
			w->reallocs++;
		}
		else {
			size_t ns = pickSize(&w->rng);
			void *p = ((r & 7u) == 0u) ? calloc(1u, ns) : malloc(ns);

			if (p == NULL) {
				w->oom++;
				continue;
			}
			live[nlive].ptr = p;
			live[nlive].size = ns;
			live[nlive].tag = (uint8_t)((r >> 16) | 1u);
			fillTag(&live[nlive]);
			nlive++;
			bytes += ns;
			w->allocs++;
		}

		if (bytes > w->bytesPeak) {
			w->bytesPeak = bytes;
		}
	}

	while (nlive > 0u) {
		nlive--;
		if (checkTag(&live[nlive]) == 0) {
			w->mismatches++;
			printf("mtstress[%u]: TAG MISMATCH (drain) at %p size %zu\n",
				w->id, live[nlive].ptr, live[nlive].size);
		}
		free(live[nlive].ptr);
		w->frees++;
	}
	return NULL;
}


int main(int argc, char **argv)
{
	unsigned int nthreads = (argc > 1) ? (unsigned int)strtoul(argv[1], NULL, 0) : 4u;
	unsigned long ops = (argc > 2) ? strtoul(argv[2], NULL, 0) : 20000ul;
	pthread_t *tid;
	worker_t *w;
	unsigned int i;
	unsigned long allocs = 0, frees = 0, reallocs = 0, mism = 0, oom = 0, peak = 0;
	time_t t0, t1;

	if ((nthreads == 0u) || (nthreads > 32u)) {
		printf("mtstress: threads must be 1..32\n");
		return 2;
	}

	tid = calloc(nthreads, sizeof(*tid));
	w = calloc(nthreads, sizeof(*w));
	if ((tid == NULL) || (w == NULL)) {
		printf("mtstress: out of memory setting up\n");
		return 2;
	}

	printf("mtstress: %u thread(s) x %lu ops, live<=%u blocks each\n",
		nthreads, ops, LIVE_MAX);
	t0 = time(NULL);

	for (i = 0; i < nthreads; i++) {
		w[i].id = i;
		w[i].ops = ops;
		w[i].rng = 0x9e3779b97f4a7c15ULL ^ ((uint64_t)(i + 1u) * 0x1000193ULL)
			^ (uint64_t)t0;
		if (pthread_create(&tid[i], NULL, worker, &w[i]) != 0) {
			printf("mtstress: pthread_create(%u) failed: %s\n", i, strerror(errno));
			return 2;
		}
	}
	for (i = 0; i < nthreads; i++) {
		pthread_join(tid[i], NULL);
		allocs += w[i].allocs;
		frees += w[i].frees;
		reallocs += w[i].reallocs;
		mism += w[i].mismatches;
		oom += w[i].oom;
		if (w[i].bytesPeak > peak) {
			peak = w[i].bytesPeak;
		}
	}
	t1 = time(NULL);

	printf("mtstress: allocs=%lu frees=%lu reallocs=%lu oom=%lu peak-live=%lu B "
		"threads=%u secs=%ld\n", allocs, frees, reallocs, oom, peak, nthreads,
		(long)(t1 - t0));
	if (mism != 0u) {
		printf("mtstress: FAIL — %lu tag mismatch(es): a live block's own bytes changed\n", mism);
		return 1;
	}
	printf("mtstress: PASS — no tag mismatch. ⚠ this says nothing about the "
		"allocator's own headers: read the log for malloc: lines (why=N).\n");
	return 0;
}
