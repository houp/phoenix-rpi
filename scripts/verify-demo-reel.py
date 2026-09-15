#!/usr/bin/env python3
"""Grade an assembled demo reel segment by segment, before anyone publishes it.

Three things have gone wrong with cuts in this project, and this checks all three:

1. A cut that runs past the end of its source run, into the capture card's
   NO-SIGNAL field -- a uniform near-black frame the card emits once the Pi is
   powered off. That is not a black *picture*, it is a dead input, and it looks
   identical to a broken segment in the published file.
2. A dead or near-dead segment (nothing on screen).
3. A frozen segment. ⚠ Judged only for segments declared MOTION; a console or a
   rendered web page is legitimately static, and grading those on motion is how a
   perfectly good text segment gets called broken.

Usage:  scripts/verify-demo-reel.py <reel.mp4> [--segments scripts/make-demo-reel.sh]

Copyright 2026 Phoenix Systems
SPDX-License-Identifier: BSD-3-Clause
"""
import argparse, subprocess, sys
import numpy as np

# Segments whose content is legitimately still; everything else must move.
STATIC_OK = ("Shell", "Dillo", "Boot")

def frames(path, w=480, h=270, fps=2):
    b = subprocess.run(["ffmpeg", "-loglevel", "error", "-i", path,
                        "-vf", f"fps={fps},scale={w}:{h}", "-f", "rawvideo",
                        "-pix_fmt", "rgb24", "-"], capture_output=True).stdout
    n = len(b) // (w * h * 3)
    if n == 0:
        sys.exit(f"verify-demo-reel: no frames decoded from {path}")
    return np.frombuffer(b, np.uint8)[:n*w*h*3].reshape(n, h, w, 3).astype(np.float32), fps

def segments(script):
    out = []
    for line in open(script):
        s = line.strip()
        if s.startswith('"2026') and s.count("|") >= 3:
            clip, start, ln, label = s.strip('"').split("|", 3)
            out.append((label.split("—")[0].strip(), int(ln)))
    return out

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("reel")
    ap.add_argument("--segments", default="scripts/make-demo-reel.sh")
    a = ap.parse_args()

    segs = segments(a.segments)
    vid, fps = frames(a.reel)
    g = vid.mean(axis=3)
    mot = np.zeros(len(g)); mot[1:] = np.abs(np.diff(g, axis=0)).mean(axis=(1, 2))

    print(f"{'segment':14s} {'window':>12s} {'lum':>6s} {'colours':>8s} {'motion':>7s} {'still%':>7s}  verdict")
    t, bad = 0, 0
    for name, L in segs:
        i0, i1 = int(t*fps) + 2, int((t+L)*fps) - 2      # trim the label/crossfade edges
        seg, mo = vid[i0:i1], mot[i0+1:i1]
        lum = seg.mean()
        cols = len(np.unique((seg // 16).astype(np.uint8).reshape(-1, 3), axis=0))
        still = 100.0 * (mo < 0.3).mean()
        # no-signal: a uniform field, i.e. no spatial structure at all
        flat = float(np.mean([f.std() for f in seg[-4:]]))
        v = []
        if flat < 1.5 and lum < 12: v.append("DEAD SIGNAL")
        elif lum < 6:               v.append("DARK")
        if still > 60 and not name.startswith(STATIC_OK): v.append("FROZEN")
        bad += len(v)
        print(f"{name:14s} {t:4.0f}-{t+L:4.0f}s {lum:6.1f} {cols:8d} "
              f"{mo.mean():7.2f} {still:6.0f}%  {'; '.join(v) if v else 'ok'}")
        t += L
    print("PASS — every segment has a live signal and content" if bad == 0
          else f"FAIL — {bad} issue(s)")
    return 1 if bad else 0

if __name__ == "__main__":
    sys.exit(main())
