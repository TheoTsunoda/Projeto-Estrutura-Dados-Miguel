#ifndef ALLOC_H
#define ALLOC_H

#include <stddef.h>

/* Counting allocator.
 *
 * Every data structure must allocate through xmalloc/xfree (NOT raw
 * malloc/free) so we can report exact bytes used per structure — this is the
 * "Uso de Memoria" benchmark metric — and so we can enforce a hard memory cap
 * for restriction R1.
 *
 * How it works: xmalloc stores the requested size in a small header just
 * before the block it returns, so xfree knows how many bytes to subtract.
 */

/* Allocate `size` bytes, tracked. Returns NULL if it would exceed the cap
 * (when a cap is set). Caller must check for NULL. */
void  *xmalloc(size_t size);

/* Free a block previously returned by xmalloc. Safe to pass NULL. */
void   xfree(void *ptr);

/* Bytes currently allocated through xmalloc and not yet freed. */
size_t xmalloc_current(void);

/* Peak bytes ever simultaneously allocated (useful for memory benchmarks). */
size_t xmalloc_peak(void);

/* Reset the current/peak counters to zero. Call between benchmark runs.
 * (Does not free memory — only zeroes the bookkeeping.) */
void   xmalloc_reset_counters(void);

/* Set a hard cap in bytes for restriction R1. After this, any xmalloc that
 * would push current usage above `cap_bytes` returns NULL instead.
 * Pass 0 to disable the cap (the default). */
void   xmalloc_set_cap(size_t cap_bytes);

#endif /* ALLOC_H */
