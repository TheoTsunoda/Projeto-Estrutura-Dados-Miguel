#ifndef OPERATIONS_H
#define OPERATIONS_H

#include "title.h"
#include "dict.h"

/* ============================================================
 *  Phase 6 — the three additional operations (+ prefix search)
 * ============================================================
 *
 * These run over the loaded Title array (the dataset). Prefix search runs
 * over a title-keyed structure (Trie or Radix) passed in as `prefix_struct`.
 * Each operation prints a terminal table and its own elapsed time.
 */

/* Op 0 — Prefix / autocomplete. Lists up to 12 titles beginning with `prefix`
 * using the title-keyed Radix/Trie, plus the total match count. */
void op_prefix_search(Dictionary *prefix_struct, const char *prefix);

/* Op 1 — Genre trend by decade (movies, tvSeries, short only): the single
 * most frequent genre per decade, then a naive linear-extrapolation guess for
 * the next decade. */
void op_genre_trend(Title *titles, size_t n);

/* Op 2 — Runtime statistics: overall mean / median / min / max, then mean
 * runtime per genre. Whether the -1 "missing" sentinel is included is
 * controlled by EXCLUDE_MISSING_RUNTIME in operations.c. */
void op_runtime_stats(Title *titles, size_t n);

/* Op 3 — Top 10 by IMDb-style Bayesian weighted rating
 *   WR = (v/(v+m))*R + (m/(v+m))*C
 * with C = mean rating over all titles and m = mean vote count over all. */
void op_top10_weighted(Title *titles, size_t n);

/* Convenience: an interactive sub-menu wiring all four to keystrokes.
 * `prefix_struct` should be a Dictionary from radix_create() (or trie_create()). */
void run_analytics_menu(Title *titles, size_t n, Dictionary *prefix_struct);

#endif /* OPERATIONS_H */
