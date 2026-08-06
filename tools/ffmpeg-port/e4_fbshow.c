/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * Copyright (C) 2026 Phoenix Systems. Author: Witold Bołt.
 *
 * E4 "decode to visible screen" -- on-target player.
 *
 * Decodes one MJPEG/baseline-JPEG frame with the Phoenix ffmpeg decode-core
 * port, converts the YUV420(J) frame to the Pi 4 framebuffer's 32bpp [R,G,B,X]
 * format, and writes it CENTERED into /dev/fb0 so it appears on HDMI (the same
 * surface as the boot console). It then re-blits in a loop for ~12 s so the
 * periodic HDMI capture catches a clean frame.
 *
 * Console-overwrite handling: pl011-tty mirrors this program's own stdout onto
 * the HDMI framebuffer, so any print draws glyphs over the image. The loop is
 * therefore structured so the blit is the LAST action before each sleep, and
 * every redraw repaints the WHOLE screen (opaque black + centered image),
 * erasing any console text anywhere. The 200 ms sleep window between redraws is
 * when the periodic grab lands, and it shows a clean frame.
 *
 * LGPL glue linking LGPL ffmpeg; kept out of the BSD Phoenix core (matches the
 * tools/*-port header convention). No --enable-gpl / --enable-nonfree feature.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>

#include "e4_fb_blit.h"

#define DEFAULT_PATH "/usr/share/e4/pattern.jpg"
#define REDRAW_MS    200
/* Hold the image on screen longer than the HDMI auto-snapshot cadence (25 s)
 * so at least one periodic grab is guaranteed to land during the display
 * window; a shorter window could fall entirely between two snapshots. */
#define RUN_SECS     30

/*
 * rpi4-fb client ABI, replicated here to avoid a phoenix-rtos-devices include
 * path (same approach as SDL_phoenixvideo.c). MUST byte-for-byte match
 * sources/phoenix-rtos-devices/video/rpi4-fb/rpi4-fb.h so _IOR('g',1,...)
 * encodes the same devctl number. sizeof == 24 (4*u16 + 2*u64).
 */
typedef struct {
	uint16_t width;
	uint16_t height;
	uint16_t bpp;
	uint16_t pitch;
	uint64_t smemlen;
	uint64_t framebuffer;
} rpi4fb_mode_t;
#define RPI4FB_GETMODE _IOR('g', 1, rpi4fb_mode_t)

#define STAGE(...)              \
	do {                        \
		printf(__VA_ARGS__);    \
		printf("\n");           \
		fflush(stdout);         \
	} while (0)

/* Write one full framebuffer image to /dev/fb0 row by row, so no single write()
 * exceeds `pitch` bytes (safe regardless of the msg payload cap) and short
 * writes are retried. Returns 0 on success. */
static int fb_present(int fd, const uint8_t *buf, int height, int pitch)
{
	int y;

	for (y = 0; y < height; y++) {
		const uint8_t *row = buf + (size_t)y * pitch;
		size_t off = 0;

		if (lseek(fd, (off_t)y * pitch, SEEK_SET) < 0) {
			return -1;
		}
		while (off < (size_t)pitch) {
			ssize_t n = write(fd, row + off, (size_t)pitch - off);
			if (n <= 0) {
				if (n < 0 && errno == EINTR) {
					continue;
				}
				return -1;
			}
			off += (size_t)n;
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	const char *path = (argc > 1) ? argv[1] : DEFAULT_PATH;
	const AVCodec *dec;
	AVCodecContext *ctx = NULL;
	AVPacket *pkt = NULL;
	AVFrame *frame = NULL;
	FILE *f = NULL;
	rpi4fb_mode_t mode;
	uint8_t *fbbuf = NULL;
	int fd = -1;
	long fsize;
	size_t nread, fbbytes;
	int ret, iters, i;

	STAGE("E4FB: start (path=%s)", path);

	/* --- open /dev/fb0 and query geometry --- */
	fd = open("/dev/fb0", O_RDWR);
	if (fd < 0) {
		STAGE("E4FB: FAIL open /dev/fb0 (errno=%d)", errno);
		return 1;
	}
	if (ioctl(fd, RPI4FB_GETMODE, &mode) != 0) {
		STAGE("E4FB: FAIL RPI4FB_GETMODE (errno=%d)", errno);
		close(fd);
		return 1;
	}
	if (mode.bpp != 32 || mode.pitch == 0 || mode.framebuffer == 0) {
		STAGE("E4FB: FAIL unusable mode bpp=%u pitch=%u", mode.bpp, mode.pitch);
		close(fd);
		return 1;
	}
	STAGE("E4FB: fb0 opened %ux%u bpp=%u pitch=%u", mode.width, mode.height,
		mode.bpp, mode.pitch);

	fbbytes = (size_t)mode.pitch * mode.height;
	fbbuf = malloc(fbbytes);
	if (fbbuf == NULL) {
		STAGE("E4FB: FAIL malloc fb buffer %zu bytes", fbbytes);
		close(fd);
		return 1;
	}

	/* --- read the whole JPEG into a padded AVPacket --- */
	f = fopen(path, "rb");
	if (f == NULL) {
		STAGE("E4FB: FAIL fopen(%s)", path);
		goto fail;
	}
	if (fseek(f, 0, SEEK_END) != 0 || (fsize = ftell(f)) < 0) {
		STAGE("E4FB: FAIL fseek/ftell");
		goto fail;
	}
	rewind(f);
	pkt = av_packet_alloc();
	frame = av_frame_alloc();
	if (pkt == NULL || frame == NULL || av_new_packet(pkt, (int)fsize) < 0) {
		STAGE("E4FB: FAIL packet/frame alloc");
		goto fail;
	}
	nread = fread(pkt->data, 1, (size_t)fsize, f);
	fclose(f);
	f = NULL;
	if (nread != (size_t)fsize) {
		STAGE("E4FB: FAIL fread %zu of %ld", nread, fsize);
		goto fail;
	}

	/* --- MJPEG decode of one frame (main-stack safe; H.264 would not be) --- */
	dec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
	if (dec == NULL) {
		STAGE("E4FB: FAIL no mjpeg decoder");
		goto fail;
	}
	ctx = avcodec_alloc_context3(dec);
	if (ctx == NULL) {
		STAGE("E4FB: FAIL alloc_context3");
		goto fail;
	}
	ctx->thread_count = 1;
	if ((ret = avcodec_open2(ctx, dec, NULL)) < 0) {
		STAGE("E4FB: FAIL avcodec_open2 ret=%d", ret);
		goto fail;
	}
	if ((ret = avcodec_send_packet(ctx, pkt)) < 0) {
		STAGE("E4FB: FAIL send_packet ret=%d", ret);
		goto fail;
	}
	if ((ret = avcodec_receive_frame(ctx, frame)) < 0) {
		STAGE("E4FB: FAIL receive_frame ret=%d", ret);
		goto fail;
	}
	STAGE("E4FB: decoded %dx%d fmt=%d", frame->width, frame->height, frame->format);

	if (frame->format != AV_PIX_FMT_YUVJ420P && frame->format != AV_PIX_FMT_YUV420P) {
		/* Convert core assumes planar 4:2:0. Baseline JPEG is always one of
		 * these two; bail loudly rather than draw garbage for anything else. */
		STAGE("E4FB: FAIL unsupported pixfmt %d (need YUV(J)420P)", frame->format);
		goto fail;
	}

	/* --- compose the full framebuffer image once (black bg + centered) --- */
	e4_yuv420_to_fb(fbbuf, mode.width, mode.height, mode.pitch,
		frame->data[0], frame->linesize[0],
		frame->data[1], frame->linesize[1],
		frame->data[2], frame->linesize[2],
		frame->width, frame->height);
	STAGE("E4FB: composed, blitting for ~%ds (redraw every %dms)", RUN_SECS, REDRAW_MS);

	/* --- present in a loop; blit is the LAST action before each sleep so the
	 * grab window shows a clean, fully-repainted frame --- */
	iters = (RUN_SECS * 1000) / REDRAW_MS;
	for (i = 0; i < iters; i++) {
		/* Progress marker first; the blit right after repaints over these
		 * glyphs, so the sleep window stays clean. */
		if ((i % 5) == 0) {
			STAGE("E4FB: redraw %d/%d", i, iters);
		}
		if (fb_present(fd, fbbuf, mode.height, mode.pitch) != 0) {
			STAGE("E4FB: FAIL fb write at redraw %d (errno=%d)", i, errno);
			goto fail;
		}
		usleep(REDRAW_MS * 1000);
	}

	STAGE("E4FB: DONE ok");

	free(fbbuf);
	av_frame_free(&frame);
	av_packet_free(&pkt);
	avcodec_free_context(&ctx);
	close(fd);
	return 0;

fail:
	if (f != NULL) {
		fclose(f);
	}
	free(fbbuf);
	av_frame_free(&frame);
	av_packet_free(&pkt);
	avcodec_free_context(&ctx);
	if (fd >= 0) {
		close(fd);
	}
	return 1;
}
