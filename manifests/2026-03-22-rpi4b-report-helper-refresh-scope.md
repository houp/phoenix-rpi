# 2026-03-22: Pi 4 report-helper refresh scope

## Scope

Add one bounded robustness improvement to the operator-side report helper:

- derive the current image SHA-256 from the actual exported file instead of
  relying on a stale default value

## Why This Fits

This is still operator-side only. It reduces mismatch risk after future image
refreshes without changing runtime behavior or widening into new automation.
