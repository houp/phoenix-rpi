# axi-pmu — Raspberry Pi 4 (BCM2711) AXI bus performance-monitor reader

A Phoenix userspace tool that reads the BCM2711 **System AXI bandwidth monitors** to
measure **real hardware bus/memory bandwidth** — the first time this project measures
bus traffic at the hardware, rather than inferring it from wall-clock throughput.

Phoenix response to the owner's LKML task: *"Look at [the AXI PMU perf patch] and see if
we could implement something similar in Phoenix-RTOS"* — the thread is Ian Rogers'
Linux `drivers/perf/` "Add Raspberry Pi AXI PMU driver". This is the equivalent core:
mmap the perf block + program a bandwidth watcher + read the counters.

## Result (HW-verified 2026-08-14, netboot, 0 faults)

A memcpy dose-response on **bus 10** (the A72 CPU↔memory AXI path, found by scanning all
16 buses) is perfectly linear and cross-checks against wall-clock:

    memcpy  4MB x4: dR=1.05M dW=1.08M | 16 B/xfer => 1.43 GB/s   (wall-clock memcpy 1.40 GB/s)
    memcpy  8MB x4: dR=2.13M dW=2.14M | 16 B/xfer => 1.42 GB/s   (wall-clock 1.40 GB/s)
    memcpy 16MB x4: dR=4.26M dW=4.24M | 16 B/xfer => 1.44 GB/s   (wall-clock 1.42 GB/s)

Verified three ways: (1) **linear dose-response** — transactions scale exactly with copy
size; (2) **16-byte AXI bursts** — bytes/transaction is constant (`readl & 0x7fffffff`,
16 MB / 1.05M transfers ≈ 16 B); (3) **absolute cross-check** — the counter-derived
bandwidth (1.43 GB/s) matches the independent wall-clock memcpy (1.40 GB/s) within ~2%.

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
