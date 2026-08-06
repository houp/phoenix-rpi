/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * Copyright (C) 2026 Phoenix Systems. Author: Witold Bołt.
 *
 * E4 "decode to visible screen" -- shared YUV420 -> framebuffer blit core.
 *
 * Factored into a header so the exact same conversion + centering code runs in
 * two builds: the on-target e4_fbshow.c (writes /dev/fb0) and the host
 * e4_host_sanity.c (writes a PPM). If the host PPM shows the right colours in
 * the right places, the decode->convert->pack->place pipeline is validated
 * before it ever touches the Pi.
 *
 * FB pixel format (ground truth, not a guess): the Pi 4 firmware HDMI
 * framebuffer is 32bpp with in-memory BYTE ORDER [R, G, B, X]. This is what the
 * live boot console draws through -- pl011-tty.c stores its palette "red" as
 * the raw word 0xff0000aa and that renders RED on HDMI, i.e. the low byte (R)
 * lands in the red channel. So per pixel we simply store bytes
 * dst[0]=R, dst[1]=G, dst[2]=B, dst[3]=0xFF (opaque), and no endian reasoning
 * is needed. (The SDL_phoenixframebuffer.c ARGB8888 comment is the UNVERIFIED
 * path and is R/B-swapped from this ground truth -- do not follow it.)
 */
#ifndef E4_FB_BLIT_H
#define E4_FB_BLIT_H

#include <stdint.h>
#include <string.h>

static inline uint8_t e4_clamp8(int v)
{
	if (v < 0) {
		return 0;
	}
	if (v > 255) {
		return 255;
	}
	return (uint8_t)v;
}

/*
 * Paint an fbw x fbh, `pitch`-bytes-per-row, 32bpp [R,G,B,X] framebuffer:
 * fill it opaque black, then overlay the decoded YUV420 image centered, with
 * clipping if the image is larger than the framebuffer.
 *
 * Y/U/V are the three planes with strides yls/uls/vls (use libav linesize[i],
 * NOT width -- planes are padded). Chroma is 4:2:0, so U/V are sampled at
 * (ix/2, iy/2). Full-range JFIF BT.601 coefficients (YUVJ420P / baseline JPEG):
 *   R = Y                 + 1.402   * (V-128)
 *   G = Y - 0.344136*(U-128) - 0.714136*(V-128)
 *   B = Y + 1.772  *(U-128)
 */
static inline void e4_yuv420_to_fb(
	uint8_t *fb, int fbw, int fbh, int pitch,
	const uint8_t *Y, int yls,
	const uint8_t *U, int uls,
	const uint8_t *V, int vls,
	int iw, int ih)
{
	int x0, y0, iy, ix, dy, dx;

	/* opaque-black background (matches the console bg word 0xff000000) */
	for (dy = 0; dy < fbh; dy++) {
		uint8_t *row = fb + (size_t)dy * pitch;
		for (dx = 0; dx < fbw; dx++) {
			row[dx * 4 + 0] = 0;
			row[dx * 4 + 1] = 0;
			row[dx * 4 + 2] = 0;
			row[dx * 4 + 3] = 0xFF;
		}
	}

	/* top-left corner of the centered image (may be negative -> clip) */
	x0 = (fbw - iw) / 2;
	y0 = (fbh - ih) / 2;

	for (iy = 0; iy < ih; iy++) {
		dy = y0 + iy;
		if (dy < 0 || dy >= fbh) {
			continue;
		}
		for (ix = 0; ix < iw; ix++) {
			int yv, uv, vv, cc, dd, ee, r, g, b;
			uint8_t *p;

			dx = x0 + ix;
			if (dx < 0 || dx >= fbw) {
				continue;
			}

			yv = Y[(size_t)iy * yls + ix];
			uv = U[(size_t)(iy / 2) * uls + (ix / 2)];
			vv = V[(size_t)(iy / 2) * vls + (ix / 2)];

			cc = yv;
			dd = uv - 128;
			ee = vv - 128;

			r = cc + ((91881 * ee) >> 16);                  /* 1.402   */
			g = cc - ((22554 * dd + 46802 * ee) >> 16);     /* .344136/.714136 */
			b = cc + ((116130 * dd) >> 16);                 /* 1.772   */

			p = fb + (size_t)dy * pitch + (size_t)dx * 4;
			p[0] = e4_clamp8(r);
			p[1] = e4_clamp8(g);
			p[2] = e4_clamp8(b);
			p[3] = 0xFF;
		}
	}
}

#endif /* E4_FB_BLIT_H */
