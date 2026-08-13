/*
 * csd_probe — minimal on-Phoenix V3D compute (CSD) bring-up harness.
 *
 * Validates the (previously untested) winsys ioc_submit_csd end-to-end by
 * dispatching a compute kernel through phoenix_v3d_ioctl(SUBMIT_CSD) and
 * reading back the result. Staged so each failure localizes to one layer
 * (advisor): STEP1 = empty thread-end kernel (handler + cfg[] + BO plumbing);
 * later steps add TMU store + gid. Breadcrumbs on UART at every step.
 *
 * Kernels come from the off-device v3d-shader-tool (tools/v3d-shader-tool).
 * BOs are mapped uncached by the winsys, so CPU readback needs no extra flush.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "v3d_drm.h"

extern int phoenix_v3d_ioctl(int fd, unsigned long request, void *arg);

/* V3D CSD cfg[] field shifts/flags (broadcom v3d_csd.h). */
#define WG_COUNT_SHIFT       16
#define WGS_PER_SG_SHIFT     8
#define BATCHES_M1_SHIFT     12
#define WG_SIZE_SHIFT        0
#define CFG5_PROPAGATE_NANS  (1u << 2)
#define CFG5_SINGLE_SEG      (1u << 1)
#define CFG5_THREADING       (1u << 0)

/* CSNOP: empty compute kernel (thread-end only), local_size=16, threads=4,
 * single_seg=1, uniforms=0. From v3d-shader-tool (shaders-dump.txt). */
static const uint64_t CSNOP[] = {
	0x3c203186bb800000ull, /* nop ; nop ; thrsw */
	0x3c003186bb800000ull, /* nop ; nop */
	0x3c003186bb800000ull, /* nop ; nop */
};

/* CSCONST: store 0xC0DE1234 to out[0] (TMU write, no gid). local_size=16,
 * threads=4, single_seg=0, uniforms={const 0xC0DE1234, SSBO base VA}. */
static const uint64_t CSCONST[] = {
	0x3c603186bb800000ull, /* nop ; nop ; thrsw ; ldunif */
	0x3db032c6bbf40000ull, /* nop ; mov tmud, r5 ; thrsw ; ldunifrf.r0 */
	0x3c003306bbe00000ull, /* nop ; mov tmua, r0 */
	0x3c203181bb815000ull, /* tmuwt r1 ; nop ; thrsw */
	0x3c003186bb800000ull, /* nop ; nop */
	0x3c003186bb800000ull, /* nop ; nop */
};

static uint32_t make_bo(int fd, uint32_t size, uint32_t *gpuva, void **cpu)
{
	struct drm_v3d_create_bo c;
	struct drm_v3d_mmap_bo m;
	int r;

	memset(&c, 0, sizeof(c));
	c.size = size;
	r = phoenix_v3d_ioctl(fd, DRM_IOCTL_V3D_CREATE_BO, &c);
	if (r != 0) {
		printf("csd-probe: CREATE_BO(size=%u) rc=%d\n", size, r);
		return 0;
	}

	memset(&m, 0, sizeof(m));
	m.handle = c.handle;
	r = phoenix_v3d_ioctl(fd, DRM_IOCTL_V3D_MMAP_BO, &m);
	if (r != 0) {
		printf("csd-probe: MMAP_BO(handle=%u) rc=%d\n", c.handle, r);
		return 0;
	}

	*gpuva = c.offset;
	*cpu = (void *)(uintptr_t)m.offset;
	printf("csd-probe: BO handle=%u gpuva=0x%08x cpu=%p size=%u\n",
		c.handle, c.offset, *cpu, size);
	return c.handle;
}

int main(void)
{
	int fd = 0; /* token, routed to phoenix_v3d_ioctl by the shim */
	struct drm_v3d_get_param gp;
	struct drm_v3d_submit_csd s;
	uint32_t shva = 0, unva = 0;
	void *shcpu = NULL, *uncpu = NULL;
	uint32_t shbo, unbo, handles[2];
	int r;

	/* Unbuffered: so breadcrumbs survive a GPU-wedge hang (block-buffered stdout
	 * would otherwise lose everything up to the hang). */
	setvbuf(stdout, NULL, _IONBF, 0);

	printf("csd-probe: START (STEP1 liveness: empty thread-end kernel)\n");

	/* GET_PARAM first: mirrors v3d_screen_create's init path + sanity-checks CSD. */
	memset(&gp, 0, sizeof(gp));
	gp.param = DRM_V3D_PARAM_SUPPORTS_CSD;
	r = phoenix_v3d_ioctl(fd, DRM_IOCTL_V3D_GET_PARAM, &gp);
	printf("csd-probe: GET_PARAM(SUPPORTS_CSD) rc=%d value=%llu\n",
		r, (unsigned long long)gp.value);

	shbo = make_bo(fd, (uint32_t)sizeof(CSNOP), &shva, &shcpu);
	if (shbo == 0) {
		printf("csd-probe: shader BO alloc FAILED\n");
		return 1;
	}
	memcpy(shcpu, CSNOP, sizeof(CSNOP));
	printf("csd-probe: shader loaded (%u words) at gpuva=0x%08x\n",
		(unsigned)(sizeof(CSNOP) / 8), shva);

	/* uniforms unused by CSNOP (0 uniforms) but give cfg[6] a valid BO. */
	unbo = make_bo(fd, 64, &unva, &uncpu);
	if (unbo == 0) {
		printf("csd-probe: uniforms BO alloc FAILED\n");
		return 1;
	}

	memset(&s, 0, sizeof(s));
	s.cfg[0] = 1u << WG_COUNT_SHIFT; /* 1 workgroup in x */
	s.cfg[1] = 1u << WG_COUNT_SHIFT;
	s.cfg[2] = 1u << WG_COUNT_SHIFT;
	s.cfg[3] = (1u << WGS_PER_SG_SHIFT)   /* wgs_per_sg=1 (wg_size%16==0) */
	         | (0u << BATCHES_M1_SHIFT)   /* batches_per_sg-1 = 0 */
	         | (16u << WG_SIZE_SHIFT);    /* wg_size=16 */
	s.cfg[4] = 0;                          /* num_batches-1 = 0 (ver<71) */
	s.cfg[5] = shva | CFG5_PROPAGATE_NANS | CFG5_SINGLE_SEG | CFG5_THREADING;
	s.cfg[6] = unva;
	handles[0] = shbo;
	handles[1] = unbo;
	s.bo_handles = (uint64_t)(uintptr_t)handles;
	s.bo_handle_count = 2;

	printf("csd-probe: pre-submit cfg0=0x%08x cfg3=0x%08x cfg4=0x%08x cfg5=0x%08x cfg6=0x%08x\n",
		s.cfg[0], s.cfg[3], s.cfg[4], s.cfg[5], s.cfg[6]);

	r = phoenix_v3d_ioctl(fd, DRM_IOCTL_V3D_SUBMIT_CSD, &s);
	printf("csd-probe: SUBMIT_CSD returned rc=%d\n", r);
	if (r == 0)
		printf("csd-probe: STEP1 PASS — CSD dispatch completed (no hang)\n");
	else
		printf("csd-probe: STEP1 FAIL rc=%d\n", r);

	/* ===== STEP 2: constant store to out[0] + readback ===== */
	printf("csd-probe: STEP2 constant-store (out[0]=0xC0DE1234)\n");
	{
		uint32_t shva2, unva2, outva2;
		void *shcpu2, *uncpu2, *outcpu2;
		uint32_t shbo2, unbo2, outbo2, h2[3];
		struct drm_v3d_submit_csd s2;
		uint32_t got;

		shbo2 = make_bo(fd, (uint32_t)sizeof(CSCONST), &shva2, &shcpu2);
		outbo2 = make_bo(fd, 64, &outva2, &outcpu2);
		unbo2 = make_bo(fd, 64, &unva2, &uncpu2);
		if (shbo2 == 0 || outbo2 == 0 || unbo2 == 0) {
			printf("csd-probe: STEP2 BO alloc FAILED\n");
			return 1;
		}
		memcpy(shcpu2, CSCONST, sizeof(CSCONST));
		memset(outcpu2, 0xEE, 64); /* sentinel: distinguishes "GPU didn't write" from "wrote 0" */
		((uint32_t *)uncpu2)[0] = 0xC0DE1234u; /* uniform[0]: const value */
		((uint32_t *)uncpu2)[1] = outva2;       /* uniform[1]: SSBO base VA */

		memset(&s2, 0, sizeof(s2));
		s2.cfg[0] = 1u << WG_COUNT_SHIFT;
		s2.cfg[1] = 1u << WG_COUNT_SHIFT;
		s2.cfg[2] = 1u << WG_COUNT_SHIFT;
		s2.cfg[3] = (1u << WGS_PER_SG_SHIFT) | (0u << BATCHES_M1_SHIFT) | (16u << WG_SIZE_SHIFT);
		s2.cfg[4] = 0;
		s2.cfg[5] = shva2 | CFG5_PROPAGATE_NANS | CFG5_THREADING; /* single_seg=0 */
		s2.cfg[6] = unva2;
		h2[0] = shbo2;
		h2[1] = outbo2;
		h2[2] = unbo2;
		s2.bo_handles = (uint64_t)(uintptr_t)h2;
		s2.bo_handle_count = 3;
		printf("csd-probe: STEP2 pre-submit shaderVA=0x%08x outVA=0x%08x unifVA=0x%08x cfg5=0x%08x\n",
			shva2, outva2, unva2, s2.cfg[5]);

		r = phoenix_v3d_ioctl(fd, DRM_IOCTL_V3D_SUBMIT_CSD, &s2);
		got = ((volatile uint32_t *)outcpu2)[0];
		printf("csd-probe: STEP2 SUBMIT_CSD rc=%d out[0..3]=0x%08x 0x%08x 0x%08x 0x%08x (0xEE=untouched, expect out[0]=0xC0DE1234)\n",
			r, got, ((volatile uint32_t *)outcpu2)[1],
			((volatile uint32_t *)outcpu2)[2], ((volatile uint32_t *)outcpu2)[3]);
		if (r == 0 && got == 0xC0DE1234u)
			printf("csd-probe: STEP2 PASS — TMU write verified on HW\n");
		else
			printf("csd-probe: STEP2 FAIL rc=%d got=0x%08x\n", r, got);
	}

	return 0;
}
