#!/usr/bin/env bash
# Run common read-only git operations across all Phoenix-RTOS sibling
# source repos and the coordination repo, using `git -C <dir>` (no `cd`,
# no shell chaining), so it can be added to .claude/settings.json as a
# safe allowlist entry.
#
# Sibling roots:
#   coordination: /Users/witoldbolt/phoenix-rpi
#   sources/*:    auto-discovered from /Users/witoldbolt/phoenix-rpi/sources
#
# Usage:
#   ./scripts/git-siblings.sh status               # short status of each repo
#   ./scripts/git-siblings.sh log [N]              # one-line log, default N=10
#   ./scripts/git-siblings.sh branch               # current branch + tracking
#   ./scripts/git-siblings.sh head                 # latest commit oneline + date
#   ./scripts/git-siblings.sh diff [REF]           # diff --stat vs REF (default HEAD)
#   ./scripts/git-siblings.sh show <SHA>           # show <SHA> in whichever repo has it
#
# Coordination repo and worktree are scanned alongside the sibling
# clones in sources/.

set -u
set -o pipefail

COORD="${PHOENIX_COORD:-/Users/witoldbolt/phoenix-rpi}"
SRC_ROOT="$COORD/sources"

usage() {
    cat <<EOF >&2
Usage: $0 <command> [args]
  status
  log [N]
  branch
  head
  diff [REF]
  show <SHA>
EOF
    exit 1
}

[ $# -ge 1 ] || usage
sub="$1"
shift || true

repos=()
# Coordination repo first.
if [ -d "$COORD/.git" ] || [ -f "$COORD/.git" ]; then
    repos+=("$COORD")
fi
# Each sibling clone under sources/*.
if [ -d "$SRC_ROOT" ]; then
    for d in "$SRC_ROOT"/*; do
        if [ -d "$d/.git" ] || [ -f "$d/.git" ]; then
            repos+=("$d")
        fi
    done
fi

if [ ${#repos[@]} -eq 0 ]; then
    echo "git-siblings: no git repos under $COORD or $SRC_ROOT" >&2
    exit 2
fi

case "$sub" in
    status)
        for r in "${repos[@]}"; do
            short=$(printf '%s' "$r" | sed -E "s|^$COORD/?||")
            [ -z "$short" ] && short="(coord)"
            br=$(git -C "$r" rev-parse --abbrev-ref HEAD 2>/dev/null || echo "?")
            s=$(git -C "$r" status --short 2>/dev/null | wc -l | tr -d ' ')
            ahead=$(git -C "$r" rev-list --count "@{upstream}..HEAD" 2>/dev/null || echo "?")
            behind=$(git -C "$r" rev-list --count "HEAD..@{upstream}" 2>/dev/null || echo "?")
            printf '%-40s br=%-40s changes=%s ahead=%s behind=%s\n' "$short" "$br" "$s" "$ahead" "$behind"
        done
        ;;
    log)
        n="${1:-10}"
        for r in "${repos[@]}"; do
            short=$(printf '%s' "$r" | sed -E "s|^$COORD/?||")
            [ -z "$short" ] && short="(coord)"
            echo "=== $short ==="
            git -C "$r" log --oneline -n "$n" 2>/dev/null || echo "(no history)"
            echo
        done
        ;;
    branch)
        for r in "${repos[@]}"; do
            short=$(printf '%s' "$r" | sed -E "s|^$COORD/?||")
            [ -z "$short" ] && short="(coord)"
            br=$(git -C "$r" rev-parse --abbrev-ref HEAD 2>/dev/null || echo "?")
            up=$(git -C "$r" rev-parse --abbrev-ref --symbolic-full-name "@{upstream}" 2>/dev/null || echo "(no upstream)")
            printf '%-40s br=%-40s upstream=%s\n' "$short" "$br" "$up"
        done
        ;;
    head)
        for r in "${repos[@]}"; do
            short=$(printf '%s' "$r" | sed -E "s|^$COORD/?||")
            [ -z "$short" ] && short="(coord)"
            line=$(git -C "$r" log -1 --pretty='%h %ad %s' --date=short 2>/dev/null || echo "(no history)")
            printf '%-40s %s\n' "$short" "$line"
        done
        ;;
    diff)
        ref="${1:-HEAD}"
        for r in "${repos[@]}"; do
            short=$(printf '%s' "$r" | sed -E "s|^$COORD/?||")
            [ -z "$short" ] && short="(coord)"
            stat=$(git -C "$r" diff --stat "$ref" 2>/dev/null)
            if [ -n "$stat" ]; then
                echo "=== $short ==="
                echo "$stat"
                echo
            fi
        done
        ;;
    show)
        sha="${1:-}"
        [ -n "$sha" ] || { echo "git-siblings: show requires <SHA>" >&2; exit 1; }
        for r in "${repos[@]}"; do
            if git -C "$r" cat-file -e "$sha^{commit}" 2>/dev/null; then
                short=$(printf '%s' "$r" | sed -E "s|^$COORD/?||")
                [ -z "$short" ] && short="(coord)"
                echo "=== $short ==="
                git -C "$r" show --stat "$sha"
                exit 0
            fi
        done
        echo "git-siblings: SHA $sha not found in any repo" >&2
        exit 2
        ;;
    *)
        usage
        ;;
esac
