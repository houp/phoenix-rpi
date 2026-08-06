/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * Copyright (C) 2026 Phoenix Systems. Author: Witold Bolt.
 *
 * E4 finale: a MOVING H.264 video player on Phoenix-RTOS.
 *
 * Decodes a multi-frame raw Annex-B H.264 clip with the Phoenix ffmpeg
 * decode-core port and displays each decoded frame in sequence, centered, on the
 * live firmware HDMI framebuffer (/dev/fb0), with wall-clock pacing -- so real
 * video plays on the Phoenix screen. It wires together three already
 * HW-validated building blocks:
 *   - H.264 decode on an 8 MB-stack pthread  (e4_decode_h264.c: DPB/deblock
 *     overflow the small main-thread stack; a big dedicated stack fixes it)
 *   - YUV420 -> 32bpp [R,G,B,X] convert + centered blit  (e4_fb_blit.h)
 *   - full-framebuffer present to /dev/fb0                (e4_fbshow.c fb_present)
 *
 * Play model: raw Annex-B carries no seekable index, so the clip is LOOPED by
 * reinit-per-pass -- reopen the file with a fresh parser + decoder each pass.
 * The clip is tiny, so reopen is free, and a fresh decoder is far more robust
 * than seeking a headerless elementary stream. The `displayed frame N` counter
 * is CUMULATIVE across passes: a climbing counter is the primary evidence that
 * frames are advancing (motion), and the solid color under the screen changing
 * between HDMI snapshots (taken ~25 s apart) is the visible motion.
 *
 * Console-overwrite handling (as in e4_fbshow.c): pl011-tty mirrors this
 * program's stdout onto the same HDMI surface, so any print draws glyphs over
 * the image. Each frame's progress marker is printed BEFORE that frame's blit,
 * and the blit repaints the WHOLE screen (opaque black + centered image), so the
 * pacing usleep window that follows always shows a clean frame.
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
#include <time.h>
#include <pthread.h>
#include <sys/ioctl.h>

#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/error.h>
#include <libavutil/pixfmt.h>

#include "e4_fb_blit.h"

#define DEFAULT_PATH "/usr/share/e4/clip.h264"
#define DEFAULT_FPS  12          /* clip authored at 12 fps; raw Annex-B has no rate */
#define RUN_SECS     40          /* total wall-clock play time, then exit cleanly */

/* H.264's DPB/deblock/deep chains overflow the small main-thread stack. */
#define DECODE_STACK_SIZE (8 * 1024 * 1024)

/* Read the input in fixed chunks; the parser is fed a sliding window. The
 * AV_INPUT_BUFFER_PADDING_SIZE tail must be present and zeroed. */
#define CHUNK (4 * 1024)

#define STAGE(...)              \
	do {                        \
		printf(__VA_ARGS__);    \
		printf("\n");           \
		fflush(stdout);         \
	} while (0)

/*
 * rpi4-fb client ABI, replicated to avoid a phoenix-rtos-devices include path
 * (same approach as e4_fbshow.c / SDL_phoenixvideo.c). MUST byte-for-byte match
 * sources/phoenix-rtos-devices/video/rpi4-fb/rpi4-fb.h. sizeof == 24.
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

/* Shared play state carried on the decode thread's stack via its arg. */
struct play_ctx {
	const char *path;
	int fps;
	int fd;                 /* /dev/fb0 */
	rpi4fb_mode_t mode;
	uint8_t *fbbuf;         /* one full-screen 32bpp compose buffer */
	long usec_per_frame;    /* pacing interval */
	long long displayed;    /* CUMULATIVE displayed-frame counter (motion evidence) */
	struct timespec t0;     /* play start */
	int rc;
};

static long long elapsed_ms(const struct timespec *t0)
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (long long)(now.tv_sec - t0->tv_sec) * 1000
		+ (now.tv_nsec - t0->tv_nsec) / 1000000;
}

/* Write one full framebuffer image to /dev/fb0 row by row (no single write()
 * exceeds `pitch` bytes; short writes retried). Returns 0 on success. */
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

/* Convert + center-blit one decoded frame and present it, then pace. Advances
 * the cumulative counter and prints a marker every 5 frames (before the blit,
 * so the blit repaints over the glyphs). Returns 0 on success. */
static int show_frame(struct play_ctx *pc, const AVFrame *fr)
{
	if (fr->format != AV_PIX_FMT_YUV420P && fr->format != AV_PIX_FMT_YUVJ420P) {
		STAGE("E4PLAY: FAIL unsupported pixfmt %d (need YUV(J)420P)", fr->format);
		return -1;
	}

	if ((pc->displayed % 5) == 0) {
		STAGE("E4PLAY: displayed frame %lld", pc->displayed);
	}

	e4_yuv420_to_fb(pc->fbbuf, pc->mode.width, pc->mode.height, pc->mode.pitch,
		fr->data[0], fr->linesize[0],
		fr->data[1], fr->linesize[1],
		fr->data[2], fr->linesize[2],
		fr->width, fr->height);
	if (fb_present(pc->fd, pc->fbbuf, pc->mode.height, pc->mode.pitch) != 0) {
		STAGE("E4PLAY: FAIL fb write (errno=%d)", errno);
		return -1;
	}
	pc->displayed++;
	usleep((useconds_t)pc->usec_per_frame);
	return 0;
}

/* Send one parsed access unit (or NULL to flush) and present every frame the
 * decoder emits. Returns >=0 frames shown this call, -1 on hard error. */
static int decode_show(struct play_ctx *pc, AVCodecContext *ctx, AVPacket *pkt,
	AVFrame *frame)
{
	int shown = 0, ret;

	ret = avcodec_send_packet(ctx, pkt);
	if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
		STAGE("E4PLAY: FAIL send_packet ret=%d", ret);
		return -1;
	}

	for (;;) {
		ret = avcodec_receive_frame(ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return shown;                 /* need more input / drained */
		}
		if (ret < 0) {
			STAGE("E4PLAY: FAIL receive_frame ret=%d", ret);
			return -1;
		}
		if (show_frame(pc, frame) != 0) {
			av_frame_unref(frame);
			return -1;
		}
		av_frame_unref(frame);
		shown++;
	}
}

/* Decode + display the whole clip once (one pass). Fresh parser + decoder so a
 * headerless raw stream restarts cleanly. Returns frames shown, -1 on error. */
static int play_pass(struct play_ctx *pc)
{
	const AVCodec *dec;
	AVCodecContext *ctx = NULL;
	AVCodecParserContext *parser = NULL;
	AVPacket *pkt = NULL;
	AVFrame *frame = NULL;
	FILE *f = NULL;
	unsigned char inbuf[CHUNK + AV_INPUT_BUFFER_PADDING_SIZE];
	int shown = 0, rc = -1, r;

	f = fopen(pc->path, "rb");
	if (f == NULL) {
		STAGE("E4PLAY: FAIL fopen(%s)", pc->path);
		return -1;
	}
	dec = avcodec_find_decoder(AV_CODEC_ID_H264);
	parser = av_parser_init(AV_CODEC_ID_H264);
	pkt = av_packet_alloc();
	frame = av_frame_alloc();
	if (dec == NULL || parser == NULL || pkt == NULL || frame == NULL) {
		STAGE("E4PLAY: FAIL decoder/parser/alloc");
		goto out;
	}
	ctx = avcodec_alloc_context3(dec);
	if (ctx == NULL) {
		STAGE("E4PLAY: FAIL alloc_context3");
		goto out;
	}
	ctx->thread_count = 1;    /* ffmpeg's own threading off; we own the big stack */
	if (avcodec_open2(ctx, dec, NULL) < 0) {
		STAGE("E4PLAY: FAIL avcodec_open2");
		goto out;
	}

	memset(inbuf + CHUNK, 0, AV_INPUT_BUFFER_PADDING_SIZE);
	for (;;) {
		size_t n = fread(inbuf, 1, CHUNK, f);
		const unsigned char *data = inbuf;
		size_t remaining = n;

		if (n == 0) {
			break;
		}
		while (remaining > 0) {
			unsigned char *au = NULL;
			int au_size = 0;
			int used = av_parser_parse2(parser, ctx, &au, &au_size,
				data, (int)remaining, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
			if (used < 0) {
				STAGE("E4PLAY: FAIL av_parser_parse2 ret=%d", used);
				goto out;
			}
			data += used;
			remaining -= (size_t)used;
			if (au_size > 0) {
				pkt->data = au;
				pkt->size = au_size;
				r = decode_show(pc, ctx, pkt, frame);
				if (r < 0) {
					goto out;
				}
				shown += r;
			}
		}
	}
	/* flush the parser's trailing AU, then flush the decoder */
	{
		unsigned char *au = NULL;
		int au_size = 0;
		av_parser_parse2(parser, ctx, &au, &au_size, NULL, 0,
			AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		if (au_size > 0) {
			pkt->data = au;
			pkt->size = au_size;
			r = decode_show(pc, ctx, pkt, frame);
			if (r < 0) {
				goto out;
			}
			shown += r;
		}
	}
	pkt->data = NULL;
	pkt->size = 0;
	r = decode_show(pc, ctx, pkt, frame);
	if (r < 0) {
		goto out;
	}
	shown += r;
	rc = shown;

out:
	if (f != NULL) {
		fclose(f);
	}
	if (parser != NULL) {
		av_parser_close(parser);
	}
	av_frame_free(&frame);
	av_packet_free(&pkt);
	avcodec_free_context(&ctx);
	return rc;
}

/* Full player: open /dev/fb0, then loop the clip until RUN_SECS elapse. Runs on
 * the 8 MB decode thread. Returns 0 on success. */
static int play_body(struct play_ctx *pc)
{
	size_t fbbytes;
	int pass = 0;

	STAGE("E4PLAY: start (path=%s fps=%d run=%ds)", pc->path, pc->fps, RUN_SECS);

	pc->fd = open("/dev/fb0", O_RDWR);
	if (pc->fd < 0) {
		STAGE("E4PLAY: FAIL open /dev/fb0 (errno=%d)", errno);
		return 1;
	}
	if (ioctl(pc->fd, RPI4FB_GETMODE, &pc->mode) != 0) {
		STAGE("E4PLAY: FAIL RPI4FB_GETMODE (errno=%d)", errno);
		close(pc->fd);
		return 1;
	}
	if (pc->mode.bpp != 32 || pc->mode.pitch == 0 || pc->mode.framebuffer == 0) {
		STAGE("E4PLAY: FAIL unusable mode bpp=%u pitch=%u", pc->mode.bpp, pc->mode.pitch);
		close(pc->fd);
		return 1;
	}
	STAGE("E4PLAY: fb0 opened %ux%u bpp=%u pitch=%u", pc->mode.width,
		pc->mode.height, pc->mode.bpp, pc->mode.pitch);

	fbbytes = (size_t)pc->mode.pitch * pc->mode.height;
	pc->fbbuf = malloc(fbbytes);
	if (pc->fbbuf == NULL) {
		STAGE("E4PLAY: FAIL malloc fb buffer %zu bytes", fbbytes);
		close(pc->fd);
		return 1;
	}

	pc->usec_per_frame = 1000000L / (pc->fps > 0 ? pc->fps : DEFAULT_FPS);
	pc->displayed = 0;
	clock_gettime(CLOCK_MONOTONIC, &pc->t0);

	/* Loop the clip until the wall-clock budget is spent. Finishing the current
	 * pass past the boundary is fine; the cumulative counter keeps climbing. */
	for (;;) {
		int shown;

		if (elapsed_ms(&pc->t0) >= (long long)RUN_SECS * 1000) {
			break;
		}
		shown = play_pass(pc);
		if (shown < 0) {
			free(pc->fbbuf);
			close(pc->fd);
			return 1;
		}
		pass++;
		STAGE("E4PLAY: pass %d done (%d frames, cumulative %lld)",
			pass, shown, pc->displayed);
		if (shown == 0) {
			/* A pass that decoded nothing would spin -- bail loudly. */
			STAGE("E4PLAY: FAIL pass decoded 0 frames");
			free(pc->fbbuf);
			close(pc->fd);
			return 1;
		}
	}

	STAGE("E4PLAY: DONE ok (%d passes, %lld frames displayed)", pass, pc->displayed);
	free(pc->fbbuf);
	close(pc->fd);
	return 0;
}

static void *play_thread(void *arg)
{
	struct play_ctx *pc = (struct play_ctx *)arg;
	pc->rc = play_body(pc);
	return NULL;
}

int main(int argc, char **argv)
{
	struct play_ctx pc;
	pthread_attr_t attr;
	pthread_t tid;
	int err;

	memset(&pc, 0, sizeof(pc));
	pc.path = (argc > 1) ? argv[1] : DEFAULT_PATH;
	pc.fps = (argc > 2) ? atoi(argv[2]) : DEFAULT_FPS;
	pc.fd = -1;
	pc.rc = 1;

	STAGE("E4PLAY: launcher (spawning 8 MB decode thread)");

	/* Whole player runs on a dedicated 8 MB-stack thread: H.264 DPB/deblock
	 * overflow the small main-thread stack (proven in e4_decode_h264.c). */
	err = pthread_attr_init(&attr);
	if (err == 0) {
		err = pthread_attr_setstacksize(&attr, DECODE_STACK_SIZE);
	}
	if (err == 0) {
		err = pthread_create(&tid, &attr, play_thread, &pc);
	}

	if (err != 0) {
		STAGE("E4PLAY: WARN pthread_create failed (err=%d) - MAIN thread, big-stack NOT exercised", err);
		pc.rc = play_body(&pc);
	}
	else {
		pthread_join(tid, NULL);
	}

	pthread_attr_destroy(&attr);
	return pc.rc;
}
