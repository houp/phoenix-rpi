# Integration State

## Summary

- Date: 2026-03-19
- Step name: Baseline Phoenix upstream clones
- Scope: Initialize local upstream worktrees under `sources/` and record their clean starting SHAs
- Validation lanes used: repository clone and SHA verification
- Result: success

## Repositories

| Repository | Remote URL | Branch | Commit SHA | Status |
| --- | --- | --- | --- | --- |
| phoenix-rtos-kernel | https://github.com/phoenix-rtos/phoenix-rtos-kernel.git | master | cbfa5d037fda72c31c50b5ed9108012313856923 | cloned |
| plo | https://github.com/phoenix-rtos/plo.git | master | a6f2ffb6af74d499a018086de10bf96123f3c2ed | cloned |
| phoenix-rtos-devices | https://github.com/phoenix-rtos/phoenix-rtos-devices.git | master | ecdd14f103634f1d72e11b1364311ca3c979d43a | cloned |
| phoenix-rtos-filesystems | https://github.com/phoenix-rtos/phoenix-rtos-filesystems.git | master | 34f9c43c9bc34f75b19ffd2144bb038f04f7c92d | cloned |
| phoenix-rtos-build | https://github.com/phoenix-rtos/phoenix-rtos-build.git | master | 488bdb942a1aca100ab28ecfd7fcabb56046ba91 | cloned |
| phoenix-rtos-project | https://github.com/phoenix-rtos/phoenix-rtos-project.git | master | f25a4953501e1b63a7835164a08d77cf08570bc0 | cloned |
| phoenix-rtos-tests | https://github.com/phoenix-rtos/phoenix-rtos-tests.git | master | 6ed92c40f8d0aaee1202a43e346ecc5d5a7fcb3d | cloned |

## Validation Evidence

- Emulator: not applicable
- Hardware: not applicable
- UART log location: not applicable
- Image or boot-tree location: not applicable

## Notes

- New constraints discovered:
  this manifest captures only the initial coordination-selected repo subset; `phoenix-rtos-project/.gitmodules` later showed that a broader sibling repo set is required for local builds
- Docs updated:
  tracking and readiness state will be updated after this manifest
- Next smallest task:
  complete the remaining project submodule repositories as sibling clones and then define the local buildroot wiring for clean builds
