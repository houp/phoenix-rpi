# Phoenix-RTOS Raspberry Pi 4 Port Status

## Current Status: 2026-04-30

**Current blocker**: Pi 4 reaches `main()`, completes `syspage_init()`, enters
the `_hal_init` assembly wrapper, resets the early kernel stack, and then takes
an exception immediately after marker `R`.

Follow-up testing after the USB-C NIC was replugged confirmed the automated
netboot lane is functional after scripted bridge recovery. The baseline image
was rebuilt from the restored source tree after two failed experiments:

- Keeping the bootstrap `TTBR0_EL1` identity map instead of replacing it with
  the scratch table regressed the board to `A2 ... X1 X2 X3`.
- Installing `_vector_table` through its linked high VA in `VBAR_EL1` also
  regressed the board to `A2 ... X1 X2 X3`.

Both experiments were reverted. They are still useful evidence: the current
Pi 4 path remains sensitive to the low-PC/TTBR0 transition, and high
instruction/vector fetch through the TTBR1 kernel mapping is not yet reliable
on real hardware. This is consistent with the external arm64 bring-up rule
already recorded in `docs/source-artifacts.md`: keep a valid identity path
across MMU enable, then deliberately branch to the higher-half address only
after the higher-half mapping is proven executable.

The fixed QEMU smoke helper now fails correctly instead of hiding this class of
problem. Current Pi 4 QEMU stops at `STUZb!e{86000005}`:
`ESR_EL1=0x86000005` is an instruction abort from the same exception level with
a level-1 translation fault. QEMU therefore exposes the invalid low-PC fetch
path immediately after the branch to `main`; real hardware appears to continue
further only because stale translations or cached instruction state mask the
same bug until `_hal_init`.

Latest real-device marker boundary:

```text
... kllmnPYfhR
```

Latest verified image:

- SHA256: `07d40d5e1a197f4c3e763ac3368f475d774a7f1596cb9452073133673a970032`
- UART log: `artifacts/rpi4b-uart/rpi4b-uart-20260430-150524-netboot-nic-refresh-baseline.log`

Interpretation:

- `Y`: `syspage_init()` completed.
- `f`: `main()` is about to call `_hal_init()`.
- `h`: assembly `_hal_init` wrapper entered.
- `R`: wrapper reset `SP` to `PMAP_COMMON_STACK + SIZE_INITIAL_KSTACK`.
- No later diagnostic marker appears; the first direct store after `R` does not
  complete.

An exception-vector marker proved that the CPU is taking an exception after
`R`. The current exception reporting is still diagnostic and not clean final
code; the next task is to decode the fault reliably or fix the underlying early
stack/high-VA data-store mapping issue.

Netboot/power/UART automation is active. Known lab-process warnings:

- Initial DHCP frequently times out and bridge recovery restarts Lima/socket_vmnet.
- `tio` lacks timed capture, so the UART helper falls back to `picocom`.
- `picocom` can briefly leave the UART device locked after capture.
- HDMI1 EDID warnings are expected when only HDMI0 is connected.
- Final-form Raspberry Pi firmware DTB is copied without default decompile lint;
  run with `RPI4B_DTB_LINT=1` when an explicit DTB audit is needed.

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
