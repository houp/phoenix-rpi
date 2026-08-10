/*
 * Phoenix-RTOS — minimal in-process dynamic loader (Phase-A dlopen PoC)
 *
 * Proof-of-concept for T-DYNLINK Phase A: load a -fPIC ET_DYN shared object
 * into a running static program, relocate it, resolve its undefined symbols
 * against a host-provided export table, and call into it. No kernel change.
 *
 * See docs/inprogress/2026-08-10-dynamic-linking-feasibility.md.
 *
 * Copyright 2026 Phoenix Systems
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _MINIDL_H_
#define _MINIDL_H_

/* One host-exported symbol the loaded object may resolve against. In a real
 * libdl this table would be the host executable's dynamic symbol table; for
 * the PoC the host registers an explicit table (the controlled-export option
 * from the feasibility doc — avoids depending on an unstripped host .symtab). */
typedef struct {
	const char *name;
	void *addr;
} minidl_hostsym_t;

typedef struct minidl_obj minidl_obj_t;

/* Load + relocate `path` (a -fPIC ET_DYN .so). Undefined symbols are resolved
 * against hostsyms[0..nhost). Returns a handle, or NULL with *err filled. */
minidl_obj_t *minidl_open(const char *path, const minidl_hostsym_t *hostsyms, int nhost,
	char *err, int errlen);

/* Look up an exported symbol in a loaded object. NULL if not found. */
void *minidl_sym(minidl_obj_t *obj, const char *name);

/* Unmap + free. */
void minidl_close(minidl_obj_t *obj);

#endif
