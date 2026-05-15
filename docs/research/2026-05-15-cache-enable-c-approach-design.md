# 2026-05-15 — Pi 4 cache enable: clean restart (Linux `__cpu_setup` pattern)

## Why we're restarting

The 2026-05-14 cache-enable work (kernel commits `3d5c9574`, `3b63677f`,
`f2b7c62f`) was reverted on 2026-05-15 after real-Pi bisect proved it
broke user-process execution while looking clean in the UART log. The
shipping milestone criterion ("no fault, reaches `proc_reap idle`") was
insufficient: every spawned user process silently failed before reaching
any of its own code, so `fbcon`, `pcie`, `psh`, `pl011-tty`, etc. never
ran. HDMI showed only the brown background plo had drawn.

Three independent failure paths surfaced under the previous design:

1. `process_load → process_validateElf64` randomly rejecting valid ELF
   headers depending on console-print timing (cache-coherency race
   between the kernel's cacheable temp ELF mapping and the source pages).
2. `process_load64` returning `-ENOMEM` from the user-stack `vm_mmap`
   on subsequent spawns (kernel page allocator state stale-cached
   across spawns).
3. `dummyfs-root` (PID 2) completing the kernel-side spawn cleanly and
   eret'ing to EL0 — but producing zero user output (likely stale user
   I-cache lines, or some related EL0 hazard).

The previous attempt tried to fix these by adding narrow targeted
invalidations (`amap_page()` invalidation, TLBI ISB hardening). It
worked around two specific paths but missed the underlying class of
problem. Bisect found `3d5c9574` alone is sufficient to break
userspace; the other two commits are partial follow-ups that didn't
close the gap.

## Reference: how Linux `__cpu_setup` works

Linux arm64 boot is the canonical reference. Key invariants in order:

1. **All page-table construction happens with MMU disabled.** Linux's
   `__create_page_tables` builds the entire `swapper_pg_dir` (kernel
   identity + kernel virtual + fixmap + early-printk) before any
   `SCTLR_EL1.M=1` write. The walker never observes a half-built
   table.

2. **Page-table memory itself is mapped cacheable, inner-shareable.**
   The walker accesses cacheable memory through the same cache
   hierarchy as ordinary CPU loads, so PT writes and walker reads stay
   coherent without explicit maintenance.

3. **TCR programs cacheable, inner-shareable walks** for both TTBR0
   and TTBR1 (`IRGN0/1 = ORGN0/1 = WBWA, SH0/1 = Inner Shareable`).
   This is the *default* shape; the previous Phoenix attempt set walks
   to Non-Cacheable/Non-Shareable as an experiment — Linux does not.

4. **`SCTLR_EL1.M | .C | .I` are turned on as a single MSR write** at
   the end of `__enable_mmu`, wrapped in barriers (`isb` before, full
   memory barrier sequence after). Linux never runs with caches
   partially on.

5. **No fancy NC-marking of the kernel image.** The kernel `.text`,
   `.rodata`, `.data`, BSS, stack, and the page tables themselves are
   all cacheable from the first instruction after MMU enable. The
   single exception is device-mapped MMIO regions.

6. **First user-text mapping does explicit PoU clean + I-cache
   invalidate.** When a new process is created and its text pages are
   loaded into the page cache (kernel D-cache from the file-read
   path), the loader runs `__flush_icache_range(start, end)` over the
   user-text VA range. That macro does
   `dc cvau; dsb ish; ic ivau; dsb ish; isb` per cache line — the
   canonical sequence to make freshly-written instructions observable
   to EL0 I-fetch.

7. **Free-page recycling invalidates the cache for the recycled PA.**
   When a page leaves the allocator's free list to become user-text,
   user-data, kernel heap, or DMA buffer, any stale lines from the
   previous owner are dropped before the new owner sees the page.
   Linux does this via `flush_cache_page()` or
   `__inval_dcache_area()`. With this discipline, every fresh mapping
   starts with a clean cache view of its PA.

8. **DMA-coherent allocations** for devices that don't snoop are
   handled by `dma_alloc_coherent` which maps the page non-cacheable
   on the CPU side. Other DMA uses explicit
   `dma_sync_single_for_*_device` calls that issue the appropriate
   `dc civac` / `dc ivac` over the buffer range. Phoenix has nothing
   equivalent yet, but xHCI rings will need it.

## Mapping each Linux invariant to Phoenix code paths

| Linux invariant | Phoenix file(s) to change | Today's state |
|---|---|---|
| 1. All PTs built pre-MMU | `hal/aarch64/_init.S` — restructure `_start` / `el1_entry` | Today the kernel writes its TTL2/TTL3 entries *after* MMU enable (TD-16 iter 10/11/12/13 attempts) |
| 2. PT memory cacheable | `hal/aarch64/_init.S` — drop the `NC_ATTRS` override on `pmap_common.kernel_ttl*` | Today `3d5c9574` made bootstrap PT memory non-cacheable as a workaround |
| 3. TCR walks cacheable + ISH | `hal/aarch64/_init.S` `TCR_EL1_VALUE` | Today (after revert) it's cacheable + ISH again — restored to baseline |
| 4. Single MSR M\|C\|I + barriers | `hal/aarch64/_init.S` — replace the existing M-only flip | Today only `SCTLR.M=1` is set |
| 5. No NC kernel-image | `hal/aarch64/_init.S` — keep `DEFAULT_ATTRS` for kernel `.text`/`.data` | Today (after revert) kernel mappings are cacheable |
| 6. First user-text PoU+ic ivau | `hal/aarch64/pmap.c` `_pmap_cacheOpAfterChange` | Today only does `ic ivau`. Linux does `dc cvau` *first* on every cache line, then `ic ivau` |
| 7. Free-page recycling flushes | `vm/page.c` `_page_free` / page-recycle path | No cache hygiene today; pages just go back on the free list |
| 8. DMA coherency | `vm/map.c` / vm_mmap MAP_UNCACHED + future `dma_map_*` API | xHCI driver currently has its own ad-hoc handling; needs a clean API |

The cache enable for Pi 4 will land all 7 of (1)-(7) before any
`SCTLR.C=1` flips. (8) can stay TODO — it only matters for DMA-heavy
drivers (xHCI, GENET) and we're not running those yet under cache.

## Implementation plan

Each step is a separate commit. After each step the kernel must still
boot to `(psh)%` on real Pi 4 (M-only or M|C as the step dictates).
After every commit: `./scripts/rebuild-rpi4b-fast.sh` then
`./scripts/test-cycle-netboot.sh --label c-stepN --capture-secs 240`
and `./scripts/uart-summary.sh c-stepN`. Manifest after every PASS.

### Step C-1 — Move kernel PT construction pre-MMU (`_init.S`)

Refactor `_start` / `el1_entry` so the sequence is:

```
# pre-MMU phase:
- compute pkernel, derive PMAP_COMMON_KERNEL_TTL{2,3}, *_DEVICES_TTL3,
  *_SCRATCH_TT, *_SCRATCH_PAGE addresses
- zero all of them via `_fill_page_zero`
- fill TTL2 (kernel + devices)
- fill KERNEL_TTL3 (kernel .text/.data via DEFAULT_ATTRS = cacheable)
- fill DEVICES_TTL3 (PL011 early VA, mailbox if applicable)
- TD-04 NC override for _hal_syspageCopied page (still needed)
- write TTBR0_EL1 (scratch_tt identity), TTBR1_EL1 (kernel_ttl2)
- write TCR_EL1 (cacheable, inner-shareable for both TTBRs)
- write MAIR_EL1, VBAR_EL1
- dsb ish; isb
# enable phase:
- SCTLR_EL1 |= M | C | I (single MSR)
- isb
# post-MMU phase:
- jump to kernel virtual address
- continue init
```

The current code does pieces of (1) post-MMU. The restructure is
mechanical but requires care: we have to avoid C function calls that
expect `sp` set up by the post-MMU phase. Today's `_fill_page_zero` /
`_fill_page_descr` are leaf assembly helpers — they work pre-MMU.

**Acceptance criterion:** kernel boots to userspace (matches today's
M-only baseline). Subsystems that used to require post-MMU PT writes
still work.

### Step C-2 — Land cacheable PT memory + cacheable kernel mappings

In `_init.S`, ensure:
- `PMAP_COMMON_KERNEL_TTL{1,2,3}` and `*_DEVICES_TTL3` reside in pages
  mapped with `DEFAULT_ATTRS` (cacheable inner-shareable).
- The kernel `.text`/`.rodata`/`.data`/BSS sections use
  `DEFAULT_ATTRS` in the `_fill_page_descr` call (already so after
  revert; explicitly re-verify).
- The 5-page `pmap_common` block stays accessible during early bring-up
  via the TTBR0 low-PA alias (which the C code uses while running at
  low PC). Keep that alias cacheable too (`DEFAULT_BLOCK_ATTRS`, not
  `NC_BLOCK_ATTRS`).
- Stack stays cacheable (already so).
- The TD-04 NC override for `_hal_syspageCopied` is the only NC
  exception kept.

**Acceptance criterion:** still boots M-only; explicit
`./scripts/test-cycle-netboot.sh --label c-step2-mc-off` shows the
same user-process output as the M-only baseline (sanity-check the
restructure didn't disturb).

### Step C-3 — Single-shot `SCTLR.M|C|I` enable

Replace the M-only flip with the canonical pattern:

```
ic   ialluis
dsb  ish
tlbi vmalle1is
dsb  ish
isb
mrs  x0, sctlr_el1
orr  x0, x0, #(1 << 0)     /* M */
orr  x0, x0, #(1 << 2)     /* C */
orr  x0, x0, #(1 << 12)    /* I */
msr  sctlr_el1, x0
isb
ic   iallu
dsb  nsh
isb
```

**Acceptance criterion:** kernel reaches `Phoenix-RTOS microkernel`
banner and successfully spawns `dummyfs-root` *with own UART output*
(`name: register /` or similar). If still silent, see C-4/C-5/C-6.

### Step C-4 — Per-user-text-mapping `dc cvau` + `ic ivau` in pmap

Edit `_pmap_cacheOpAfterChange` to do the **full** PoU sync, not just
`ic ivau`:

```c
if ((newEntry & (DESCR_PXN | DESCR_UXN)) == 0U) {
    hal_cpuCleanDataCacheToPoU(vaddr, vaddr + SIZE_PAGE);
    hal_cpuInvalInstrCache(vaddr, vaddr + SIZE_PAGE);
}
```

Add the helper if missing — Phoenix `cache.c` already has `dc cvac`
(to PoC); a PoU variant is one more inline asm wrapper.

**Acceptance criterion:** psh actually spawns and reaches a prompt,
HDMI shows text. Several spawn cycles in `psh` (`ls`, `cat`) work.

### Step C-5 — `dc ivac` on kernel ELF temp mappings

In `proc/process.c` `process_load`, after `vm_mmap` returns `ehdr`,
invalidate the D-cache lines for the freshly-mapped VA range so the
kernel's reads come from RAM (not from stale cache lines that
firmware/plo may have left for those PAs).

The earlier attempt at this faulted because `vm_mmap` with `MAP_NONE`
left some pages PTE-unmapped. Either:
- Force the mapping to be fully resident (extend `vm_mmap` to handle
  `MAP_POPULATE`-style eager mapping), OR
- Use the low-PA alias (kernel TTBR0 identity) to invalidate the PAs
  directly. The `o->paddr + base` calculation gives the PA range; we
  can issue `dc ivac` over the corresponding low-VA-of-PA addresses.

Pick whichever is simpler. The low-PA approach is more robust for
generic objects (any backing store).

**Acceptance criterion:** add diagnostic prints (or use the M|C|I
output as the signal) and verify `process_validateElf64` consistently
passes for every spawned binary on every boot.

### Step C-6 — Free-page recycle hygiene

In `vm/page.c`, wherever a page leaves the free list (allocator,
zone, anonymous map allocation), invalidate the D-cache for its PA
range. Wherever a page returns to the free list, clean+invalidate.
Two simple wrappers in `cache.c` (`hal_cpuPageReuseClean()`,
`hal_cpuPageReuseInval()`); call sites in `_page_free()` and
`_page_alloc()`. Skip for kernel-allocator pages that never leave the
kernel.

**Acceptance criterion:** subsequent spawns (PID 3+) no longer suffer
`-ENOMEM` from user-stack `vm_mmap` after the same kernel page was
recycled.

### Step C-7 — Real Pi 4 verification + manifest

Full netboot run, confirm:
- `fbcon: ok` shows up on UART
- `pcie:` produces its bring-up output (LinkUp, BAR programming,
  VL805 reset notification)
- `xhci:` capProbe runs (even if it fails ENODEV later — that's a
  separate USB-firmware issue, not cache)
- `psh: tty open / ready`
- `(psh)%` prompt appears on UART
- HDMI shows the same prompt as a kernel framebuffer console
- Interactive psh via `./scripts/test-cycle-psh-interact.sh` accepts
  `help` and replies

Manifest: `manifests/2026-05-15-cache-enable-c-approach.md` snapshots
the sibling SHAs of the passing image.

## Risk register

* **A72 erratum we still haven't applied.** The armstub now applies
  859971 and 1319367; TF-A's full SDEN list has more (832075, 853709,
  852421, etc.). If a future iteration of this design hits a
  walker-coherency or speculative-side-effect problem we haven't
  named, check the full SDEN list before chasing software issues.
* **The TD-04 NC override for `_hal_syspageCopied`.** This is kept
  because plo writes through the same PA and we can't change plo's
  side easily. Document it as a known asymmetry.
* **Pre-MMU PT writes are large.** With `pmap_common` covering 5 pages
  and KERNEL_TTL3 needing 512 entries plus 2-3 NC overrides, the
  pre-MMU init does many stores. With MMU off, those stores use
  default memory attribute (typically Device-nGnRnE per ARM ARM
  B2.6.2), which is strongly-ordered and slow. Acceptable for boot
  (one-shot).
* **No QEMU rpi4b verification possible.** The QEMU rpi4b machine
  model doesn't run the kernel through plo cleanly (PC=0x200 trap
  documented separately). Real-Pi UART is the only authoritative
  test. Each step requires a physical boot cycle.

## What this design explicitly does not do

* Re-enable the `vm/zone.c` cacheable backing pages experiment that
  failed three times in 2026-05-14. That work is parked behind a
  separate hypothesis (zone allocator's free-list pointer needs
  specific cache discipline before initialization).
* Touch the user/kernel context-switch path. Phoenix's context switch
  doesn't currently flush caches across processes; if I-cache aliasing
  becomes a problem after C-4 lands, it will manifest as inter-process
  text corruption and we'll add a `flush_icache_all` to the scheduler
  switch path then.
* Address USB+keyboard. That work is independent and can resume once
  the kernel is cache-enabled and userspace runs reliably.

## Estimated effort

* **C-1 + C-2**: half a day. Mostly mechanical refactor with careful
  review.
* **C-3 + C-4**: 2-4 hours total. Single-MSR change plus 2-line pmap
  edit, then physical verification.
* **C-5 + C-6**: 4-8 hours each. Low-PA invalidation path needs care
  to handle objects backed by non-DDR memory (mailbox region,
  initramfs).
* **C-7**: 1-2 hours for full validation + manifest.

Total: ~1.5 days physical work, spread across multiple netboot cycles.
