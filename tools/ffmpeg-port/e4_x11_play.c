/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * Copyright (C) 2026 Phoenix Systems. Author: Witold Bolt.
 *
 * E4 windowed video player: a MOVING H.264 video player in an X11 window on
 * Phoenix-RTOS (Raspberry Pi 4, Xphoenix server).
 *
 * This is the windowed sibling of e4_play.c. It reuses that program's proven
 * decode machinery VERBATIM -- the 8 MB-stack decode pthread, the reinit-per-pass
 * clip loop (raw Annex-B has no seekable index), av_parser_parse2 +
 * avcodec_send_packet/receive_frame, the YUV(J)420P guard, and the wall-clock
 * pacing -- but REPLACES the fullscreen /dev/fb0 present path with an ordinary
 * libX11 client (the same present pattern proven in tools/x11-port/gl_x11_window.c):
 * open the Xphoenix display, create a WM-decorated window, and blit each decoded
 * frame with XPutImage under the Xphoenix server.
 *
 * Lazy window: raw H.264 carries no stream header, so the frame dimensions are
 * unknown until the first frame decodes. The X window + XImage are therefore
 * created lazily on the first decoded frame, sized to fr->width x fr->height, with
 * WM size-hints (USPosition|USSize) so twm decorates + places the window at a fixed
 * offset immediately (no interactive rubber-band placement, which is useless for an
 * automated HDMI grab). The window is fixed to the video size (min==max).
 *
 * Per frame: convert YUV420 -> a frame-sized 32bpp [R,G,B,X] buffer with
 * e4_yuv420_to_fb (surface == image == fr->width x fr->height, pitch = w*4, so
 * x0=y0=0 and the whole buffer is filled with no centering), then repack that into
 * the XImage honouring the window visual's channel masks -- the same mask
 * shift/width pack as gl_x11_window.c. Note e4_yuv420_to_fb writes byte order
 * [R,G,B,X] (identical layout to glReadPixels GL_RGBA), so the pack reads
 * buf[i*4+0]=R,+1=G,+2=B verbatim; there is NO vertical flip here (e4's blit is
 * top-left origin, same as X, unlike glReadPixels' bottom-left origin).
 *
 * Whole player runs on the 8 MB decode thread (single-threaded X, no XInitThreads);
 * the cumulative `displayed frame N` counter climbing is the primary evidence of
 * motion, and the window contents changing between periodic HDMI snapshots (~25 s
 * apart) is the visible motion.
 *
 * LGPL glue linking LGPL ffmpeg; kept out of the BSD Phoenix core (matches the
 * tools/*-port header convention). No --enable-gpl / --enable-nonfree feature.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/error.h>
#include <libavutil/pixfmt.h>

#include "e4_fb_blit.h"

/* X11 client headers LAST so nothing shadows Xlib's tokens. */
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define DEFAULT_PATH "/usr/share/e4/clip.h264"
#define DEFAULT_FPS  12          /* clip authored at 12 fps; raw Annex-B has no rate */
#define RUN_SECS     300         /* long play window so the periodic (~25s) HDMI
                                  * snapshots reliably land on the live video window
                                  * (the 40s default fell between snapshots) */

/* Fixed window placement offset (not 0,0), so it's clearly a windowed, WM-decorated
 * client rather than a fullscreen root paint. */
#define WIN_X 200
#define WIN_Y 120

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

/* Trailing-zero count of a channel mask -> the shift needed to place an 8-bit
 * channel value into that mask's low bit (same helper as gl_x11_window.c). */
static int mask_shift(unsigned long mask)
{
	if (mask == 0) {
		return 0;
	}
	return __builtin_ctzl(mask);
}

/* Number of set bits in a channel mask (channel width, typ. 8). */
static int mask_width(unsigned long mask)
{
	int w = 0;
	while (mask) {
		w += (int)(mask & 1ul);
		mask >>= 1;
	}
	return w;
}

/* Shared play state carried on the decode thread's stack via its arg. */
struct play_ctx {
	const char *path;
	int fps;
	long usec_per_frame;    /* pacing interval */
	long long displayed;    /* CUMULATIVE displayed-frame counter (motion evidence) */
	struct timespec t0;     /* play start */
	int rc;

	/* X11 client state (window + image created lazily on the first frame). */
	Display *dpy;
	Window win;
	GC gc;
	XImage *img;
	int created;            /* window+image built (dimensions known)? */
	int w, h;               /* video/window dimensions (== first frame) */
	uint32_t *ximg_data;    /* XImage backing, visual-masked 32bpp words */
	uint8_t *blitbuf;       /* e4 [R,G,B,X] convert target, w*h*4 */
	/* window visual channel masks + derived shift/width */
	unsigned long rmask, gmask, bmask;
	int rsh, gsh, bsh, rwd, gwd, bwd;
};

static long long elapsed_ms(const struct timespec *t0)
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (long long)(now.tv_sec - t0->tv_sec) * 1000
		+ (now.tv_nsec - t0->tv_nsec) / 1000000;
}

/* Create the X window + XImage + scratch buffers once the video dimensions are
 * known (first decoded frame). Returns 0 on success. */
static int ensure_window(struct play_ctx *pc, int w, int h)
{
	int screen;
	Window root;
	XSizeHints *hints;
	XWindowAttributes wa;
	Visual *vis;
	int depth;

	pc->w = w;
	pc->h = h;

	screen = DefaultScreen(pc->dpy);
	root = RootWindow(pc->dpy, screen);
	pc->win = XCreateSimpleWindow(pc->dpy, root, WIN_X, WIN_Y, w, h, 0,
		BlackPixel(pc->dpy, screen), BlackPixel(pc->dpy, screen));
	XStoreName(pc->dpy, pc->win, "Phoenix ffmpeg video");

	/* WM size hints with USPosition|USSize: twm honours the user-specified
	 * position and decorates + places the window immediately (no interactive
	 * rubber-band placement). Fixed size (min==max): the video is w x h. */
	hints = XAllocSizeHints();
	if (hints != NULL) {
		hints->flags = USPosition | USSize | PPosition | PSize | PMinSize | PMaxSize;
		hints->x = WIN_X;
		hints->y = WIN_Y;
		hints->width = w;
		hints->height = h;
		hints->min_width = hints->max_width = w;
		hints->min_height = hints->max_height = h;
		XSetWMNormalHints(pc->dpy, pc->win, hints);
		XFree(hints);
	}
	XSelectInput(pc->dpy, pc->win, ExposureMask);
	XMapWindow(pc->dpy, pc->win);
	pc->gc = XCreateGC(pc->dpy, pc->win, 0, NULL);

	/* Query the window's actual visual + depth so the XImage packing matches the
	 * server's pixel format. */
	XGetWindowAttributes(pc->dpy, pc->win, &wa);
	vis = wa.visual;
	depth = wa.depth;
	pc->rmask = vis->red_mask;
	pc->gmask = vis->green_mask;
	pc->bmask = vis->blue_mask;
	pc->rsh = mask_shift(pc->rmask);
	pc->gsh = mask_shift(pc->gmask);
	pc->bsh = mask_shift(pc->bmask);
	pc->rwd = mask_width(pc->rmask);
	pc->gwd = mask_width(pc->gmask);
	pc->bwd = mask_width(pc->bmask);
	STAGE("E4X11: window %dx%d visual depth=%d masks r=0x%lx g=0x%lx b=0x%lx byte_order=%s",
		w, h, depth, pc->rmask, pc->gmask, pc->bmask,
		ImageByteOrder(pc->dpy) == LSBFirst ? "LSBFirst" : "MSBFirst");

	/* e4 [R,G,B,X] convert target (w*h*4). */
	pc->blitbuf = malloc((size_t)w * h * 4);
	if (pc->blitbuf == NULL) {
		STAGE("E4X11: FAIL OOM blitbuf");
		return -1;
	}

	/* 32-bpp client-side XImage buffer (w*h*4). */
	pc->ximg_data = malloc((size_t)w * h * 4);
	if (pc->ximg_data == NULL) {
		STAGE("E4X11: FAIL OOM ximg");
		return -1;
	}
	pc->img = XCreateImage(pc->dpy, vis, depth, ZPixmap, 0,
		(char *)pc->ximg_data, w, h, 32, 0);
	if (pc->img == NULL) {
		STAGE("E4X11: FAIL XCreateImage");
		return -1;
	}
	/* We fill the buffer as native-endian 32-bit words below; declare the image's
	 * byte order to match the host so XPutImage does not byte-swap. Xphoenix and
	 * the aarch64 client are both little-endian. */
	pc->img->byte_order = LSBFirst;

	pc->created = 1;
	return 0;
}

/* Convert one decoded frame to [R,G,B,X], repack into the XImage honouring the
 * visual masks, present it, then pace. Advances the cumulative counter and prints a
 * marker every 5 frames. Returns 0 on success. */
static int show_frame(struct play_ctx *pc, const AVFrame *fr)
{
	int y, x;

	if (fr->format != AV_PIX_FMT_YUV420P && fr->format != AV_PIX_FMT_YUVJ420P) {
		STAGE("E4X11: FAIL unsupported pixfmt %d (need YUV(J)420P)", fr->format);
		return -1;
	}

	/* Lazily build the window sized to the first frame's dimensions. */
	if (!pc->created) {
		if (ensure_window(pc, fr->width, fr->height) != 0) {
			return -1;
		}
	}
	if (fr->width != pc->w || fr->height != pc->h) {
		STAGE("E4X11: FAIL frame %dx%d != window %dx%d", fr->width, fr->height, pc->w, pc->h);
		return -1;
	}

	if ((pc->displayed % 5) == 0) {
		STAGE("E4X11: displayed frame %lld", pc->displayed);
	}

	/* Whole-frame blit into a w*h*4 [R,G,B,X] buffer: surface == image size and
	 * pitch = w*4, so x0=y0=0 and there is no centering offset. */
	e4_yuv420_to_fb(pc->blitbuf, pc->w, pc->h, pc->w * 4,
		fr->data[0], fr->linesize[0],
		fr->data[1], fr->linesize[1],
		fr->data[2], fr->linesize[2],
		fr->width, fr->height);

	/* Repack [R,G,B,X] -> visual-masked XImage words. e4's blit is top-left
	 * origin (same as X): NO vertical flip. */
	for (y = 0; y < pc->h; y++) {
		const uint8_t *srow = pc->blitbuf + (size_t)y * pc->w * 4;
		uint32_t *drow = pc->ximg_data + (size_t)y * pc->w;
		for (x = 0; x < pc->w; x++) {
			unsigned r = srow[x * 4 + 0];
			unsigned g = srow[x * 4 + 1];
			unsigned b = srow[x * 4 + 2];
			unsigned long pr = ((unsigned long)r >> (8 - pc->rwd)) << pc->rsh;
			unsigned long pg = ((unsigned long)g >> (8 - pc->gwd)) << pc->gsh;
			unsigned long pb = ((unsigned long)b >> (8 - pc->bwd)) << pc->bsh;
			drow[x] = (uint32_t)((pr & pc->rmask) | (pg & pc->gmask) | (pb & pc->bmask));
		}
	}

	XPutImage(pc->dpy, pc->win, pc->gc, pc->img, 0, 0, 0, 0, pc->w, pc->h);
	XFlush(pc->dpy);

	/* Drain pending events; re-present the last frame on Expose, ignore the rest. */
	while (XPending(pc->dpy)) {
		XEvent ev;
		XNextEvent(pc->dpy, &ev);
		if (ev.type == Expose) {
			XPutImage(pc->dpy, pc->win, pc->gc, pc->img, 0, 0, 0, 0, pc->w, pc->h);
			XFlush(pc->dpy);
		}
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
		STAGE("E4X11: FAIL send_packet ret=%d", ret);
		return -1;
	}

	for (;;) {
		ret = avcodec_receive_frame(ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return shown;                 /* need more input / drained */
		}
		if (ret < 0) {
			STAGE("E4X11: FAIL receive_frame ret=%d", ret);
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
		STAGE("E4X11: FAIL fopen(%s)", pc->path);
		return -1;
	}
	dec = avcodec_find_decoder(AV_CODEC_ID_H264);
	parser = av_parser_init(AV_CODEC_ID_H264);
	pkt = av_packet_alloc();
	frame = av_frame_alloc();
	if (dec == NULL || parser == NULL || pkt == NULL || frame == NULL) {
		STAGE("E4X11: FAIL decoder/parser/alloc");
		goto out;
	}
	ctx = avcodec_alloc_context3(dec);
	if (ctx == NULL) {
		STAGE("E4X11: FAIL alloc_context3");
		goto out;
	}
	ctx->thread_count = 1;    /* ffmpeg's own threading off; we own the big stack */
	if (avcodec_open2(ctx, dec, NULL) < 0) {
		STAGE("E4X11: FAIL avcodec_open2");
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
				STAGE("E4X11: FAIL av_parser_parse2 ret=%d", used);
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

/* Full player: open the Xphoenix display, then loop the clip until RUN_SECS
 * elapse. The window is created lazily on the first decoded frame (its size).
 * Runs on the 8 MB decode thread. Returns 0 on success. */
static int play_body(struct play_ctx *pc)
{
	int pass = 0;

	STAGE("E4X11: start (path=%s fps=%d run=%ds)", pc->path, pc->fps, RUN_SECS);

	pc->dpy = XOpenDisplay(NULL);
	if (pc->dpy == NULL) {
		pc->dpy = XOpenDisplay(":0");
	}
	if (pc->dpy == NULL) {
		STAGE("E4X11: FAIL XOpenDisplay (DISPLAY=%s)",
			getenv("DISPLAY") ? getenv("DISPLAY") : "(unset)");
		return 1;
	}
	STAGE("E4X11: display opened");

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
			return 1;
		}
		pass++;
		STAGE("E4X11: pass %d done (%d frames, cumulative %lld)",
			pass, shown, pc->displayed);
		if (shown == 0) {
			/* A pass that decoded nothing would spin -- bail loudly. */
			STAGE("E4X11: FAIL pass decoded 0 frames");
			return 1;
		}
	}

	STAGE("E4X11: DONE ok (%d passes, %lld frames displayed)", pass, pc->displayed);

	if (pc->img != NULL) {
		XDestroyImage(pc->img);    /* frees ximg_data too */
	}
	if (pc->created) {
		XFreeGC(pc->dpy, pc->gc);
		XDestroyWindow(pc->dpy, pc->win);
	}
	free(pc->blitbuf);
	XCloseDisplay(pc->dpy);
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

	/* Progress markers must land unbuffered so the periodic capture sees them. */
	setvbuf(stdout, NULL, _IONBF, 0);

	memset(&pc, 0, sizeof(pc));
	pc.path = (argc > 1) ? argv[1] : DEFAULT_PATH;
	pc.fps = (argc > 2) ? atoi(argv[2]) : DEFAULT_FPS;
	pc.rc = 1;

	STAGE("E4X11: launcher (spawning 8 MB decode thread)");

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
		STAGE("E4X11: WARN pthread_create failed (err=%d) - MAIN thread, big-stack NOT exercised", err);
		pc.rc = play_body(&pc);
	}
	else {
		pthread_join(tid, NULL);
	}

	pthread_attr_destroy(&attr);
	return pc.rc;
}
