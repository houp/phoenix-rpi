/*
 * axi-pmu — Raspberry Pi 4 (BCM2711) AXI bus performance-monitor reader.
 *
 * Phoenix userspace port of the idea behind Linux's "perf: Add Raspberry Pi AXI
 * PMU driver" (Ian Rogers): reads the BCM2711 System AXI bandwidth monitors to
 * measure real hardware bus/memory transaction counts. Register map from the
 * Linux DT (bcm270x.dtsi axiperf) + vendor driver raspberrypi_axi_monitor.c.
 *
 * Verification (advisor): the counter has no external oracle, so this is
 * self-checking by construction — an IDLE baseline (control) + a monotonic
 * memcpy DOSE-RESPONSE (4/8/16 MB): read+write transactions must scale ~linearly
 * with copy size. The ratio is the oracle; the absolute bytes/transaction is
 * derived from the slope (known bytes moved / transaction delta).
 *
 * Observer only for the buses; the sole writes are to the perf block itself
 * (GEN_CTRL enable + BW0_CTRL configure) — no side effects on the monitored bus.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>

#define AXIPERF_BASE   0xfe009800u   /* BCM2711 System AXI monitor (DT 0x7e009800) */
#define AXIPERF_PAGE   0xfe009000u   /* page-aligned mmap base (MAP_PHYSMEM needs alignment) */
#define AXIPERF_OFF    0x00000800u   /* block offset within the page */
#define GEN_CTRL       0x00u
#define GEN_CTL_ENABLE (1u << 0)
#define GEN_CTL_RESET  (1u << 1)
#define GEN_CTL_WATCH  (1u << 2)
#define BW0_CTRL       0x40u
#define BW_ATRANS      0x04u
#define BW_WTRANS      0x10u
#define BW_RTRANS      0x1cu
#define BW_CTRL_RESET  (1u << 31)
#define BW_CTRL_ENABLE (1u << 30)
#define BUS_ARM_L2     11u           /* system_bus_string_2711[11] = ARM_L2 (CPU<->memory) */

static volatile uint32_t *pmu;

static uint32_t rd(uint32_t off) { return pmu[off / 4] & 0x7fffffffu; }
static void wr(uint32_t off, uint32_t v) { pmu[off / 4] = v; }

static double now_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1.0e6;
}

int main(void)
{
	void *m;
	uint32_t r0, w0, r1, w1;
	struct timespec idle = { 0, 200000000 }; /* 200 ms */
	size_t sizes[3] = { 4u << 20, 8u << 20, 16u << 20 };
	int k;

	setvbuf(stdout, NULL, _IONBF, 0);
	m = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
		MAP_DEVICE | MAP_UNCACHED | MAP_PHYSMEM | MAP_ANONYMOUS, -1, (off_t)AXIPERF_PAGE);
	if (m == MAP_FAILED) {
		printf("axi-pmu: mmap(0x%08x) FAILED\n", AXIPERF_PAGE);
		return 1;
	}
	pmu = (volatile uint32_t *)((char *)m + AXIPERF_OFF);
	printf("axi-pmu: mapped BCM2711 System AXI monitor @0x%08x; watching ARM_L2 (bus %u)\n",
		AXIPERF_BASE, BUS_ARM_L2);

	/* Vendor sequence (raspberrypi_axi_monitor.c): reset monitor, reset watcher,
	 * configure watcher (enable|bus, NO reset bit), then enable monitor WITH the
	 * WATCH bit (the piece that actually starts counting). */
	wr(GEN_CTRL, GEN_CTL_RESET);
	wr(BW0_CTRL, BW_CTRL_RESET);
	wr(BW0_CTRL, BW_CTRL_ENABLE | (BUS_ARM_L2 & 0x3fu));
	wr(GEN_CTRL, GEN_CTL_ENABLE | GEN_CTL_WATCH);
	printf("axi-pmu: GEN_CTRL=0x%08x BW0_CTRL=0x%08x\n", rd(GEN_CTRL), rd(BW0_CTRL));

	/* Control: idle baseline. */
	r0 = rd(BW0_CTRL + BW_RTRANS);
	w0 = rd(BW0_CTRL + BW_WTRANS);
	nanosleep(&idle, NULL);
	r1 = rd(BW0_CTRL + BW_RTRANS);
	w1 = rd(BW0_CTRL + BW_WTRANS);
	printf("axi-pmu: IDLE 200ms baseline: dRTRANS=%u dWTRANS=%u (expect ~small)\n",
		r1 - r0, w1 - w0);

	/* Dose-response: memcpy 4/8/16 MB x4 reps; transactions should scale linearly. */
	for (k = 0; k < 3; k++) {
		size_t sz = sizes[k];
		char *src = malloc(sz);
		char *dst = malloc(sz);
		uint32_t ra, wa, aa, rb, wb, ab;
		double t0, t1;
		int rep;

		if (src == NULL || dst == NULL) {
			printf("axi-pmu: malloc %zuMB FAILED\n", sz >> 20);
			free(src); free(dst);
			break;
		}
		memset(src, k + 1, sz); /* fault in + fill src */
		memset(dst, 0, sz);

		ra = rd(BW0_CTRL + BW_RTRANS);
		wa = rd(BW0_CTRL + BW_WTRANS);
		aa = rd(BW0_CTRL + BW_ATRANS);
		t0 = now_ms();
		for (rep = 0; rep < 4; rep++) {
			memcpy(dst, src, sz);
			src[rep] = dst[rep] + 1; /* chain deps so copies aren't coalesced/elided */
			__asm__ volatile("" ::: "memory");
		}
		__asm__ volatile("" ::: "memory"); /* pin all copies before the counter read */
		t1 = now_ms();
		rb = rd(BW0_CTRL + BW_RTRANS);
		wb = rd(BW0_CTRL + BW_WTRANS);
		ab = rd(BW0_CTRL + BW_ATRANS);
		{
			volatile uint64_t chk = 0;
			size_t z;
			for (z = 0; z < sz; z += 4096)
				chk += (unsigned char)dst[z]; /* read dst so the memcpy is not dead */
			(void)chk;
		}

		printf("axi-pmu: memcpy %2zuMB x4 (%zu bytes moved, %.2f ms): dRTRANS=%u dWTRANS=%u dATRANS=%u | ~%.1f B/RTRANS\n",
			sz >> 20, (size_t)4 * 2 * sz, t1 - t0, rb - ra, wb - wa, ab - aa,
			(rb - ra) ? (double)((size_t)4 * sz) / (double)(rb - ra) : 0.0);
		free(src);
		free(dst);
	}

	printf("axi-pmu: done (verify: idle~0, transactions scale ~linearly with copy size)\n");
	return 0;
}
