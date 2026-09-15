#!/usr/bin/env python3
"""Score a V3D_BO_TRACE=1 run for BO double-ownership.

Capture with:

    ./scripts/test-cycle-psh-interact.sh --label botrace --wait-secs 220 \
        --inter-cmd-secs 8 --idle-secs 300 --max-cmd-secs 330 -- \
        "export V3D_BO_TRACE=1" "stk --track=hacienda --numkarts=4 --profile-laps=2"

then run this on the UART log.

WHICH NUMBER MEANS SOMETHING
----------------------------
The obvious metric -- "how many CPU addresses served more than one BO handle" --
is the WRONG one on a build where ioc_close_bo() unmaps (devices 7a1e3db, which
is the default). Once a closed BO's mapping is really returned to the process,
the address is free and a later mmap is *supposed* to be able to take it. A
healthy 2-lap SuperTuxKart run shows ~117 such reuses; reading that as a
regression against the historical "56" would be a false alarm, because the 56
was measured with unmapping OFF, where any reuse necessarily meant two live
owners.

The metric that means something is TEMPORAL OVERLAP: a CREATE or MMAP handing an
address to a new handle while the previous handle at that address has NOT been
closed. That is two owners of one mapping, which is how BO pages ended up as
allocator metadata (see docs/KNOWN-ISSUES.md `freebin-corruption`). It must be 0.

Copyright 2026 Phoenix Systems
SPDX-License-Identifier: BSD-3-Clause
"""

import argparse
import re
import sys
from collections import Counter, defaultdict

# v3d-bo: CREATE handle=12 gpuva=0x02100000 size=8192 cpu=0000000002baf000
LINE = re.compile(
    rb"v3d-bo: (CREATE|MMAP|CLOSE)\s+handle=(\d+) gpuva=0x([0-9a-f]+) "
    rb"size=(\d+) cpu=([0-9a-f]+)"
)


def analyze(path):
    owner = {}        # cpu address -> handle currently owning that mapping
    live = set()      # handles created/mapped and not yet closed
    reuse = defaultdict(set)  # cpu address -> every handle ever seen there
    overlap = []      # the defect: (cpu, old handle, new handle, event index)
    ops = Counter()
    order = 0

    with open(path, "rb") as fh:
        blob = fh.read()

    for m in LINE.finditer(blob):
        order += 1
        op = m.group(1).decode()
        handle = int(m.group(2))
        cpu = m.group(5).decode()
        ops[op] += 1

        # A null cpu is a BO with no CPU mapping; it cannot be double-owned.
        if int(cpu, 16) == 0:
            continue

        if op == "CLOSE":
            live.discard(handle)
            if owner.get(cpu) == handle:
                owner.pop(cpu, None)
            continue

        prev = owner.get(cpu)
        if prev is not None and prev != handle and prev in live:
            overlap.append((cpu, prev, handle, order))
        owner[cpu] = handle
        live.add(handle)
        reuse[cpu].add(handle)

    return ops, reuse, overlap, order


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("log", help="UART log captured with V3D_BO_TRACE=1")
    ap.add_argument("--quiet", action="store_true",
                    help="print only the verdict line")
    args = ap.parse_args()

    ops, reuse, overlap, order = analyze(args.log)

    if order == 0:
        print("bo-trace: NO v3d-bo lines in %s -- was V3D_BO_TRACE=1 exported "
              "before the app started?" % args.log, file=sys.stderr)
        return 2

    recycled = sum(1 for handles in reuse.values() if len(handles) > 1)

    if not args.quiet:
        print("events         : %d  (CREATE %d, MMAP %d, CLOSE %d)"
              % (order, ops["CREATE"], ops["MMAP"], ops["CLOSE"]))
        print("cpu addresses  : %d" % len(reuse))
        print("address reuse  : %d  (expected, not a defect -- see the header)"
              % recycled)
        for cpu, old, new, at in overlap[:20]:
            print("  OVERLAP cpu=%s old=%d (still open) new=%d @event %d"
                  % (cpu, old, new, at))

    if overlap:
        print("bo-trace: FAIL -- %d overlapping reuse(s): a mapping had two "
              "live owners" % len(overlap))
        return 1

    print("bo-trace: PASS -- 0 overlapping reuses in %d events "
          "(%d address reuses, all after the previous owner closed)"
          % (order, recycled))
    return 0


if __name__ == "__main__":
    sys.exit(main())
