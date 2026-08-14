#!/usr/bin/env bash
# Build SQLite (public domain) for Phoenix-RTOS / aarch64 (Pi 4).
# The amalgamation cross-compiles with a single command — NO patches, NO
# libphoenix changes needed. See README.md.
set -euo pipefail
TC="${TOOLCHAIN:-/home/houp/phoenix-rpi/.toolchain/aarch64-phoenix/bin}"
CC="${CC:-$TC/aarch64-phoenix-gcc}"
AMALG_URL="https://www.sqlite.org/2026/sqlite-amalgamation-3530400.zip"
AMALG_SHA="1e71ddf93849c6a6ecf58b827c0692073d2dd7ee40196158068f7b29f422e87d"
WORK="${1:-/tmp/sqlite-build}"
mkdir -p "$WORK" && cd "$WORK"
[ -f sqlite-amalg.zip ] || curl -fsSL -o sqlite-amalg.zip "$AMALG_URL"
echo "$AMALG_SHA  sqlite-amalg.zip" | sha256sum -c -
rm -rf sqlite-amalgamation-* && unzip -oq sqlite-amalg.zip
cd sqlite-amalgamation-*
"$CC" -O2 -static \
  -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_OMIT_WAL -DHAVE_READLINE=0 \
  shell.c sqlite3.c -o sqlite3
echo "built: $PWD/sqlite3"
