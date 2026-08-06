/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * Copyright (C) 2026 Phoenix Systems. Author: Witold Bołt.
 *
 * E4 runtime decode demo for the ffmpeg decode-core Phoenix port.
 *
 * Unlike e4_decode_demo.c (a link-only drain that decodes nothing), this
 * program performs a REAL MJPEG decode of a file on the target: it reads the
 * whole JPEG into an AVPacket, opens the MJPEG decoder, sends the packet,
 * receives one frame, and reports the decoded geometry + a whole-plane-0
 * average so the parent can verify the on-Pi result against a host-computed
 * baseline.
 *
 * Stage markers are printed and stdout-flushed at every step so that a fault
 * on the UART pins the exact failing stage.
 *
 * Being LGPL glue that links LGPL ffmpeg, this file is LGPL-2.1-or-later, kept
 * out of the BSD-licensed Phoenix core (matches the tools/quake3-port header
 * convention). No --enable-gpl / --enable-nonfree ffmpeg feature is used.
 */
#include <stdio.h>
#include <stdlib.h>

#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/error.h>

#define DEFAULT_PATH "/usr/share/e4/test.jpg"

/* Print a stage marker and flush immediately so a mid-way fault leaves the
 * last completed stage visible on the UART. */
#define STAGE(...)                     \
	do {                               \
		printf(__VA_ARGS__);           \
		printf("\n");                  \
		fflush(stdout);                \
	} while (0)

int main(int argc, char **argv)
{
	const char *path = (argc > 1) ? argv[1] : DEFAULT_PATH;
	const AVCodec *dec;
	AVCodecContext *ctx = NULL;
	AVPacket *pkt = NULL;
	AVFrame *frame = NULL;
	FILE *f = NULL;
	long fsize;
	size_t nread;
	int ret;

	STAGE("E4: start");

	/* --- read the whole JPEG file into memory --- */
	f = fopen(path, "rb");
	if (!f) {
		STAGE("E4: FAIL fopen(%s)", path);
		return 1;
	}
	if (fseek(f, 0, SEEK_END) != 0 || (fsize = ftell(f)) < 0) {
		STAGE("E4: FAIL fseek/ftell");
		fclose(f);
		return 1;
	}
	rewind(f);
	STAGE("E4: file opened %ld bytes", fsize);

	/* av_new_packet allocates a padded, aligned buffer (decoders read a few
	 * bytes past the payload; the AV_INPUT_BUFFER_PADDING_SIZE tail is why we
	 * do not just point at a bare malloc). */
	pkt = av_packet_alloc();
	frame = av_frame_alloc();
	if (!pkt || !frame) {
		STAGE("E4: FAIL packet/frame alloc");
		goto fail;
	}
	if (av_new_packet(pkt, (int)fsize) < 0) {
		STAGE("E4: FAIL av_new_packet");
		goto fail;
	}
	nread = fread(pkt->data, 1, (size_t)fsize, f);
	fclose(f);
	f = NULL;
	if (nread != (size_t)fsize) {
		STAGE("E4: FAIL fread got %zu of %ld", nread, fsize);
		goto fail;
	}
	STAGE("E4: packet filled %d bytes", pkt->size);

	/* --- find + open the MJPEG decoder --- */
	dec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
	if (!dec) {
		STAGE("E4: FAIL decoder not present");
		goto fail;
	}
	STAGE("E4: decoder found %s", dec->name);

	ctx = avcodec_alloc_context3(dec);
	if (!ctx) {
		STAGE("E4: FAIL alloc_context3");
		goto fail;
	}
	/* Single-threaded first bring-up: ffmpeg pthread frame/slice threading is
	 * unproven under load on Phoenix (see README remaining-work note). */
	ctx->thread_count = 1;

	ret = avcodec_open2(ctx, dec, NULL);
	if (ret < 0) {
		STAGE("E4: FAIL avcodec_open2 ret=%d", ret);
		goto fail;
	}
	STAGE("E4: codec opened");

	/* --- decode one frame --- */
	ret = avcodec_send_packet(ctx, pkt);
	if (ret < 0) {
		STAGE("E4: FAIL send_packet ret=%d", ret);
		goto fail;
	}
	STAGE("E4: packet sent");

	ret = avcodec_receive_frame(ctx, frame);
	if (ret < 0) {
		STAGE("E4: FAIL receive_frame ret=%d", ret);
		goto fail;
	}
	STAGE("E4: frame decoded %dx%d fmt=%d", frame->width, frame->height,
		frame->format);

	/* --- whole-plane-0 average (luma for a YUV JPEG) --- */
	if (frame->data[0] && frame->width > 0 && frame->height > 0) {
		unsigned long long sum = 0;
		long long count = (long long)frame->width * frame->height;
		int x, y;
		for (y = 0; y < frame->height; y++) {
			const unsigned char *row = frame->data[0] + (long)y * frame->linesize[0];
			for (x = 0; x < frame->width; x++)
				sum += row[x];
		}
		STAGE("E4: plane0 avg=%d", (int)(sum / (unsigned long long)count));
	}
	else {
		STAGE("E4: plane0 unavailable");
	}

	STAGE("E4: DONE ok");

	av_frame_free(&frame);
	av_packet_free(&pkt);
	avcodec_free_context(&ctx);
	return 0;

fail:
	if (f)
		fclose(f);
	av_frame_free(&frame);
	av_packet_free(&pkt);
	avcodec_free_context(&ctx);
	return 1;
}
