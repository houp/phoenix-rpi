# TD-19 is not theoretical: a missing `isb` halted the Pi on 2026-09-09

TD-19 has been carried for months as "validated, latent, no observed failure". It has one observed
failure. This is the evidence chain, every link checked rather than inferred.

## The artifact

`artifacts/rpi4b-uart/rpi4b-uart-20260909-005614-x-damagerows2.log`, an X11 damage-rows run:

```
x24=0000000000000001 x25=ffffffffc002b000 x26=0000000000000132 x27=0000000005332000
x28=0000000000000000  fp=ffffffffc4320a90  lr=ffffffffc000918c  sp=ffffffffc4320a90
psr=00000000600003c5  pc=ffffffffc000926c esr=0000000096000003 far=ffffffffc002f990
```

`esr=0x96000003` = EL1 data abort, **level-3 translation fault**, on `far=0xffffffffc002f990` — a
**kernel** VA. **The log ends 25 characters later.** The machine stopped.

## Symbolizing it — and why the first attempt was wrong

That boot ran kernel **`76e0adbc71d4`** (from its own `/etc/build-versions`). Resolving the `pc`
against *today's* kernel gives `pmap_enter`, which is close enough to be tempting and is **wrong**:
the same lookup puts `lr` in `pmap_destroy`, which cannot call it. Addresses had moved.

Built `76e0adbc71d4` in a worktree and resolved against that:

| | address | resolves to |
|---|---|---|
| `pc` | `0xffffffffc000926c` | **`_pmap_writeTtl3`, `hal/aarch64/pmap.c:459`** (inlined → `_pmap_enter:554` → `pmap_enter:566`) |
| `lr` | `0xffffffffc000918c` | `pmap_enter:566` — a coherent caller |

Both now agree, which is the check the first attempt failed.

## The faulting instruction, and why the address proves it

`pmap.c:459` in that revision is the **first read through the scratch mapping**:

```c
oldDescr = pmap_common.scratch_tt[idx];
```

The register file settles it: **`x26 = 0x132`**, and `0x132 × 8 = 0x990`, matching
`far = 0xffffffffc002f990`. The faulting address *is* `scratch_tt[idx]` at exactly the index held
in `x26`.

## The sequence

`_pmap_enter`, `pmap.c:549-553`:

```c
_pmap_mapScratch(pmap_common.scratch_tt, addr);   /* remaps the scratch VA */
tt = pmap_common.scratch_tt;
_pmap_writeTtl3(vaddr, pa, attr, asid);           /* :459 dereferences it -> FAULT */
```

and `_pmap_mapScratch` (`pmap.c:190-201`) ends:

```c
ttl3[TTL_IDX(3U, va)] = DESCR_PA(pa) | DESCR_VALID | ...;
hal_cpuDataSyncBarrier();                 /* dsb */
__asm__ volatile("tlbi vaale1, %0" : : "r"(tlbiArg));
hal_cpuDataSyncBarrier();                 /* dsb -- and nothing after */
```

No `isb`. The translation of the scratch VA is changed and the very next instruction stream
dereferences it. Per ARM ARM D8.16.1 the sequence is not complete for the PE that will *use* the new
translation until a context-synchronization event, and there is none: the caller holds
`pmap_common.lock`, whose `hal_spinlockSet` does `msr daifSet, #3` (`hal/aarch64/spinlock.c:25-68`),
so interrupts are masked and no exception entry/return can supply one incidentally.

## How strong is this

**Strong, and one occurrence.** Everything above is read, not inferred: the SHA from the log, the
symbols from a build of that SHA, the faulting address from `x26`, the sequence from the source at
that revision.

⚠ **Not excluded:** that `_pmap_mapScratch` wrote a genuinely invalid descriptor for another reason,
which would fault identically. Nothing in the log distinguishes those. What can be said is that the
signature is exactly the one the missing `isb` predicts, at exactly the instruction it predicts, in
a sequence with no context synchronization.

⚠ **This is also site A, not site B.** The earlier analysis flagged `_pmap_writeTtl3`'s
invalid→valid kernel path as the likely exposure. The fault is in the *scratch mapping* instead —
which matters, because `_pmap_mapScratch` uses a **raw inline `tlbi`**, not one of the five
`hal_tlbInval*` helpers. **Adding `isb` to those five helpers would not have prevented this fault.**

## What follows

The targeted fix (`docs/TEMPORARY-FIXES-AND-FUTURE-CLEANUP.md`, TD-19) is now justified by evidence
rather than by the architecture manual alone:

1. `pmap.c:201` — `isb` after the second `dsb` in `_pmap_mapScratch`, **and** break-before-make
   (that site also changes a valid→valid output address without it).
2. `pmap.c:511` — `isb` gated on `va >= VADDR_KERNEL`, mirroring Linux's `pte_valid_not_user`.
3. `pmap.c:987`, `pmap.c:433` — cheap, one boot-time and one effectively unreachable.

⊖ Rate: **one occurrence in the whole archive** (265 distinct `far=` values; only two are kernel VAs
and the other is a deliberate use-after-free injection). So this is rare — but its outcome is a
**halt**, not a recoverable fault.
