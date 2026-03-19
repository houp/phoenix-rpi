# Manifests

This directory is for recording tested multi-repository integration states.

Use it because the Phoenix Raspberry Pi port will span multiple upstream git repositories that need to be kept in sync without collapsing them into one repository.

Recommended usage:

1. copy [integration-state-template.md](/Users/witoldbolt/phoenix-rpi/manifests/integration-state-template.md) to a dated or milestone-specific file
2. fill in exact repository SHAs after each validated implementation step
3. commit the manifest update in this coordination repo

Example future filenames:

- `2026-03-25-baseline-clones.md`
- `2026-03-28-rpi4-plo-uart-boot.md`
- `2026-04-02-common-aarch64-dtb-parser.md`
