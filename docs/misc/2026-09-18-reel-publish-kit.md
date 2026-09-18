# Demo reel — everything needed to publish it

*2026-09-18. The reel itself has been upload-ready since 2026-09-16; what was missing was the copy
that goes around it. Nothing here changes the video.*

**File:** `artifacts/hdmi-video/20260916-160506-phoenix-rtos-rpi4-showcase.mp4`
**3:59 (239 s) · 146 848 364 B · H.264 High@L4.0 · 1920x1080 · 30 fps · bt709 · faststart set · no audio track**
Upload as-is — no remux, no re-encode. Verified 11/11 segments by `scripts/verify-demo-reel.py`
(the hardened checker, which fails an empty frame window instead of grading it "ok").

⚠ **It was recorded on the pre-sweep build (2026-09-15/16 captures).** The current tree renders the
same scenes at the same rates, so it stays representative — but it is not a recording of today's
build, and nobody should claim it is.

## Chapters

Each segment is a straight cut of exactly the length below — no title card, no fades — so these
timestamps are exact, not approximate. Paste as-is into a YouTube description to get chapter markers
(the first entry must be `0:00`).

```
0:00  Boot — kernel → drivers → lwIP → NFS root → psh, on real hardware
0:17  Shell and the ported userland — uname, Lua 5.4.7, jq 1.7.1, SQLite 3.53.4 and /usr/bin
0:40  Python 3.14 + ncurses — Conway's Game of Life, 239x66 on the HDMI console
1:02  X11 desktop — Window Maker on glamor GPU-accelerated X — a live OpenGL window, Game of Life, top, xbill, xclock
1:28  Dillo web browser — a page fetched over Phoenix's own TCP/IP stack, rendered under X
1:41  Hardware H.265 decode — BCM2711 rpivid decoding a 1080p phone recording, full screen at 21.7 fps
2:05  QuakeSpasm (GLQuake) — OpenGL on Mesa v3d, id1 demo1, ~37 fps
2:27  Quake II — yQuake2 on OpenGL ES, q2demo1, ~35 fps
2:49  vkQuake — Vulkan via V3DV, id1 demo2, page-flipped present, 43 fps average at the page flip
3:11  Quake III Arena — 5-bot deathmatch on q3dm1, orbiting third-person camera, 36 fps
3:35  SuperTuxKart 1.4 — OpenGL ES 3.1, 4-kart AI race, 7-8 fps at the page flip
```

## Suggested title

> Phoenix-RTOS on a Raspberry Pi 4 — X11, five 3D games, a web browser and hardware video decode

## Suggested description

> Phoenix-RTOS is a microkernel real-time operating system. This is it running on stock Raspberry Pi 4
> hardware: booting to a shell over the network, bringing up an X11 desktop with GPU-accelerated
> 2D, browsing the web over its own TCP/IP stack, decoding 1080p H.265 on the BCM2711's video block,
> and running five OpenGL/Vulkan games — QuakeSpasm, Quake II, vkQuake, Quake III Arena and
> SuperTuxKart 1.4 — through a ported Mesa on the V3D 4.2 GPU.
>
> Everything shown is captured from the Pi's own HDMI output at 1080p30. The frame rates in the
> captions are measured at the page flip, not read off an in-game counter.
>
> The port, the drivers and the tooling are at https://github.com/rpi-phoenix-rtos — see
> `docs/PHOENIX-RTOS-RPI4-CHANGES.md` for what was written, and `docs/KNOWN-ISSUES.md` for what is
> still broken.

## Honest caveats, if the description should carry them

- **SuperTuxKart runs at 7-8 fps.** It is a modern GLES 3.1 deferred renderer on a 2019 VideoCore —
  the achievement is that it renders correctly, not that it is playable.
- **The audio path is not in this recording.** The reel has no audio track; PWM audio works on the
  jack but the sign-off is attended and the driver has one contained intermittent defect
  (`q2-sdl-openaudio-hang` in KNOWN-ISSUES).
- **Quake III's camera is an orbiting spectator**, not a player — deliberate, so the scene shows the
  renderer rather than someone's aim.
