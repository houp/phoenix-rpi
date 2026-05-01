# Phoenix-RTOS Raspberry Pi 4 Port Status

## Current Status: 2026-05-01

**Current blocker**: TD-13 `proc_mutexCreate` hang was fixed by avoiding
exclusive-access atomics on the current single-core AArch64 target, but the
real Pi still does not show a clean shell prompt. The latest hardware log now
reaches `threads: psh user scheduled`, so the next boundary is post-`psh`
console/stdin/stdout or a later syscall, not initial process creation.

Latest verified image:

- Integration manifest: `manifests/2026-05-01-td13-atomic-fallback.md`
- Kernel: `agent/rpi4-program-reloc` @ `23b9a127` (single-core AArch64
  atomic fallback plus TD-13 mutex substep probes)
- Devices: `master` @ `8984455` (TD-13 pl011-tty progress markers)
- UART log: `artifacts/rpi4b-uart/rpi4b-uart-20260501-191724-netboot-td13-atomic-fallback.log`
- QEMU smoke: still reaches `(psh)% help` interactively.
- Image SHA256: `3e89b7c2c738892b5d71f03460e2fe026e0f0099cdb0cdec0b9749182e2e588b`

Real-device boundary on `td13-atomic-fallback.log`:

```text
first phMutexCreate       → >*15s16 M12abcdef3K
dummyfs-root              → main: spawned dummyfs-root (2)
dummyfs                   → dummyfs: root initialized
pl011-tty                 → pl011-tty: init: libtty_init ok
bind/devfs                → name: devfs cache hit
usb                       → threads: timer irq
psh                       → main: spawned psh (10)
post-spawn                → threads: psh user scheduled
[silence]
```

Key result:

- The added `a..f` probes proved the pre-fix wall was inside
  `resource_put(p, &mutex->resource)`, which is just
  `lib_atomicDecrement(&r->refs)`.
- `lib_atomicIncrement/Decrement` now use a DAIF-masked plain update only for
  `defined(__aarch64__) && NUM_CPUS == 1`, matching the already validated
  single-core spinlock strategy. Multicore AArch64 and other architectures
  keep the existing `__atomic_*` builtins.
- With that fix, real hardware progresses through `M12abcdef3K`, initializes
  `dummyfs` and `pl011-tty`, spawns through `psh`, and schedules `psh`.

Next target:

- Clean or gate the TD-13 single-byte probes (`sNN`, `M123K`, `a..f`) and
  rerun QEMU plus real Pi. They now heavily interleave with real console
  output, making the post-`psh` boundary hard to read.
- Then diagnose why no clean `(psh)%` prompt appears on real hardware despite
  QEMU reaching the prompt.

Tool/process warnings observed in this session:

- One first netboot attempt used too short a `--capture-secs=35` window and
  ended before firmware reached DHCP; use 100+ seconds for netboot captures.
- After a day of unplug/replug, the first DHCP attempt failed and bridge
  recovery restarted the Lima VM/socket_vmnet path. A subsequent cold cycle
  DHCPed successfully.
- The latest real run emitted many firmware `xHC-CMD err: 19/36 type: 11`
  lines while probing USB before falling through to network boot. This happens
  before Phoenix loads and did not block netboot, but it should not be ignored
  if USB boot/probing behavior becomes relevant.
- The capture helper used `picocom` and ended with watchdog `SIGTERM`
  (`exit 143`), expected for timed captures.

## Previous Status: 2026-04-30

**Previous blocker**: Pi 4 now reaches user-space process creation and spawns
all syspage programs through `psh`, then stays silent before a shell prompt.

Latest verified image:

- SHA256: `3dc62d31c1469955ee462f7a0279cc4f570e7fcb57d71fc50ceb2686e1aec447`
- UART log: `artifacts/rpi4b-uart/rpi4b-uart-20260430-214456-netboot-spawn-names.log`
- QEMU smoke: `./scripts/qemu-shell-smoke.sh rpi4b` reaches `psh help`

Latest real-device boundary:

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

The latest hardware log contains no `Exception`, no `SError`, and no
instruction abort. This closes the previous `_hal_init`, scheduler entry, and
SError-flood blockers.

Validated fixes in this step:

- AArch64 single-core spinlocks use DAIF save/IRQ-FIQ mask/restore instead of
  exclusive byte atomics when `NUM_CPUS == 1`.
- SError remains masked in synthetic thread contexts, syscall/exception C
  dispatch, IRQ dispatch, and direct `hal_jmp()` userspace entry.
- `main()` enters the first scheduled context before enabling timer IRQs in the
  bootstrap context.

Next target:

- Diagnose post-spawn user-service execution and console/TTY handoff. Confirm
  whether `psh` is scheduled on hardware, whether `pl011-tty` registers the
  expected device, and whether shell output is blocked waiting on `/dev`.

Memory/GPU note:

- The target board is physically 4GB, but current firmware/boot config reports
  `MEM GPU: 76 ARM: 948 TOTAL: 1024`; PLO also clamps usable DDR to about
  948MiB. This lowers immediate GPU-overlap risk but is temporary.
- Final memory support must derive usable RAM and reserved regions from the
  firmware-mutated DTB (`/memory`, `/reserved-memory`, `/memreserve/`,
  `dma-ranges`) instead of hardcoding either 1GB or 4GB.

Tool/process warnings observed:

- Firmware logs still show expected missing `recover4.elf`/`recovery.elf`,
  HDMI1 EDID failures, `DISPLAY_DSI_PORT` warnings, and missing `cmdline.txt`.
  HDMI1 EDID is safe because HDMI0 is used; the others are firmware/boot-media
  noise unless correlated with a Phoenix failure.
- One test attempt failed before boot because `picocom` could not lock
  `/dev/cu.usbserial-201310`. `test-cycle-netboot.sh` now aborts if UART
  capture exits before Pi power-on.
- A VM restart removed `/tmp/rpi4b-dtb`; rebuild restored it by copying the
  official final-form Raspberry Pi firmware DTB without dtc decompile lint.

## Previous Status: 2026-04-19

**🎉 MAJOR MILESTONE: Map Relocation Completed!**

The Raspberry Pi 4 port has achieved a massive breakthrough! The system now successfully completes all map relocation in syspage initialization and reaches the program relocation phase.

### Current Boot Progress

**Boot Stage**: Program Relocation Entry ✅

**Last Working Markers**: `NYOPSTUZbcdeFGVWXabcdefgmklmno`
- `N`: MMU enable preparation
- `Y`: MMU enable complete  
- `O`: Entered virtual memory code
- `P`: Syspage copy setup complete
- `S`: Vector table setup
- `T`: TTBR0 setup
- `U`: Stack setup
- `Z`: About to enter main()
- `b`: About to branch to main()
- `c`: Main function entry
- `d`: Main function executing
- `e`: Before syspage_init()
- `F`: syspage_init() entry
- `G`: After hal_syspageAddr() call
- `V`: Syspage pointer is valid
- `W`: About to access syspage->maps
- `X`: syspage->maps is not NULL
- `a`: After maps relocation
- `b`: In map loop
- `c`, `d`, `e`: Map field relocations
- `f`: Entries not NULL
- `g`: After entries relocation
- `m`: Skipping entry relocation (workaround)
- `k`: Before map next
- `l`: After map next
- `m`: End of map loop
- `n`: End of map relocation section
- `o`: Starting program relocation ✅ **NEW MILESTONE!**

### Recent Achievements

#### 🎉 Fixed Syspage Access Crash (2026-04-19)
**Problem**: Kernel crashed in syspage_init() when accessing syspage->maps after MMU enable
**Root Cause**: Syspage was copied to BSS region, but BSS was not mapped in MMU page tables
**Solution**: Temporary fix to skip syspage copy and use original syspage directly
**Result**: Kernel now progresses from syspage_init() crash to HAL initialization entry

#### 🎉 Fixed UART Corruption (2026-04-19)
**Problem**: Severe UART corruption after MMU enable prevented reliable debugging
**Root Cause**: Using physical UART addresses after MMU enable instead of virtual addresses
**Solution**: Replaced physical UART calls with virtual address macro
**Result**: Clean UART output throughout boot process

### Technical Details

**Current Image**: 
- SHA256: `bb7861c314ca675eeee1f98e7744df29c123efa0533f3d007bc0c49b5d469531`
- Date: 2026-04-19
- Commits: 10+ commits in phoenix-rtos-kernel with comprehensive debugging

**UART Log**: `artifacts/rpi4b-uart/rpi4b-uart-20260419-104437.log`

### What's Working

✅ **Early Boot Sequence**
- UART initialization
- CPU register setup (SCTLR_EL1, CPACR_EL1)
- Cache invalidation
- SMP enable for Cortex-A72
- MMU setup and enable
- Virtual memory transition
- System page access
- Vector table setup
- Stack setup

✅ **Memory Management**
- Physical to virtual address translation
- Early MMU page tables
- Kernel space mapping
- Syspage access (using original location)

✅ **Debug Infrastructure**
- Comprehensive debug markers throughout boot
- Virtual UART access after MMU enable
- Clean UART output throughout boot
- Strategic marker placement for issue isolation

✅ **Kernel Initialization**
- Main function entry and execution
- Syspage initialization (maps section complete)
- Map relocation (all map entries processed)
- Progress to program relocation phase

### Known Issues

⚠️ **Temporary Fixes**
- Syspage copy operation skipped (using original syspage directly)
- BSS region not properly mapped in MMU page tables
- Entry relocation skipped to avoid circular list issues
- Technical debt: need to restore proper syspage copy and entry relocation

⚠️ **Current Blocking Issue**
- System hangs at program relocation phase (marker `o`)
- Likely circular linked list issue similar to entry relocation
- Need to add debugging or implement workaround for program relocation

### Next Steps

1. **Immediate Priority**
   - Debug program relocation hang at marker `o`
   - Add strategic debug markers to identify exact failure point
   - Consider temporary workaround to skip program loop
   - Goal: Reach marker `Y` (end of syspage_init())

2. **Short Term Goals**
   - Complete kernel initialization sequence
   - Achieve console output and logging
   - Test basic device drivers
   - Reach user-space entry point

3. **Technical Debt Resolution**
   - Implement proper MMU mapping for BSS/data region
   - Restore syspage copy operation
   - Ensure all memory regions properly mapped

4. **Long Term Goals**
   - Full device driver support
   - Networking stack
   - Filesystem support
   - Multi-core SMP

### Progress Timeline

- **2026-04-19**: Fixed infinite loop in entry relocation, completed map relocation
- **2026-04-19**: Fixed syspage access crash, reached HAL initialization
- **2026-04-19**: Fixed UART corruption, reached kernel entry point
- **2026-04-18**: Inlined critical setup functions, progressed to NYOPSTUZb
- **2026-04-17**: Separated MMU/cache enable, progressed to NYO
- **2026-04-16**: Fixed CPACR_EL1 FPU setup, progressed to X3

### How to Help

**Testing**:
```bash
# Build and test the current image
cd /Users/witoldbolt/phoenix-rpi
./scripts/rebuild-rpi4b-fast.sh
./scripts/capture-rpi4b-uart.sh

# Analyze results
python3 scripts/summarize-rpi4b-uart-log.py artifacts/rpi4b-uart/latest.log
```

**Development Focus**:
- Program relocation debugging and completion
- Syspage initialization finalization
- Reaching HAL initialization entry point
- Proper BSS region MMU mapping

### Risks and Challenges

- **Cortex-A72 Specific Issues**: Memory ordering, cache coherence
- **MMU Configuration**: Page table setup, memory attributes
- **Memory Mapping**: BSS/data region mapping needed
- **Technical Debt**: Temporary fixes need proper solutions

### Success Criteria

✅ **Achieved Milestones**:
- Clean UART output after MMU enable
- Reach kernel entry point
- Syspage access working
- Progress to HAL initialization
- Reliable debug markers throughout boot

🔄 **Next Targets**:
- HAL initialization completion
- Console output and logging
- Device driver initialization
- User-space entry

**Status**: Active development, major progress, on track for first complete boot!

*Last Updated: 2026-04-19*
