#ifndef RADIX_H
#define RADIX_H

#include "dict.h"

/* ============================================================
 *  Radix / Patricia tree — the OPTIMIZED Trie (Phase 5)
 * ============================================================
 *
 * Same job and same interface as the plain Trie (keyed on primaryTitle,
 * exact lookup + prefix search), but with one structural change: every chain
 * of single-child nodes is COMPRESSED into a single edge labelled with the
 * whole substring. So "Star Wars" is one edge, not 9 nodes; and crucially each
 * node stores only its actual children in a small dynamic array instead of a
 * 256-wide array. Both changes cut node count and memory dramatically while
 * keeping lookups O(L). This is the same abstract structure as the Trie, so
 * comparing the two (memory, node count, search time, prefix time) is a valid,
 * apples-to-apples optimization comparison.
 */
Dictionary radix_create(void);

/* Same extra capability as the Trie, so the two can be compared on prefix
 * queries too. Pass the Dictionary returned by radix_create(). */
size_t radix_count_prefix(Dictionary *d, const char *prefix);

/* Collect up to `max` stored titles that begin with `prefix`. Returns count
 * written. Order follows tree traversal (not strictly alphabetical). */
size_t radix_collect_prefix(Dictionary *d, const char *prefix,
                            Title **out, size_t max);

#endif /* RADIX_H */
