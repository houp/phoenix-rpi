/* Phoenix-RTOS aarch64 in-process debug/backtrace facility (UART). See dbg.c.
 *
 * NOTE: this facility has been promoted to a reusable corelib — see
 * sources/phoenix-rtos-corelibs/libdbg (link -ldbg, include <dbg.h>). This copy is kept as the
 * standalone HW bring-up probe / test harness (test-dbg.c, test-sig.c). Prefer libdbg for new code.
 *
 * Copyright 2026 Phoenix Systems  %LICENSE%
 */
#ifndef PHX_DBG_H
#define PHX_DBG_H

/* Install fault handlers (SIGSEGV/SIGILL/SIGBUS/SIGFPE/SIGABRT). On a crash, prints the faulting
 * PC + a frame-pointer backtrace (return addresses) over stdout/UART, then exits. */
void dbg_init(void);

/* Dump a backtrace now: if called from a signal handler, unwinds the INTERRUPTED code (via the
 * cpu_context_t the libphoenix trampoline stashed); otherwise unwinds the caller. Host-side:
 *   aarch64-phoenix-addr2line -f -e <elf> <ret-addrs...>   */
void dbg_backtrace(const char *tag);

/* Arm a SIGALRM watchdog: after `secs`, dump the backtrace of wherever the process is stuck
 * (locates a HANG), then re-arm. Call before entering a region that may hang. */
void dbg_arm_watchdog(unsigned int secs);

#endif
