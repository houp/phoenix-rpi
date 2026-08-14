# axi-pmu — Raspberry Pi 4 (BCM2711) AXI bus performance-monitor reader

A Phoenix userspace tool that reads the BCM2711 **System AXI bandwidth monitors** to
measure **real hardware bus/memory traffic** — the first time this project reads bus
counters at the hardware, rather than inferring bandwidth from wall-clock throughput.

Scope of what's validated: the **mechanism** is proven on **one bus (bus 10) with a CPU
memcpy**. It is NOT yet a general "measure any master's bandwidth" tool — the counter
reports *total* traffic on a bus over the window (background is real and large, see the
idle reading), and each master's (genet / V3D / DMA) bus index + background attribution
are un-validated. Those are the follow-up work, not done here.

Phoenix response to the owner's LKML task: *"Look at [the AXI PMU perf patch] and see if
we could implement something similar in Phoenix-RTOS"* — the thread is Ian Rogers'
Linux `drivers/perf/` "Add Raspberry Pi AXI PMU driver". This is the equivalent core:
mmap the perf block + program a bandwidth watcher + read the counters.

## Result (HW-verified 2026-08-14, netboot, 0 faults)

A memcpy dose-response on **bus 10** (empirically the bus that tracks a CPU memcpy, found
by scanning all 16 buses — note the vendor enum labels 10=ARM_UC and 11=ARM_L2, but 11
read zero, so treat "bus 10 = CPU memcpy path" as measured, not a confirmed architectural
identity) is perfectly linear:

    memcpy  4MB x4: dR=1.05M dW=1.08M | 16 B/xfer => 1.43 GB/s   (wall-clock memcpy 1.40 GB/s)
    memcpy  8MB x4: dR=2.13M dW=2.14M | 16 B/xfer => 1.42 GB/s   (wall-clock 1.40 GB/s)
    memcpy 16MB x4: dR=4.26M dW=4.24M | 16 B/xfer => 1.44 GB/s   (wall-clock 1.42 GB/s)

What's actually load-bearing (≈2.5 checks, stated honestly): (1) **linear dose-response**
— transactions scale exactly 2× per copy-size step; (2) **read ≈ write symmetry** —
matches a memcpy (each is neither circular); (3) **bytes/transaction is a STABLE 16
across all three sizes** — a hardware-plausible 128-bit AXI burst. Note the GB/s figure
uses a *hardcoded* 16 B/xfer back-derived from known_bytes/transactions, so "1.43 vs 1.40
GB/s" is partly definitional (near-tautological) — the real evidence is linearity +
R≈W + the constant-16 burst, not an independent absolute oracle. The idle "control" came
back large (~4M reads/200 ms), not the ~0 predicted — labeled "never truly idle" but
unreconciled; don't lean on it.

## How it works

- Base **0xfe009800** (BCM2711 System AXI monitor; DT `bcm270x.dtsi` axiperf `0x7e009800`).
  MAP_PHYSMEM needs page alignment → mmap 0xfe009000 + 0x800 offset.
- 3 bandwidth watchers (BW0/1/2 @ 0x40/0x80/0xc0); each counts A/W/R transactions+waits+max
  for one selected bus (BUS_WATCH field [5:0]).
- **Enable sequence** (from Linux vendor driver `raspberrypi_axi_monitor.c`): reset monitor
  (`GEN_CTRL=GEN_CTL_RESET`) → reset watcher (`BWn_CTRL=BW_CTRL_RESET`) → configure
  (`BWn_CTRL=BW_CTRL_ENABLE|bus`) → **enable with the WATCH bit** (`GEN_CTRL=ENABLE|WATCH`).
  The WATCH bit (BIT2) is what actually starts counting — omitting it reads 0.
- Bus enum (`system_bus_string_2711`): the tool scans 0..15; bus 6 = HVS display refresh
  (reads only), bus 10 = A72 CPU memory (reads+writes), bus 13 = writes.

## Build / run

    aarch64-phoenix-gcc -O2 -static axi-pmu.c -o axi-pmu     # links libphoenix only
    # stage into the netboot NFS root, then: /bin/axi-pmu

## Scope / deferred

System-monitor MMIO only. Deferred: the VPU monitor (2nd block 0xfee08000, via VideoCore
mailbox — Phoenix has libvcmbox); a `/dev/axiperf` device; wiring the counters into the F2
perf work (NFS/genet/V3D bandwidth now directly measurable at the bus).
