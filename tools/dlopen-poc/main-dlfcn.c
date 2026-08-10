/*
 * Phoenix-RTOS — dlopen PoC host using the REAL libphoenix <dlfcn.h> API.
 *
 * Unlike main.c (which used the standalone minidl loader + an explicit host
 * symbol table), this exercises the shipped dlopen/dlsym/dlclose/dlerror in
 * libphoenix, which resolves the plugin's undefined symbols (printf, host_add)
 * automatically against THIS host's own .symtab. The host must therefore be
 * linked unstripped.
 *
 * Copyright 2026 Phoenix Systems
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <dlfcn.h>

/* the host callback the plugin resolves + calls back into (via its .symtab) */
int host_add(int a, int b)
{
	printf("[host]   host_add(%d, %d) called back from the plugin\n", a, b);
	return a + b;
}

int main(void)
{
	const char *sopath = "/usr/bin/plugin.so";
	void *h;
	int (*entry)(int);
	int rc;

	printf("=== dlfcn PoC: dlopen(%s) via libphoenix ===\n", sopath);

	h = dlopen(sopath, RTLD_NOW);
	if (h == NULL) {
		printf("dlfcn PoC FAIL: dlopen: %s\n", dlerror());
		return 1;
	}
	printf("[host]   dlopen OK (symbols auto-resolved against host .symtab)\n");

	entry = (int (*)(int))dlsym(h, "plugin_entry");
	if (entry == NULL) {
		printf("dlfcn PoC FAIL: dlsym: %s\n", dlerror());
		(void)dlclose(h);
		return 1;
	}
	printf("[host]   dlsym(plugin_entry) @ %p\n", (void *)entry);

	rc = entry(35);
	printf("[host]   plugin_entry(35) returned %d (expect 42)\n", rc);

	(void)dlclose(h);

	if (rc == 42) {
		printf("=== dlfcn PoC: PASS ===\n");
		return 0;
	}
	printf("=== dlfcn PoC: FAIL (bad return) ===\n");
	return 1;
}
