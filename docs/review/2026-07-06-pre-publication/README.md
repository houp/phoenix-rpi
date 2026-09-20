# Pre-publication code review — 2026-07-06

Final full review of the Phoenix-RTOS RPi4 (BCM2711) port before the repos go public.
Goal: human/top-professional-OS-developer quality. No AI slop.

## What we're looking for

1. **Correctness** — logic bugs, races, memory errors, off-by-one, wrong error handling,
   endianness, alignment, integer overflow.
2. **Obvious errors** — typos, copy-paste mistakes, botched merges, dead/unreachable code,
   wrong constants.
3. **Duplication** — copy-pasted blocks that should be shared; near-identical functions.
4. **Hacks / temporary work** — `TODO`, `FIXME`, `HACK`, `XXX`, `TD-NN` markers, diagnostic
   code left in, disproved-hypothesis probes, magic numbers.
5. **Legal** — GPL / Linux-kernel / otherwise-restricted code copied into the Phoenix (BSD)
   repos; vendored third-party code missing its license/attribution.
6. **Quality** — readability, over-long or noisy comments, over-engineering, inconsistent
   style vs the surrounding upstream code, poor names.

## Scope (local changes vs origin/master merge-base, measured 2026-07-06)

| repo | commits | files | lines |
|---|---|---|---|
| phoenix-rtos-devices | 305 | 61 | ~19.5k |
| phoenix-rtos-kernel | 230 | 50 | ~4.0k |
| phoenix-rtos-project | 161 | 33 | ~3.2k |
| phoenix-rtos-lwip | 143 | 17 | ~2.2k |
| plo | 75 | 22 | ~2.3k |
| phoenix-rtos-filesystems | 35 | 14 | ~2.2k |
| phoenix-rtos-utils | 38 | 12 | ~1.0k |
| libphoenix | 33 | 28 | ~1.0k |
| phoenix-rtos-usb | 32 | 8 | ~0.4k |
| phoenix-rtos-build | 13 | 6 | ~0.1k |
| phoenix-rtos-ports | 9 | 13 | ~1.1k |
| phoenix-rtos-posixsrv | 2 | 1 | ~29 |
| phoenix-rtos-corelibs | 1 | 1 | ~16 |

Plus: coordination repo (`phoenix-rpi`) — `tools/` porting glue (v3d-driver-port,
quakespasm-port, x11-port) and `scripts/`; and external forks `external/{mesa,quakespasm,vkquake}`.

## Method

Multi-pass, staying in the loop between passes:
1. Discovery — per-repo changed-file work-list.
2. Review — fan out reviewers per file-group × dimension (structured findings).
3. Verify — adversarially confirm each finding is real (no false positives).
4. Fix — implement safe, high-confidence fixes; build-test; commit per repo.

A prior review exists: `docs/review/2026-06-06-*` (17-area, applied 3 cleanup waves,
documented B1–B14 HW-gated bugs). This pass re-checks everything + all post-06-06 work.

## Findings

See `findings.md` (accumulated, ranked). Fixes applied are noted with commit SHAs.
Tasks/decisions for the user: `PENDING-USER-TASKS.md`.

## Progress log

- **2026-07-06/07 pass 1 (manual complement + workflow launch):**
  - Legal/provenance scan: CLEAN (no GPL/Linux copies; genet fresh w/ behavioral refs;
    bcm2711-emmc is Phoenix's own 2023 driver; teken keeps FreeBSD/Kuhn headers). One low:
    teken_phoenix.h missing Phoenix header.
  - B1–B14 (prior review) re-checked: B4 (main.c cross-arch link break + debug print) and
    B10 (a53 GIC base) still LIVE; B6/B7/B8 resolved.
  - Workflow-gap files reviewed manually (main.c, log.c, posix/*, lib.h, hal/*.h): main.c B4,
    log.c shared-default mirror (med), unix.c misleading "wakeupPending" comment (low).
  - vkQuake port: WIP diagnostics → user decision.
  - Review workflow (26 units, review→verify) launched: run wf_c0a9383a-8ba.
  - NEXT (pass 2): merge workflow findings + manual findings, triage, fix per repo with build
    tests, commit locally. Then deepen coverage of any remaining gaps (tools/demo-apps,
    tools/ports, tools/stress, external/vkquake had no workflow unit).
