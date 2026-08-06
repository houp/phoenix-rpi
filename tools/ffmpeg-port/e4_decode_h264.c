/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * Copyright (C) 2026 Phoenix Systems. Author: Witold Bołt.
 *
 * E4 runtime H.264 decode demo for the ffmpeg decode-core Phoenix port.
 *
 * The MJPEG sibling (e4_decode_file.c) can send one whole-file packet and get
 * one frame back. H.264 is a streaming codec: the raw Annex-B (.h264) byte
 * stream must be split into access units by the H.264 parser before each unit
 * is handed to the decoder, and the decoder may buffer (return EAGAIN) until it
 * has enough of the stream to emit the first frame. This program therefore
 * follows ffmpeg's canonical doc/examples/decode_video.c shape:
 *
 *   read file in padded chunks -> av_parser_parse2 (split into AUs) ->
 *   avcodec_send_packet -> drain avcodec_receive_frame on EAGAIN ->
 *   flush (send NULL) and drain again.
 *
 * It reports the FIRST decoded frame's geometry + a whole-plane-0 (luma)
 * average, so the parent can match the on-Pi result against a host baseline.
 *
 * Stage markers are printed and stdout-flushed at every step so a fault on the
 * UART pins the exact failing stage.
 *
 * STACK: H.264 decode (DPB management, deblocking, deep call chains) needs far
 * more stack than the mjpeg single-frame path. The Phoenix main-thread stack is
 * small, so the whole decode body runs on a dedicated pthread created with an
 * 8 MB stack via pthread_attr_setstacksize. On Phoenix libphoenix, a requested
 * stacksize is mmap'd fresh for the thread (pthread/pthread.c), so the entire
 * setup + decode call chain executes on that large stack. If the thread cannot
 * be created we fall back to the main thread with a loud marker so a subsequent
 * fault is not mistaken for a stack fix that silently did nothing.
 *
 * Being LGPL glue that links LGPL ffmpeg, this file is LGPL-2.1-or-later, kept
 * out of the BSD-licensed Phoenix core (matches the tools/quake3-port header
 * convention). No --enable-gpl / --enable-nonfree ffmpeg feature is used.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/error.h>

#define DEFAULT_PATH "/usr/share/e4/test.h264"

/* Dedicated decode-thread stack. mjpeg's single-frame path fits the small
 * main-thread stack; H.264's DPB/deblock/deep chains do not. */
#define DECODE_STACK_SIZE (8 * 1024 * 1024)

/* Read the input in fixed chunks; the parser is fed a sliding window. The
 * AV_INPUT_BUFFER_PADDING_SIZE tail must be present and zeroed (the bitstream
 * readers over-read a few bytes past the payload). */
#define CHUNK (4 * 1024)

/* Print a stage marker and flush immediately so a mid-way fault leaves the last
 * completed stage visible on the UART. */
#define STAGE(...)                     \
	do {                               \
		printf(__VA_ARGS__);           \
		printf("\n");                  \
		fflush(stdout);                \
	} while (0)

/* Whole-plane-0 average of the first decoded frame; reported exactly once. */
static int g_reported = 0;

static void report_first_frame(const AVFrame *frame)
{
	unsigned long long sum = 0;
	long long count;
	int x, y;

	if (g_reported)
		return;
	g_reported = 1;

	STAGE("E4: frame decoded %dx%d fmt=%d", frame->width, frame->height,
		frame->format);

	if (frame->data[0] && frame->width > 0 && frame->height > 0) {
		count = (long long)frame->width * frame->height;
		for (y = 0; y < frame->height; y++) {
			const unsigned char *row =
				frame->data[0] + (long)y * frame->linesize[0];
			for (x = 0; x < frame->width; x++)
				sum += row[x];
		}
		STAGE("E4: plane0 avg=%d", (int)(sum / (unsigned long long)count));
	}
	else {
		STAGE("E4: plane0 unavailable");
	}
}

/* Send one parsed access unit (or NULL to flush) and drain every frame the
 * decoder is willing to emit. Returns 0 on success, -1 on a hard decode error. */
static int decode_drain(AVCodecContext *ctx, AVPacket *pkt, AVFrame *frame)
{
	int ret;

	ret = avcodec_send_packet(ctx, pkt);
	if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
		STAGE("E4: FAIL send_packet ret=%d", ret);
		return -1;
	}

	for (;;) {
		ret = avcodec_receive_frame(ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return 0; /* need more input, or fully drained */
		if (ret < 0) {
			STAGE("E4: FAIL receive_frame ret=%d", ret);
			return -1;
		}
		report_first_frame(frame);
		av_frame_unref(frame);
	}
}

/* The entire decode work: open the file, set up the decoder + parser, and run
 * the parse/decode/drain loop. Runs on the large decode-thread stack (see
 * decode_thread). Returns 0 on success, non-zero on failure. */
static int decode_body(const char *path)
{
	const AVCodec *dec;
	AVCodecContext *ctx = NULL;
	AVCodecParserContext *parser = NULL;
	AVPacket *pkt = NULL;
	AVFrame *frame = NULL;
	FILE *f = NULL;
	unsigned char inbuf[CHUNK + AV_INPUT_BUFFER_PADDING_SIZE];
	int rc = 1;

	f = fopen(path, "rb");
	if (!f) {
		STAGE("E4: FAIL fopen(%s)", path);
		return 1;
	}
	STAGE("E4: file opened %s", path);

	pkt = av_packet_alloc();
	frame = av_frame_alloc();
	if (!pkt || !frame) {
		STAGE("E4: FAIL packet/frame alloc");
		goto out;
	}

	dec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!dec) {
		STAGE("E4: FAIL decoder not present");
		goto out;
	}
	STAGE("E4: decoder found %s", dec->name);

	/* The parser splits the raw Annex-B byte stream into access units. It is a
	 * separate compiled component from the decoder: av_parser_init returns NULL
	 * if --enable-parser=h264 was not configured, so assert it explicitly. */
	parser = av_parser_init(AV_CODEC_ID_H264);
	if (!parser) {
		STAGE("E4: FAIL parser not present (need --enable-parser=h264)");
		goto out;
	}
	STAGE("E4: parser ready");

	ctx = avcodec_alloc_context3(dec);
	if (!ctx) {
		STAGE("E4: FAIL alloc_context3");
		goto out;
	}
	/* Single-threaded first bring-up: ffmpeg pthread frame/slice threading is
	 * unproven under load on Phoenix (see README remaining-work note). We supply
	 * exactly one big-stack thread ourselves; ffmpeg's own threading stays off. */
	ctx->thread_count = 1;

	if (avcodec_open2(ctx, dec, NULL) < 0) {
		STAGE("E4: FAIL avcodec_open2");
		goto out;
	}
	STAGE("E4: codec opened");

	/* --- parse + decode loop --- */
	STAGE("E4: parse+decode loop");
	memset(inbuf + CHUNK, 0, AV_INPUT_BUFFER_PADDING_SIZE);
	for (;;) {
		size_t n = fread(inbuf, 1, CHUNK, f);
		const unsigned char *data = inbuf;
		size_t remaining = n;

		if (n == 0)
			break; /* EOF */

		while (remaining > 0) {
			unsigned char *au = NULL;
			int au_size = 0;
			int used = av_parser_parse2(parser, ctx, &au, &au_size,
				data, (int)remaining, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
			if (used < 0) {
				STAGE("E4: FAIL av_parser_parse2 ret=%d", used);
				goto out;
			}
			data += used;
			remaining -= (size_t)used;
			if (au_size > 0) {
				pkt->data = au;
				pkt->size = au_size;
				if (decode_drain(ctx, pkt, frame) < 0)
					goto out;
			}
		}
	}

	/* Flush the parser (a trailing AU may still be buffered), then flush the
	 * decoder by sending NULL and draining. */
	{
		unsigned char *au = NULL;
		int au_size = 0;
		av_parser_parse2(parser, ctx, &au, &au_size, NULL, 0,
			AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		if (au_size > 0) {
			pkt->data = au;
			pkt->size = au_size;
			if (decode_drain(ctx, pkt, frame) < 0)
				goto out;
		}
	}
	pkt->data = NULL;
	pkt->size = 0;
	if (decode_drain(ctx, pkt, frame) < 0)
		goto out;

	if (!g_reported) {
		STAGE("E4: FAIL no frame decoded");
		goto out;
	}

	STAGE("E4: DONE ok");
	rc = 0;

out:
	if (f)
		fclose(f);
	if (parser)
		av_parser_close(parser);
	av_frame_free(&frame);
	av_packet_free(&pkt);
	avcodec_free_context(&ctx);
	return rc;
}

/* pthread trampoline: run decode_body on the large stack and stash its rc. */
struct decode_arg {
	const char *path;
	int rc;
};

static void *decode_thread(void *arg)
{
	struct decode_arg *da = (struct decode_arg *)arg;
	da->rc = decode_body(da->path);
	return NULL;
}

int main(int argc, char **argv)
{
	const char *path = (argc > 1) ? argv[1] : DEFAULT_PATH;
	struct decode_arg da = { .path = path, .rc = 1 };
	pthread_attr_t attr;
	pthread_t tid;
	int err;

	STAGE("E4: start");

	/* Run the whole decode on a dedicated 8 MB-stack thread. On Phoenix the
	 * requested stacksize is mmap'd fresh (libphoenix pthread/pthread.c), so
	 * setup + the deep H.264 decode chain all execute on the large stack. */
	err = pthread_attr_init(&attr);
	if (err == 0)
		err = pthread_attr_setstacksize(&attr, DECODE_STACK_SIZE);
	if (err == 0)
		err = pthread_create(&tid, &attr, decode_thread, &da);

	if (err != 0) {
		/* Loud fallback: if this then faults it must NOT be read as a working
		 * big-stack fix — the big stack was never exercised. */
		STAGE("E4: WARN pthread_create failed (err=%d) - decode on MAIN thread, big-stack NOT exercised", err);
		da.rc = decode_body(path);
	}
	else {
		STAGE("E4: decode thread spawned (stack=%d MB)", DECODE_STACK_SIZE / (1024 * 1024));
		pthread_join(tid, NULL);
		STAGE("E4: thread joined rc=%d", da.rc);
	}

	pthread_attr_destroy(&attr);
	return da.rc;
}
