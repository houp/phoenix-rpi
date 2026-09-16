# SuperTuxKart's ~8.5 fps is NOT fill-rate bound — lowering resolution buys nothing

*2026-09-16. One measurement, recorded so nobody runs it again.*

STK is the weakest app in the showcase; the owner's own note was "still not smooth". The cheapest
possible win would have been rendering at a lower resolution — at reel scale the loss is barely
visible. It does not work.

| render size | `flipstat` (steady state) |
|---|---|
| 1920×1080 | 8.40 / 8.53 / 8.42 fps |
| 1280×720 (`--screensize=1280x720`) | 8.43 / 8.45 / 8.52 fps |

**Identical.** Roughly half the pixels, same frame rate ⇒ the bottleneck is not pixel fill. 0 faults in
both runs. Measured with the winsys `flipstat` counter at the page-flip, never the in-game HUD.

## What that leaves

Geometry / draw-call submission or CPU-side work, not rasterisation. Supporting evidence from the era
of the 1 fps chrono defect (patch 0012's header): even then GPU command-list **submits** were ~150 ms
per frame, against ~23 ms physics and ~0.15 ms for the page-flip mailbox. Today's ~117 ms/frame sits in
that same range, which is consistent with submit-bound rather than fill-bound.

## Recommendation: leave it

Anything further is real optimisation work — draw-call batching, scene/pipeline simplification, or
driver-level submit-path work — with uncertain payoff on an app that already renders correctly and that
the owner called "super" at this frame rate. Not worth opening while nothing is blocking.

⚠ If someone does pick it up: measure with `flipstat`, not the HUD, and note that the scanout stays
1920×1080 regardless of `--screensize`, so a "720p" run is still presenting a 1080p surface.
