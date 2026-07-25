/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * gl_det_harness.c — #67 GLQuake glitch determinism probe on the *OpenGL* path.
 *
 * The earlier Vulkan (v3dv) determinism stages only exercised the SHARED winsys submit
 * layer (libv3d-phoenix.a). They could NOT reach the GL gallium frontend Quakespasm
 * actually drives — in particular the RCL / tile-rendering-mode / EZ config emission in
 * gallium `v3dx_rcl.c`, which the earlier #67 investigation implicated (CT1 stalling on
 * the RCL's first TILE_RENDERING_MODE_CFG/EZ packet).
 *
 * This harness renders through the EXACT stack Quake uses: v3d_screen_create ->
 * st_create_context(API_OPENGL_COMPAT) -> a DRAM RGBA8 + DEPTH24 FBO with depth-test
 * LEQUAL (the EZ trigger) -> many DISTINCT depth-tested triangles (immediate mode, the
 * world-render path) re-rendered each iteration (the GL frontend allocates fresh CLs/BOs
 * per frame == Quake's per-frame churn). It CRCs a center band of the readback each
 * iteration; a varying band CRC on identical geometry = the #67 non-determinism reproduced
 * on the OpenGL path. A coverage guard (band CRC != all-clear CRC) proves the band covers
 * rendered geometry. Standalone binary — never touches the user's rpi4-quake.
 *
 * Context/FBO setup mirrors pl_phoenix_glctx.c (qsv3d_init, DRAM fallback path).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>

#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "main/menums.h"
#include "frontend/api.h"
#include "main/mtypes.h"
#include "state_tracker/st_context.h"
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include "GL/gl.h"
#include "GL/glext.h"

struct pipe_screen_config;
struct renderonly;
struct pipe_screen *v3d_screen_create(int fd, const struct pipe_screen_config *config,
                                      struct renderonly *ro);
extern unsigned char _mesa_make_current(struct gl_context *ctx,
                                        struct gl_framebuffer *drawFb,
                                        struct gl_framebuffer *readFb);
extern int v3d_phoenix_powerOn(void);

/* Mesa's trace gallium wrapper is referenced by the GL state tracker but not built into
 * libv3d-phoenix; we never enable GALLIUM_TRACE, so pass the context through (same shim
 * as pl_phoenix_glctx.c). */
struct pipe_context *trace_context_create_threaded(struct pipe_screen *screen,
                                                    struct pipe_context *pipe)
{
	(void)screen;
	return pipe;
}

/* Phoenix libc lacks pthread_getcpuclockid (referenced by Mesa's u_thread timing);
 * monotonic-clock stand-in, same stub as pl_phoenix_stubs.c. */
int pthread_getcpuclockid(pthread_t thread, clockid_t *clock_id)
{
	(void)thread;
	if (clock_id)
		*clock_id = CLOCK_MONOTONIC;
	return 0;
}

static uint32_t crc32_le(const void *buf, uint32_t len)
{
	const uint8_t *p = (const uint8_t *)buf;
	uint32_t crc = 0xffffffffu;
	for (uint32_t i = 0; i < len; i++) {
		crc ^= p[i];
		for (int k = 0; k < 8; k++)
			crc = (crc >> 1) ^ (0xedb88320u & (uint32_t)(-(int32_t)(crc & 1u)));
	}
	return ~crc;
}

#define W 1024
#define H 768
#define GRID 32u
#define MAXTRIS (GRID * GRID)   /* 1024 distinct triangles */

/* Draw `ntris` DISTINCT small triangles, bit-reversed grid placement (any prefix is spread
 * across the whole screen so the center band is populated at every count), varied per-triangle
 * DEPTH (drives depth-test/EZ) and color. Immediate mode = the gallium path Quake's world uses. */
static void draw_scene(unsigned ntris)
{
	glBegin(GL_TRIANGLES);
	for (unsigned t = 0; t < ntris; t++) {
		unsigned cell = 0, tt = t;
		for (int b = 0; b < 10; b++) { cell = (cell << 1) | (tt & 1u); tt >>= 1; }
		unsigned gx = cell % GRID, gy = cell / GRID;
		float cx = -1.0f + ((float)gx + 0.5f) * (2.0f / (float)GRID);
		float cy = -1.0f + ((float)gy + 0.5f) * (2.0f / (float)GRID);
		float hh = (2.0f / (float)GRID) * 0.42f;
		/* varied depth in [-0.9, 0.9] from a cheap hash of the cell so LEQUAL depth-test
		 * accepts/rejects differently across the mesh (exercises EZ). */
		float z = (((float)((cell * 2654435761u) & 0xffffu)) / 65535.0f) * 1.8f - 0.9f;
		glColor3f((float)gx / (float)GRID, (float)gy / (float)GRID, 0.5f);
		glTexCoord2f(0.5f, 0.0f); glVertex3f(cx,      cy - hh, z);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(cx + hh, cy + hh, z);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(cx - hh, cy + hh, z);
	}
	glEnd();
}

int main(void)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	printf("gl-det: start\n");
	v3d_phoenix_powerOn();

	struct pipe_screen_config cfg;
	memset(&cfg, 0, sizeof(cfg));
	struct pipe_screen *pscreen = v3d_screen_create(0, &cfg, NULL);
	if (!pscreen) { printf("gl-det: pipe_screen NULL\n"); return 1; }
	struct pipe_context *pipe = pscreen->context_create(pscreen, NULL, 0);
	if (!pipe) { printf("gl-det: pipe_context NULL\n"); return 1; }
	struct gl_config visual; struct st_config_options opts;
	memset(&visual, 0, sizeof(visual)); memset(&opts, 0, sizeof(opts));
	struct st_context *st = st_create_context(API_OPENGL_COMPAT, pipe, &visual, NULL, &opts, 0, 0);
	if (!st) { printf("gl-det: st_create_context NULL\n"); return 1; }
	_mesa_make_current(st->ctx, NULL, NULL);
	printf("gl-det: GL up; %s / %s\n", (const char *)glGetString(GL_VERSION),
	       (const char *)glGetString(GL_RENDERER));

	/* DRAM color+depth FBO (mirrors qsv3d_init's fallback path) — glReadPixels reads its
	 * CPU-mapped BO directly (no scanout/fb0, so no fbcon interaction). */
	GLuint fbo = 0, rbColor = 0, rbDepth = 0;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glGenRenderbuffers(1, &rbColor);
	glBindRenderbuffer(GL_RENDERBUFFER, rbColor);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, W, H);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbColor);
	glGenRenderbuffers(1, &rbDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rbDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, W, H);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbDepth);
	GLenum fbs = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	printf("gl-det: FBO %dx%d status=0x%x (complete=0x%x)\n", W, H, fbs, GL_FRAMEBUFFER_COMPLETE);
	if (fbs != GL_FRAMEBUFFER_COMPLETE) { printf("gl-det: FBO incomplete ABORT\n"); return 2; }

	glViewport(0, 0, W, H);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);            /* matches Quake's global depth func (the EZ path) */
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	/* TEXTURE: a 128x128 RGBA8 texture (Quake lightmap-block size), varied pattern so sampling
	 * reads non-trivial texels, MODULATE + LINEAR (the world FF texturing path). Each iteration
	 * re-uploads the SAME data via glTexSubImage2D — mimicking Quake's per-frame dynamic-lightmap
	 * upload. Because the data is identical every frame, a COHERENT upload->sample path must
	 * render byte-identically; divergence = the GPU sampled a stale/half-updated texture (the
	 * CPU-texel-write racing the GPU TMU fetch — the #67 texture-coherency hypothesis). */
#define TEXW 128
#define TEXH 128
	unsigned char *texdata = (unsigned char *)malloc((size_t)TEXW * TEXH * 4);
	for (int y = 0; y < TEXH; y++) {
		for (int x = 0; x < TEXW; x++) {
			int i = (y * TEXW + x) * 4;
			unsigned char c = ((x ^ y) & 8) ? 0xff : 0x40;   /* 8px checker */
			texdata[i+0] = c;
			texdata[i+1] = (unsigned char)(x * 2);
			texdata[i+2] = (unsigned char)(y * 2);
			texdata[i+3] = 0xff;
		}
	}
	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXW, TEXH, 0, GL_RGBA, GL_UNSIGNED_BYTE, texdata);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	printf("gl-det: texturing ON (%dx%d, MODULATE/LINEAR, per-frame glTexSubImage2D re-upload)\n", TEXW, TEXH);

	/* center band read-back region + all-clear reference */
	int band_rows = 64, band_y = H / 2 - 32;
	uint32_t band_len = (uint32_t)W * band_rows * 3;
	unsigned char *band = (unsigned char *)malloc(band_len);
	unsigned char *zero = (unsigned char *)calloc(band_len, 1);
	uint32_t clear_crc = crc32_le(zero, band_len);   /* all-black band */
	free(zero);

	unsigned counts[] = { 64u, 256u, 1024u };
	for (unsigned cix = 0; cix < 3u; cix++) {
		unsigned ntris = counts[cix];
		uint32_t crc0 = 0, last = 0, distinct = 0, coverage_ok = 0;
		unsigned iters = 10;
		for (unsigned it = 0; it < iters; it++) {
			/* re-render the identical scene from scratch each iteration: the GL frontend
			 * builds a FRESH control list + BOs every frame == Quake's per-frame churn. */
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			/* per-frame texture re-upload of IDENTICAL data (mimics the dynamic-lightmap
			 * glTexSubImage2D); a coherent upload->sample path renders identically every iter. */
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TEXW, TEXH, GL_RGBA, GL_UNSIGNED_BYTE, texdata);
			draw_scene(ntris);
			glFinish();
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glReadPixels(0, band_y, W, band_rows, GL_RGB, GL_UNSIGNED_BYTE, band);
			uint32_t bcrc = crc32_le(band, band_len);
			if (it == 0) { crc0 = last = bcrc; distinct = 1; coverage_ok = (bcrc != clear_crc); }
			else if (bcrc != last) { distinct++; last = bcrc; }
			printf("gl-det: N=%u it=%u band_crc=0x%08x%s\n", ntris, it, bcrc,
			       (bcrc != crc0) ? "  <-- RENDER DIVERGED" : "");
		}
		printf("gl-det: RESULT N=%u: render %s | coverage %s (distinct band-CRC transitions=%u over %u iters)\n",
		       ntris, (distinct <= 1) ? "STABLE" : "DIVERGED (glitch reproduced on GL!)",
		       coverage_ok ? "OK (band != clear)" : "VACUOUS (band == clear)", distinct, iters);
	}
	free(band);
	printf("gl-det: PASS\n");
	return 0;
}
