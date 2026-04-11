#!/usr/bin/env python3

from __future__ import annotations

from dataclasses import asdict, dataclass


@dataclass(frozen=True)
class StageDef:
    code: int
    label: str
    meaning: str
    source_file: str
    source_symbol: str

    def bits(self, code_bits: int) -> str:
        return format(self.code, f"0{code_bits}b")


@dataclass(frozen=True)
class LayoutDef:
    name: str
    description: str
    code_bits: int
    sync_pulses: int
    stage_order: tuple[int, ...]
    stages: dict[int, StageDef]
    notes: tuple[str, ...]


CURRENT_LAYOUT_NAME = "pi4_firmware_entry_contract_map_2026_04_11"


_CURRENT_LAYOUT = LayoutDef(
    name=CURRENT_LAYOUT_NAME,
    description="Pi 4 compact GPIO42 telemetry with duplicated focus-stage bursts around the restored firmware handoff contract: dtb_ptr32, kernel_entry32, kernel fallback to 0x80000, and branch into plo",
    code_bits=5,
    sync_pulses=1,
    stage_order=(1, 2, 3, 23, 24, 25, 26, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22),
    stages={
        1: StageDef(
            code=1,
            label="armstub primary entry",
            meaning="Primary-core custom armstub entry reached.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S",
            source_symbol="gpio42_stage1",
        ),
        2: StageDef(
            code=2,
            label="armstub after timer/gic",
            meaning="Custom armstub completed early local timer and GIC preparation.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S",
            source_symbol="gpio42_stage2",
        ),
        3: StageDef(
            code=3,
            label="armstub before firmware jump",
            meaning="Custom armstub reached the final pre-handoff point before loading the firmware-provided entry slots.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S",
            source_symbol="gpio42_stage3",
        ),
        23: StageDef(
            code=23,
            label="armstub late seam entry",
            meaning="Primary-core armstub entered the restored firmware-handoff seam after stage 3.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S",
            source_symbol="primary_cpu_late",
        ),
        24: StageDef(
            code=24,
            label="armstub dtb_ptr32 loaded",
            meaning="Loaded the firmware-provided DTB pointer from dtb_ptr32 into the temporary handoff register.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S",
            source_symbol="primary_cpu_late",
        ),
        25: StageDef(
            code=25,
            label="armstub kernel_entry32 loaded",
            meaning="Loaded the firmware-provided kernel entry from kernel_entry32 into x4.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S",
            source_symbol="primary_cpu_late",
        ),
        26: StageDef(
            code=26,
            label="armstub kernel entry nonzero",
            meaning="The firmware-provided kernel entry was nonzero, and the armstub is about to branch after restoring x0.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S",
            source_symbol="primary_cpu_late",
        ),
        29: StageDef(
            code=29,
            label="armstub dtb fallback x0",
            meaning="The dtb_ptr32 slot was zero, so the armstub fell back to the firmware entry x0 value for the DTB pointer.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S",
            source_symbol="primary_cpu_late",
        ),
        30: StageDef(
            code=30,
            label="armstub kernel fallback 0x80000",
            meaning="The kernel_entry32 slot was zero, so the armstub fell back to the observed Pi 4 firmware relocation target 0x80000.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S",
            source_symbol="primary_cpu_late",
        ),
        4: StageDef(
            code=4,
            label="armstub branch imminent",
            meaning="Custom armstub completed the firmware-slot handoff preparation and is about to branch into plo.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S",
            source_symbol="gpio42_stage4",
        ),
        5: StageDef(
            code=5,
            label="fixed entry veneer",
            meaning="The fixed-address branch target veneer in plo was entered.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S",
            source_symbol="_start",
        ),
        6: StageDef(
            code=6,
            label="generic _start body",
            meaning="The old generic plo _start body was reached after the fixed-entry veneer.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S",
            source_symbol="_start_real",
        ),
        7: StageDef(
            code=7,
            label="after clear x0-x7",
            meaning="Reached the first quarter of the register-clearing block.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S",
            source_symbol="_start_real",
        ),
        8: StageDef(
            code=8,
            label="after clear x8-x15",
            meaning="Reached the second quarter of the register-clearing block.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S",
            source_symbol="_start_real",
        ),
        9: StageDef(
            code=9,
            label="after clear x16-x23",
            meaning="Reached the third quarter of the register-clearing block.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S",
            source_symbol="_start_real",
        ),
        10: StageDef(
            code=10,
            label="after clear x24-x30",
            meaning="Completed the register-clearing block.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S",
            source_symbol="_start_real",
        ),
        11: StageDef(
            code=11,
            label="after dsb/isb",
            meaning="Reached the barrier pair just before sampling currentEL.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S",
            source_symbol="_start_real",
        ),
        12: StageDef(
            code=12,
            label="after mrs currentEL",
            meaning="Sampled currentEL successfully.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S",
            source_symbol="_start_real",
        ),
        13: StageDef(
            code=13,
            label="start_el3",
            meaning="Entered the EL3-specific plo path.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S",
            source_symbol="start_el3",
        ),
        14: StageDef(
            code=14,
            label="start_el2",
            meaning="Entered the EL2-specific plo path.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S",
            source_symbol="start_el2",
        ),
        15: StageDef(
            code=15,
            label="start_el1",
            meaning="Entered the EL1-specific plo path.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S",
            source_symbol="start_el1",
        ),
        16: StageDef(
            code=16,
            label="el3 path complete",
            meaning="Completed EL3 path, just before start_common.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S",
            source_symbol="start_el3",
        ),
        17: StageDef(
            code=17,
            label="el2 path complete",
            meaning="Completed EL2 path, just before start_common.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S",
            source_symbol="start_el2",
        ),
        18: StageDef(
            code=18,
            label="el1 path complete",
            meaning="Completed EL1 path, just before start_common.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S",
            source_symbol="start_el1",
        ),
        19: StageDef(
            code=19,
            label="start_common",
            meaning="Entered start_common.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S",
            source_symbol="start_common",
        ),
        20: StageDef(
            code=20,
            label="after stack setup",
            meaning="Completed early stack setup in start_common.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S",
            source_symbol="start_common",
        ),
        21: StageDef(
            code=21,
            label="core-0 branch to _startc",
            meaning="Reached the core-0-only branch into _startc after parking secondary CPUs.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S",
            source_symbol="start_core_0",
        ),
        22: StageDef(
            code=22,
            label="unexpected EL trap",
            meaning="Reached the generic AArch64 unexpected-EL trap path.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S",
            source_symbol="start_el_unknown",
        ),
        0: StageDef(
            code=0,
            label="armstub el2 exception",
            meaning="An EL2 exception vector was taken during the armstub diagnostic seam and the handler halted after emitting code 0.",
            source_file="/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S",
            source_symbol="armstub_exception",
        ),
    },
    notes=(
        "Compact GPIO42 protocol: one sync pulse, then 5 bits MSB-first.",
        "Short on-time encodes 0, long on-time encodes 1.",
        "Long off gap separates stage bursts.",
        "Focus seam stages are emitted twice with an extra long gap to reduce decode ambiguity in phone video.",
        "The current firmware-handoff seam reads dtb_ptr32 and kernel_entry32 from the armstub slots, but falls back to entry x0 for DTB and 0x80000 for kernel entry if the slots are empty.",
        "Initial ACT LED activity during firmware SD-card reads is preamble noise and must be ignored unless it decodes into a later valid contiguous Phoenix stage run.",
        "Stage 29 is a special DTB-fallback stage, not part of the normal contiguous boot sequence.",
        "Stage 30 is a special kernel-entry fallback stage, not part of the normal contiguous boot sequence.",
        "Stage 0 is a special EL2 exception-fault stage, not part of the normal contiguous boot sequence.",
    ),
)


LAYOUTS: dict[str, LayoutDef] = {
    CURRENT_LAYOUT_NAME: _CURRENT_LAYOUT,
    "current": _CURRENT_LAYOUT,
}


def get_layout(name: str = "current") -> LayoutDef:
    try:
        return LAYOUTS[name]
    except KeyError as exc:
        known = ", ".join(sorted(LAYOUTS))
        raise KeyError(f"unknown layout '{name}', known: {known}") from exc


def layout_to_dict(layout: LayoutDef) -> dict:
    return {
        "name": layout.name,
        "description": layout.description,
        "code_bits": layout.code_bits,
        "sync_pulses": layout.sync_pulses,
        "stage_order": list(layout.stage_order),
        "stages": {
            str(code): {
                **asdict(stage),
                "bits": stage.bits(layout.code_bits),
            }
            for code, stage in sorted(layout.stages.items())
        },
        "notes": list(layout.notes),
    }
