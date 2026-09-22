/* Host shim: real msg_t layout (from the kernel header), stubbed transport.
 * The harness calls libext2_handler() directly, so nothing is ever sent. */
#ifndef _SHIM_SYS_MSG_H
#define _SHIM_SYS_MSG_H
#include <stddef.h>
#include <sys/types.h>
#include <phoenix/types.h>
#include <phoenix/msg.h>
static inline int portCreate(uint32_t *port) { *port = 1; return 0; }
static inline void portDestroy(uint32_t port) { (void)port; }
static inline int portRegister(uint32_t p, const char *n, oid_t *o) { (void)p; (void)n; (void)o; return 0; }
static inline int portUnregister(const char *n) { (void)n; return 0; }
static inline int lookup(const char *n, oid_t *f, oid_t *d) { (void)n; (void)f; (void)d; return -1; }
static inline int msgSend(uint32_t p, msg_t *m) { (void)p; (void)m; return -1; }
static inline int msgPulse(uint32_t p, msg_t *m) { (void)p; (void)m; return -1; }
static inline int msgRecv(uint32_t p, msg_t *m, msg_rid_t *r) { (void)p; (void)m; (void)r; return -1; }
static inline int msgRespond(uint32_t p, msg_t *m, msg_rid_t r) { (void)p; (void)m; (void)r; return -1; }
#endif
