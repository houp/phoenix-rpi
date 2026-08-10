/*
 * Phoenix-RTOS — dlopen PoC test plugin (-fPIC ET_DYN)
 *
 * Exercises every relocation type the loader must handle:
 *   - a global pointer to a static     -> R_AARCH64_RELATIVE + R_AARCH64_GLOB_DAT
 *   - a string literal pointer          -> R_AARCH64_RELATIVE
 *   - calls to host printf / host_add   -> R_AARCH64_JUMP_SLOT (undefined -> host)
 *   - a constructor                     -> DT_INIT_ARRAY
 *
 * Built -shared -fPIC leaving printf/host_add UNDEFINED (resolved from the host
 * at load time) so there is no second libc instance (no two-heap hazard).
 *
 * Copyright 2026 Phoenix Systems
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* provided by the host, resolved at load time */
extern int printf(const char *fmt, ...);
extern int host_add(int a, int b);

static int counter = 7;
int *plugin_pctr = &counter;                 /* GLOB_DAT + RELATIVE */
static const char *plugin_msg = "hi from the dynamically loaded plugin";

int plugin_ctor_ran = 0;

__attribute__((constructor)) static void plugin_ctor(void)
{
	plugin_ctor_ran = 1; /* proves DT_INIT_ARRAY executed before entry */
}

/* the entry point the host will dlsym + call */
int plugin_entry(int x)
{
	printf("[plugin] entry: x=%d counter=%d ctor_ran=%d msg='%s'\n",
		x, *plugin_pctr, plugin_ctor_ran, plugin_msg);
	return host_add(x, *plugin_pctr); /* x + 7, computed via a HOST callback */
}
