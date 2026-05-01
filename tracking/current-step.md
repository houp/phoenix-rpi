# Current Implementation Step

## Step: Diagnose `/dev/console` `resolve_path` Hang on Pi 4 (TD-14)

**Status**: IN PROGRESS

**Date**: 2026-05-02

**Manifest**: `manifests/2026-05-02-td13-resolve-path-boundary.md`

**Sibling commits at this checkpoint**:
- kernel `agent/rpi4-program-reloc` @ `37fcc58e` — TD-13 mutex/atomic wall
  fixed (DAIF-masked plain atomics for `__aarch64__ && NUM_CPUS == 1`),
  byte probes removed, `vm_mapBelongs()` validation restored.
- devices `master` @ `7929591` — `pl011_fbcon_init()` deferred from
  `pl011_init()` to `main()` *after* kbd/pool threads start; lets
  `/dev/ttyUL0` register before fbcon touches the GPU mailbox.
- libphoenix `master` @ `fd8d243` — `crt0/_libc_init/open` debug() trace
  (skips `stat()` pre-check on `/dev/console` while debugging ttyopen).
- utils `master` @ `c787d3b` — psh main/ttyopen/run debug() trace.

## 2026-05-02 Update — TD-13 closed, TD-14 active

**TD-13 RESOLVED.** All fixes for the post-spawn user-mode silence
have landed and validated:

1. `_hal_syspageCopied` mapped Normal Non-Cacheable (kernel
   `cff18d49`, the original TD-04 NC-dest fix).
2. Hard cap on the spawn loop in `main()` (kernel `c5c21c6e`,
   TD-13-spawn-cap, ~32 iterations).
3. **Single-core AArch64 atomic fallback** (kernel `23b9a127`).
   `lib_atomicIncrement/Decrement` use DAIF-masked plain updates
   when `defined(__aarch64__) && NUM_CPUS == 1`. Mirrors the TD-11
   spinlock approach. The exact wall was inside
   `proc_mutexCreate -> resource_put -> lib_atomicDecrement(&r->refs)`,
   which expanded to `__atomic_fetch_sub` (LDXR/STXR exclusives) and
   never returned on real BCM2711 silicon.
4. Probe cleanup (kernel `37fcc58e`) — removed M/1/2/3/E/K, sNN,
   a..f, `*15`, and `>` byte probes; restored `vm_mapBelongs()` in
   `syscalls_phMutexCreate`.
5. `pl011_fbcon_init` deferred (devices `7929591`).

**TD-14 (NEW ACTIVE BLOCKER).** With those landed, real Pi 4 boot
now reaches:

```text
main: spawned psh (10)
threads: psh user scheduled
psh: main enter
psh: keepidle done
psh: root lookup done
psh: tcgetpgrp done
psh: basename done
psh: findapp enter
psh: app run enter
pshapp: run enter
pshapp: sleep done
pshapp: tty loop enter
pshapp: ttyopen attempt
psh: ttyopen open enter
open: console enter
open: console stat skipped
open: console resolve enter
[silence]
```

Reference log:
`artifacts/rpi4b-uart/rpi4b-uart-20260501-220933-netboot-console-open-skip-stat.log`

**The wall is libphoenix `resolve_path("/dev/console", NULL, 1, 1)`**
inside `psh_ttyopen()`. `resolve_path` walks the path component-by-
component via namespace-server IPC (sys_lookup → bind/devfs). One
of those IPC round-trips is hanging.

QEMU smoke still reaches `(psh)% help` interactively, so the QEMU
namespace path works. Difference is BCM2711-only: candidates are
(a) bind/devfs IPC port not actually serving on hardware,
(b) IPC blocked because `pl011-tty` registered late or under wrong
parent oid (deferred fbcon side-effect?),
(c) another TD-04-class cache-coherency issue on a message-queue or
oid table backing memory.

## Next action

One short netboot test cycle with one targeted instrumentation:
add a debug() print inside libphoenix `resolve_path` per
component (and in `sys_lookup` shim) to see which path component
the lookup hangs on. Decision tree:

- Hangs on `/` → root oid resolution (init thread / posix port).
- Hangs on `dev` → bind didn't bind devfs at `/dev`, or bind itself
  is unresponsive.
- Hangs on `console` → devfs is up but the `console` entry was
  never registered (pl011-tty hasn't bound /dev/console yet).

Independently verify on the same log whether `pl011-tty:
fbcon init deferred ok/unavailable` and `pl011-tty: console ready`
print — they tell us whether pl011-tty's main thread reached the
post-init phase at all.

## Build/test commands

```bash
./scripts/rebuild-rpi4b-fast.sh
./scripts/qemu-shell-smoke.sh rpi4b
./scripts/test-cycle-netboot.sh --label td14-resolve-path-trace --capture-secs 120 --dhcp-wait-secs 120
python3 scripts/summarize-rpi4b-uart-log.py artifacts/rpi4b-uart/<latest>.log
```

## 2026-05-01 Update — TD-13 probe cleanup validated

The TD-13 single-byte probes were removed from:

- `syscalls_dispatch()` (`sNN` syscall-number stream)
- `syscalls_phMutexCreate()` (`M/1/2/3/E/K` stream)
- `proc_mutexCreate()` (`a..f` stream)
- AArch64 exception/user-entry assembly (`*15` EC stream and `>` pre-`eret`)

The temporary `TD-13-mtxbypass` in `syscalls_phMutexCreate()` was removed and
both user pointers are again validated with `vm_mapBelongs()`.

Validation evidence:

- Build/export/verify: OK, image SHA256
  `03e1988da8390512df2737d8efaa9b994725cd9873e465f318910af5e1ea6f0d`
- QEMU: `./scripts/qemu-shell-smoke.sh rpi4b` reaches `(psh)% help`
- Real Pi:
  `artifacts/rpi4b-uart/rpi4b-uart-20260501-214225-netboot-td13-clean-probes.log`
  reaches `dummyfs: root initialized`, `pl011-tty: init: libtty_init ok`,
  `main: spawned psh (10)`, and `threads: psh user scheduled`

Result:

- Restoring `vm_mapBelongs()` did **not** reintroduce the mutex wall. The
  single-core AArch64 atomic fallback is enough for this path.
- The remaining blocker is no clean `(psh)%` prompt on real hardware after
  `psh` is scheduled.

Warnings observed:

- The first DHCP wait failed after the laptop/NIC reconnect. The canonical
  helper restarted the Lima/socket_vmnet bridge, then DHCP/TFTP succeeded.
- Timed `picocom` capture ended with watchdog SIGTERM/exit 143, expected.

Next action:

- Instrument the next smallest post-`psh` boundary with readable logs:
  `psh` startup, fd/devfs lookup, tty open, and first blocking read/write.
  Prefer normal console logs or existing debug helpers over raw byte probes.

## 2026-05-01 Update — TD-13 atomic wall fixed

The planned `a..f` probes were added to `proc_mutexCreate` and validated on
real Pi 4:

```text
pre-fix:  M12abcde       (hang before f/3)
post-fix: M12abcdef3K    (resource_put returns, phMutexCreate succeeds)
```

The exact wall was `resource_put(p, &mutex->resource)`, which reduces to
`lib_atomicDecrement(&r->refs)`. Kernel commit `23b9a127` changes
`lib_atomicIncrement/Decrement` only for `defined(__aarch64__) &&
NUM_CPUS == 1` to use DAIF-masked plain updates instead of GCC
`__atomic_*` builtins. This mirrors the already validated single-core AArch64
spinlock path and leaves multicore/other-architecture builds unchanged.

Validation evidence:

- Build/export/verify: OK, image SHA256
  `3e89b7c2c738892b5d71f03460e2fe026e0f0099cdb0cdec0b9749182e2e588b`
- QEMU: `./scripts/qemu-shell-smoke.sh rpi4b` reaches `(psh)% help`
- Real Pi:
  `artifacts/rpi4b-uart/rpi4b-uart-20260501-191724-netboot-td13-atomic-fallback.log`
  reaches `dummyfs: root initialized`, `pl011-tty: init: libtty_init ok`,
  `main: spawned psh (10)`, and `threads: psh user scheduled`

Current remaining boundary:

- No clean `(psh)%` prompt appears yet on real hardware.
- The active TD-13 single-byte probes now heavily interleave with useful
  console output, so the next step should clean or gate them before drawing
  conclusions about shell/console behavior.

Warnings observed:

- Firmware-side USB probing emitted repeated `xHC-CMD err: 19/36 type: 11`
  before netboot. This is pre-Phoenix and did not block the run.
- Timed `picocom` captures end with watchdog SIGTERM/exit 143, expected.
- A first validation attempt used too short a capture window and ended before
  DHCP; use 100+ second capture windows for netboot runs.

## 2026-05-01 Update — TD-13 narrowed

The previous "user processes are silent after eret" framing has been
replaced by hard data:

1. Every user process that gets dispatched (eret, `>` marker) does
   take a real EL0-source synchronous exception with EC=0x15
   (AArch64 SVC) — `*15` probe.
2. The first (and only) syscall any user process makes is #16 =
   `phMutexCreate`, called from libphoenix `_errno_init`'s
   `mutexCreate(&errno_common.lock)` — `s16` probe.
3. Inside `syscalls_phMutexCreate`, every process reaches the
   `proc_mutexCreate(attr)` call (markers `M`, `1`, `2` — the
   last only printed because TD-13-mtxbypass skips the
   `vm_mapBelongs` validation), and **none** ever returns from
   it (no `3`, no `K`, no `E`).
4. `dummyfs` (pid 3) never even reaches the SVC — it has no `>`,
   `*`, `s`, or `M` event in the log. Either it never runs, or
   it crashes between eret and its first SVC. Out of scope for
   the immediate next probe.

So the wall is one of these four calls inside `proc_mutexCreate`
(`sources/phoenix-rtos-kernel/proc/mutex.c:51-80`):

```text
mutex = vm_kmalloc(sizeof(*mutex));   /* candidate a */
id    = resource_alloc(p, &mutex->resource); /* candidate b */
proc_lockInit(&mutex->lock, attr, "user.mutex"); /* candidate c */
resource_put(p, &mutex->resource);    /* candidate d */
```

`vm_kmalloc` is the front-runner suspect (TD-04-class: heap
free-list traversal touching uncached/stale memory after the
syspage handoff). `proc_lockInit` is a close second (initializes a
spinlock — see TD-11 single-core spinlock).

## Build/test commands

```bash
./scripts/rebuild-rpi4b-fast.sh
./scripts/qemu-shell-smoke.sh rpi4b
./scripts/test-cycle-netboot.sh --label psh-console-boundary --capture-secs 120 --dhcp-wait-secs 120
python3 scripts/summarize-rpi4b-uart-log.py artifacts/rpi4b-uart/<latest>.log
```

## Historical commits leading here

- pending: Pi 4 SError masking, single-core spinlock fix, and post-spawn diagnostics
- d1996d8f: Fix infinite loop in entry relocation by using original_entries for loop condition
- aff01622: Add more detailed markers in entry loop to diagnose infinite loop
- 2f0b391f: Add detailed debug markers in map relocation loop to pinpoint crash location
- d609a196: Add Y marker at end of syspage_init() to confirm completion
- b62fe368: Add W and X debug markers to syspage_init() to diagnose crash point

**Note**: Broken commits (1bb7f806, 1c6a5267, 5e74c3c9) have been removed from history

## 2026-04-30 Update

Latest hardware result:

- Image SHA256: `3dc62d31c1469955ee462f7a0279cc4f570e7fcb57d71fc50ceb2686e1aec447`
- UART log: `artifacts/rpi4b-uart/rpi4b-uart-20260430-214456-netboot-spawn-names.log`
- QEMU smoke reaches `psh help`
- Real Pi reaches `main: spawned psh (9)`
- No `Exception`, `SError`, or instruction abort appears in the latest real-device log

Confirmed fixes:

- Single-core AArch64 spinlocks no longer depend on exclusive byte atomics.
- SError is kept masked across thread creation, IRQ dispatch, syscall/exception
  C dispatch, and direct `hal_jmp()` user entry.
- First scheduling happens before timer IRQs are enabled in the bootstrap
  context.

Current boundary:

```text
main: spawned dummyfs-root (2)
main: spawned dummyfs (3)
main: spawned pl011-tty (4)
main: spawned mkdir (5)
main: spawned bind (6)
main: spawned pcie (7)
main: spawned usb (8)
main: spawned psh (9)
```

Next action: instrument or debug post-spawn scheduling and console handoff.
The immediate question is whether `psh` is scheduled on hardware and whether
the shell is blocked waiting for `pl011-tty`/`devfs`/`/dev`.

Automated netboot/power/UART testing is now available and was used for the
current diagnostic loop:

```bash
./scripts/rebuild-rpi4b-fast.sh
./scripts/test-cycle-netboot.sh --label <label> --capture-secs <seconds>
```

Latest verified image in this step:

- SHA256: `07d40d5e1a197f4c3e763ac3368f475d774a7f1596cb9452073133673a970032`
- UART log: `artifacts/rpi4b-uart/rpi4b-uart-20260430-150524-netboot-nic-refresh-baseline.log`

Current marker boundary:

```text
... kllmnPYfhR
```

Meaning:

- `Y`: `syspage_init()` completed.
- `f`: `main()` reached the point immediately before `_hal_init()`.
- `h`: assembly `_hal_init` wrapper was entered before the C prologue.
- `R`: wrapper reset `SP` to the initial kernel stack.
- No later `q/w/u/W` marker appears, so the first direct data-store probe after
  `R` does not complete.

An inline exception-vector marker confirmed that the CPU takes an exception
after `R`; the normal exception path then faults recursively because it saves
state on the same suspect stack. A later attempt to print a clean ESR marker and
halt did not produce a clean ESR in the final capture, so the current confirmed
fact is exception-after-`R`, not yet a decoded syndrome.

Tool/process warnings observed during this step:

- First netboot DHCP attempt often times out; `test-cycle-netboot.sh` recovers
  by restarting the Lima/socket_vmnet bridge and then DHCP succeeds.
- `tio` has no timed capture mode in this setup; the canonical helper falls
  back to `picocom`.
- `picocom` can briefly leave `/dev/cu.usbserial-201310` locked after a capture;
  verify with `lsof /dev/cu.usbserial-201310` before rerunning if capture fails.
- `picocom --exit-after` did not terminate cleanly when the DUT emitted a very
  large stream of blank lines during failed `X3` experiments; the stuck capture
  had to be killed so the test-cycle trap could power the Pi off.
- The QEMU smoke helper previously returned success after timing out before a
  shell prompt; it has been tightened so `expect` timeout/eof paths exit
  nonzero.
- After that fix, `./scripts/qemu-shell-smoke.sh rpi4b` correctly exits `124`.
  The QEMU log reaches `STUZb!e{86000005}`. Decoding `ESR_EL1=0x86000005`
  gives an instruction abort from the same exception level with a level-1
  translation fault. That is strong evidence that QEMU is exposing the
  low-PC/invalid-TTBR0 problem immediately after the branch to `main`, while
  real Pi 4 hardware continues further only because stale translations or
  cached instruction state mask the bug for longer.
- Firmware HDMI1 EDID warnings are expected when only HDMI0 is connected.
- `prepare-rpi4b-dtb.sh` now defaults to copying the final-form upstream DTB
  without decompiling/linting it; set `RPI4B_DTB_LINT=1` for explicit DTB audits.

Failed experiments on 2026-04-30, both reverted before the baseline rebuild:

- Keeping the bootstrap `TTBR0_EL1` identity map active in the primary path
  regressed real hardware from `...YfhR` to `A2 ... X1 X2 X3`.
- Loading the normal exception vector base through the linked high VA also
  regressed real hardware from `...YfhR` to `A2 ... X1 X2 X3`.

Current conclusion: the real board still cannot reliably fetch instructions or
exception vectors through the TTBR1 higher-half kernel mapping. The next useful
work item is not more local `_hal_init` probing, but a focused rebuild of the
MMU transition so it matches the Linux/bare-metal pattern: keep identity fetch
valid, prove the higher-half executable mapping, branch once to the linked high
VA, then retire the identity path.

### Major Achievement
The system has successfully completed all map relocation in syspage initialization! This represents a massive milestone in the Raspberry Pi 4 bring-up process.

### Current Progress
**Last Working Markers**: `NYOPSTUZbcdeFGVWXabcdefgmklmno`

The system now:
- ✅ Completes early boot sequence
- ✅ Sets up MMU and virtual memory
- ✅ Enters syspage_init() successfully
- ✅ Validates syspage pointer
- ✅ Processes all map entries with relocation
- ✅ Reaches program relocation phase (marker `o`)

### Current Blocking Issue
The system hangs at marker `o` (starting program relocation). This is likely due to a circular linked list issue similar to what was encountered in entry relocation.

### Next Actions

#### Immediate (Current Blocker)
1. **Add strategic debug markers** around `hal_syspageRelocate(syspage_common.syspage->progs)` call:
   - Check if `syspage_common.syspage->progs` is NULL before calling `hal_syspageRelocate`
   - Add markers before and after the `hal_syspageRelocate` call
   - Test with enhanced debugging to identify exact failure point

2. **Determine root cause** of the hang at marker `o`:
   - NULL pointer dereference
   - Infinite loop in `hal_syspageRelocate`
   - Circular linked list in program entries
   - Memory access violation

3. **Implement targeted fix** based on diagnosis:
   - If circular list: Implement proper validation and termination
   - If NULL pointer: Add proper NULL check and error handling
   - If memory issue: Fix memory mapping or access pattern

#### Short-term (After Unblocking)
1. **Reach marker `Y`**: Complete syspage_init() successfully
2. **Reach marker `f`**: Enter _hal_init() successfully
3. **Complete syspage initialization** and enter HAL initialization phase

#### Medium-term (Next Milestones)
1. **Fix SMP enable** for multi-core support
2. **Re-enable syspage copy** with proper BSS mapping
3. **Implement proper cache management** for Cortex-A72
4. **Consolidate debug infrastructure** into configurable system

### Technical Details
- **Current Image SHA256**: `fecdc6b78fc1d55e4198ad27e34eee53dd866be6497894a11b64c7184344ccab`
- **UART Log**: `artifacts/rpi4b-uart/rpi4b-uart-20260419-104437.log`
- **Test Command**: `./scripts/capture-rpi4b-uart.sh`

### Success Criteria
- ✅ System reaches marker `o` (starting program relocation)
- ⏳ System reaches marker `Y` (end of syspage_init())
- ⏳ System reaches marker `f` (before _hal_init())
- ⏳ System completes syspage initialization and enters HAL initialization

### Rollback Information
Previous working commit: `b0a93572` (reverted problematic debug marker before TLB invalidation)
