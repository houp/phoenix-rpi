# The rpi4-audio DMA stall: three captures, four hypotheses retired, one fix

*2026-09-18, overnight. Full working record; the weekly log carries only the verdict.*

The `q2-sdl-openaudio-hang` defect had been open since 2026-09-04 and had **never been observed in
the act** — every entry about it was inference from a missing log line. This is the night it was
captured, what each capture ruled out, and the one defect that got fixed along the way.

**Verdict in one line:** the stall is real, contained, has a single consistent register signature,
is **not** any of the four things it looked like, and is **timing-sensitive** — its rate moved
3-in-44 boots to 0-in-127 across a relink with no functional change, so short runs cannot settle it.
Separately, `audio_pwmInit()` was latching a **bus error on 100% of boots**; that is fixed and
verified 12/12 (devices `11f182a`).

---

**★★ CAUGHT IT — the `rpi4-audio` DMA stall fired on hardware for the first time, twice, with the
diagnostics that were built for exactly this (2026-09-18, boots 4 and 8 of a 20-boot hunt).**

```
self-test ABORTED after 0 samples — the write path stalled (DMA not draining)
STA=0x00000100  DMA_CS=0x00000021
```
**`DMA_CS=0x21` = ACTIVE | DREQ_STOPPED** — the channel is alive and waiting for a DREQ that never
comes. **`PWM_STA=0x100`** has *neither* FULL1 *nor* EMPT1 and still claims STA1: the FIFO holds data
the PWM is **not clocking out**. And `0 samples` means it never started, rather than falling behind.
⇒ the stall is on the **PWM/consumer** side, not the ring/producer side — which is the discrimination
the print was added for, answered on its first firing.
⊕ **Third capture (boot 19 of 26) with the fuller print — and it KILLED my own hypothesis:**
```
STA=0x100  DMA_CS=0x21  ring w=15 r=16
CM_PWMCTL=0x91  CM_PWMDIV=0x2000  PWM_CTL=0xa1a1  PWM_DMAC=0x80000804
```
`CM_PWMCTL=0x91` still has **BUSY set** — the PWM clock generator *is* running at the stall — and
DIV, CTL and DMAC all read back exactly what the driver programmed, with the ring **full** and the
FIFO non-empty. So "the clock stops after ready" is dead, and it died **before** I built a fix on it.
Everything configured is configured correctly, and the PWM still will not drain.
⏭ The one value no print has shown is the **period**: `RNG1=0` would leave the channel enabled,
clocked and fed while consuming nothing — one value that explains every register in the capture.
Devices `0d4e6ed` prints `RNG1`/`DAT1` plus `STA` re-read 2 ms later (frozen vs merely slow).
ⓘ Rate so far: **3 aborts in 44 post-sweep boots (~7%)**, all with the same register signature.
⊖ **Two more hypotheses dead, and one of my own readings corrected.** A new probe (`bin/pwmwrite`,
tools/pwm-write-probe) hammers the PWM write path ~200 k times per run on the **unused PWM0**
instance instead of once per 2.5-minute boot: **0 dropped writes in 205 000 trials** and **0 bus
errors in 21 000**, at all three spacings. So unpaced writes neither vanish nor fault — and PWM
`RNG1`'s reset value is **32, not 0**, so a lost period write could not stall the channel anyway.
↩ **I also misread all three captures.** `PWM_STA` bit 8 is **BERR** (a latched bus error), bit 9 is
STA1 — I had them swapped, so "STA1 set, transmitting" was really "a write failed, FIFO not empty,
**not** transmitting". Corrected everywhere, with the bit map in the driver so the next reader cannot
repeat it. ⓘ It also means the driver latches a bus error on **every** boot (`STA=0x102`); the entry
print now landing says whether that is even ours.
⏭ Still standing: an enabled, clocked, FIFO-fed channel that does not transmit — which `RNG1 = 0`
would explain exactly. `RNG1` is now on the **ready line**, so every boot samples it (devices
`f5af072`, building) rather than only the ~7% that stall.

★ **FIXED a defect that was firing on 100% of boots** (devices `11f182a`) — not the stall, and the
commit says so. Measured: `PWM_STA` reads **0x2 at driver entry** (BERR clear) and **0x102 at ready**
(BERR **set**), so `audio_pwmInit()` latched a **bus error every single boot** and nothing decoded
bit 8 to notice. The BCM doc sets BERR when the bus writes "successive cycles to the same set of
registers" — and that function wrote `PWM_CTL` three times back to back. Now paced (read-back + 10 µs
per write), BERR cleared so the ready line reports this init rather than history, and `RNG1`/`CTL`
verified with a named complaint if either does not take. ✅ **Verified 12/12 boots: BERR clear, `RNG1=612`, 0 aborts** (was `STA=0x102` on every boot before).
⏳ Six-app gate running on the paced build — the games use `/dev/audio0`, so a driver-init change
earns a full gate before it counts as landed.

⚠ **Why I stopped hunting the stall itself: it is a Heisenbug.** Rate went **3 in 44 boots → 0 in
115** across a relink whose only audio change was a print inside the abort branch — unreachable on a
healthy boot (Fisher p ≈ 0.01). Relinking moves code layout and timing, so each instrument I add
perturbs the race. 30 more boots also show `RNG1=612` **correct on every one**. Conclusion recorded
rather than a fix claimed: the stall is real (3 captures, one signature), contained (bounded ~10 s,
app still starts), and **not settleable by short runs**.
🔎 **And the driver's own start check cannot see this stall**: `audio_dmaStart()` accepts the channel
if `CS` has ACTIVE and no ERROR — which `0x21` (ACTIVE|**DREQ_STOPPED**) satisfies. So the driver has
been calling a parked channel "started" and then blocking in the first write. The fix that follows is
a **progress** check (the ring streams silence, so a healthy channel advances `DMA_SOURCE_AD` ~3.5 kB
in 20 ms; the stalled one moves nothing) plus one PWM re-arm — drafted, deliberately **not applied
until the next capture says whether the clock or the DREQ threshold is at fault**. Fixing before
reading that would be fixing in the dark.
✅ **APPLIED 2026-09-18 (devices `5cd9cc3` + the re-arm commit).** The gate it was waiting on is
answered: capture 3's `CM_PWMCTL=0x91` closed the clock branch, and the DREQ branch is now answered
*by the fix itself* rather than by another capture — the re-arm print says whether a paced
`CTL=0 + CLRF1 + re-enable` un-sticks the parked channel, which is the same discrimination a fourth
capture would have bought, except it arrives on the next occurrence instead of the next hunt. The
driver now (a) prints the **progress figure on every boot** — a continuous sample instead of a
~7% event — (b) re-arms up to 3 times with the full engine state printed, including `DMA_DEBUG`,
`CONBLK`, `SRC`, `DEST` and `LEN`, none of which any capture has ever shown, and (c) degrades to a
**paced null sink** if it still will not stream, so the user-visible symptom (`q2-sdl-openaudio-hang`)
is gone even when the underlying defect fires. ⚠ That is containment plus instrumentation, **not** a
root cause: the state itself is still unexplained.
🔧 **The hunt exposed a harness bug that had been eating boot-only benches: picocom kills its whole
PROCESS GROUP.** A 30-boot run stopped after trial 1, twice. Mechanism: picocom signals its group
when terminated, and a bare background launch puts it in *ours* — so the capture watchdog's kill took
down `capture-rpi4b-uart.sh` (exit 15, before its own summary print), `test-cycle-netboot.sh`
(exit 143, skipping its **entire** stage table) and `test-cycle-bench.sh` with it. The psh-interact
path was immune because it drives pyserial, which is why benches *with commands* looped fine all
night. Fixed by launching picocom under `setsid` (capture `a7514505f`); verified capture rc 0 with its
summary, a netboot cycle rc 0 with the full stage table, and a 3-trial boot-only bench completing.
↩ **And it corrects something I claimed yesterday:** "both test-cycle scripts now grade their own
log" was half true — the netboot one's fault-count line was unreachable, because the script was dead
before reaching it. It prints now (`[✓] no fault patterns`).
⏳ 26-boot hunt relaunched on the fixed harness (~2.5 min a sample).
⚠ **Not yet a regression claim:** 2 in 32 post-sweep boots vs 0 in 37 pre-sweep boots with the abort
code present is p ≈ 0.2 — it may simply be the first captures of a rare event. The hunt continues on
boots alone (the self-test runs every boot, so Quake II is not needed).

**⏳ Overnight: hunting the one defect that can still show on stage.** `q2-sdl-openaudio-hang` is the
only failure a live demo would notice (Quake II starts black; ~10 s on the current build, ~350 s on
the card). The stall has never been caught in the act — but since 2026-09-17 the abort prints
`DMA_CS` + the ring cursors, so **one** occurrence names which half stalled. Rate is ~1 in 41, a
previous 20-start hunt came back empty. ⚠ *First attempt was mis-shaped and I scrapped it:* Quake II
never exits, so three launches per boot only ever produced **one** start (the rest queued behind a
still-running game) — 8 starts, not 24. Re-running as **20 boots × 1 start**, ~110 s each, which is
all it takes to see `SDL audio initialized` or the hang. ~40% chance of a catch; a miss still
tightens the bound. 3 starts so far, all healthy.

---

## ↩ The correction the fix produced on its first healthy boot

**`DMA_CS = 0x21` is NOT a stall signature. It is the HEALTHY steady state.** Every capture above
leads with it, and this document called it "the channel is parked waiting for a DREQ that never
comes". Measured 2026-09-18 on 10 consecutive healthy netboot boots with the new progress print:
**9 of 10 read `CS=0x00000021` while streaming at ~1 800 ring words per 20 ms**, and the tenth read
`0x00000001`. Of course they do — the channel is DREQ-paced, so between DREQs it is *always* held.
Linux names the bits and says so: `BCM2835_DMA_ISHELD` (bit 5) is "Is held by DREQ flow control" and
bit 3 is the live DREQ state (`external/linux/drivers/dma/bcm2835-dma.c:131-137`), and its abort path
comments that a peripheral failing to complete "is expected when dreqs are enabled but not asserted"
(`:732-738`).

So of the register set in the three captures, only **`0 samples` and the full ring** ever carried
information. The one value that discriminates is **progress**, which nothing was measuring — and
upstream says exactly that about the check this driver used to make: *"A zero control block address
means the channel is idle. (The ACTIVE flag in the CS register is not a reliable indicator.)"*
(`bcm2835-dma.c:682-683`, repeated at `:711-712`).

**Negative control for the new code, 10/10 boots:** streaming on arm 1, **1 784-2 352 words** in the
20 ms settle against a **64-word** threshold (a ~28× margin, and the parked channel moves ≤ 16), 0
re-arms, 0 null sinks, `self-test fed 8960 samples` every boot, stage table 10/10 on all six stages.
The containment cannot fire on a healthy boot.

## What upstream says about the rest of our sequence

A read-only sweep of `external/` (Linux 6.18 RPi fork, u-boot, barebox) for a comparator:

- **There is no upstream PWM-FIFO/DMA audio driver anywhere.** No `PWM_DMAC` or `PWM_FIF1` writer in
  the whole Linux tree, no `dmas` property on any PWM DT node, and no bcm283x PWM driver at all in
  u-boot or barebox. The analogue jack is driven by VideoCore firmware over VCHI. So there are **no
  known-good DREQ/PANIC thresholds to compare ours (`DREQ=4, PANIC=8`) against** — the only sources
  are the datasheet and our own measurements.
- **Our DMA side already matches upstream.** Transfer info `WAIT_RESP | PER_MAP | DEST_DREQ | SRC_INC`
  is byte-for-byte what `bcm2835_dma_prep_dma_cyclic` builds (`bcm2835-dma.c:1064,1098,1113`), and the
  arm order (RESET → `CONBLK_AD` → `ACTIVE`) is `bcm2835_dma_start_desc` (`:768-773`). **Channel 5 is
  legitimately ours**: `brcm,dma-channel-mask = <0x07f5>` (`bcm2711.dtsi:106`) reserves 1 and 3, not
  0-4 — which retires the "the firmware owns our channel" hypothesis and the TODO that carried it.
- **The clock side did NOT match, and now does** (see the commit): SRC and ENAB were changing on one
  bus cycle and DIV was written before CTL, where `clk-bcm2835.c:1147-1172` splits them deliberately
  ("we have to pause clock generation while updating the control and div regs"); and `CM_GATE` (bit 6)
  was never set although `bcm2835_clock_on` sets it on every enable (`:1115-1120`). Ranked #1 of the
  surviving candidates precisely because it is a race by construction and leaves every register
  reading back correct — which is what the failing boots look like.
- ⏭ Still unsampled, now printed: whether the **firmware leaves the PWM clock enabled** before we
  touch it. If it does, the old single-write SRC change was happening on a *running* generator on
  every boot — the exact case upstream's comment guards. The entry line now carries `CM_PWMCTL` and
  `CM_PWMDIV`.

## ⊖ Fifth hypothesis retired: the PWM starts fine — it is the DMA path

`bin/pwmwrite --start-test` runs the driver's exact init (`CTL=0` → `CLRF1` → `RNG1/RNG2=612` →
`USEF|MSEN|PWEN` on both channels) on the **unused PWM0** and then feeds 8 duty words by **PIO**, no
DMA anywhere, thousands of times per boot instead of once per 2.5-minute boot. A trial counts as the
defect only if the channel neither sets `STA1` nor drains the FIFO.

| shape | trials | never transmitted | bus errors |
|---|---|---|---|
| back-to-back (enable, then feed immediately) | **5 000** | **0** | 0 |
| enable, then sit **20 ms** with an EMPTY FIFO, then feed | **2 000** | **0** | 0 |

The second row exists because the first does not test the driver's real shape: `audio_pwmInit()`
enables the channel and only then runs `portCreate`, `create_dev` and two `printf`s before
`audio_dmaStart()` arms the DMA, so on a real boot the channel sits enabled and empty for
milliseconds. Both shapes: **7 000 starts, 0 failures, `STA1` seen on every one.**

⚠ **Scope of that null, stated exactly:** the probe *refuses* to reconfigure a running clock (so it
cannot cut off the driver that owns it), so every one of the 7 000 trials ran on a generator that had
been stable for minutes. It therefore retires "the init sequence alone" and "the idle-enabled
window" — and says **nothing** about PWEN being asserted microseconds after the generator starts,
which is the driver's shape on every boot and exactly the race the clock reorder addresses. Two
branches survive, not one.

⇒ **The PWM state machine starts reliably given a settled clock. Branch (i), the stall needs the DMA
path** — the DREQ handshake, the
threshold, or the first burst arriving while the channel is coming up. That is where the next probe
goes: drive PWM0 from a spare DMA channel exactly as the driver drives PWM1 (PWM0's DREQ is 5, PWM1's
is 1), which samples the actual failing path thousands of times per boot instead of ~7 times in 100.
ⓘ Sampling gain over per-boot hunting: ~4 000×, the same lever that settled the allocator work.
⇒ **Branch (ii): the clock-start → PWM-enable proximity.** Untestable by this probe by construction;
only the reordered `audio_clockInit()` (devices `65e8623`) and a long rate run on a frozen build can
speak to it.

