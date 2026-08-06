#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
# Copyright (C) 2026 Phoenix Systems. Author: Witold Bolt.
#
# E4 finale: generate a COLOR-CYCLING H.264 test clip for the moving player.
#
# We author the YUV420 planes directly (full-range JFIF BT.601, the exact
# forward coefficients e4_fb_blit.h uses to convert back to RGB), then feed the
# raw frames to libx264 as a baseline-profile Annex-B stream. Direct YUV
# authoring guarantees the on-Pi decoded colors match the intended sequence, and
# baseline profile (no B-frames, shallow DPB) + a small keyint keep the on-target
# 8 MB decode stack comfortable.

import os
import subprocess
import sys

W, H = 320, 240
FPS = 12
FRAMES_PER_COLOR = 6            # 0.5 s hold per color at 12 fps
CW, CH = W // 2, H // 2         # chroma plane dims (4:2:0)

# (name, R, G, B) -- vivid, unmistakable, well separated on HDMI.
COLORS = [
    ("RED",     255,   0,   0),
    ("GREEN",     0, 255,   0),
    ("BLUE",      0,   0, 255),
    ("WHITE",   255, 255, 255),
    ("YELLOW",  255, 255,   0),
    ("CYAN",      0, 255, 255),
    ("MAGENTA", 255,   0, 255),
]

HERE = os.path.dirname(os.path.abspath(__file__))
YUV = os.path.join(HERE, "frames.yuv")
OUT = os.path.join(HERE, "clip.h264")


def clamp8(v):
    return 0 if v < 0 else (255 if v > 255 else int(v))


def rgb_to_yuv_jfif(r, g, b):
    """Full-range BT.601 JFIF (matches e4_fb_blit.h inverse exactly)."""
    y = 0.299 * r + 0.587 * g + 0.114 * b
    u = -0.168736 * r - 0.331264 * g + 0.5 * b + 128.0
    v = 0.5 * r - 0.418688 * g - 0.081312 * b + 128.0
    return clamp8(round(y)), clamp8(round(u)), clamp8(round(v))


def main():
    total = len(COLORS) * FRAMES_PER_COLOR
    print("=== authoring %d raw YUV420 frames (%dx%d) ===" % (total, W, H))
    seq = []
    with open(YUV, "wb") as f:
        for name, r, g, b in COLORS:
            y, u, v = rgb_to_yuv_jfif(r, g, b)
            yplane = bytes([y]) * (W * H)
            uplane = bytes([u]) * (CW * CH)
            vplane = bytes([v]) * (CW * CH)
            frame = yplane + uplane + vplane
            for _ in range(FRAMES_PER_COLOR):
                f.write(frame)
            seq.append((name, r, g, b, y, u, v))
            print("  %-8s RGB=(%3d,%3d,%3d) -> YUV=(%3d,%3d,%3d)  x%d frames"
                  % (name, r, g, b, y, u, v, FRAMES_PER_COLOR))

    print("=== encoding baseline H.264 Annex-B (-g 6) -> %s ===" % OUT)
    if os.path.exists(OUT):
        os.remove(OUT)
    cmd = [
        "ffmpeg", "-y", "-hide_banner", "-loglevel", "error",
        "-f", "rawvideo", "-pixel_format", "yuv420p",
        "-video_size", "%dx%d" % (W, H), "-framerate", str(FPS),
        "-i", YUV,
        "-c:v", "libx264", "-profile:v", "baseline",
        "-g", "6", "-keyint_min", "6",
        "-pix_fmt", "yuv420p", "-color_range", "pc",
        "-f", "h264", OUT,
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print("ffmpeg FAILED:\n" + r.stderr)
        return 1
    os.remove(YUV)

    size = os.path.getsize(OUT)
    print("=== clip.h264: %d bytes ===" % size)

    # Confirm exact frame count + fps with ffprobe.
    probe = subprocess.run(
        ["ffprobe", "-hide_banner", "-v", "error",
         "-count_frames", "-select_streams", "v:0",
         "-show_entries",
         "stream=nb_read_frames,avg_frame_rate,width,height,profile",
         "-of", "default=noprint_wrappers=1", OUT],
        capture_output=True, text=True)
    print("=== ffprobe ===")
    print(probe.stdout.strip())
    if probe.stderr.strip():
        print(probe.stderr.strip())

    print("\n=== COLOR SEQUENCE (one loop = %d frames = %.2f s) ==="
          % (total, total / FPS))
    idx = 0
    for name, r, g, b, y, u, v in seq:
        print("  frames %2d-%2d : %-8s RGB=(%3d,%3d,%3d)"
              % (idx, idx + FRAMES_PER_COLOR - 1, name, r, g, b))
        idx += FRAMES_PER_COLOR
    return 0


if __name__ == "__main__":
    sys.exit(main())
