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

int main(int argc, char **argv)
{
	setvbuf(stdout, NULL, _IONBF, 0);

	if (argc < 2) {
		printf("usage: nfs-read-bench <path> [chunk_kib]\n");
		return 1;
	}
	const char *path = argv[1];
	size_t chunk = (argc >= 3) ? (size_t)strtoul(argv[2], NULL, 10) * 1024u : (256u * 1024u);
	if (chunk < 4096u)
		chunk = 4096u;

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

	unsigned long long total = 0;
	unsigned long long nreads = 0;
	for (;;) {
		ssize_t n = read(fd, buf, chunk);
		if (n < 0) {
			printf("nfs-bench: read error after %llu bytes: %s\n", total, strerror(errno));
			break;
		}
		if (n == 0)
			break; /* EOF */
		total += (unsigned long long)n;
		nreads++;
	}

	clock_gettime(CLOCK_MONOTONIC, &t1);
	close(fd);
	free(buf);

	double secs = (double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;
	double mib = (double)total / (1024.0 * 1024.0);
	printf("nfs-bench: path=%s chunk=%zuKiB reads=%llu bytes=%llu (%.2f MiB) time=%.3f s throughput=%.2f MiB/s\n",
	       path, chunk / 1024u, nreads, total, mib, secs, (secs > 0.0) ? (mib / secs) : 0.0);
	return 0;
}
