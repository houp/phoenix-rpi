# A 56-year clock step landing inside an app's startup silently kills it

Found 2026-09-19 while grading a `q3dm7` bench. One trial of six rendered **zero** frames, and the
cause turned out to be worth more than the bench.

## The observation

`rpi4b-uart-20260919-083454-q3dm7arm-T1.log`, the last four lines before the capture window closed:

```
] ...loaded 5823 faces, 189 meshes, 49 trisurfs, 37 flares
]System time in UTC was Thu Jan 1 00:00:24 1970
System time set to UTC Sat Sep 19 06:35:56 2026
] CL_InitCGame: -1201626.86 seconds
] Com_TouchMemory: 0 msec
]
```

`q3dm7` loaded (all 5 823 faces). Then the wall clock jumped from 1970 to 2026 **inside**
`CL_InitCGame`, which timed itself across the step and got **−1 201 626.86 seconds**. The client
never reached `UnnamedPlayer entered the game` and sat at the console until the window closed.

Across the six trials:

| trial | clock step at line | `CL_InitCGame` | entered the game | frames |
|---|---|---|---|---|
| T1 | **356 of 360** (inside client init) | **−1 201 626.86 s** | **no** | **0** |
| T2–T5 | ~130 of ~475 (early boot) | normal | yes | 8 031–8 627 |

## The mechanism

`psh_clockSync()` — `sources/phoenix-rtos-utils/psh/pshapp/pshapp.c:1641`. The Pi 4 has no RTC, so
psh sets the clock from the network when a session starts and the clock looks unset:

- it **forks and does not wait** (`:1651-1657`), deliberately — the comment records that an earlier
  synchronous version "cost the shell entirely" when a step did not return;
- the child waits up to **30 s** for `/bin` to appear (`PSH_CLOCK_BINWAIT_S`, NFS/SD root mounting
  late), then execs `ntpclient -w 90` (`PSH_CLOCK_WINDOW_S`), which itself waits for DHCP and DNS.

So the step can land anywhere in a window of roughly **two minutes** after psh reaches its prompt —
including in the middle of whatever the user launched in the meantime.

## Why it matters beyond one bench trial

The README already tells the owner: *"If an app prints nothing and just sits there, relaunch it. A
cold start occasionally fails to get going."* This is **a** mechanism behind that symptom, now with
a name and a signature. It is not a hang and not a crash: the app is alive, it simply computed a
nonsensical elapsed time during init. Relaunching works because by then the clock is already set,
so `psh_clockSync()` skips (`time(NULL) >= PSH_CLOCK_PLAUSIBLE`) and there is no second step.

⚠ Any app that measures a duration with wall-clock time across its startup is exposed, not just
Quake III. The rate observed here is 1 in 6, but that is one sample of a race whose width depends on
how long DHCP takes on the day.

## Archive census — how often this actually bites

Across every UART log mentioning `CL_InitCGame` (Quake III is the only app in the archive that
prints a startup duration, so it is the only one that can be counted this way):

| measure | count |
|---|---|
| quake3 logs total | **174** |
| clock step lands **before** game init | 133 |
| clock step lands **after** game init began (exposed) | **5** (~3.6 % of the 138 with both markers) |
| no clock line at all (already set, or no network) | 36 |
| **corrupted `CL_InitCGame`** | **2** — `-1201626.86 s` and `+2141867.04 s` |
| **app actually failed to start** | **1** (`q3dm7arm-T1`) |

⚠ **Exposure is not failure.** Of the 5 exposed logs, three showed a normal init time and ran fine,
one showed an absurd *positive* duration (`+2141867.04 s`) and still entered the game, and only one
— the negative case — never started. The step has to land inside the specific region an app is
timing, which is a much narrower window than "after startup began".

So the honest historical rate for a *visible* failure is **~1 in 174 Quake III runs**, not the 1-in-6
this bench happened to show. The 1-in-6 is one small sample of a race whose width depends on how long
DHCP takes that day; the archive figure is the better estimate.
ⓘ One of the five (`20260908-213714-q3-follow`) never entered the game **with a normal 2.64 s init**
— so it is a *different* failure, and must not be folded into this one.

### The "loaded but never entered" population

Of the same 174 quake3 logs, **4** never print `entered the game`. Two are misclassifications —
`20260823-095429-live-test` is an X-server failure (`xlaunch: server exited during init`) and
`20260822-020006-q3-capture-cfg` quit deliberately after 131 screenshots. That leaves **2 genuine
start failures in 174 runs (1.1 %)**:

- `q3dm7arm-T1` — corrupted duration, clock step **established** as the cause;
- `20260908-213714-q3-follow` — ends with the *identical* terminal signature (`CL_InitCGame: 2.64
  seconds` → `Com_TouchMemory` → `]`, stuck at the console) but a **normal** init time.

★ Both are in the 5-log "exposed" set, i.e. the clock step landed after game init began in both. So
the step is *present* in both failures and *absent* from none of them — but causation is established
only for T1, where the duration is visibly corrupt. For `q3-follow` the step could have broken a
later timing (connect, spawn) without touching the one number the game prints, or the cause could be
unrelated. **Not decided, and not counted as this defect.**

⚠ **The trap this now creates:** "stuck at the console after `Com_TouchMemory`" is a *shared*
signature, not a fingerprint. Now that the clock step has a name it will be tempting to attribute
every such log to it. Only a corrupted duration proves it.

## What to do about it

**In measurements — available today, no code change.** `test-cycle-psh-interact.sh` already has
`--ready-line <ERE>` (`:62`): pass `--ready-line 'System time set to'` so the harness waits for the
step before sending the command. Every future GPU bench should do this; without it a trial can be
silently degraded and still look like a clean run.

**In the product — a real tradeoff, and the owner's call.** The obvious fix (have psh wait for the
clock before its first prompt) is exactly what the code's comment says was tried and reverted. A
bounded version — wait a few seconds, then continue regardless — would keep the shell safe and make
the common case deterministic, at the cost of delaying the prompt on every cold boot on **every**
target, since `pshapp.c` is shared. Not changed unilaterally.

⛔ **Do not "fix" this by making the step smaller or slewing instead of stepping.** The jump is
1970 → now; there is no slew that covers 56 years, and the exposure is the *window*, not the size.
