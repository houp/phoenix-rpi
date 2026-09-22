/* Host shim: enough of libstorage to COMPILE the fs->strg branches.
 * The harness always sets fs->strg = NULL and uses the legacy callbacks,
 * so none of this is ever dereferenced at runtime. */
#ifndef _SHIM_STORAGE_H
#define _SHIM_STORAGE_H
#include <stdint.h>
#include <sys/types.h>
#include <sys/msg.h>
typedef struct _storage_t storage_t;
typedef struct { ssize_t (*read)(storage_t *, off_t, void *, size_t);
                 ssize_t (*write)(storage_t *, off_t, const void *, size_t);
                 int (*sync)(storage_t *); } storage_blkops_t;
typedef struct { const storage_blkops_t *ops; } storage_blk_t;
typedef struct { int (*read)(storage_t *, off_t, void *, size_t, size_t *);
                 int (*write)(storage_t *, off_t, const void *, size_t, size_t *);
                 int (*erase)(storage_t *, off_t, size_t); } storage_mtdops_t;
typedef struct { const storage_mtdops_t *ops; size_t writesz; size_t writeBuffsz; size_t erasesz; } storage_mtd_t;
typedef struct { storage_blk_t *blk; storage_mtd_t *mtd; void *ctx; } storage_dev_t;
typedef struct _storage_fs_t storage_fs_t;
struct _storage_t { storage_dev_t *dev; off_t start; size_t size; storage_t *parent; };
struct _storage_fs_t { void *info; oid_t root; };
#endif
