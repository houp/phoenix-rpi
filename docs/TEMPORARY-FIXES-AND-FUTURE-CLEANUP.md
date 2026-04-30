# Temporary Fixes and Future Cleanup Work

This document tracks all temporary fixes, workarounds, and technical debt introduced during the Raspberry Pi 4 bring-up process. These changes enabled progress but will need to be revisited for proper implementation.

## 2026-04-30 Pi 4 Bring-Up Temporaries

### SError Masking Policy
- **Location**: `hal/aarch64/_exceptions.S`, `hal/aarch64/cpu.c`
- **Reason**: Real Pi 4 delivered asynchronous SError during early IRQ,
  syscall, and user-entry paths before Phoenix had a platform policy/handler.
- **Current State**: SError is kept masked in created contexts, syscall and
  exception C dispatch, IRQ dispatch, and `hal_jmp()` user entry.
- **Cleanup**: Replace with an explicit AArch64 SError policy and handler once
  the system reaches stable shell/user-space operation.

### Single-Core AArch64 Spinlock Path
- **Location**: `hal/aarch64/spinlock.c`
- **Reason**: On the current `NUM_CPUS == 1` Pi 4 target, real hardware hung
  when switching from early spinlock bypass to exclusive-byte spinlocks.
- **Current State**: Single-core builds use DAIF save/IRQ-FIQ mask/restore for
  spinlocks; SMP builds retain the exclusive-byte implementation.
- **Cleanup**: Revisit before enabling secondary Cortex-A72 cores. SMP bring-up
  must validate exclusive monitors, memory attributes, and cache/shareability.

### UART Diagnostics
- **Location**: `_init.S`, `main.c`, `syspage.c`, `vm/vm.c`, `proc/threads.c`
- **Reason**: Needed to pinpoint hardware-only boot boundaries.
- **Current State**: Latest useful boundary is `main: spawned psh (9)` with no
  SError/abort.
- **Cleanup**: Remove or gate diagnostics once the post-spawn console handoff is
  fixed.

### Memory Size Clamp
- **Location**: boot configuration/PLO memory assumptions
- **Reason**: Current firmware reports `MEM GPU: 76 ARM: 948 TOTAL: 1024` on a
  physical 4GB board; PLO also clamps DDR to about 948MiB.
- **Current State**: This avoids immediate high-memory/GPU overlap risk but is
  not final.
- **Cleanup**: Derive usable RAM and reserved regions from the firmware-mutated
  DTB (`/memory`, `/reserved-memory`, `/memreserve/`, `dma-ranges`) and support
  different Pi 4 RAM sizes.

## Table of Contents
- [SMP and CPU Initialization](#smp-and-cpu-initialization)
- [Cache and TLB Management](#cache-and-tlb-management)
- [Memory Management](#memory-management)
- [Syspage Initialization](#syspage-initialization)
- [Debug Infrastructure](#debug-infrastructure)
- [Device Tree Handling](#device-tree-handling)
- [Future Cleanup Priorities](#future-cleanup-priorities)

## SMP and CPU Initialization

### Current State
- **Temporary Fix**: SMP enable code for Cortex-A72 is disabled
- **Location**: `hal/aarch64/_init.S` (lines 226-233)
- **Reason**: SMP enable was causing system hangs during early boot
- **Code**:
  ```assembly
  #if defined(__TARGET_AARCH64A53) || defined(__TARGET_AARCH64A72)
  /* Enable SMP for A53 or A72 - TEMPORARILY DISABLED */
  /* uart_putc 83 */ /* DEBUG: Before SMP enable */
  /* mov x0, #(1 << 6) */ /* CPUECTLR_EL1_SMPEN only */
  /* msr s3_1_c15_c2_1, x0 */
  /* isb */ /* Ensure write completes */
  /* uart_putc 84 */ /* DEBUG: After SMP enable */
  #endif
  ```

### Future Work
- ✅ Research proper Cortex-A72 SMP enable sequence
- ✅ Test with Circle OS and bare metal examples as reference
- ✅ Implement proper secondary core bring-up
- ✅ Add proper CPU hotplug support
- ✅ Test SMP performance and stability

## Cache and TLB Management

### Current State
- **Temporary Fix**: Pre-MMU cache invalidation disabled for Cortex-A72
- **Location**: `hal/aarch64/_init.S` (lines 300-315)
- **Reason**: Cache maintenance with MMU disabled causes hangs on Cortex-A72
- **Code**:
  ```assembly
  /* However, on Cortex-A72, cache maintenance with MMU disabled can cause hangs.
   * Skip this invalidation for now and rely on the post-MMU-enable invalidation.
   */
  /* adrp x0, PMAP_COMMON_KERNEL_TTL2 */
  /* adrp x1, PMAP_COMMON_STACK */
  /* bl _inval_dcache_range */
  ```

### Future Work
- ✅ Research Cortex-A72-specific cache requirements
- ✅ Implement proper cache invalidation sequence
- ✅ Test with different cache configurations
- ✅ Add runtime cache maintenance validation

## Memory Management

### Current State
- **Temporary Fix**: Syspage copy to BSS region disabled
- **Location**: `hal/aarch64/_init.S` (lines 420-440)
- **Reason**: BSS region not properly mapped in early MMU setup
- **Code**:
  ```assembly
  /* Copy syspage to the designated spot in virtual memory */
  /* TEMPORARILY DISABLED - BSS not mapped yet */
  /* adrp x0, _start */
  /* adrp x1, VADDR_SYSPAGE */
  /* add x1, x1, :lo12:VADDR_SYSPAGE */
  /* ldr x2, [x9, #(SYSPAGE_SIZE_OFFSET)] */
  /* 1: */
  /* cmp x0, #8 */
  /* b.lo 2f */
  /* ldr x3, [x9], #8 */
  /* str x3, [x1], #8 */
  /* sub x0, x0, #8 */
  /* b 1b */
  /* 2: */
  /* cbz x0, 3f */
  /* ldrb w3, [x9], #1 */
  /* strb w3, [x1], #1 */
  /* sub x0, x0, #1 */
  /* b 2b */
  /* 3: */
  ```

### Future Work
- ✅ Fix BSS mapping in early MMU setup
- ✅ Re-enable proper syspage copy
- ✅ Test syspage relocation with different memory configurations
- ✅ Add syspage integrity validation

## Syspage Initialization

### Current State
- **Temporary Fix**: Program loop skipped to avoid circular list issue
- **Location**: `syspage.c` (lines 262-272)
- **Reason**: Potential circular linked list in program entries
- **Code**:
  ```c
  if (syspage_common.syspage->progs != NULL) {
      while (*uartfr & 0x20) {}
      *uart = 'o'; /* o marker - starting program relocation */
      syspage_common.syspage->progs = hal_syspageRelocate(syspage_common.syspage->progs);
      while (*uartfr & 0x20) {}
      *uart = 'p'; /* p marker - after progs relocation */
      prog = syspage_common.syspage->progs;

      /* TEMPORARY WORKAROUND: Skip program loop to avoid potential circular list issue */
      while (*uartfr & 0x20) {}
      *uart = 'q'; /* q marker - skipping program loop */
  }
  ```

### Future Work
- ✅ Diagnose circular list issue in program entries
- ✅ Implement proper program entry validation
- ✅ Re-enable program loop with proper termination
- ✅ Test with multiple programs in syspage

## Debug Infrastructure

### Current State
- **Temporary Fix**: Extensive UART debug markers throughout boot process
- **Location**: Multiple files (`_init.S`, `main.c`, `syspage.c`, etc.)
- **Reason**: Needed for diagnosing boot failures
- **Impact**: May affect boot timing and UART bandwidth

### Future Work
- ✅ Consolidate debug markers into configurable system
- ✅ Add compile-time debug level control
- ✅ Implement runtime debug enable/disable
- ✅ Add debug marker filtering
- ✅ Optimize UART output for performance

## Device Tree Handling

### Current State
- **Temporary Fix**: Hardcoded device tree assumptions
- **Location**: `hal/aarch64/dtb.c`
- **Reason**: Early bring-up needed fixed DTB structure
- **Issues**:
  - Assumes specific memory layout
  - Limited error handling for DTB parsing
  - Fixed interrupt controller expectations

### Future Work
- ✅ Implement robust DTB parsing with error handling
- ✅ Add dynamic device tree support
- ✅ Implement DTB validation
- ✅ Support multiple board configurations
- ✅ Add runtime DTB modification capability

## Immediate Next Steps (Current Blocker)

### Primary Objective: Diagnose Program Relocation Hang
**Current State**: System hangs at marker `o` (starting program relocation)

**Immediate Actions Required**:
1. **Add strategic debug markers** around `hal_syspageRelocate(syspage_common.syspage->progs)` call
2. **Check if `syspage_common.syspage->progs` is NULL before calling `hal_syspageRelocate`**
3. **Add markers before and after the `hal_syspageRelocate` call**
4. **Test with enhanced debugging to identify exact failure point**
5. **Determine if circular list issue exists in program entries**

**Expected Outcome**: Identify whether the hang is due to:
- NULL pointer dereference
- Infinite loop in `hal_syspageRelocate`
- Circular linked list in program entries
- Memory access violation
- Other undefined behavior

## Future Cleanup Priorities

### High Priority (Blockers for Next Milestones)
1. **Fix SMP enable** - Required for multi-core support
   - Research proper Cortex-A72 SMP enable sequence
   - Test with Circle OS and bare metal examples as reference
   - Implement proper secondary core bring-up

2. **Re-enable syspage copy** - Required for proper memory management
   - Fix BSS mapping in early MMU setup
   - Re-enable proper syspage copy to virtual memory
   - Test syspage relocation with different memory configurations

3. **Fix program loop** - Required for complete syspage initialization
   - Diagnose circular list issue in program entries
   - Implement proper program entry validation
   - Re-enable program loop with proper termination

4. **Proper cache management** - Required for system stability
   - Research Cortex-A72-specific cache requirements
   - Implement proper cache invalidation sequence
   - Test with different cache configurations

### Medium Priority (Important but Not Blocking)
1. **Consolidate debug infrastructure** - Improve maintainability
   - Create configurable debug system with levels (ERROR, WARN, INFO, DEBUG, TRACE)
   - Add compile-time debug level control
   - Implement runtime debug enable/disable via sysctl or similar

2. **Robust DTB handling** - Improve portability
   - Implement robust DTB parsing with comprehensive error handling
   - Add dynamic device tree support for different board configurations
   - Implement DTB validation and sanity checks

3. **Memory management cleanup** - Improve reliability
   - Review all memory allocation patterns
   - Add proper error handling for memory operations
   - Implement memory leak detection for development builds

### Low Priority (Nice to Have)
1. **Performance optimization** - Cache, UART, etc.
   - Optimize cache management for Cortex-A72
   - Improve UART output performance
   - Review and optimize critical boot paths

2. **Code documentation** - Add comments for temporary fixes
   - Document all temporary fixes with clear markers
   - Add explanations for non-obvious implementation choices
   - Create architecture decision records (ADRs)

3. **Test coverage** - Add regression tests for fixed issues
   - Create unit tests for critical subsystems
   - Add integration tests for boot sequence
   - Implement automated testing for different configurations

## Tracking

Use this document to track cleanup progress:
- [ ] SMP enable implemented properly
- [ ] Syspage copy re-enabled
- [ ] Program loop fixed
- [ ] Cache management optimized
- [ ] Debug infrastructure consolidated
- [ ] DTB handling robust
- [ ] All temporary fixes documented

## Notes

This document serves as a living record of technical debt. As fixes are implemented, update this document and mark items as completed. The goal is to have zero temporary fixes by the time the port is considered production-ready.
