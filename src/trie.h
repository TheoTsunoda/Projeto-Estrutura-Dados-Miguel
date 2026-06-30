#ifndef TRIE_H
#define TRIE_H

#include "dict.h"

/* ============================================================
 *  Trie (prefix tree) — string dictionary (outside-syllabus #2)
 * ============================================================
 *
 * Keyed on primaryTitle (NOT tconst), because the Trie's reason to exist in
 * this project is PREFIX search / autocomplete — something no other structure
 * can do efficiently. Each character of the title is one step down the tree;
 * a node marks the end of a stored title. Exact lookup is O(L) in the title
 * length L (independent of how many titles are stored), and prefix queries
 * walk to the prefix node then count the subtree.
 *
 * Memory note (important for Phase 5): every node carries a full 256-wide
 * child array, so long unique title tails waste a lot of space. That waste is
 * exactly what the Radix/Patricia optimization removes — so we MEASURE it here
 * rather than fixing it.
 */
Dictionary trie_create(void);

/* Beyond the Dictionary interface: count how many stored titles begin with
 * `prefix`. Pass the Dictionary returned by trie_create(). */
size_t trie_count_prefix(Dictionary *d, const char *prefix);

/* Collect up to `max` stored titles that begin with `prefix`, writing their
 * Title* into out[]. Returns how many were written. Order is byte-sorted
 * (≈alphabetical) because children are visited in 0..255 order. */
size_t trie_collect_prefix(Dictionary *d, const char *prefix,
                           Title **out, size_t max);

#endif /* TRIE_H */
