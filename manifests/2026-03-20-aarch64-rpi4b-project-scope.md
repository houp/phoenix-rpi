# Manifest: First Pi 4 Project-Local Scaffold Scope

- Date: `2026-03-20`
- Step: `STEP-0098`
- Result: `completed`

## Scope

- inspect the current generic runtime boundary together with the Pi 4 platform notes
- choose the smallest Pi 4-specific scaffold change that can be validated without real hardware
- stop before implementing that scaffold change

## Findings

- the existing `aarch64a53-generic` target family/subfamily already provides the reusable generic AArch64 loader, kernel, and userspace build flow needed for early Pi 4 project scaffolding
- introducing a new Pi 4 family or subfamily immediately would force avoidable changes across multiple repos before any board-local value is added
- Phoenix project overrides already support project-local `build.project` and `board_config.h`, which is enough for the first board-specific step

## Selected Next Step

- add a new `phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/` project
- keep target reuse in `aarch64a53-generic`
- add Pi 4-specific `board_config.h` seeded with the official PL011 MMIO base and current documented UART0 default clock
- validate the new project with a no-hardware `host project image` build in `phoenix-dev`
