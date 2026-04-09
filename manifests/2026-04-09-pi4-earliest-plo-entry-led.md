# Pi 4 Earliest `plo` `_start` GPIO42 Entry Proof

Date: `2026-04-09`

## Scope

Implement the smallest hardware-visible split after the fixed-address custom
armstub handoff experiment.

The new proof must answer whether the current Pi 4 fixed-address branch reaches
the very first `plo` `_start` instructions on real hardware.

## Trigger

The latest real Pi 4 board result on the fixed-address armstub image produced:

- both LEDs on at power-up
- green off
- green briefly on again
- green off again
- green on later and then steady on
- blank screen
- no keyboard-visible reaction

That changed sequence made it clear that the fixed-address handoff altered the
earliest hardware behavior, but still did not prove that `plo` itself had been
entered.

## Change

In [board_config.h](/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/board_config.h):

- add Pi-4-only ACT LED diagnostic constants for the earliest `plo` entry
  experiment

In [generic _init.S](/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S):

- add a Pi-4-only GPIO42 pattern at the very top of `_start`
- run it before the usual register clearing and exception-level path
- pattern used:
  - LED on
  - delay
  - LED off
  - delay
  - LED on
  - delay
  - LED off

## Validation

- Pi 4 A72 rebuild in `phoenix-dev`: pass
- direct Pi 4 QEMU serial-log sanity lane: pass
  - still reaches:
    - `go!`
    - `hal: jump exit el1`
    - `A3`
    - `KLM`
- refreshed SD image export via canonical helper: pass
- host-side FAT-aware image verification: pass

## Rebuilt Artifact

- image:
  [artifacts/rpi4b/rpi4b-sd.img](/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img)
- SHA-256:
  `e5f8662aca8c859464bed6c23e9742afd196bf1136a09f453e9c975e06b6441c`

## Commits

- `plo`: `a562026` `aarch64: add pi4 earliest plo entry led proof`
- `phoenix-rtos-project`: `c1e5268` `project: add pi4 plo led proof constants`

## Next Step

Retry the real Pi 4 board with this image and classify:

- whether the new earliest `plo` pulse is visible as a changed ACT LED pattern
- whether HDMI remains blank
- whether any keyboard-visible reaction appears
