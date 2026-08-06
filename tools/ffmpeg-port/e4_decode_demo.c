/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * Copyright (C) 2026 Phoenix Systems. Author: Witold Bołt.
 *
 * E4 minimal decode demo for the ffmpeg decode-core Phoenix port.
 *
 * This links the LGPL ffmpeg decode core (libav{util,codec,format}.a) into a
 * single static aarch64-phoenix ELF, exercising the real MJPEG decode call
 * graph so the link is honest (the open/close + send/receive machinery is
 * actually referenced, not just named): avcodec_find_decoder(MJPEG) ->
 * avcodec_alloc_context3 -> avcodec_open2 -> av_packet_alloc / av_frame_alloc
 * -> avcodec_send_packet (drain) -> avcodec_receive_frame -> free.
 *
 * Phase 1 goal is a clean static link (0 undefined externals), not a running
 * decode: there is no real input here. A drain-only send pulls the decoder's
 * open/close path; a runtime demo (staged clip -> frames -> /dev/fb0) is the
 * infra-gated next step (see README.md).
 *
 * Being LGPL glue that links LGPL ffmpeg, this file is LGPL-2.1-or-later, kept
 * out of the BSD-licensed Phoenix core (matches the tools/quake3-port header
 * convention). No --enable-gpl / --enable-nonfree ffmpeg feature is used.
 */
#include <stdio.h>

#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>

int main(void)
{
	const AVCodec *dec;
	AVCodecContext *ctx;
	AVPacket *pkt;
	AVFrame *frame;
	int ret;

	dec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
	if (!dec) {
		fprintf(stderr, "MJPEG decoder not present in this build\n");
		return 1;
	}
	printf("decoder: %s (%s)\n", dec->name, dec->long_name);

	ctx = avcodec_alloc_context3(dec);
	if (!ctx) {
		fprintf(stderr, "avcodec_alloc_context3 failed\n");
		return 1;
	}

	ret = avcodec_open2(ctx, dec, NULL);
	if (ret < 0) {
		fprintf(stderr, "avcodec_open2 failed: %d\n", ret);
		avcodec_free_context(&ctx);
		return 1;
	}

	pkt = av_packet_alloc();
	frame = av_frame_alloc();
	if (!pkt || !frame) {
		fprintf(stderr, "packet/frame alloc failed\n");
		av_frame_free(&frame);
		av_packet_free(&pkt);
		avcodec_free_context(&ctx);
		return 1;
	}

	/* No real input in Phase 1: drain the decoder (NULL packet = flush) to
	 * pull the send/receive machinery through the link. */
	ret = avcodec_send_packet(ctx, NULL);
	while (ret >= 0) {
		ret = avcodec_receive_frame(ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		if (ret < 0)
			break;
		av_frame_unref(frame);
	}

	printf("decode call graph linked and exercised (drain ret=%d)\n", ret);

	av_frame_free(&frame);
	av_packet_free(&pkt);
	avcodec_free_context(&ctx);
	return 0;
}
