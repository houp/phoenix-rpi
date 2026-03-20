# Manifest: Generic AArch64 QEMU First Boot-Output Fix Scope

- Date: `2026-03-20`
- Step: `STEP-0062`
- Result: `completed`

## Scope

- inspect the silent generic QEMU lane after QEMU startup began succeeding
- compare the generic `plo` console assumptions against the QEMU `virt,secure=on` UART layout
- choose the smallest first output-recovery change

## Upstream Repositories

- none

## Findings

- the generic `plo` console is hardwired to `UART0_BASE_ADDRESS = 0x09000000`
- a local QEMU `virt,secure=on,gic-version=2` DTB dump shows:
  - `chosen.stdout-path = "/pl011@9000000"`
  - `secure-chosen.stdout-path = "/pl011@9040000"`
- the current launcher routes two serial backends with:
  - `-serial null`
  - `-serial mon:stdio`

## Selected Fix

- keep the current generic `plo` console address unchanged
- change only the launcher serial routing so the non-secure PL011 at `0x09000000` is sent to stdio first

## Notes

- this is preferred over changing the generic `plo` console or AArch64 entry path because the current evidence points to output being discarded before any loader code can be observed
- if the rerun still stays silent after the serial-routing fix, the next step should inspect earliest `plo` entry behavior instead of widening further in the launcher

## Selected Next Step

- route the non-secure generic QEMU PL011 to stdio and rerun the unchanged smoke command
