/* SPDX-License-Identifier: Zlib
 *
 * nfs-read-bench.c — sequential read throughput micro-benchmark for the
 * Phoenix-RTOS Raspberry Pi 4 port.
 *
 * Opens a file (typically on the netboot NFS root), reads it sequentially in
 * 256 KiB chunks (discarding the data), times the read with CLOCK_MONOTONIC
 * (the EL0 fast counter, enabled by the CNTKCTL fix), and prints MiB/s to
 * stdout (psh wires a program's stdout to the console; stderr is not). Use it
 * to compare Phoenix NFS throughput against the Linux-Pi4 reference
 * (nfsbench ~11.4 MiB/s on NFSv3) and to measure before/after a poll()/socket
 * kernel change.
 *
 *   nfs-read-bench <path> [chunk_kib]
 *
 * Standalone static aarch64-phoenix ELF; links libphoenix only.
 *
 * Copyright 2026 Phoenix Systems
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>

/* read() mode: sequential read() throughput (bulk streaming path). */
static int mode_read(const char *path, size_t chunk)
{
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("nfs-bench: open('%s') failed: %s\n", path, strerror(errno));
		return 2;
	}
	char *buf = malloc(chunk);
	if (buf == NULL) {
		printf("nfs-bench: OOM (chunk=%zu)\n", chunk);
		close(fd);
		return 3;
	}
	struct timespec t0, t1;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	unsigned long long total = 0, nreads = 0;
	for (;;) {
		ssize_t n = read(fd, buf, chunk);
		if (n < 0) { printf("nfs-bench: read error after %llu bytes: %s\n", total, strerror(errno)); break; }
		if (n == 0) break;
		total += (unsigned long long)n;
		nreads++;
	}
	clock_gettime(CLOCK_MONOTONIC, &t1);
	close(fd);
	free(buf);
	double secs = (double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;
	double mib = (double)total / (1024.0 * 1024.0);
	printf("nfs-bench: mode=read path=%s chunk=%zuKiB reads=%llu bytes=%llu (%.2f MiB) time=%.3f s throughput=%.2f MiB/s\n",
	       path, chunk / 1024u, nreads, total, mib, secs, (secs > 0.0) ? (mib / secs) : 0.0);
	return 0;
}

/* mmap mode: mmap the file MAP_PRIVATE and touch one byte per page sequentially,
 * forcing demand-paging faults — the SAME vm/object.c object_fetch path the ELF
 * loader/exec uses. This isolates the per-page-fault cost (and whether read-ahead
 * clustering engages) from the bulk read() streaming path. A large gap read-vs-mmap
 * = the game-load (demand-paging) bottleneck. */
static int mode_mmap(const char *path)
{
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("nfs-bench: open('%s') failed: %s\n", path, strerror(errno));
		return 2;
	}
	struct stat st;
	if (fstat(fd, &st) < 0) { printf("nfs-bench: fstat failed: %s\n", strerror(errno)); close(fd); return 2; }
	size_t sz = (size_t)st.st_size;
	long pg = sysconf(_SC_PAGESIZE);
	if (pg <= 0) pg = 4096;

	/* Time mmap() INCLUSIVELY: Phoenix's userspace file-backed mmap populates
	 * EAGERLY at map time (measured 2026-08-08 — the post-mmap touch loop was
	 * 0.000s, i.e. pages already resident), so the I/O cost is in mmap() itself,
	 * not the fault loop. NOTE this is therefore NOT a faithful probe of the KERNEL
	 * exec/ELF-loader demand-paging path (vm/object.c object_fetch on fault) — it
	 * measures userspace mmap population, which may use a different (bulk) path. */
	struct timespec t0, t1;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	void *m = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fd, 0);
	if (m == MAP_FAILED) {
		printf("nfs-bench: mmap(%zu) failed: %s (file-backed mmap may be unsupported)\n", sz, strerror(errno));
		close(fd);
		return 5;
	}
	volatile unsigned long acc = 0;
	unsigned long long npages = 0;
	for (size_t off = 0; off < sz; off += (size_t)pg) {
		acc += ((volatile unsigned char *)m)[off]; /* fault the page in */
		npages++;
	}
	clock_gettime(CLOCK_MONOTONIC, &t1);
	(void)acc;
	munmap(m, sz);
	close(fd);
	double secs = (double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;
	double mib = (double)sz / (1024.0 * 1024.0);
	printf("nfs-bench: mode=mmap path=%s pages=%llu (%ldB) bytes=%zu (%.2f MiB) time=%.3f s throughput=%.2f MiB/s (%.3f ms/page)\n",
	       path, npages, pg, sz, mib, secs, (secs > 0.0) ? (mib / secs) : 0.0,
	       (npages > 0) ? (secs * 1000.0 / (double)npages) : 0.0);
	return 0;
}

/* rand mode: `nreads` reads of `chunk` bytes at pseudo-random block-aligned offsets
 * within the file, timed. This mimics a game's asset-load access pattern — many small
 * SCATTERED reads within one open pak file — which is the LATENCY-bound path: every
 * non-sequential read is a fresh NFS READ RPC (~1 round-trip), not coalesced by
 * read-ahead. A large NFS-vs-RAM(/tmp) gap here, while bulk `read` throughput is
 * comparable, is direct evidence that game-load time is dominated by per-read RPC
 * latency (not the ~cable-capped bulk bandwidth), i.e. the owner's RAM-staging /
 * pre-download workaround would help. Deterministic LCG so NFS and RAM runs are
 * comparable (no rand()/time seeding). */
static int mode_rand(const char *path, size_t chunk, unsigned long nreads)
{
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("nfs-bench: open('%s') failed: %s\n", path, strerror(errno));
		return 2;
	}
	struct stat st;
	if (fstat(fd, &st) < 0) {
		printf("nfs-bench: fstat failed: %s\n", strerror(errno));
		close(fd);
		return 2;
	}
	size_t sz = (size_t)st.st_size;
	if (sz < chunk) {
		printf("nfs-bench: file (%zu B) smaller than chunk (%zu B)\n", sz, chunk);
		close(fd);
		return 2;
	}
	char *buf = malloc(chunk);
	if (buf == NULL) {
		printf("nfs-bench: OOM (chunk=%zu)\n", chunk);
		close(fd);
		return 3;
	}
	size_t nblocks = sz / chunk;
	unsigned long lcg = 0x12345678u;
	struct timespec t0, t1;
	unsigned long long total = 0;
	unsigned long ok = 0;
	unsigned long i;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (i = 0; i < nreads; i++) {
		size_t blk;
		off_t off;
		ssize_t n;
		lcg = lcg * 1103515245u + 12345u;
		blk = (size_t)((lcg >> 8) % nblocks);
		off = (off_t)(blk * chunk);
		if (lseek(fd, off, SEEK_SET) != off) {
			printf("nfs-bench: lseek failed: %s\n", strerror(errno));
			break;
		}
		n = read(fd, buf, chunk);
		if (n < 0) {
			printf("nfs-bench: read error: %s\n", strerror(errno));
			break;
		}
		if (n == 0) {
			continue;
		}
		total += (unsigned long long)n;
		ok++;
	}
	clock_gettime(CLOCK_MONOTONIC, &t1);
	close(fd);
	free(buf);
	double secs = (double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;
	printf("nfs-bench: mode=rand path=%s chunk=%zuB reads=%lu bytes=%llu time=%.3f s -> %.1f reads/s, %.3f ms/read, %.2f MiB/s\n",
	       path, chunk, ok, total, secs,
	       (secs > 0.0) ? (double)ok / secs : 0.0,
	       (ok > 0) ? (secs * 1000.0 / (double)ok) : 0.0,
	       (secs > 0.0) ? ((double)total / (1024.0 * 1024.0) / secs) : 0.0);
	return 0;
}

int main(int argc, char **argv)
{
	setvbuf(stdout, NULL, _IONBF, 0);

	if (argc < 2) {
		printf("usage: nfs-read-bench <path> [read <chunk_kib> | mmap | rand <chunk_b> <nreads>]\n");
		return 1;
	}
	const char *path = argv[1];
	const char *mode = (argc >= 3) ? argv[2] : "read";

	if (strcmp(mode, "mmap") == 0)
		return mode_mmap(path);

	if (strcmp(mode, "rand") == 0) {
		size_t rchunk = (argc >= 4) ? (size_t)strtoul(argv[3], NULL, 10) : 4096u;
		unsigned long rn = (argc >= 5) ? strtoul(argv[4], NULL, 10) : 2000u;
		if (rchunk < 1u)
			rchunk = 4096u;
		return mode_rand(path, rchunk, rn);
	}

	/* read mode: optional chunk_kib as argv[2] (back-compat) or argv[3]. */
	const char *ck = NULL;
	if (strcmp(mode, "read") == 0)
		ck = (argc >= 4) ? argv[3] : NULL;
	else
		ck = argv[2]; /* back-compat: `nfs-read-bench <path> <chunk_kib>` */
	size_t chunk = ck ? (size_t)strtoul(ck, NULL, 10) * 1024u : (256u * 1024u);
	if (chunk < 4096u)
		chunk = 4096u;
	return mode_read(path, chunk);
}
