#include "menu.h"
#include "alloc.h"
#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Lê uma inha, com um certo limite, do teclado e guarda na variável buf. No final, tira o \t do enter e coloca \0. */
static int read_line(char *buf, size_t cap) {
    if (!fgets(buf, (int)cap, stdin)) return 0;
    size_t n = strlen(buf);
    if (n && buf[n - 1] == '\n') buf[n - 1] = '\0';
    return 1;
}

/* Essas são a struct e funções para títulos gerados sinteticamente, introduzidos pelo usuário */
typedef struct {
    Title **items;
    size_t  count;
    size_t  cap;
} OwnedRecords;

static void owned_init(OwnedRecords *o) { o->items = NULL; o->count = 0; o->cap = 0; }

static void owned_add(OwnedRecords *o, Title *t) {
    if (o->count == o->cap) {
        o->cap = o->cap ? o->cap * 2 : 8;
        o->items = (Title **)realloc(o->items, o->cap * sizeof(Title *));
    }
    o->items[o->count++] = t;
}

static void owned_free_all(OwnedRecords *o) {
    for (size_t i = 0; i < o->count; i++) {
        Title *t = o->items[i];
        xfree(t->tconst);
        xfree(t->primaryTitle);
        xfree(t);
    }
    free(o->items);
}

static Title *make_synthetic(const char *tconst, const char *title) {
    Title *t = (Title *)xmalloc(sizeof(Title));
    memset(t, 0, sizeof(Title));
    size_t a = strlen(tconst) + 1, b = strlen(title) + 1;
    t->tconst       = (char *)xmalloc(a); memcpy(t->tconst, tconst, a);
    t->primaryTitle = (char *)xmalloc(b); memcpy(t->primaryTitle, title, b);
    t->titleType    = NULL;
    t->originalTitle = NULL;
    t->isAdult        = 0;
    t->startYear      = SENTINEL_MISSING;
    t->endYear        = SENTINEL_MISSING;
    t->runtimeMinutes = SENTINEL_MISSING;
    t->genreCount     = 0;
    t->averageRating  = 0.0;
    t->numVotes       = 0;
    return t;
}

/* "Print" de um título */
static void print_title(const Title *t) {
    if (!t) { printf("  (record is NULL)\n"); return; }
    printf("  tconst        : %s\n", t->tconst ? t->tconst : "(null)");
    printf("  primaryTitle  : %s\n", t->primaryTitle ? t->primaryTitle : "(null)");
    printf("  titleType     : %s\n", t->titleType ? t->titleType : "(n/a)");
    if (t->startYear != SENTINEL_MISSING) printf("  startYear     : %d\n", t->startYear);
    else                                  printf("  startYear     : (missing)\n");
    if (t->runtimeMinutes != SENTINEL_MISSING) printf("  runtimeMinutes: %d\n", t->runtimeMinutes);
    else                                       printf("  runtimeMinutes: (missing)\n");
    if (t->genreCount > 0) {
        printf("  genres        : ");
        for (int i = 0; i < t->genreCount; i++)
            printf("%s%s", t->genres[i], i + 1 < t->genreCount ? ", " : "");
        printf("\n");
    } else {
        printf("  genres        : (none)\n");
    }
    printf("  averageRating : %.1f  (%ld votes)\n", t->averageRating, t->numVotes);
}

/* ------------------------------------------------------------------
 * As 5 funções
 * ------------------------------------------------------------------ */

static void action_search(Dictionary *d) {
    char key[256];
    printf("Enter tconst to search (e.g. tt0000001): ");
    if (!read_line(key, sizeof(key)) || key[0] == '\0') { printf("Cancelled.\n"); return; }

    uint64_t t0 = now_ns();
    Title *found = dict_search(d, key);
    uint64_t dt = now_ns() - t0;

    if (found) { printf("Found in %.3f us:\n", dt / 1000.0); print_title(found); }
    else        printf("Not found (%.3f us).\n", dt / 1000.0);
}


static Title *action_remove(Dictionary *d) {
    char key[256];
    printf("Enter tconst to remove: ");
    if (!read_line(key, sizeof(key)) || key[0] == '\0') { printf("Cancelled.\n"); return NULL; }

    /* Grab the pointer first so we can re-insert later if the user wants. */
    Title *victim = dict_search(d, key);

    uint64_t t0 = now_ns();
    int removed = dict_remove(d, key);
    uint64_t dt = now_ns() - t0;

    if (removed) {
        printf("Removed in %.3f us. Size is now %zu.\n", dt / 1000.0, dict_size(d));
        return victim;
    }
    printf("Key not present (%.3f us).\n", dt / 1000.0);
    return NULL;
}

static void action_insert_new(Dictionary *d, OwnedRecords *owned) {
    char tconst[256], title[512];
    printf("New record — enter tconst: ");
    if (!read_line(tconst, sizeof(tconst)) || tconst[0] == '\0') { printf("Cancelled.\n"); return; }
    printf("New record — enter primaryTitle: ");
    if (!read_line(title, sizeof(title))) { printf("Cancelled.\n"); return; }

    if (dict_search(d, tconst)) {
        printf("A record with that tconst already exists; insert will update it in place.\n");
    }

    Title *t = make_synthetic(tconst, title);
    owned_add(owned, t);

    uint64_t t0 = now_ns();
    dict_insert(d, t);
    uint64_t dt = now_ns() - t0;

    printf("Inserted in %.3f us. Size is now %zu.\n", dt / 1000.0, dict_size(d));
}

static void action_stats(Dictionary *d) {
    size_t n = dict_size(d);
    size_t b = dict_mem_bytes(d);
    printf("Structure       : %s\n", d->name);
    printf("Records stored  : %zu\n", n);
    printf("Structural memory: %zu bytes (%.2f MB)\n", b, b / (1024.0 * 1024.0));
    if (n) printf("Bytes per record: %.1f\n", (double)b / (double)n);
}


static void action_selftest(Dictionary *d, OwnedRecords *owned) {
    const int N = 5;
    char buf[32];
    Title *made[5];
    size_t before = dict_size(d);

    printf("Self-test: inserting %d synthetic records, finding, then removing them...\n", N);

    for (int i = 0; i < N; i++) {
        snprintf(buf, sizeof(buf), "ttSELFTEST%02d", i);
        made[i] = make_synthetic(buf, "self-test record");
        owned_add(owned, made[i]);
        dict_insert(d, made[i]);
    }
    int found_ok = 1;
    for (int i = 0; i < N; i++) {
        snprintf(buf, sizeof(buf), "ttSELFTEST%02d", i);
        if (dict_search(d, buf) != made[i]) found_ok = 0;
    }
    int removed_ok = 1;
    for (int i = 0; i < N; i++) {
        snprintf(buf, sizeof(buf), "ttSELFTEST%02d", i);
        if (!dict_remove(d, buf)) removed_ok = 0;
    }
    size_t after = dict_size(d);

    printf("  insert/search consistent : %s\n", found_ok ? "PASS" : "FAIL");
    printf("  all removed              : %s\n", removed_ok ? "PASS" : "FAIL");
    printf("  size restored (%zu==%zu)  : %s\n",
           after, before, after == before ? "PASS" : "FAIL");
}

/* ------------------------------------------------------------------
 * O Loop
 * ------------------------------------------------------------------ */
void run_menu(Dictionary *d) {
    OwnedRecords owned; owned_init(&owned);
    Title *last_removed = NULL;
    char choice[16];

    for (;;) {
        printf("\n===== IMDb Catalog (%s) =====\n", d->name);
        printf("Records loaded: %zu\n", dict_size(d));
        printf("  1) Search by tconst\n");
        printf("  2) Remove by tconst\n");
        printf("  3) Insert a new record\n");
        if (last_removed)
            printf("  4) Re-insert last removed (%s)\n", last_removed->tconst);
        printf("  5) Statistics\n");
        printf("  6) Run correctness self-test\n");
        printf("  0) Exit\n");
        printf("Choice: ");

        if (!read_line(choice, sizeof(choice))) break;

        switch (choice[0]) {
            case '1': action_search(d); break;
            case '2': last_removed = action_remove(d); break;
            case '3': action_insert_new(d, &owned); break;
            case '4':
                if (last_removed) {
                    dict_insert(d, last_removed);
                    printf("Re-inserted %s. Size is now %zu.\n",
                           last_removed->tconst, dict_size(d));
                    last_removed = NULL;
                } else {
                    printf("Unknown option.\n");
                }
                break;
            case '5': action_stats(d); break;
            case '6': action_selftest(d, &owned); break;
            case '0': owned_free_all(&owned); return;
            default:  printf("Unknown option.\n"); break;
        }
    }
    owned_free_all(&owned);
}
