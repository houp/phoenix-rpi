#!/usr/bin/env python3

import argparse
import math
from dataclasses import dataclass

import cv2
import numpy as np


@dataclass
class Segment:
    start: int
    end: int

    @property
    def frames(self) -> int:
        return self.end - self.start + 1


def parse_roi(value: str) -> tuple[int, int, int, int]:
    parts = [int(part) for part in value.split(",")]
    if len(parts) != 4:
        raise argparse.ArgumentTypeError("ROI must be x1,y1,x2,y2")
    x1, y1, x2, y2 = parts
    if x2 < x1 or y2 < y1:
        raise argparse.ArgumentTypeError("ROI must satisfy x2>=x1 and y2>=y1")
    return x1, y1, x2, y2


def kmeans_threshold(values: np.ndarray, p_lo: float = 10, p_hi: float = 90) -> tuple[float, float, float]:
    lo = float(np.percentile(values, p_lo))
    hi = float(np.percentile(values, p_hi))

    for _ in range(16):
        midpoint = (lo + hi) / 2.0
        lower = values[values <= midpoint]
        upper = values[values > midpoint]
        if len(lower) == 0 or len(upper) == 0:
            break
        new_lo = float(lower.mean())
        new_hi = float(upper.mean())
        if abs(new_lo - lo) < 1e-6 and abs(new_hi - hi) < 1e-6:
            break
        lo, hi = new_lo, new_hi

    return lo, hi, (lo + hi) / 2.0


def kmeans3(values: np.ndarray) -> tuple[float, float, float]:
    centers = [
        float(np.percentile(values, 15)),
        float(np.percentile(values, 50)),
        float(np.percentile(values, 85)),
    ]

    for _ in range(24):
        buckets = [[], [], []]
        for value in values:
            idx = min(range(3), key=lambda i: abs(value - centers[i]))
            buckets[idx].append(float(value))

        new_centers = []
        for idx, bucket in enumerate(buckets):
            if bucket:
                new_centers.append(float(np.mean(bucket)))
            else:
                new_centers.append(centers[idx])

        if all(abs(a - b) < 1e-6 for a, b in zip(new_centers, centers)):
            break
        centers = new_centers

    centers.sort()
    return centers[0], centers[1], centers[2]


def merge_short_gaps(mask: np.ndarray, max_gap_frames: int) -> np.ndarray:
    mask = mask.copy()
    i = 0
    n = len(mask)

    while i < n:
        if mask[i]:
            i += 1
            continue

        gap_start = i
        while i < n and not mask[i]:
            i += 1
        gap_end = i - 1

        if gap_start == 0 or gap_end == n - 1:
            continue

        if gap_end - gap_start + 1 <= max_gap_frames:
            mask[gap_start : gap_end + 1] = True

    return mask


def mask_to_segments(mask: np.ndarray, min_frames: int) -> list[Segment]:
    segments: list[Segment] = []
    start = None

    for idx, is_on in enumerate(mask):
        if is_on and start is None:
            start = idx
        elif not is_on and start is not None:
            if idx - start >= min_frames:
                segments.append(Segment(start, idx - 1))
            start = None

    if start is not None and len(mask) - start >= min_frames:
        segments.append(Segment(start, len(mask) - 1))

    return segments


def seconds(frames: int, fps: float) -> float:
    return frames / fps


def main() -> int:
    parser = argparse.ArgumentParser(description="Decode Raspberry Pi 4 ACT LED stage video")
    parser.add_argument("video")
    parser.add_argument("--roi", type=parse_roi, default="92,108,117,118", help="x1,y1,x2,y2")
    parser.add_argument("--smooth", type=int, default=7, help="moving-average window in frames")
    parser.add_argument("--min-on", type=float, default=0.04, help="minimum on segment duration in seconds")
    parser.add_argument("--merge-gap", type=float, default=0.02, help="merge off gaps shorter than this, in seconds")
    parser.add_argument("--group-gap", type=float, default=0.50, help="start a new stage group after this off gap, in seconds")
    args = parser.parse_args()

    x1, y1, x2, y2 = args.roi
    cap = cv2.VideoCapture(args.video)
    if not cap.isOpened():
        raise SystemExit(f"failed to open video: {args.video}")

    fps = cap.get(cv2.CAP_PROP_FPS)
    vals = []

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        roi = frame[y1 : y2 + 1, x1 : x2 + 1].astype(np.int16)
        metric = roi[:, :, 1] - ((roi[:, :, 2] + roi[:, :, 0]) // 2)
        vals.append(float(metric.mean()))

    cap.release()

    values = np.array(vals, dtype=np.float64)
    if len(values) == 0:
        raise SystemExit("video has no frames")

    if args.smooth > 1:
        kernel = np.ones(args.smooth, dtype=np.float64) / args.smooth
        values = np.convolve(values, kernel, mode="same")

    low_mean, high_mean, threshold = kmeans_threshold(values)
    mask = values > threshold

    merge_gap_frames = max(1, int(round(args.merge_gap * fps)))
    min_on_frames = max(1, int(round(args.min_on * fps)))
    mask = merge_short_gaps(mask, merge_gap_frames)
    segments = mask_to_segments(mask, min_on_frames)

    filtered_durations = np.array(
        [seconds(seg.frames, fps) for seg in segments if seconds(seg.frames, fps) < 1.0],
        dtype=np.float64,
    )

    if len(filtered_durations) >= 3:
        short_mean, long_mean, sync_mean = kmeans3(filtered_durations)
        bit_threshold = (short_mean + long_mean) / 2.0
        sync_threshold = (long_mean + sync_mean) / 2.0
    else:
        short_mean = long_mean = sync_mean = bit_threshold = sync_threshold = math.nan

    groups: list[list[Segment]] = []
    current: list[Segment] = []
    for segment in segments:
        duration = seconds(segment.frames, fps)
        if math.isfinite(sync_threshold) and duration >= sync_threshold:
            if current:
                groups.append(current)
            current = [segment]
            continue

        if current and len(current) < 6:
            current.append(segment)

    if current:
        groups.append(current)

    print(f"video={args.video}")
    print(f"fps={fps:.5f}")
    print(f"roi={x1},{y1},{x2},{y2}")
    print(f"threshold={threshold:.4f} low_mean={low_mean:.4f} high_mean={high_mean:.4f}")
    if len(filtered_durations) >= 3:
        print(
            "pulse_classes="
            f"short:{short_mean:.4f}s long:{long_mean:.4f}s sync:{sync_mean:.4f}s "
            f"bit_threshold={bit_threshold:.4f}s sync_threshold={sync_threshold:.4f}s"
        )

    print("\non_segments:")
    for seg in segments:
        print(
            f"  {seconds(seg.start, fps):.3f}-{seconds(seg.end + 1, fps):.3f} "
            f"dur={seconds(seg.frames, fps):.3f}"
        )

    print("\nstage_groups:")
    for idx, group in enumerate(groups, start=1):
        print(f"  group {idx}:")
        for pulse_idx, seg in enumerate(group, start=1):
            label = "sync" if pulse_idx == 1 else f"bit{pulse_idx - 1}"
            print(
                f"    {label}: {seconds(seg.start, fps):.3f}-{seconds(seg.end + 1, fps):.3f} "
                f"dur={seconds(seg.frames, fps):.3f}"
            )

        if len(group) >= 6 and math.isfinite(bit_threshold):
            bits = []
            for seg in group[1:6]:
                bit = "1" if seconds(seg.frames, fps) > bit_threshold else "0"
                bits.append(bit)
            bit_string = "".join(bits)
            print(f"    decode: {bit_string} ({int(bit_string, 2)})")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
