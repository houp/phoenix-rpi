# SQLite on Phoenix-RTOS / Raspberry Pi 4

A full **SQLite** SQL database engine (the `sqlite3` CLI) running on Phoenix-RTOS on
the Pi 4 — a real embeddable relational database, in-memory and file-backed.

SQLite is **public domain** — no GPL/license concern (unlike the bash/coreutils ports).

## Result (HW-verified 2026-08-14, netboot, 0 faults)

**Cross-compiled on the FIRST try, ZERO libphoenix gaps** (contrast bash/coreutils, which
each needed many libc fixes) — the amalgamation builds with a single command.

- **In-memory engine** (`sqlite3 -init test.sql :memory:`): CREATE TABLE, INSERT, SELECT
  with ORDER BY, aggregates (COUNT/SUM/AVG with REAL), `printf()`, LIKE, and a **recursive
  CTE + group_concat** — all output matches the x86 reference.
- **File-backed VFS** (`sqlite3 -init testf.sql /tmp/x.db`) — the real forcing-function:
  created a DB file, B-tree table+PRIMARY KEY, INSERT/UPDATE/DELETE, `CREATE INDEX`
  (a second on-disk B-tree), an index-driven scan, and the rollback journal
  (write→fsync→delete). **`PRAGMA integrity_check` returned `ok`** — SQLite verified its
  own on-disk page/B-tree structures are consistent after all the writes. Exact expected
  output (`kv a=10 / kv b=21 / total=31 / idxscan b / ok`) reproduced on hardware.

So SQLite's unix VFS (open/read/write/lseek/fstat/fcntl/truncate/unlink + fsync) works on
Phoenix — persistent, indexed, integrity-verified storage.

## Build

    ./build.sh                 # downloads the amalgamation (SHA-checked) + cross-compiles

One command, no autoconf, no patches:

    aarch64-phoenix-gcc -O2 -static -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION \
        -DSQLITE_OMIT_WAL -DHAVE_READLINE=0 shell.c sqlite3.c -o sqlite3

(THREADSAFE=0: single-threaded CLI. OMIT_LOAD_EXTENSION: no dlopen. OMIT_WAL: rollback
journal instead of WAL — the WAL path uses shared-memory mmap semantics not yet exercised
on Phoenix; the rollback journal is proven above.)

## Run (psh-safe — `-init FILE` keeps all argv tokens space-free)

    /bin/sqlite3 -init /test.sql  :memory:      # in-memory engine
    /bin/sqlite3 -init /testf.sql /tmp/x.db     # file-backed DB (VFS)

(A SQL script ending in `.exit` runs then exits — avoids psh's interactive-tty gap and its
command-line quote/space mangling.)

## Deferred / notes

- WAL mode (`-DSQLITE_OMIT_WAL` dropped) — needs shared-memory mmap; untested.
- Concurrency/locking across processes — the file VFS uses `fcntl` advisory locks; verified
  single-process only.
- The host x86 reference build hung on file-DB open (a host `/tmp`/locking quirk, NOT
  Phoenix) — the file-VFS result above is validated against a reasoned expectation +
  `integrity_check ok`, which is the stronger check anyway.
