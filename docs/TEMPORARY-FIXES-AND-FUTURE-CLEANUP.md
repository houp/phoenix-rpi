# Temporary Fixes and Future Cleanup

This document is the registry of transitional shortcuts and workarounds
accepted during the Raspberry Pi 4 bring-up. Each item has a stable ID
(`TD-NN`) used to link from source code, commits, and future cleanup steps.

Ordering rule: once the Pi 4 boots to a usable state, every item here becomes
mandatory cleanup. Until then, progress on the boot path takes priority.

## Conventions

- **IDs are stable.** Never re-number. If an item is merged into another,
  add a `merged into TD-NN` note rather than deleting the entry.
- **Status** is one of:
  - `PENDING` — shortcut still active in source
  - `IN-PROGRESS` — cleanup step open against it
  - `RESOLVED` — cleanup committed and validated, record kept as history
- **Linking from source.** Every transitional fix in upstream source should
  carry an inline marker: `TODO(TD-NN): <short hint>`. Grep for `TD-NN` to
  find all sites of a given shortcut.
- **Location snapshots may drift.** Line numbers in this file reflect state
  at the time the entry was written. Re-verify against current source before
  acting — the code changes faster than this doc.

---

## TD-01: SMP enable disabled on Cortex-A72

- **Status:** PENDING
- **First observed:** 2026-04 bring-up
- **Where:** `sources/phoenix-rtos-kernel/hal/aarch64/_init.S`, the
  `CPUECTLR_EL1` SMPEN block behind `__TARGET_AARCH64A72`.
- **What was done:** The SMP-enable MSR sequence is commented out and the
  only remaining effect is the debug markers around it.
- **Why:** Enabling SMP on A72 produced an early-boot hang; cause not
  diagnosed yet.
- **Risk accepted:** A72 coherency behavior with Inner-Shareable memory is
  undefined without this bit. Current code avoids Inner-Shareable in early
  boot, which is itself a related transitional compromise.
- **Resolution requirements:**
  - Reproduce the hang with a bounded diagnostic (GDB over QEMU gdbstub, or
    one minimal marker pair) and capture the fault.
  - Follow the Cortex-A72 TRM SMP enable sequence; compare against Circle OS
    and similar bare-metal references.
  - Re-enable SMP, then switch early boot back to Inner-Shareable and
    confirm boot on real hardware across multiple cold resets.

## TD-02: Pre-MMU cache invalidation disabled

- **Status:** PENDING
- **First observed:** 2026-04 bring-up
- **Where:** `sources/phoenix-rtos-kernel/hal/aarch64/_init.S`, the
  `PMAP_COMMON_KERNEL_TTL2 … PMAP_COMMON_STACK` `_inval_dcache_range` call
  before MMU enable.
- **What was done:** The pre-MMU data-cache invalidation sweep is commented
  out. The code now relies solely on the post-MMU-enable invalidation and
  on `dsb ish; isb` to make table writes visible.
- **Why:** Cache maintenance with the MMU disabled hung the board in
  observed runs. Linux arm64 performs this sweep unconditionally.
- **Risk accepted:** Speculatively loaded stale lines for the page-table
  regions can survive into early MMU walks. So far no observed corruption,
  but that is not a guarantee.
- **Resolution requirements:**
  - Identify the A72-specific precondition that makes the generic sequence
    hang (likely ordering or an earlier missing setup step).
  - Restore the invalidation, or document the exact reason a narrower form
    is correct for this platform.

## TD-03: Syspage copy / BSS mapping shortcut

- **Status:** PENDING
- **First observed:** 2026-04 bring-up
- **Where:** Interaction between `hal/aarch64/_init.S` (virtual syspage
  copy) and `syspage.c` (syspage access after MMU enable). BSS region is
  not reliably mapped in the early MMU page tables.
- **What was done:** Per `docs/status.md`, syspage access was stabilized by
  side-stepping the copied-into-BSS location and working with the original
  syspage. Intent and current source may diverge: **verify before acting.**
- **Why:** The early MMU page tables did not cover the BSS region into
  which the syspage was being copied.
- **Risk accepted:** Any code path that assumes the copied virtual syspage
  is authoritative may read stale data or wrong addresses.
- **Resolution requirements:**
  - Extend early MMU setup to map the BSS region (or move the syspage copy
    target to an already-mapped region).
  - Re-enable the canonical syspage copy path and validate that every
    consumer reads from the virtual location.
  - Add a syspage integrity check (size and a simple checksum) to the
    post-copy path.

## TD-04: Entry-relocation loop stabilized (circular-list mitigation)

- **Status:** PENDING
- **First observed:** 2026-04 bring-up
- **Where:** `sources/phoenix-rtos-kernel/syspage.c`, `syspage_init()` map
  and entry relocation loops.
- **What was done:** The entry loop now saves the pre-relocation pointer as
  `original_entries` and terminates against it, and relocates
  `map->entries` twice (once directly, once through the saved pointer),
  which is unusual. Program-relocation loop currently hangs at marker `o`.
- **Why:** An earlier version of the loop diverged because the termination
  pointer was itself being relocated inside the loop.
- **Risk accepted:** Double-relocation of `map->entries` is fragile and may
  corrupt state under non-canonical syspage layouts.
- **Resolution requirements:**
  - Diagnose the program-relocation hang (current blocker).
  - Replace the double-relocation with a clean pre-capture pattern, or hoist
    the invariant into a small helper shared by map and program loops.
  - Add defensive validation (null, self-referential head) so the loop
    fails loudly on malformed syspage rather than looping forever.

## TD-05: UART debug-marker scaffolding

- **Status:** PENDING
- **First observed:** 2026-04 bring-up (pervasive)
- **Where:** `hal/aarch64/_init.S`, `syspage.c`, `main.c`, and related
  boot-path files.
- **What was done:** Dozens of `uart_putc` and `uart` ring-buffer writes
  scattered through the early boot path to produce the
  `NYOPSTUZbcdeFGVWXabcdefgmklmno` progress trace.
- **Why:** The trace is how we locate hangs when no other diagnostic is
  available; there is no working console or fault reporting yet.
- **Risk accepted:** The markers affect boot timing, burn UART bandwidth,
  and make diffs noisy. Individual markers are easy to leave behind once
  they served their purpose.
- **Resolution requirements:**
  - Replace ad hoc markers with a compile-time-gated debug macro
    (`RPI4_BOOT_MARKER(c)`) so they can all be disabled in one place.
  - Establish a rule that every marker added to test a hypothesis is
    removed when the hypothesis is disproved (already in
    [code-quality-and-upstreaming.md](code-quality-and-upstreaming.md)).
  - Before upstreaming, strip or gate every remaining marker.

## TD-06: DTB handling assumptions

- **Status:** PENDING
- **First observed:** 2026-04 bring-up
- **Where:** `sources/phoenix-rtos-kernel/hal/aarch64/dtb.c`.
- **What was done:** Early parsing assumes a fixed memory layout, a single
  known interrupt controller, and limited error paths.
- **Why:** Early bring-up needed a DTB path with no surprises; robust
  parsing was not on the critical boot lane.
- **Risk accepted:** Any future board variant or firmware change silently
  reuses the fixed assumptions.
- **Resolution requirements:**
  - Drive memory layout from the actual DTB, not compile-time constants.
  - Validate required nodes at parse time and fail with a useful message.
  - Add multi-board support (Pi 4B variants, Pi 5) as the scope expands.

---

## Priority Ladder

**Blocks "first Pi 4 boots to userspace" milestone:**
- TD-04 (currently active — hang at marker `o`)
- TD-03 (unblocks proper virtual syspage access)

**Blocks upstream-ready quality:**
- TD-05 (debug-marker strip/gate)
- TD-01 (SMP enable, required for anything beyond single-core)

**Medium-term:**
- TD-02 (cache invalidation correctness)
- TD-06 (DTB robustness, portability)

## Tracking Checklist

| ID | Status | Blocker? |
| --- | --- | --- |
| TD-01 | PENDING | multi-core work |
| TD-02 | PENDING | stability risk |
| TD-03 | PENDING | milestone |
| TD-04 | PENDING | active step |
| TD-05 | PENDING | upstream quality |
| TD-06 | PENDING | portability |

When resolving an item:

1. Create a `tracking/current-step.md` scoped to that single ID.
2. Remove the corresponding `TODO(TD-NN):` marker(s) from upstream source.
3. Commit the upstream repo change and snapshot an integration manifest.
4. Flip the status to `RESOLVED` in this file with the commit SHA and date.
