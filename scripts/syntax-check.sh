#!/usr/bin/env bash
# Cross-compile ONE source file from a sibling repo and report errors -- without
# producing any build artifact.
#
# Why this exists: a `--scope core` build ends in the image stage, which
# overwrites the TFTP `loader.disk`. That makes a full build impossible whenever
# a Pi bench is measuring the current loader -- which is most of a long
# unattended run. This checks a single file under the REAL compile flags
# (-Werror included) and writes nothing, so it is safe at any time.
#
# It earned its keep immediately: it caught `*prev/*next` inside a block comment
# in vm/map.c (the `/*` sequence trips -Werror=comment), which would otherwise
# have failed a kernel gate build hours later.
#
# Usage:
#   ./scripts/syntax-check.sh <sibling-repo> <path/in/repo.c> [target]
#   ./scripts/syntax-check.sh phoenix-rtos-kernel vm/map.c
#   ./scripts/syntax-check.sh phoenix-rtos-devices audio/rpi4-audio/rpi4-audio.c
#
# Checks the file as it is in sources/ right now: it stages that copy into the
# matching .buildroot/ tree first, because .buildroot is a COPY that a real
# build refreshes with rsync --delete. Nothing else is touched, and the next
# real build overwrites the staged file anyway.
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
repo="${1:?usage: syntax-check.sh <sibling-repo> <path/in/repo.c> [target]}"
rel="${2:?usage: syntax-check.sh <sibling-repo> <path/in/repo.c> [target]}"
target="${3:-aarch64a72-generic-rpi4b}"

src="${repo_root}/sources/${repo}/${rel}"
bdir="${repo_root}/.buildroot/${repo}"
obj="${repo_root}/.buildroot/_build/${target}/${repo}/${rel%.c}.o"
proj="${repo_root}/.buildroot/_projects/${target}"
tc="${repo_root}/.toolchain/aarch64-phoenix/bin"

[ -f "$src" ]   || { echo "syntax-check: no such source: $src" >&2; exit 2; }
[ -d "$bdir" ]  || { echo "syntax-check: no buildroot copy of $repo -- run a build once first" >&2; exit 2; }
[ -d "$tc" ]    || { echo "syntax-check: toolchain not found at $tc" >&2; exit 2; }

mkdir -p "$(dirname "${bdir}/${rel}")"
cp "$src" "${bdir}/${rel}"

# Ask make what it WOULD run for that object, and take the compiler line.
cmd=$(cd "$bdir" && PATH="${tc}:$PATH" TARGET="$target" make -n "$obj" 2>/dev/null \
        | grep -m1 -- '-phoenix-gcc ' || true)
if [ -z "$cmd" ]; then
    echo "syntax-check: could not recover a compile command for ${repo}/${rel}." >&2
    echo "  (is it actually built for ${target}? some files are per-target)" >&2
    exit 2
fi

# -fsyntax-only writes nothing. Drop the output and dependency flags so gcc does
# not object to them, and add the board include: board_config.h lives in
# _projects/<target>/ and the top level injects that -I through the environment,
# so it is absent from a bare `make -n`. Confirmed against the real build's own
# .d file, which records board_config.h resolving from exactly that directory.
cmd=$(printf '%s\n' "$cmd" \
    | sed -e 's/ -c / /' \
          -e 's/ -o "[^"]*"//g' -e 's/ -o [^ ]*//g' \
          -e 's/ -MD//g' -e 's/ -MP//g' \
          -e 's/ -MF [^ ]*//g' -e 's/ -MT "[^"]*"//g' -e 's/ -MT [^ ]*//g')

printf 'syntax-check: %s/%s  (target %s)\n' "$repo" "$rel" "$target"
if (cd "$bdir" && PATH="${tc}:$PATH" eval "$cmd -fsyntax-only -I${proj}"); then
    printf 'syntax-check: CLEAN (compiles under the real flags, -Werror included)\n'
else
    rc=$?
    printf 'syntax-check: FAILED (rc=%d) -- fix before spending a build on it\n' "$rc" >&2
    exit "$rc"
fi
