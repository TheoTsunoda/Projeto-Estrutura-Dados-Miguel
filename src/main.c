#include "title.h"
#include "loader.h"
#include "alloc.h"
#include "timer.h"
#include "dict.h"
#include "linkedlist.h"
#include "hashtable.h"
#include "AVL.h"
#include "skiplist.h"
#include "trie.h"
#include "radix.h"
#include "menu.h"
#include "operations.h"
#include "benchmark.h"
#include <stdio.h>

#define DATA_PATH  "../data/imdb_clean.tsv"
#define LOAD_LIMIT 0          /* 0 = all ~818,436; lower it to test faster */

int main(void) {
    printf("Loading dataset from %s ...\n", DATA_PATH);
    size_t count = 0;
    Title *titles = load_titles(DATA_PATH, &count, LOAD_LIMIT);
    if (!titles) { fprintf(stderr, "Load failed. Check DATA_PATH.\n"); return 1; }
    printf("Loaded %zu records.\n", count);

    /* --- pick the tconst-keyed structure for the basic insert/search/remove
     *     menu (item 1). Swap this one line to benchmark a different one:
     *     linkedlist_create / hashtable_create / avl_create / skiplist_create. */
    Dictionary d = hashtable_create();
    printf("Building %s by streaming %zu inserts...\n", d.name, count);
    uint64_t t0 = now_ns();
    for (size_t i = 0; i < count; i++) dict_insert(&d, &titles[i]);
    printf("Built in %.1f ms. Structural memory: %.2f MB.\n",
           (now_ns() - t0) / 1e6, dict_mem_bytes(&d) / (1024.0 * 1024.0));

    /* --- title-keyed structure for prefix/autocomplete (Phase 6) --- */
    Dictionary rx = radix_create();
    for (size_t i = 0; i < count; i++) dict_insert(&rx, &titles[i]);

    //run_benchmark(titles, count);*****************

    /* --- basic operations menu (item 1) --- */
    run_menu(&d);

    /* --- additional operations menu (item 2 / Phase 6) --- */
    run_analytics_menu(titles, count, &rx);

    /* --- teardown --- */
    dict_destroy(&d);
    dict_destroy(&rx);
    free_titles(titles, count);
    printf("Bytes still allocated after teardown: %zu (should be 0).\n",
           xmalloc_current());
    return 0;
}
