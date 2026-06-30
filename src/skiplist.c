#include "skiplist.h"
#include "alloc.h"
#include <string.h>
#include <stdlib.h>

/* SKIP_MAX_LEVEL caps tower height. 16 levels supports ~2^16 = 65k expected
 * items comfortably; for ~818k it is still fine (a few searches touch the cap)
 * and keeps node overhead bounded. SKIP_P = 0.5 is the standard coin-flip:
 * each level up happens with probability 1/2, giving expected O(log n). */
#define SKIP_MAX_LEVEL 16
#define SKIP_P 0.5

typedef struct SkipNode {
    Title            *t;        /* shared record, NOT owned here */
    int               level;    /* tower height = number of forward pointers */
    struct SkipNode **forward;  /* array[level] of next-pointers, one per lane */
} SkipNode;

typedef struct {
    SkipNode *header;           /* sentinel head; its towers point into lane 0..max */
    int       level;            /* highest lane currently in use (>=1) */
    size_t    count;
    size_t    bytes;
} SkipList;

/* Coin-flip tower height: start at 1, keep promoting while a coin says so. */
static int random_level(void) {
    int lvl = 1;
    while ((rand() < (int)(RAND_MAX * SKIP_P)) && lvl < SKIP_MAX_LEVEL) {
        lvl++;
    }
    return lvl;
}

static SkipNode *make_node(SkipList *S, Title *t, int level) {
    SkipNode *n = (SkipNode *)xmalloc(sizeof(SkipNode));
    if (!n) return NULL;
    n->forward = (SkipNode **)xmalloc(level * sizeof(SkipNode *));
    if (!n->forward) { xfree(n); return NULL; }
    n->t = t;
    n->level = level;
    for (int i = 0; i < level; i++) n->forward[i] = NULL;
    S->bytes += sizeof(SkipNode) + level * sizeof(SkipNode *);
    return n;
}

static void sl_insert(void *self, Title *t) {
    SkipList *S = (SkipList *)self;
    const char *key = t->tconst;
    SkipNode *update[SKIP_MAX_LEVEL];   /* the node we dropped down from, per lane */
    SkipNode *x = S->header;

    /* Walk down from the top lane, recording where we leave each lane. */
    for (int i = S->level - 1; i >= 0; i--) {
        while (x->forward[i] && strcmp(x->forward[i]->t->tconst, key) < 0)
            x = x->forward[i];
        update[i] = x;
    }

    /* The candidate at lane 0 is the first node >= key. */
    x = x->forward[0];
    if (x && strcmp(x->t->tconst, key) == 0) {
        x->t = t;                        /* key exists -> update payload */
        return;
    }

    int lvl = random_level();
    if (lvl > S->level) {                /* new node is taller than the list */
        for (int i = S->level; i < lvl; i++) update[i] = S->header;
        S->level = lvl;
    }

    SkipNode *node = make_node(S, t, lvl);
    if (!node) return;                   /* respects R1 memory cap */
    for (int i = 0; i < lvl; i++) {      /* splice into each lane it occupies */
        node->forward[i]    = update[i]->forward[i];
        update[i]->forward[i] = node;
    }
    S->count++;
}

static Title *sl_search(void *self, const char *key) {
    SkipList *S = (SkipList *)self;
    SkipNode *x = S->header;
    for (int i = S->level - 1; i >= 0; i--) {
        while (x->forward[i] && strcmp(x->forward[i]->t->tconst, key) < 0)
            x = x->forward[i];
    }
    x = x->forward[0];
    if (x && strcmp(x->t->tconst, key) == 0) return x->t;
    return NULL;
}

static int sl_remove(void *self, const char *key) {
    SkipList *S = (SkipList *)self;
    SkipNode *update[SKIP_MAX_LEVEL];
    SkipNode *x = S->header;

    for (int i = S->level - 1; i >= 0; i--) {
        while (x->forward[i] && strcmp(x->forward[i]->t->tconst, key) < 0)
            x = x->forward[i];
        update[i] = x;
    }
    x = x->forward[0];
    if (!x || strcmp(x->t->tconst, key) != 0) return 0;   /* not present */

    /* Unlink x from every lane that pointed at it. */
    for (int i = 0; i < S->level; i++) {
        if (update[i]->forward[i] != x) break;  /* x not in higher lanes */
        update[i]->forward[i] = x->forward[i];
    }
    S->bytes -= sizeof(SkipNode) + x->level * sizeof(SkipNode *);
    xfree(x->forward);
    xfree(x);
    S->count--;

    /* Shrink the list's level if the top lanes are now empty. */
    while (S->level > 1 && S->header->forward[S->level - 1] == NULL)
        S->level--;
    return 1;
}

static size_t sl_size(void *self)      { return ((SkipList *)self)->count; }
static size_t sl_mem_bytes(void *self) { return ((SkipList *)self)->bytes; }

static void sl_destroy(void *self) {
    SkipList *S = (SkipList *)self;
    SkipNode *n = S->header->forward[0];     /* walk lane 0 = every node once */
    while (n) {
        SkipNode *next = n->forward[0];
        xfree(n->forward);
        xfree(n);
        n = next;
    }
    xfree(S->header->forward);
    xfree(S->header);
    xfree(S);
}

Dictionary skiplist_create(void) {
    SkipList *S = (SkipList *)xmalloc(sizeof(SkipList));
    S->level = 1;
    S->count = 0;
    S->header = (SkipNode *)xmalloc(sizeof(SkipNode));
    S->header->t = NULL;
    S->header->level = SKIP_MAX_LEVEL;
    S->header->forward = (SkipNode **)xmalloc(SKIP_MAX_LEVEL * sizeof(SkipNode *));
    for (int i = 0; i < SKIP_MAX_LEVEL; i++) S->header->forward[i] = NULL;
    S->bytes = sizeof(SkipList) + sizeof(SkipNode) + SKIP_MAX_LEVEL * sizeof(SkipNode *);

    Dictionary d;
    d.self = S; d.name = "Skip List";
    d.insert = sl_insert; d.search = sl_search; d.remove = sl_remove;
    d.size = sl_size; d.mem_bytes = sl_mem_bytes; d.destroy = sl_destroy;
    return d;
}
