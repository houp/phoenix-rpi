# Phoenix-RTOS Raspberry Pi 4 (BCM2711) — Hardware Support Matrix

**Updated:** 2026-06-26. Canonical "where are we" reference for the Pi 4 port.
One row per peripheral/subsystem. For narrative gap analysis see
`docs/knowledge/scope-pi4-uncovered.md`; for live progress see `docs/inprogress/status.md`.

> **STATUS (2026-06-26):** since the 2026-06-18 pass — the X11 software desktop is fully
> live on HW (Xphoenix kdrive fbdev DDX + kbd/mouse input + JWM + Window Maker WMs + xterm
> running a BusyBox shell, #30/#35/#36); GLQuake is the working flagship (mouse #24, QUIT/
> fbcon restore #25, LAN multiplayer #26, NFS-root #27, torch flame #28); vkQuake reached
> 2D GPU raster on HW, paused at the no-WSI texture-upload gap (#29); VideoCore mailbox is
> serialized via the rpi4-vcmbox server; logging→/var/log shipped (#31); a stress-test
> suite ran clean across all layers (#38-40). Rows below updated accordingly.

**Status legend:**
- ✅ **done** — works on hardware, committed, validated.
- 🟡 **partial** — usable but incomplete / a known sub-feature missing.
- 🔬 **groundwork** — mechanism proven, but a deliberate decision/step remains.
- ⏸ **attended** — implementable but deferred to a human-attended session
  (boot-risk, statistical-regression, or needs a screen/scope/bench rig).
- ⛔ **blocked** — stuck on an external dependency (datasheet/JTAG/firmware/HW).
- ⬜ **not started**.

| Subsystem | Status | Evidence / entry point | Remaining |
|---|---|---|---|
| CPU bring-up, EL2→EL1, MMU | ✅ done | boots to userspace; **caches ON** (SCTLR.{M,C,I}, all Normal RAM WB-cacheable) since 2026-05-17 (TD-16 RESOLVED) | the once-proposed "make the GENET RX DMA pool cacheable" lever (Policy B) was TRIED and **CONCLUDED UNVIABLE** — corrupts the GPU framebuffer under load (#11 RE-OPENED, default-off); no global cache switch remains |
| SMP (4 cores) | ✅ done | **4-core SMP scheduling works** (`NUM_CPUS=4U`; secondaries re-arm their own CNTV + run the scheduler; TD-01/TD-11 resolved) (`project_smp_d7_d8_findings`) | the old "cpu0-only" state is FIXED — do not cite |
| Generic Timer | ✅ done | scheduler tick / delays | — |
| Interrupts (GIC-400) | ✅ done | GENET/USB/SD IRQs live | — |
| PL011 UART console | ✅ done | primary console + klog mirror | TD-14 two-owner UART polish (#127) |
| VideoCore property mailbox | ✅ done | userspace (thermal/clocks/power) | kernel-internal primitive ⏸ (for WiFi/BT/DVFS) |
| HDMI framebuffer **console** (fbcon) | ✅ done | klog+psh on HDMI (Tier 0) | slow fills (CPU writes to the uncached fb pages; caches are globally ON) |
| HDMI framebuffer **device** `/dev/fb0` | 🟡 partial | device LANDED + HW-validated netboot (#148): read/write + `RPI4FB_GETMODE` devctl, `video/rpi4-fb/` | attended (#149): fbdev `FBIOGET_*` veneer (Tiny-X), true `mmap(fd,0)` kernel backing, drawing/display-ownership |
| GENET Ethernet | ✅ done | Tier 5, IRQ-driven, ping ~0.9 ms | — |
| lwIP / DHCP / ICMP / UDP | ✅ done | autonomous DHCP, diag-udp :9999 | — |
| USB host (PCIe→VL805 xHCI) | ✅ done | **enum 11/11 cold boots** after the #129 two-step-BSR AddressDevice fix (devices `53383d1`) + TRSTRCY (usb `47eede9`) + #121 dc-civac uncached-page eviction (usb `12c4fe8`) | IRQ event path #145 (perf) ⏸; daemon hardening #142/#143 ⏸ |
| USB HID (kbd + mouse) | ✅ done | `/dev/kbd0`+`/dev/mouse0`, live keys→psh (#122/#124/#126) | — |
| USB mass storage | ⬜ not started | — | umass driver |
| PCIe RC / VL805 inbound abort (TD-10) | ⏸ attended | SError handler in (#109); abort isolated to PCIe/USB bring-up | unmask SError = boot-risk; root-cause #144 |
| SD card (EMMC2 SDHCI) | ✅ done | `/dev/mmcblk0[pN]`, PIO reads, MBR (#119) | high-throughput needs DMA |
| ext2 persistent rootfs (#120) | ✅ done | mounts as `/`, binaries exec from it (`ifconfig`), boots to psh stably; HW-validated SD-boot 0/10 faults. Crash root cause was a **fs pool-thread stack overflow** (8 KB default too small) — fixed by `storage_run(2, 16*_PAGE_SIZE)`, full multithreading kept, ext2 unchanged | residuals: noisy-but-recovering 50 MHz Data-CRC (signal polish), single-block-only CMD24/CMD18 (perf) |
| SoC thermal + throttle | ✅ done | `/dev/thermal`,`/dev/throttled` (2026-06-05) | firmware owns the trip (telemetry only) |
| Hardware RNG (RNG200) | ✅ done | `/dev/hwrng` (2026-06-05); **now also backs `/dev/urandom`** (posixsrv reads `/dev/hwrng` for entropy, rand() fallback) — HW-verified 2026-06-17 | kernel `getrandom()`/pool wiring (libc-level) still PRNG |
| Watchdog / reboot / poweroff | ⏸ attended | works via diag-udp `r`/`h` (PM block #43) | productionize `_hal_systemReset` (kernel, boot-risk) |
| WiFi (BCM43455 SDIO) | ⛔ blocked | fw+NVRAM load + CR4 release were built (in the now-**deleted** diag-udp.c) | **firmware not executing** (#91, image-scan proven). NVRAM-trailer lead DISPROVEN (2026-06-07); real suspects = download/clock ordering + SDIO-core intstatus-clear + rstvec semantics. Live downloader must be reintroduced first. Needs HW/JTAG |
| Bluetooth (BCM43455 UART HCI) | ⬜ not started | plan only | needs mailbox+GPIO alt-fn + `.hcd` blob |
| GPIO / pinctrl | 🟡 partial | `/dev/gpio` read-only observer device (#150): snapshot + per-pin `RPI4GPIO_GETPIN` devctl, `gpio/rpi4-gpio/` | **outputs** (GPSET/GPCLR/fsel set) need a bench rig to validate (⏸) |
| I²C / SPI / PWM | ⬜ not started | plans exist | need GPIO alt-fn + clock-manager |
| GPU (V3D 4.2) — OpenGL | ✅ done | ported Mesa gallium v3d driver + GL frontend (`tools/v3d-driver-port/`); **GLQuake (quakespasm) runs ~40-50fps@1080p** via render-to-scanout; R/B color + particle render-stall fixed (2026-06-16/17); **early-Z re-enabled** (06-22) + **triple-buffer page-flip** (0 render/bin wedges) landed; mouse #24, QUIT/fbcon-restore #25, LAN multiplayer #26, NFS-root #27, torch flame #28 all done | gamma retune (cosmetic), audible audio sign-off (attended), formal multi-boot soak |
| GPU (V3D 4.2) — Vulkan (V3DV) | ✅ working | full ported Mesa V3DV (`libv3dv`); **vkQuake renders textured 3D on the V3D** (real SPIR-V VS+FS → NIR→QPU, render passes, TFU texture uploads land); no-WSI fb0 scanout; the torch/fullbright **alpha-scanout** bug is FIXED (opaque present alpha=1, `project_vkquake_torches_dark_fullbright`, vkQuake d3e329c pushed) | TFU **LINEAR-tiling striping** on some texture sizes (winsys, shared w/ Quake2); RT gated off (V3D lacks ray_query) |
| Audio (PWM / I²S / HDMI) | 🟡 partial | PWM driver `/dev/audio0` (`audio/rpi4-audio/`): **continuous streaming DMA** (free-running self-chained ring, PWM1=DREQ 1) feeds the FIFO; `write()` fills the ring w/ usleep backpressure (driver sleeps, no spin); PIO fallback retained. **Quakespasm SNDDMA backend** (feeder thread) mixes over it — "Audio: 16 bit, stereo, 44100 Hz", demo renders, 0 faults/underruns (2026-06-17). **SDL2 audio driver** over `/dev/audio0` HW-validated (driver=phoenix, 44100/S16/2ch, tone played, 0 faults, 2026-08-05) | audible jack sign-off ⏸ (headphones); vkQuake reuses the backend; underrun→ring-loop artifact (steady state ok) |
| DMA framework | 🟡 partial | legacy-DMA channel bring-up proven + in production for audio (`rpi4-audio`: self-chained streaming CB, DREQ-paced, low-1GB C0 bus alias) | generalize into a reusable DMA helper; line-rate SD (CMD18) still PIO |
| RTC | 🟡 capability present | Pi 4 has no on-SoC RTC. The **`ntpclient` psh applet** queries SNTP + calls `settimeofday` (kernel `settime` syscall + libphoenix `settimeofday`/`clock_settime` all present) → NTP-over-GENET works once a server is reachable | defaults to `pool.ntp.org` (needs an internet route or a host-side ntpd on the netboot link); not yet auto-run at boot |
| Camera (CSI-2) / DSI display | ⬜ not started | — | — |
| posixsrv / psh userspace | ✅ done | pipes, ptys, `/dev/{null,zero,urandom,full}` (urandom now HW-RNG-backed), interactive psh; **AF_UNIX SOCK_STREAM** + **libc `getrandom()`/`getentropy()`** validated on HW (`misc/rpi4-ipcprobe`, 2026-06-17) | psh has no `|` pipe parsing |
| X11 / windowing (kdrive) | ✅ done | host-side `tools/x11-port/`: full client+render+font+toolkit lib stack + kdrive xorg-server core build for aarch64-phoenix. **LIVE ON HW:** Xphoenix (fbdev DDX → shadow → /dev/fb0, periodic full-screen flush) with real kbd+mouse input (`/dev/kbd0`+`/dev/mouse0` via the DDX after FBCON_DISABLED), running **xeyes (mouse-tracking)**, the **JWM** and **Window Maker** window managers (#30/#35), and **xterm** with a live BusyBox shell (#36). | accelerated/GPU-X (Glamor/EGL via a multi-client GPU arbiter) remains a research stretch (`2026-06-16-x11-accelerated-desktop-plan.md`) |

## Ported libraries & applications

| Component | Status | Notes |
|---|---|---|
| Mesa V3D OpenGL stack (`libGL/libv3d-phoenix.a`) | ✅ | GL 2.1 on real V3D 4.2, in-process winsys, no-WSI fb0 scanout (`project_pi4_v3d_scout`) |
| Mesa V3DV Vulkan stack (`libv3dv`) | ✅ | SPIR-V → NIR → QPU; textured 3D on HW; no WSI (fb0 scanout) |
| **SDL2 2.30.12** (`ports/sdl2`) | ✅ HW-validated | fullscreen GL + input (kbd0/mouse0) + audio (/dev/audio0) all proven on Pi; phoenix video/GL/input/audio drivers; org `ports c191d20`. Vulkan backend = phase 2 (needs V3DV WSI). `dlopen`→static, GPL-glue kept out of zlib `libSDL2.a` (`project_sdl2_port`) |
| X11 desktop (kdrive/Xphoenix) | ✅ HW-validated | fbdev DDX → /dev/fb0, kbd+mouse input, xeyes/xterm/xcalc/xedit + JWM/Window Maker WMs (`project_x11_lib_port`) |
| QuakeSpasm (GLQuake) | ✅ HW-validated | textured GLQuake ~40fps@1080p, demos + SP map, LAN MP, 0 faults (`project_quakespasm_port`) |
| vkQuake (Vulkan Quake) | ✅ HW-validated | textured 3D via Vulkan on V3D; torch/alpha-scanout fixed (d3e329c). Remaining: TFU tiling striping, phantom-kbd (`project_vkquake_torches_dark_fullbright`) |
| yQuake2 (Quake II, `ref_gl1`) | 🟡 runs | single-ELF (dlopen→static), loads maps ("Outer Base", 38 entities), renders 2D + connects; full 3D map load is **infra-bound** (slow 100Mbps NFS + large-binary exec) not a port bug; needs `allow_download 0` (`project_quake2_port`) |
| Dillo / mc / glib2 | ✅ | render on fbcon (`project_pi4_glib2_mc`) |

## Build / test infrastructure (✅)

- Two build variants: `rebuild-rpi4b-fast.sh --variant netboot|sd` (2026-06-05).
- Netboot loop: `test-cycle-netboot.sh` (UART + HDMI snapshots + diag-udp `--probe`).
- Network observability: diag-udp responder on :9999 — full command + `/dev`-node
  reference in **[docs/knowledge/diag-udp-reference.md](../knowledge/diag-udp-reference.md)** (`c` clocks+thermal,
  `r`/`h` reboot/halt, `g` GPIO, `V` framebuffer probe, `R` device-read smoke test,
  `D` devnodes, plus the WiFi/SDIO bring-up set).
- Deterministic rollback: `snapshot-/restore-integration-state.sh` + `manifests/`.

## What "fully supported" still needs (priority order)

1. **USB** is functionally complete (enum + HID); the remaining items (#142/#143/#144/#145)
   are *hardening/perf/root-cause* and are **attended** (statistical regression or boot-risk).
2. **ext2 rootfs** (#120) — DONE (mounts as `/`, exec-from-card, boots to psh); **NFS rootfs**
   also DONE + HW-proven (`project_nfs_rootfs_feasibility`). **Direct exec from NFS FIXED** (the
   `object_fetch` short-read corruption — kernel `f145658f`); residual (root-caused 2026-08-05,
   `project_large_binary_exec_hang`): large-**BSS** binaries (~19 MB text / big BSS, e.g. yquake2's
   26.5 MB BSS) intermittently **silently hang** at exec — Phoenix eagerly commits BSS page-by-page
   + `hal_memset`s it under `map->lock`, and that long exec window stalls over the flaky netboot
   NFS. NOT the old `-ENOMEM at process_load:704` (that note is STALE — current code forces only ELF
   headers). Mitigation: trim the linked stack; proper fix: demand-page exec-time anon (kernel).
   Other residuals: #156 first-access ENOENT (boot-order race), perf/signal polish.
3. **fb0 driver** — decide ABI + display ownership, then implement (attended).
4. **X11** — DONE: the software kdrive desktop (Xphoenix + fbdev DDX + kbd/mouse input + JWM/
   Window Maker WMs + xterm) is live on HW. Remaining is the *accelerated* GPU-X research stretch.
5. **WiFi #91** — the one true *blocker*; firmware-execution gate needs deeper HW visibility.
6. Greenfield: DMA framework → audio/I²C/SPI/PWM; Bluetooth; GPIO full driver.

## Unattended-vs-attended note

Overnight/autonomous netboot work is restricted to **additive + deterministic-self-log +
cannot-silently-regress** items (see `feedback_unattended_scoping` memory). The ⏸ rows above are
attended precisely because their failure is either physically unrecoverable over netboot
(kernel/reboot), statistically invisible to single-boot validation (USB daemon internals), or
needs human judgement (a screen/scope/bench rig).
