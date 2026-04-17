#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import sys
from typing import Any

from rpi4_actled_probe_layout import get_layout, layout_to_dict


def load_json(path: str) -> dict[str, Any]:
    if path == "-":
        return json.load(sys.stdin)
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def extract_decodes(analysis: dict[str, Any]) -> list[dict[str, Any]]:
    decodes: list[dict[str, Any]] = []
    for group in analysis.get("stage_groups", []):
        decode = group.get("decode")
        if not decode:
            continue
        if not decode.get("valid", False):
            continue
        code = decode.get("code")
        if isinstance(code, int):
            decodes.append(group)
    return decodes


def extract_pulse_groups(analysis: dict[str, Any], allowed_codes: set[int]) -> list[dict[str, Any]]:
    groups: list[dict[str, Any]] = []
    for group in analysis.get("stage_groups", []):
        pulse_count = group.get("pulse_count")
        if isinstance(pulse_count, int) and pulse_count in allowed_codes:
            groups.append(group)
    return groups


def build_interpretation(analysis: dict[str, Any], layout_name: str) -> dict[str, Any]:
    layout = get_layout(layout_name)
    if layout.mode == "pulse_count":
        valid_groups = extract_pulse_groups(analysis, set(layout.stages))
    else:
        valid_groups = extract_decodes(analysis)
    stage_index = {code: idx for idx, code in enumerate(layout.stage_order)}
    special_codes = {code for code in layout.stages if code not in stage_index}

    best_groups: list[dict[str, Any]] = []
    best_expected_idx = 0

    for start_idx, group in enumerate(valid_groups):
        if layout.mode == "pulse_count":
            start_code = int(group["pulse_count"])
        else:
            start_code = int(group["decode"]["code"])
        if start_code not in stage_index:
            continue

        expected_idx = stage_index[start_code] + 1
        candidate = [group]
        last_code = start_code

        for later in valid_groups[start_idx + 1 :]:
            if layout.mode == "pulse_count":
                code = int(later["pulse_count"])
            else:
                code = int(later["decode"]["code"])
            if code == last_code:
                continue
            if expected_idx < len(layout.stage_order) and code == layout.stage_order[expected_idx]:
                candidate.append(later)
                last_code = code
                expected_idx += 1

        if not best_groups:
            best_groups = candidate
            best_expected_idx = expected_idx
            continue

        best_start_code = int(best_groups[0]["decode"]["code"])
        if len(candidate) > len(best_groups):
            best_groups = candidate
            best_expected_idx = expected_idx
        elif len(candidate) == len(best_groups) and start_code < best_start_code:
            best_groups = candidate
            best_expected_idx = expected_idx
        elif len(candidate) == len(best_groups) and start_code == best_start_code:
            if int(candidate[-1]["decode"]["code"]) > int(best_groups[-1]["decode"]["code"]):
                best_groups = candidate
                best_expected_idx = expected_idx

    matched: list[dict[str, Any]] = []
    for group in best_groups:
        if layout.mode == "pulse_count":
            code = int(group["pulse_count"])
            bits = ""
        else:
            code = int(group["decode"]["code"])
            bits = group["decode"]["bits"]
        stage = layout.stages[code]
        matched.append(
            {
                "code": code,
                "bits": bits,
                "label": stage.label,
                "meaning": stage.meaning,
                "group_index": group["group_index"],
                "start_s": group["start_s"],
                "end_s": group["end_s"],
            }
        )

    matched_ids = {item["group_index"] for item in matched}
    unmatched = []
    for group in valid_groups:
        if group["group_index"] in matched_ids:
            continue
        unmatched.append(group)

    highest = matched[-1] if matched else None
    next_expected_code = layout.stage_order[best_expected_idx] if best_expected_idx < len(layout.stage_order) else None
    next_expected = None
    if next_expected_code is not None:
        stage = layout.stages[next_expected_code]
        next_expected = {
            "code": next_expected_code,
            "bits": stage.bits(layout.code_bits),
            "label": stage.label,
            "meaning": stage.meaning,
        }

    special_terminal = None
    for group in valid_groups:
        if layout.mode == "pulse_count":
            code = int(group["pulse_count"])
        else:
            code = int(group["decode"]["code"])
        if code not in special_codes:
            continue
        if highest is not None and group["start_s"] < highest["end_s"]:
            continue
        stage = layout.stages[code]
        special_terminal = {
            "code": code,
            "bits": "" if layout.mode == "pulse_count" else group["decode"]["bits"],
            "label": stage.label,
            "meaning": stage.meaning,
            "group_index": group["group_index"],
            "start_s": group["start_s"],
            "end_s": group["end_s"],
        }
        break

    if highest is None:
        inference = "No valid Phoenix stage burst decoded."
    elif special_terminal is not None:
        inference = (
            f"Best contiguous decoded run reaches stage {highest['code']} ({highest['label']}), "
            f"then special terminal stage {special_terminal['code']} ({special_terminal['label']}) appears."
        )
    elif next_expected is None:
        inference = f"Reached final known stage {highest['code']} ({highest['label']})."
    else:
        inference = (
            f"Best contiguous decoded run reaches stage {highest['code']} ({highest['label']}), "
            f"no later valid stage {next_expected['code']} ({next_expected['label']}) seen."
        )

    return {
        "layout": layout_to_dict(layout),
        "matched_sequence": matched,
        "highest_completed": highest,
        "next_expected": next_expected,
        "special_terminal": special_terminal,
        "inference": inference,
        "valid_group_count": len(valid_groups),
        "unmatched_groups": [
            {
                "group_index": group["group_index"],
                "start_s": group["start_s"],
                "end_s": group["end_s"],
                "decode": group.get("decode"),
                "pulse_count": group.get("pulse_count"),
            }
            for group in unmatched
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Interpret Raspberry Pi 4 ACT LED analysis JSON")
    parser.add_argument("analysis_json", help="Path to analysis JSON or - for stdin")
    parser.add_argument("--layout", default="current", help="Probe layout name")
    parser.add_argument("--json", action="store_true", help="Emit JSON instead of text")
    args = parser.parse_args()

    analysis = load_json(args.analysis_json)
    interpretation = build_interpretation(analysis, args.layout)

    if args.json:
        json.dump(interpretation, sys.stdout, indent=2, sort_keys=True)
        sys.stdout.write("\n")
        return 0

    print(f"layout={interpretation['layout']['name']}")
    print(f"inference={interpretation['inference']}")

    highest = interpretation["highest_completed"]
    if highest is not None:
        print(
            "highest_completed="
            f"{highest['code']} {highest['bits']} {highest['label']} "
            f"@ group {highest['group_index']} {highest['start_s']:.3f}-{highest['end_s']:.3f}s"
        )
    else:
        print("highest_completed=none")

    next_expected = interpretation["next_expected"]
    if next_expected is not None:
        print(
            "next_expected="
            f"{next_expected['code']} {next_expected['bits']} {next_expected['label']}"
        )

    special_terminal = interpretation["special_terminal"]
    if special_terminal is not None:
        print(
            "special_terminal="
            f"{special_terminal['code']} {special_terminal['bits']} {special_terminal['label']} "
            f"@ group {special_terminal['group_index']} "
            f"{special_terminal['start_s']:.3f}-{special_terminal['end_s']:.3f}s"
        )

    print("matched_sequence:")
    for item in interpretation["matched_sequence"]:
        print(
            f"  {item['code']:>2} {item['bits']} {item['label']} "
            f"{item['start_s']:.3f}-{item['end_s']:.3f}s"
        )

    if interpretation["unmatched_groups"]:
        print("unmatched_groups:")
        for group in interpretation["unmatched_groups"]:
            decode = group.get("decode", {})
            label = ""
            if interpretation["layout"].get("mode") == "pulse_count":
                code = group.get("pulse_count")
            else:
                code = decode.get("code")
            if isinstance(code, int) and str(code) in interpretation["layout"]["stages"]:
                label = f" {interpretation['layout']['stages'][str(code)]['label']}"
            print(
                f"  group {group['group_index']}: "
                f"{decode.get('bits', '')} ({code if code is not None else '?'}){label} "
                f"{group['start_s']:.3f}-{group['end_s']:.3f}s"
            )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
