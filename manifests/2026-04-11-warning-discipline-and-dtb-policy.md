# Warning Discipline And DTB Policy Tightening

Date:

- `2026-04-11`

Purpose:

- make warnings and recoverable tool errors visible, actionable, and less
  repeatable across future sessions
- apply extra caution to DTS, DTSI, and DTB handling because those artifacts
  can directly affect boot

Policy changes:

- warnings and recoverable errors must now be:
  - surfaced to the user
  - classified as important, risky, or benign-but-removable
  - fixed or explicitly justified in the same session where they occur
- DTB-source preference is now:
  1. final-form authoritative DTB blob
  2. project-local validated DTB blob
  3. only as fallback, local DTS compilation

Helper changes:

- [prepare-rpi4b-dtb.sh](/Users/witoldbolt/phoenix-rpi/scripts/prepare-rpi4b-dtb.sh)
  now:
  - prefers final DTB blobs over local DTS compilation
  - treats DTS / DTB warnings as significant by default
  - aborts on warnings unless `RPI4B_DTB_ALLOW_WARNINGS=1` is set
  - avoids duplicate post-compile warning spam by validating copied DTBs but
    not re-reporting the same compile warnings twice

Observed strict-helper result:

- current command:
  - [prepare-rpi4b-dtb.sh](/Users/witoldbolt/phoenix-rpi/scripts/prepare-rpi4b-dtb.sh)
- current result:
  - failed as designed under strict warning policy
- surfaced warning classes from the Raspberry Pi Linux DTS tree:
  - `unit_address_vs_reg`
  - `simple_bus_reg`
  - `avoid_unnecessary_addr_size`
  - `unique_unit_address`
  - `gpios_property`

Current decision:

- do not silently suppress or ignore those warnings
- prefer final DTB blobs for Pi 4 work whenever possible
- only use the warning-tolerant override if the session explicitly documents
  why that fallback is still acceptable
