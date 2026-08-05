# Porting an RTOS to the Raspberry Pi 4, entirely by AI — a field report

*Draft. Covers ~2026-03-19 → 2026-08-05 (~4.5 months); ~1400 coordination-repo commits.
The whole Phoenix-RTOS Raspberry Pi 4 (BCM2711) bring-up was done by an AI agent driven
only through chat — no human wrote code. A human operator steered by prompt, ran the
occasional physical action (swap an SD card, hold a keyboard), and gave the occasional
decisive clarification. This is an honest account of what that looked like: what the AI
found easy, what took dozens of cycles, and why hardware is uniquely hard for a text agent.*

## The arc

Phoenix-RTOS is a small microkernel OS. The Pi 4 is a quad-core Cortex-A72 with a
famously under-documented, firmware-mediated boot and a grab-bag of Broadcom peripherals
(VideoCore GPU, GENET Ethernet, a PCIe-attached VL805 USB controller, an SDHCI eMMC).
Over ~4.5 months the port went from "does not boot" to:

- **Boot & core**: EL3→EL1 handoff, caches + MMU, GICv2, generic timer, **4-core SMP**.
- **Console/IO**: PL011 UART + an HDMI framebuffer console (driving a vendored FreeBSD
  `teken` terminal emulator), GENET Ethernet + lwIP, **an NFS root over netboot**.
- **Peripherals**: PCIe→VL805 xHCI with USB HID keyboard+mouse, SD/eMMC storage with an
  ext2 root, thermal, hardware RNG, GPIO, PWM audio.
- **Graphics**: the full Mesa **V3D** stack — **OpenGL and Vulkan** — running on the real
  GPU with no DRM kernel driver and no window-system compositor.
- **Userspace**: an X11 desktop (kdrive), **SDL2**, and a stack of ported apps: **GLQuake
  (QuakeSpasm), vkQuake (Vulkan), Quake II**, Dillo, Midnight Commander.

## What turned out *easy* for the AI

Broadly: **anything with a tight, textual feedback loop and a good reference.**

- **Reusing upstream instead of reinventing.** The GPU is the headline: rather than
  hand-encode QPU shaders, the agent *ported Mesa's existing V3D driver* and wrote only a
  thin OS-specific "winsys". Same for lwIP, teken, SDL2, the Quake engines. When a correct
  reference exists and the compiler + a test give fast feedback, the AI is very effective.
- **Mechanical breadth.** Cross-compiling 45 X11 archives, wiring up a dozen device
  drivers, chasing undefined symbols to zero in a single-ELF link — the kind of wide,
  tedious work that exhausts humans, the agent does tirelessly and consistently.
- **Systematic bisection with a fast signal.** "Which of these 140 translation units
  breaks the build" or "which config string does the map loader hang on" — when each trial
  is a quick build or a grep, the loop converges fast.
- **Applying a known pattern to a new instance.** Once the first userspace driver worked
  (mmap the MMIO uncached, poke registers), thermal / GPIO / hwrng / audio followed quickly.

## What turned out *hard* — and why

The hard cases share a root: **the failure's cause was invisible in the text the agent
could see.** Some took *dozens* of build-boot cycles.

- **The vanishing torches (~40 cycles).** In vkQuake, two wall torches simply didn't appear.
  The agent chased "the texture is dark on the GPU" for many boots — the texture was
  actually *fine*. The real cause: the no-WSI framebuffer scanout keys on the color
  buffer's **alpha** channel, and the torch's fullbright pixels had alpha≈0, so the scanout
  silently dropped them. Nothing in any log said "alpha". It took forcing constant colors +
  alpha=1 in the shader to *see* the flame reappear and finally reason backwards to the
  cause. Lesson that then paid off everywhere: **on this display path, opaque geometry must
  write alpha=1.**
- **The #67 false-metric trap.** A model-geometry glitch was declared "fixed" more than
  once — by a metric (cross-boot determinism) that didn't actually measure the bug. The
  real cause was a single-pose vertex buffer straddling a 4 KB page boundary. The lesson is
  about *AI failure modes specifically*: an agent will happily converge on a green metric
  that doesn't test the real thing. Building a *faithful* reproduction (a model gallery with
  per-model attribution) was what finally cracked it.
- **The netboot flakiness that wasn't what it looked like (this autonomous run).** Game
  binaries "failed to exec" ~50% of boots. A careful kernel investigation built a detailed,
  plausible theory (eager BSS commit stalling exec). It was *wrong about the dominant case*:
  the real failure was `psh: <bin> not found` — a boot-order race where the shell runs
  before the NFS root takeover completes. One `ls` to warm the path, then the exec, and it
  was reliable — then a two-line harness change made it permanent. A reminder that a
  well-argued root-cause can still be the wrong one; the empirical `not found` in the log
  was the tell, once actually read.
- **SMP, caches, PCIe, USB enumeration.** Each was a multi-week saga of the same shape:
  a register or DMA or cache-coherency state that is *real* but *not printed anywhere*, so
  the agent had to build instrumentation to make the invisible visible, form a hardware
  mental model, and test it. USB enumeration went from ~50% flaky to 11/11 only after the
  agent found *two* independent bugs (a controller `AddressDevice` that never completed, and
  a DMA pool smashed by stale cache lines on recycled pages) — neither visible without
  purpose-built probes and a Linux driver read as an oracle.

## Techniques that mattered most

1. **Make the invisible visible.** The single highest-leverage investment was
   *observability*: capturing HDMI frames and analyzing them with pixel math; a UDP
   "diag" responder to poke a running board; an in-process backtrace facility; QEMU's
   gdbstub for register-level state; per-stage `stderr` probes. Almost every hard bug was
   cracked the cycle *after* the agent added the right probe.
2. **Deterministic rollback.** Every validated integration state was snapshotted to a
   manifest of sibling-repo SHAs, so a regression could be undone across seven repos with
   one command. This is what makes long autonomous runs safe.
3. **Read the real oracle.** For USB, PCIe, GENET, the agent read the Linux/FreeBSD driver
   for the *authoritative* init sequence rather than guessing from datasheets.
4. **Distrust your own green metric.** The recurring AI failure mode here was declaring
   victory on a proxy. Faithful, adversarial reproduction beat convenient metrics every time.

## The human's actual impact

The operator wrote no code, but a handful of one-line clarifications each saved days:

- *"The keyboard isn't being touched — nobody is using it."* This reframed a "phantom
  keyboard input" bug from a hardware glitch into a software bug, redirecting the search.
- *"Are you sure the flame texture is dark? It looks like the whole object isn't drawn."*
  This pushed the torch investigation off the (wrong) texture-brightness track.
- Decisive scoping: *"reuse Mesa, don't rewrite"*; *"push to our org"*; *"work as long as
  needed."* Direction, not implementation.

The pattern: the human is most valuable not as a coder but as a **source of ground truth
the agent cannot observe** (what's physically happening in the room, what the goal really
is) and as a **circuit-breaker** on a search that has wandered.

## Why hardware is uniquely hard for a text agent

Ordinary software bugs leave a trail *in text* — a stack trace, a failed assertion, a diff.
Hardware bugs often don't. The state that matters (a DMA descriptor's ownership bit, whether
a cache line was evicted, what tiling format the GPU actually wrote, whether a controller's
internal command ring was cleared by a reset) is **not in any log unless you build the log**.
So the agent's real task on hardware is less "fix the code" and more "**construct a mental
model of an unobservable machine from indirect evidence**" — then design an experiment that
makes one hypothesis distinguishable from another in the few bytes of text it can see. That
is genuinely hard, it is where the multi-week sagas came from, and it is, we suspect, the
frontier for AI-driven systems work: not code generation, but *hypothesis design under
observability constraints*.

## The autonomous phase

The final stretch ran unattended: the operator went on vacation and left a task list. The
agent worked in a self-scheduled loop against a durable task board, committing and pushing
small verified changes, retrying through intermittent network/build failures, and
maintaining its own memory across context resets. It shipped a full SDL2 port (HW-validated:
GL + input + audio), got Quake II running, fixed the netboot reliability that had been
silently taxing every test, cleaned ~650 lines of dead diagnostics for publication, and kept
the documentation current — all without a human in the loop. The main limits it hit were
*physical* (a 100 Mbps netboot link it couldn't rewire; an SD card it couldn't insert), not
cognitive.

## Takeaways

- For AI-driven low-level work, **invest in observability first** — the agent is only as
  good as the feedback loop you (or it) can build.
- **Reuse beats generation.** The best code the agent wrote was the glue around code it
  didn't write.
- The hard part isn't writing the fix; it's **seeing the bug**. On hardware, that means the
  agent must become an experimentalist, not just a programmer.
- A long-horizon agent with durable memory, rollback discipline, and a self-verifying loop
  can sustain real engineering progress for days without supervision — bounded mostly by
  what it physically cannot touch.
