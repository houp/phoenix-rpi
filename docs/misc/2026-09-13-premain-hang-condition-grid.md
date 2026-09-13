# premain-hang: bounding the trigger to COLD BOOT + HEAVY launch (2026-09-13)

Working detail moved out of the weekly log. Live summary is in the current week's log and the
`premain-hang` row of `docs/KNOWN-ISSUES.md`.

## 5d. ⏳ `premain-hang` — deploying the startup trace on STK

The remaining demo risk: **4.2% of app launches produce no output at all** (~23% chance of hitting
one across a six-app demo). Workaround is trivial — **relaunch it** — but it is worth root-causing.
★ Measured today: a **minimal** launch does not reproduce it (`--version`, **180 launches / 0 silent**,
Fisher p=0.006) ⚠ but a storm only makes launch **1** cold, so that samples **2** cold launches and
says nothing about the cold path. ⓘ Every bench trial *is* a cold launch, so **4.2% already is the
cold rate** — more storming cannot help.
✅ **Trace deployed and verified in the LIVE binaries** — all **8** markers in `supertuxkart` and
`psh` (`--scope core` alone would have left them out; the `--ports-only` relink is required).
📊 **24 traced cold launches: 24/24 reached all 8 pre-`main` markers, 0 silent, 0 partial.**
⚠ **That does NOT show the hang is gone.** At 4.2% it predicts ~1 event, so zero is unremarkable
(p≈0.36), and the two limits recorded *before* the run stand: the old 4.2% used a **different
detector** (no output in 300 s, which also catches a merely slow start), and the trace adds `debug()`
syscalls early in libc init, so it **may mask** a timing-sensitive paging race. Honest reading: *24
traced cold launches did not reproduce it*, and there was nothing to localise.
★★ **It is a COLD-START phenomenon — and that unseats the "binary-specific" claim.** Three storms,
**300 warm launches, 0 silent** (`supertuxkart --version` direct, and `/bin/stk --version` through the
748 KB launcher), while every bench trial — one boot, one launch — fails at **4.2%**. A storm makes
only launch **1** cold, so 300 launches sample **3** cold ones.
⚠ The old "binary-specific" finding rests on ~4200 **storm** launches of bash/python3/cxxprobe —
**all warm** — so those binaries were never tested cold. It does not hold.
ⓘ What pointed here: a silent run emits **54 bytes**, the command echo and nothing else — not even
the launcher's own `DATADIR` line — so the failure precedes the first write of the first process.
★★★ **`premain-hang` TRIGGER BOUNDED: it needs a COLD BOOT *and* a HEAVY launch.** Four conditions,
**571 launches**: warm storms **0/300** · binary-cold but system-warm **0/60** · system-cold +
*light* **0/20** · system-cold + *heavy* (full `stk` after boot) **7/191 = 3.7%**. Everything except
cold+heavy is **0 in 380** — **Fisher p = 4.4e-04**. Neither coldness nor size alone does it.
⚠ **The old "binary-specific" claim is withdrawn**: it rested on ~4200 **warm** storm launches of
bash/python3/cxxprobe, so those binaries were never tested cold.
⏳ **Last cell of the grid running:** 24 boots × one cold **non-STK heavy** launch
(`quakespasm -loadbench`) — separates *heavy* from *STK*.
★ **Demo workaround available now:** if an app prints nothing, **relaunch it** — the second launch is
warm and has never failed in 300 tries.
🐞 **Harness bug found and fixed — it silently produced 20 empty trials.** `test-cycle-bench.sh` broke
out of its option loop on an unknown flag (`--inter-cmd-secs`, which belongs to
`test-cycle-psh-interact.sh`), leaving that flag where `--` was expected ⇒ the command list came out
**empty** ⇒ it fell back to a plain boot capture. The trials *looked* like they ran and sent nothing;
the only tell was the log name gaining `-netboot-`. Unknown flags are now **fatal**.
⚠ My first fix rejected the bare `--` separator too, and my "positive control" was `bash -n` — a
syntax check that cannot exercise a parse. Now tested both ways.
✅ **Diagnostic build reverted and the restored tree re-verified.** Trace is **0 in every binary**,
staged and live (checked by artifact — turning a `-D` knob off does not invalidate the objects it
changed, and the build log's own "knob changed" line never appeared). Re-booted to prove the tree
still works rather than leaving it unverified: **unix-socket 38/0 `OK`, stdlib 93/0 `OK`, 0 faults**.
Manifest `2026-09-13-post-afunix-clean.md`.
⚠ Diagnostic build — must not ship; the delivered image `afd534a7` predates it and is untouched.

