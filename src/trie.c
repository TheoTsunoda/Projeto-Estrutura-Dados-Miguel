#include "trie.h"
#include "alloc.h"
#include <string.h>
#include <stdlib.h>

/* Branch on raw bytes: 256 possible child slots per node. This handles UTF-8
 * titles (accents etc.) with zero Unicode logic, because we just treat the
 * title as a sequence of bytes. Simple to explain; memory-heavy on purpose. */
#define TRIE_ALPHABET 256

typedef struct TrieNode {
    struct TrieNode *children[TRIE_ALPHABET];
    Title           *t;     /* non-NULL => a stored title ends at this node */
} TrieNode;

typedef struct {
    TrieNode *root;
    size_t    count;        /* number of distinct stored titles */
    size_t    bytes;
} Trie;

static TrieNode *make_node(Trie *T) {
    TrieNode *n = (TrieNode *)xmalloc(sizeof(TrieNode));
    if (!n) return NULL;
    for (int i = 0; i < TRIE_ALPHABET; i++) n->children[i] = NULL;
    n->t = NULL;
    T->bytes += sizeof(TrieNode);
    return n;
}

static void tr_insert(void *self, Title *t) {
    Trie *T = (Trie *)self;
    const unsigned char *s = (const unsigned char *)t->primaryTitle;
    TrieNode *node = T->root;
    for (; *s; s++) {                       /* one step per byte of the title */
        if (!node->children[*s]) {
            node->children[*s] = make_node(T);
            if (!node->children[*s]) return; /* R1 cap hit */
        }
        node = node->children[*s];
    }
    if (node->t == NULL) T->count++;        /* new key (else: duplicate title) */
    node->t = t;                            /* last writer wins for duplicates */
}

static Title *tr_search(void *self, const char *key) {
    Trie *T = (Trie *)self;
    const unsigned char *s = (const unsigned char *)key;
    TrieNode *node = T->root;
    for (; *s; s++) {
        if (!node->children[*s]) return NULL;
        node = node->children[*s];
    }
    return node->t;                          /* NULL if it's only a prefix */
}

/* Remove = clear the terminal marker. We do NOT prune/free nodes, so search
 * stays correct and the cost is O(L). (Physical reclamation is omitted for
 * clarity and to keep Trie vs Radix a fair, same-policy comparison.) */
static int tr_remove(void *self, const char *key) {
    Trie *T = (Trie *)self;
    const unsigned char *s = (const unsigned char *)key;
    TrieNode *node = T->root;
    for (; *s; s++) {
        if (!node->children[*s]) return 0;
        node = node->children[*s];
    }
    if (node->t == NULL) return 0;
    node->t = NULL;
    T->count--;
    return 1;
}

static size_t tr_size(void *self)      { return ((Trie *)self)->count; }
static size_t tr_mem_bytes(void *self) { return ((Trie *)self)->bytes; }

static void free_subtree(TrieNode *n) {
    if (!n) return;
    for (int i = 0; i < TRIE_ALPHABET; i++) free_subtree(n->children[i]);
    xfree(n);
}

static void tr_destroy(void *self) {
    Trie *T = (Trie *)self;
    free_subtree(T->root);
    xfree(T);
}

/* count every terminal in the subtree rooted at n */
static size_t count_terminals(TrieNode *n) {
    if (!n) return 0;
    size_t c = (n->t != NULL) ? 1 : 0;
    for (int i = 0; i < TRIE_ALPHABET; i++) c += count_terminals(n->children[i]);
    return c;
}

size_t trie_count_prefix(Dictionary *d, const char *prefix) {
    Trie *T = (Trie *)d->self;
    const unsigned char *s = (const unsigned char *)prefix;
    TrieNode *node = T->root;
    for (; *s; s++) {                        /* walk to the prefix node */
        if (!node->children[*s]) return 0;
        node = node->children[*s];
    }
    return count_terminals(node);            /* all titles under it match */
}

/* DFS in child order, collecting terminals until we hit `max`. */
static void collect_dfs(TrieNode *n, Title **out, size_t max, size_t *cnt) {
    if (!n || *cnt >= max) return;
    if (n->t) out[(*cnt)++] = n->t;
    for (int i = 0; i < TRIE_ALPHABET && *cnt < max; i++)
        collect_dfs(n->children[i], out, max, cnt);
}

size_t trie_collect_prefix(Dictionary *d, const char *prefix,
                           Title **out, size_t max) {
    Trie *T = (Trie *)d->self;
    const unsigned char *s = (const unsigned char *)prefix;
    TrieNode *node = T->root;
    for (; *s; s++) {
        if (!node->children[*s]) return 0;
        node = node->children[*s];
    }
    size_t cnt = 0;
    collect_dfs(node, out, max, &cnt);
    return cnt;
}

Dictionary trie_create(void) {
    Trie *T = (Trie *)xmalloc(sizeof(Trie));
    T->count = 0;
    T->bytes = sizeof(Trie);
    T->root = make_node(T);

    Dictionary d;
    d.self = T; d.name = "Trie";
    d.insert = tr_insert; d.search = tr_search; d.remove = tr_remove;
    d.size = tr_size; d.mem_bytes = tr_mem_bytes; d.destroy = tr_destroy;
    return d;
}
