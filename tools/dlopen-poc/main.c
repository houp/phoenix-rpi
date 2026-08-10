/*
 * Phoenix-RTOS — dlopen PoC host (static ET_EXEC)
 *
 * Loads plugin.so at runtime via the minidl loader, resolves the plugin's
 * undefined symbols (printf, host_add) against an explicit host export table,
 * calls the plugin entry, and checks the result. A HOST callback (host_add)
 * invoked BY the plugin proves bidirectional symbol resolution.
 *
 * Success criteria (printed): the plugin prints its line (proving text executed
 * from a file-backed R-X mapping + RELATIVE/GLOB_DAT relocations applied), the
 * constructor ran (DT_INIT_ARRAY), and entry(35) returns 35+7 == 42 via the host
 * callback (JUMP_SLOT resolution both directions).
 *
 * Copyright 2026 Phoenix Systems
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <string.h>

#include "minidl.h"

/* host callback the plugin calls back into */
int host_add(int a, int b)
{
	printf("[host]   host_add(%d, %d) called back from the plugin\n", a, b);
	return a + b;
}

int main(void)
{
	char err[128] = { 0 };
	const char *sopath = "/usr/bin/plugin.so"; /* staged into the NFS/root export */
	minidl_obj_t *obj;
	int (*entry)(int);
	int rc;

	/* Host export table: the symbols the plugin may resolve against. */
	static const minidl_hostsym_t hostsyms[] = {
		{ "printf", (void *)printf },
		{ "host_add", (void *)host_add },
	};

	printf("=== dlopen PoC: loading %s ===\n", sopath);

	obj = minidl_open(sopath, hostsyms, (int)(sizeof(hostsyms) / sizeof(hostsyms[0])),
		err, (int)sizeof(err));
	if (obj == NULL) {
		printf("PoC FAIL: minidl_open: %s\n", err);
		return 1;
	}
	printf("[host]   loaded + relocated OK\n");

	entry = (int (*)(int))minidl_sym(obj, "plugin_entry");
	if (entry == NULL) {
		printf("PoC FAIL: minidl_sym(plugin_entry) not found\n");
		minidl_close(obj);
		return 1;
	}
	printf("[host]   resolved plugin_entry @ %p\n", (void *)entry);

	rc = entry(35);
	printf("[host]   plugin_entry(35) returned %d (expect 42)\n", rc);

	minidl_close(obj);

	if (rc == 42) {
		printf("=== dlopen PoC: PASS ===\n");
		return 0;
	}
	printf("=== dlopen PoC: FAIL (bad return) ===\n");
	return 1;
}
