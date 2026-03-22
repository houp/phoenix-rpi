# 2026-03-22: Pi 4 report-helper image-fingerprint refresh

## Scope

Close `STEP-0406` by making the first-trial report helper derive the current
image fingerprint from the actual exported SD image.

## Changes

Updated:

- `/Users/witoldbolt/phoenix-rpi/scripts/create-rpi4b-first-trial-report.sh`
- `/Users/witoldbolt/phoenix-rpi/docs/status.md`

## Validation

Executed:

```sh
/Users/witoldbolt/phoenix-rpi/scripts/create-rpi4b-first-trial-report.sh
```

Observed generated file:

- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-reports/pi4-first-trial-20260322-030706.md`

Confirmed content includes:

- current image path
- SHA-256 derived from the actual exported image file

## Outcome

The report helper no longer depends on a stale hardcoded checksum by default.
