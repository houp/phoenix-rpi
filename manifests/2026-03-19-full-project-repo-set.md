# Integration State

## Summary

- Date: 2026-03-19
- Step name: Full local Phoenix repo inventory aligned with `phoenix-rtos-project/.gitmodules`
- Scope: Clone the remaining sibling repositories needed for a clean local `phoenix-rtos-project` build and record the resulting SHAs
- Validation lanes used: `.gitmodules` inventory comparison and SHA verification
- Result: success

## Repositories

| Repository | Remote URL | Branch | Commit SHA | Status |
| --- | --- | --- | --- | --- |
| libphoenix | https://github.com/phoenix-rtos/libphoenix | master | 74852cbbd30bc6b3bcb60a5e2685d25fbeabf0a5 | cloned |
| phoenix-rtos-build | https://github.com/phoenix-rtos/phoenix-rtos-build.git | master | 488bdb942a1aca100ab28ecfd7fcabb56046ba91 | cloned |
| phoenix-rtos-corelibs | https://github.com/phoenix-rtos/phoenix-rtos-corelibs/ | master | ff6870be35405ee63bac73b155816f62d05f755d | cloned |
| phoenix-rtos-devices | https://github.com/phoenix-rtos/phoenix-rtos-devices.git | master | ecdd14f103634f1d72e11b1364311ca3c979d43a | cloned |
| phoenix-rtos-doc | https://github.com/phoenix-rtos/phoenix-rtos-doc | master | 77d6931bc1bd20e651227cc5f4760f6b5228b3a2 | cloned |
| phoenix-rtos-filesystems | https://github.com/phoenix-rtos/phoenix-rtos-filesystems.git | master | 34f9c43c9bc34f75b19ffd2144bb038f04f7c92d | cloned |
| phoenix-rtos-hostutils | https://github.com/phoenix-rtos/phoenix-rtos-hostutils/ | master | 3865813e559bbd4c63b47e8e30ed490c3ef910ff | cloned |
| phoenix-rtos-kernel | https://github.com/phoenix-rtos/phoenix-rtos-kernel.git | master | cbfa5d037fda72c31c50b5ed9108012313856923 | cloned |
| phoenix-rtos-lwip | https://github.com/phoenix-rtos/phoenix-rtos-lwip | master | a40f098a5e9446ef705e3c836648f98e423a1ec8 | cloned |
| phoenix-rtos-ports | https://github.com/phoenix-rtos/phoenix-rtos-ports | master | 53a9a3f3a5844a07713b4276328aeb15ce160683 | cloned |
| phoenix-rtos-posixsrv | https://github.com/phoenix-rtos/phoenix-rtos-posixsrv | master | 45250145e393e0aa468e6a9bbde349d903e9eef3 | cloned |
| phoenix-rtos-project | https://github.com/phoenix-rtos/phoenix-rtos-project.git | master | f25a4953501e1b63a7835164a08d77cf08570bc0 | cloned |
| phoenix-rtos-tests | https://github.com/phoenix-rtos/phoenix-rtos-tests.git | master | 6ed92c40f8d0aaee1202a43e346ecc5d5a7fcb3d | cloned |
| phoenix-rtos-usb | https://github.com/phoenix-rtos/phoenix-rtos-usb | master | 3ffbe3cc72e104a7e5b1db595b0fe0d08ad3f792 | cloned |
| phoenix-rtos-utils | https://github.com/phoenix-rtos/phoenix-rtos-utils/ | master | c855109dc526d4c3cf8cd23db72d499a3f58367a | cloned |
| plo | https://github.com/phoenix-rtos/plo.git | master | a6f2ffb6af74d499a018086de10bf96123f3c2ed | cloned |

## Validation Evidence

- Compared the local `sources/` directory set against `sources/phoenix-rtos-project/.gitmodules`
- Verified each repository resolves a concrete HEAD SHA locally

## Notes

- New constraints discovered:
  `phoenix-rtos-project` still expects a populated submodule-shaped tree, so sibling clones alone are not yet enough for builds without an explicit local buildroot strategy
- Docs updated:
  `docs/status.md`, `docs/git-repository-strategy.md`, `docs/manual-operator-instructions.md`, `docs/source-artifacts.md`, and tracking files
- Next smallest task:
  define the local buildroot wiring between `phoenix-rtos-project` and the sibling clones, then verify one clean baseline Phoenix build
