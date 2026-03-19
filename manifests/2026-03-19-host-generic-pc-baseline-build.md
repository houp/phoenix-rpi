# Integration State

## Summary

- Date: 2026-03-19
- Step name: Baseline upstream Phoenix build in `phoenix-dev`
- Scope: Verify one clean upstream Phoenix build inside the Linux VM using the documented sibling-clone buildroot workflow
- Validation lanes used: Linux VM build
- Result: success

## Repositories

| Repository | Commit SHA | Status |
| --- | --- | --- |
| libphoenix | 74852cbbd30bc6b3bcb60a5e2685d25fbeabf0a5 | used |
| phoenix-rtos-build | 488bdb942a1aca100ab28ecfd7fcabb56046ba91 | used |
| phoenix-rtos-corelibs | ff6870be35405ee63bac73b155816f62d05f755d | used |
| phoenix-rtos-devices | ecdd14f103634f1d72e11b1364311ca3c979d43a | used |
| phoenix-rtos-doc | 77d6931bc1bd20e651227cc5f4760f6b5228b3a2 | present |
| phoenix-rtos-filesystems | 34f9c43c9bc34f75b19ffd2144bb038f04f7c92d | used |
| phoenix-rtos-hostutils | 3865813e559bbd4c63b47e8e30ed490c3ef910ff | used |
| phoenix-rtos-kernel | cbfa5d037fda72c31c50b5ed9108012313856923 | present |
| phoenix-rtos-lwip | a40f098a5e9446ef705e3c836648f98e423a1ec8 | present |
| phoenix-rtos-ports | 53a9a3f3a5844a07713b4276328aeb15ce160683 | present |
| phoenix-rtos-posixsrv | 45250145e393e0aa468e6a9bbde349d903e9eef3 | present |
| phoenix-rtos-project | f25a4953501e1b63a7835164a08d77cf08570bc0 | used |
| phoenix-rtos-tests | 6ed92c40f8d0aaee1202a43e346ecc5d5a7fcb3d | used |
| phoenix-rtos-usb | 3ffbe3cc72e104a7e5b1db595b0fe0d08ad3f792 | present |
| phoenix-rtos-utils | c855109dc526d4c3cf8cd23db72d499a3f58367a | present |
| plo | a6f2ffb6af74d499a018086de10bf96123f3c2ed | present |

## Validation Evidence

- Build command:
  `TARGET=host-generic-pc ./phoenix-rtos-build/build.sh clean host core fs test project image`
- Buildroot path:
  `/home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project`
- Verified output paths:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project/_build/host-generic-pc`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project/_fs/host-generic-pc/root`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project/_boot/host-generic-pc`

## Notes

- New constraints discovered:
  the current VM bootstrap is sufficient for host-side Phoenix builds, but not yet for AArch64 target validation because `aarch64-phoenix-gcc` is not installed
- Docs updated:
  `docs/status.md` and tracking files
- Next smallest task:
  provision the Phoenix AArch64 toolchain in `phoenix-dev`, then start the first small AArch64 build-glue refactor
