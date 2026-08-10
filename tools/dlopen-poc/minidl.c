/*
 * Phoenix-RTOS — minimal in-process dynamic loader (Phase-A dlopen PoC)
 *
 * Loads a -fPIC ET_DYN aarch64 shared object entirely from userspace using the
 * primitives that already exist on Phoenix (open/read/mmap/mprotect):
 *
 *   - text/RO segments  : mapped FILE-BACKED at their FINAL protection (R-X / R),
 *                         so .text is never written and the W^X policy
 *                         (vm_mprotect rejects escalation past protOrig) is
 *                         never triggered.
 *   - data/RW segments  : mapped ANONYMOUS R-W, then the file contents copied in;
 *                         .bss is naturally zero (anonymous). Relocations only
 *                         ever write here.
 *
 * Handles the four relocation types a PIC .so emits on aarch64: R_AARCH64_RELATIVE,
 * GLOB_DAT, JUMP_SLOT, ABS64. Undefined symbols resolve against a host export table.
 *
 * Copyright 2026 Phoenix Systems
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>

#include "minidl.h"

/* --- minimal ELF64 definitions (aarch64), self-contained --- */
typedef struct {
	unsigned char e_ident[16];
	uint16_t e_type, e_machine;
	uint32_t e_version;
	uint64_t e_entry, e_phoff, e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx;
} Elf64_Ehdr;

typedef struct {
	uint32_t p_type, p_flags;
	uint64_t p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_align;
} Elf64_Phdr;

typedef struct {
	int64_t d_tag;
	uint64_t d_val; /* union d_ptr */
} Elf64_Dyn;

typedef struct {
	uint32_t st_name;
	unsigned char st_info, st_other;
	uint16_t st_shndx;
	uint64_t st_value, st_size;
} Elf64_Sym;

typedef struct {
	uint64_t r_offset, r_info, r_addend;
} Elf64_Rela;

#define ET_DYN       3
#define EM_AARCH64   183
#define PT_LOAD      1
#define PT_DYNAMIC   2
#define PF_X         0x1
#define PF_W         0x2
#define PF_R         0x4
#define SHN_UNDEF    0

#define DT_NULL        0
#define DT_HASH        4
#define DT_STRTAB      5
#define DT_SYMTAB      6
#define DT_RELA        7
#define DT_RELASZ      8
#define DT_RELAENT     9
#define DT_SYMENT      11
#define DT_INIT_ARRAY  25
#define DT_INIT_ARRAYSZ 27
#define DT_PLTRELSZ    2
#define DT_JMPREL      23

#define ELF64_R_SYM(i)  ((uint32_t)((i) >> 32))
#define ELF64_R_TYPE(i) ((uint32_t)((i) & 0xffffffffU))

#define R_AARCH64_ABS64     257
#define R_AARCH64_GLOB_DAT  1025
#define R_AARCH64_JUMP_SLOT 1026
#define R_AARCH64_RELATIVE  1027

#define PAGE_SZ 0x1000UL
#define PAGE_DOWN(x) ((x) & ~(PAGE_SZ - 1UL))
#define PAGE_UP(x)   PAGE_DOWN((x) + PAGE_SZ - 1UL)

struct minidl_obj {
	uintptr_t bias;          /* load bias (mapped base - min vaddr) */
	uintptr_t map_base;      /* start of the reserved span */
	size_t map_span;         /* size of the reserved span */
	int fd;                  /* kept open — file-backed text mapping references it */
	Elf64_Sym *symtab;       /* mapped .dynsym */
	const char *strtab;      /* mapped .dynstr */
	uint32_t symcount;       /* from DT_HASH nchain */
};

static void seterr(char *err, int errlen, const char *msg)
{
	if (err != NULL && errlen > 0) {
		(void)snprintf(err, (size_t)errlen, "%s", msg);
	}
}

/* resolve a symbol index to a runtime address */
static void *resolve(minidl_obj_t *o, uint32_t symidx,
	const minidl_hostsym_t *hostsyms, int nhost)
{
	Elf64_Sym *s = &o->symtab[symidx];
	const char *name = o->strtab + s->st_name;
	int i;

	if (s->st_shndx != SHN_UNDEF) {
		/* defined in this object */
		return (void *)(o->bias + s->st_value);
	}
	/* undefined -> host export table */
	for (i = 0; i < nhost; i++) {
		if (strcmp(hostsyms[i].name, name) == 0) {
			return hostsyms[i].addr;
		}
	}
	return NULL;
}

minidl_obj_t *minidl_open(const char *path, const minidl_hostsym_t *hostsyms, int nhost,
	char *err, int errlen)
{
	int fd, i;
	off_t fsize;
	unsigned char *fbuf = NULL;
	Elf64_Ehdr *eh;
	Elf64_Phdr *ph;
	minidl_obj_t *o = NULL;
	uint64_t vmin = ~0ULL, vmax = 0, dyn_vaddr = 0;
	const Elf64_Dyn *dyn;
	uint64_t rela = 0, relasz = 0, jmprel = 0, pltrelsz = 0, hashv = 0;
	uintptr_t bias;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		seterr(err, errlen, "open failed");
		return NULL;
	}
	fsize = lseek(fd, 0, SEEK_END);
	(void)lseek(fd, 0, SEEK_SET);
	if (fsize <= 0) {
		seterr(err, errlen, "empty file");
		close(fd);
		return NULL;
	}
	fbuf = malloc((size_t)fsize);
	if (fbuf == NULL || read(fd, fbuf, (size_t)fsize) != (ssize_t)fsize) {
		seterr(err, errlen, "read failed");
		goto fail;
	}

	eh = (Elf64_Ehdr *)fbuf;
	if (memcmp(eh->e_ident, "\177ELF", 4) != 0 || eh->e_ident[4] != 2 /*ELFCLASS64*/) {
		seterr(err, errlen, "not ELF64");
		goto fail;
	}
	if (eh->e_type != ET_DYN || eh->e_machine != EM_AARCH64) {
		seterr(err, errlen, "not aarch64 ET_DYN");
		goto fail;
	}

	ph = (Elf64_Phdr *)(fbuf + eh->e_phoff);
	for (i = 0; i < eh->e_phnum; i++) {
		if (ph[i].p_type == PT_LOAD) {
			if (ph[i].p_vaddr < vmin) {
				vmin = ph[i].p_vaddr;
			}
			if (ph[i].p_vaddr + ph[i].p_memsz > vmax) {
				vmax = ph[i].p_vaddr + ph[i].p_memsz;
			}
		}
		else if (ph[i].p_type == PT_DYNAMIC) {
			dyn_vaddr = ph[i].p_vaddr;
		}
	}
	if (vmin == ~0ULL || dyn_vaddr == 0) {
		seterr(err, errlen, "no PT_LOAD / PT_DYNAMIC");
		goto fail;
	}

	o = calloc(1, sizeof(*o));
	if (o == NULL) {
		seterr(err, errlen, "oom");
		goto fail;
	}
	o->fd = fd;

	/* Reserve a contiguous span, then MAP_FIXED each segment into it. */
	o->map_span = PAGE_UP(vmax) - PAGE_DOWN(vmin);
	o->map_base = (uintptr_t)mmap(NULL, o->map_span, PROT_READ,
		MAP_ANONYMOUS, -1, 0);
	if (o->map_base == (uintptr_t)MAP_FAILED) {
		seterr(err, errlen, "reserve mmap failed");
		goto fail;
	}
	bias = o->map_base - (uintptr_t)PAGE_DOWN(vmin);
	o->bias = bias;

	for (i = 0; i < eh->e_phnum; i++) {
		uint64_t segstart, segoff, mapend;
		int prot;
		void *want, *got;

		if (ph[i].p_type != PT_LOAD) {
			continue;
		}
		segstart = PAGE_DOWN(ph[i].p_vaddr);
		segoff = PAGE_DOWN(ph[i].p_offset);
		want = (void *)(bias + segstart);

		if ((ph[i].p_flags & PF_W) != 0) {
			/* writable data: anonymous R-W, copy file bytes in, bss auto-zero */
			mapend = PAGE_UP(ph[i].p_vaddr + ph[i].p_memsz);
			got = mmap(want, (size_t)(mapend - segstart), PROT_READ | PROT_WRITE,
				MAP_FIXED | MAP_ANONYMOUS, -1, 0);
			if (got != want) {
				seterr(err, errlen, "data mmap failed");
				goto fail;
			}
			memcpy((void *)(bias + ph[i].p_vaddr), fbuf + ph[i].p_offset,
				(size_t)ph[i].p_filesz);
		}
		else {
			/* read-only/exec: file-backed at FINAL protection (never written) */
			mapend = PAGE_UP(ph[i].p_vaddr + ph[i].p_filesz);
			prot = PROT_READ | (((ph[i].p_flags & PF_X) != 0) ? PROT_EXEC : 0);
			got = mmap(want, (size_t)(mapend - segstart), prot,
				MAP_FIXED, fd, (off_t)segoff);
			if (got != want) {
				seterr(err, errlen, "text mmap failed");
				goto fail;
			}
		}
	}

	/* Walk PT_DYNAMIC (now mapped) to find symtab/strtab/relocs. */
	dyn = (const Elf64_Dyn *)(bias + dyn_vaddr);
	for (; dyn->d_tag != DT_NULL; dyn++) {
		switch (dyn->d_tag) {
			case DT_SYMTAB:  o->symtab = (Elf64_Sym *)(bias + dyn->d_val); break;
			case DT_STRTAB:  o->strtab = (const char *)(bias + dyn->d_val); break;
			case DT_HASH:    hashv = bias + dyn->d_val; break;
			case DT_RELA:    rela = bias + dyn->d_val; break;
			case DT_RELASZ:  relasz = dyn->d_val; break;
			case DT_JMPREL:  jmprel = bias + dyn->d_val; break;
			case DT_PLTRELSZ:pltrelsz = dyn->d_val; break;
			default: break;
		}
	}
	if (o->symtab == NULL || o->strtab == NULL) {
		seterr(err, errlen, "no dynsym/dynstr");
		goto fail;
	}
	/* DT_HASH: [nbucket, nchain, ...]; nchain == number of dynsym entries. */
	o->symcount = (hashv != 0) ? ((uint32_t *)hashv)[1] : 0;

	/* Apply .rela.dyn then .rela.plt (same handling). */
	for (int pass = 0; pass < 2; pass++) {
		uint64_t base = (pass == 0) ? rela : jmprel;
		uint64_t sz = (pass == 0) ? relasz : pltrelsz;
		uint64_t off;

		for (off = 0; off + sizeof(Elf64_Rela) <= sz; off += sizeof(Elf64_Rela)) {
			Elf64_Rela *r = (Elf64_Rela *)(base + off);
			uint32_t type = ELF64_R_TYPE(r->r_info);
			uint32_t sym = ELF64_R_SYM(r->r_info);
			uint64_t *where = (uint64_t *)(bias + r->r_offset);
			void *val;

			switch (type) {
				case R_AARCH64_RELATIVE:
					*where = (uint64_t)bias + r->r_addend;
					break;
				case R_AARCH64_GLOB_DAT:
				case R_AARCH64_JUMP_SLOT:
				case R_AARCH64_ABS64:
					val = resolve(o, sym, hostsyms, nhost);
					if (val == NULL) {
						char m[96];
						(void)snprintf(m, sizeof(m), "unresolved symbol: %s",
							o->strtab + o->symtab[sym].st_name);
						seterr(err, errlen, m);
						goto fail;
					}
					*where = (uint64_t)val + r->r_addend;
					break;
				default:
					seterr(err, errlen, "unsupported reloc type");
					goto fail;
			}
		}
	}

	/* Run DT_INIT_ARRAY (re-walk; needs the RW data already relocated). */
	dyn = (const Elf64_Dyn *)(bias + dyn_vaddr);
	{
		uint64_t ia = 0, iasz = 0;
		for (; dyn->d_tag != DT_NULL; dyn++) {
			if (dyn->d_tag == DT_INIT_ARRAY) { ia = bias + dyn->d_val; }
			else if (dyn->d_tag == DT_INIT_ARRAYSZ) { iasz = dyn->d_val; }
		}
		if (ia != 0) {
			void (**fns)(void) = (void (**)(void))ia;
			for (uint64_t k = 0; k < iasz / sizeof(void *); k++) {
				if (fns[k] != NULL) {
					fns[k]();
				}
			}
		}
	}

	free(fbuf);
	return o;

fail:
	free(fbuf);
	if (o != NULL) {
		if (o->map_base != 0 && o->map_base != (uintptr_t)MAP_FAILED) {
			(void)munmap((void *)o->map_base, o->map_span);
		}
		free(o);
	}
	close(fd);
	return NULL;
}

void *minidl_sym(minidl_obj_t *o, const char *name)
{
	uint32_t i;

	if (o == NULL) {
		return NULL;
	}
	for (i = 0; i < o->symcount; i++) {
		if (o->symtab[i].st_shndx != SHN_UNDEF &&
			strcmp(o->strtab + o->symtab[i].st_name, name) == 0) {
			return (void *)(o->bias + o->symtab[i].st_value);
		}
	}
	return NULL;
}

void minidl_close(minidl_obj_t *o)
{
	if (o == NULL) {
		return;
	}
	if (o->map_base != 0 && o->map_base != (uintptr_t)MAP_FAILED) {
		(void)munmap((void *)o->map_base, o->map_span);
	}
	close(o->fd);
	free(o);
}
