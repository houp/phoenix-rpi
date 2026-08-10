# Quake3 (C5) VM-exec — re-characterized 2026-08-10

The board had C5 banked at: *"JIT'd QVM faults (Data Abort, far=0x10014329f = VM
offset 0x1432a0 with a stray bit-32) — a codegen bug in vm_aarch64.c's
dataMask/dataBase address computation."* This heartbeat's investigation changes
that on three counts.

## 1. The JIT codegen is NOT the bug (static analysis)

- `emit_MOVXi` (vm_aarch64.c:714) that loads `rDATABASE` is **correct**: for a
  `dataBase < 4GB` it emits `MOVZ64`(bits 0-15, which zeroes the whole X reg) +
  `MOVK64_16`(16-31) and returns, leaving bits 32-63 = 0. The `emit_MOVXi64`
  variant used for `rLITBASE` is only a *fixed-size* (always-4-instruction)
  encoding, not a correctness fix — so the load-vs-fixed inconsistency is a
  red herring, not the bit-32 source.
- Data accesses mask via `AND32(reg, rDATAMASK)` (zeroes the upper 32 bits) then
  `LDR32/STR32(rDATABASE, offset)` — correct for a valid 64-bit `dataBase`.
- So the JIT's dataBase/mask/access codegen does not introduce bit-32.

## 2. The 2026-08-05 fault was a WRITE in ENGINE code, not the JIT (advisor)

- `esr=0x92000045` → **WnR=1 (write)**, DFSC=0x05 (L1 translation fault). Prior
  notes called it a "read"; it is a store.
- `pc=0x402c48` is ~4 MB = low static `.text` = **engine C code**, not the JIT'd
  RWX mmap (which Phoenix places elsewhere). So the fault was engine code
  dereferencing a VM-translated pointer (the `VM_ArgPtr`/`VM_BlockCopy`/syscall-
  arg class), not JIT-emitted load/store.
- Runtime values captured (Q3JIT-DIAG, HW): `dataBase=0x0b09eb80` (LOW, bit-32=0),
  `dataMask=0x000fffff`, `dataAlloc=0x100400`. Discriminator result:
  `far=0x10014329f = 0x100000000 | 0x0014329f` — the low part `0x14329f` is
  **unmasked** (> dataMask 0xFFFFF, and > dataAlloc, i.e. OOB), `dataBase` is
  **absent**, and a lone bit-32 is present. That is an engine-side translation
  that dropped the base + skipped the mask + gained bit-32 — NOT a masked JIT
  access (those are always `& 0xFFFFF`, ≤ in-bounds).

## 3. The current build no longer reproduces the Data Abort (HW, 2 runs)

A fresh rebuild from the current `external/quake3e` + patch (plus a 2-line
Q3JIT-DIAG `Com_Printf`) boots Q3 **past** `VM_Compile(ui)` to the interactive
tty console (`]`) + IPv4 socket, with **no Data Abort anywhere in the log**, in
two cycles (q3diag 200 s, q3diag2 290 s). So the headline VM-exec fault does not
reproduce here.

**BUT the screen is BLACK** — the UI menu does not render. The log shows a **V3D
GPU wedge during R_Init**: `v3d-winsys: BIN TIMEOUT ... GPU wedged — true reset +
drop this frame (HW-marginal depth-pipeline drain stall)`. So the live blocker
moved from a VM-exec Data Abort to a **rendering** failure (GPU wedge → black),
with the engine otherwise up.

### Honest caveats (do not over-claim "fixed")

- 2 runs of an **instrumented** build. Not yet re-verified with pristine source,
  so no-fault cannot be cleanly attributed to source-drift vs a `.text`-layout
  shift from the added prints vs the fault being intermittent.
- The `bad opStack 8` warning at `VM_Compile(ui)` (jump target 11, instr 13586)
  still prints — the VM validation still flags it, but it is a warning, not fatal.
- The GPU wedge is tagged "HW-marginal"; whether the black screen is the wedge or
  the UI VM not drawing is unconfirmed.

## Next steps (fresh session)

1. Re-verify with **pristine** source (strip the Q3JIT-DIAG prints, rebuild, one
   cycle) to confirm the no-Data-Abort baseline is real, not layout-luck.
2. If stable: the blocker is the **R_Init GPU wedge** → investigate the V3D
   depth-pipeline drain stall (a HW-marginal winsys issue, shared with the
   vkQuake/quakespasm render paths) rather than the VM. Check whether the UI VM's
   draw calls reach the winsys at all (add a one-frame present log).
3. If the Data Abort returns intermittently: instrument the engine-side
   `VM_ArgPtr`/`VM_BlockCopy`/syscall pointer translation (vm.c ~241-275) to log
   the un-translated vs translated pointer and catch the bit-32 leak at its site.

Q3JIT-DIAG instrumentation is currently left in `external/quake3e` (local clone,
uncommitted) for step 1.
