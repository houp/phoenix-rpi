# `vkq-tile-flicker`: vkQuake renders single-buffered straight into the displayed scanout

*2026-09-15. Source-derived root cause for the single-frame black-tile flicker the owner saw and
reported as "some lightmaps on walls are wrong".*

## The observation

One frame (1/30 s) shows hard-edged, **screen-axis-aligned** black rectangles; the frames immediately
before and after, at the same viewpoint, are clean. A band crosses the left wall, the arch recess, the
right pillar and the right wall **at identical screen rows** — four surfaces whose lightmaps are
independent in texture space, and whose stone courses recede in perspective while the block edges do
not. So it is not a lightmap defect and never was.

## The cause

vkQuake on this port has **no WSI swapchain**. It renders into a single VkImage whose memory the
winsys backs with the **fb0 physical pages**, and the render pass's `storeOp=STORE` is the "present"
(`pl_phoenix_vk_vid.c:104-109, 473-474, 1153-1156`):

> "we render into the SINGLE fb0-backed scanout VkImage … This single-image scanout IS the *present*
> (no WSI swapchain, no `vkQueuePresentKHR`)."

That means **the GPU writes into the buffer the display hardware is scanning out, while it is being
scanned out.** V3D is a tile-based renderer: at end-of-render-pass it stores tiles to memory one tile
at a time. A display refresh that lands mid-store sees some tiles holding the new frame and some still
holding the previous frame or the clear — which is *exactly* a screen-space, tile-shaped, one-frame
artefact.

⚠ `v3d_phoenix_set_next_scanout()` is **not** a page flip, despite the name. It sets a flag so the
*next BO allocation* is treated as scanout-backed (forcing RASTER tiling instead of UIF, because the
HVS cannot scan a tiled surface). vkQuake calls it once, at image creation (`:588`). There is no
per-frame flip anywhere in the vkQuake present path.

## Why QuakeSpasm is the clean control

The GL/gallium path page-flips. The winsys detects a scanout region at least twice the physical
height and keeps `scanout_nbuf` / `scanout_double` / `scanout_disp_off`
(`v3d_phoenix_winsys.c:299-303, 670-675`): the GPU renders into the back buffer and the displayed
offset is moved only once the frame is complete. That is why QuakeSpasm, on the same demo, the same
day and the same capture card, is clean across 72/72 consecutive frames where vkQuake blocks — and it
also rules out the grabber and H.264.

ⓘ This is the same class as [`project_pi4_quake_flicker_vcmbox`](../../README.md): the earlier Quake
dynamic-model flicker was also single-buffering.

## The fix shape (not yet implemented)

Give the Vulkan path the double-buffering the GL path already has: allocate the scanout VkImage over
the **back** buffer, render + wait as today, then flip the displayed offset and alternate. The winsys
already knows whether a second buffer was granted; what is missing is an entry point the Vulkan side
can call per frame, plus alternating the image's backing offset.

Until then the defect is cosmetic and intermittent, and it does **not** appear in the reel's chosen
vkQuake window (the worst single-frame black spike there is the camera passing a dark doorway).

## What NOT to spend time on again

- **Rate measurement by automatic detection.** Seven variants have failed: scoring "how black is the
  frame" against a neighbour median fires on 18% of frames under fast camera motion; "darker than both
  neighbours" picks a zombie-and-blood frame as its argmax and *misses* a frame confirmed by eye. Every
  verdict so far is contiguous-window visual review at 30 fps. Quote no percentage without a detector
  validated against known positives AND known negatives.
- **The lightmap path.** Format support, swizzle, tiling, barriers and caches were all checked and are
  clean negatives, and the geometry of the artefact rules the whole class out anyway.
