#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "title.h"
#include <stddef.h>

/* ============================================================
 *  Phase 7 / item 9 — Benchmark harness
 * ============================================================
 *
 * Measures, for each of the five structures and across a range of input
 * sizes (tiers): insertion time, search time, average (mixed) query time,
 * removal time, and memory usage. Writes:
 *
 *   results/benchmark.csv    raw numbers (one row per structure x tier)
 *   results/insert.svg       per-op insertion time vs n   (log-log)
 *   results/search.svg       per-op search-hit time vs n  (log-log)
 *   results/query.svg        per-op mixed-query time vs n (log-log)
 *   results/remove.svg       per-op removal time vs n     (log-log)
 *   results/memory.svg       memory used vs n             (log-log)
 *   results/benchmark.html   dashboard embedding all charts + the data table
 *
 * Methodology note baked into the output: the four tconst-keyed structures
 * (linked list, hash table, AVL, skip list) are the valid like-for-like
 * comparison group; the Trie is keyed on primaryTitle (variable-length) and
 * its cost is O(key length), so it is shown but flagged as a different basis.
 */
void run_benchmark(Title *titles, size_t count);

#endif /* BENCHMARK_H */
