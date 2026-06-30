#include "radix.h"
#include "alloc.h"
#include <string.h>
#include <stdlib.h>

/* Each node owns the edge label leading INTO it, plus a compact dynamic array
 * of children (each child's edge starts with a distinct first byte). The root
 * has an empty edge. A node is a stored key iff t != NULL. */
typedef struct RNode {
    char          *edge;     /* substring on the edge into this node (owned) */
    Title         *t;        /* non-NULL => a stored title ends here */
    struct RNode **kids;     /* dynamic array of child pointers */
    int            n_kids;
} RNode;

typedef struct {
    RNode *root;
    size_t count;
    size_t bytes;
} Radix;

/* longest common prefix length of two strings */
static int common_len(const char *a, const char *b) {
    int i = 0;
    while (a[i] && b[i] && a[i] == b[i]) i++;
    return i;
}

/* allocate a node whose edge is the first `len` bytes of `s` */
static RNode *rnode_new(Radix *R, const char *s, int len) {
    RNode *n = (RNode *)xmalloc(sizeof(RNode));
    if (!n) return NULL;
    n->edge = (char *)xmalloc(len + 1);
    memcpy(n->edge, s, len);
    n->edge[len] = '\0';
    n->t = NULL;
    n->kids = NULL;
    n->n_kids = 0;
    R->bytes += sizeof(RNode) + (len + 1);
    return n;
}

/* replace a node's edge with a shorter suffix of itself (used when splitting) */
static void shorten_edge(Radix *R, RNode *n, int from) {
    int oldlen = (int)strlen(n->edge);
    int newlen = oldlen - from;
    char *ne = (char *)xmalloc(newlen + 1);
    memcpy(ne, n->edge + from, newlen);
    ne[newlen] = '\0';
    xfree(n->edge);
    R->bytes += (newlen + 1) - (oldlen + 1);
    n->edge = ne;
}

static RNode *find_child(RNode *node, char c) {
    for (int i = 0; i < node->n_kids; i++)
        if (node->kids[i]->edge[0] == c) return node->kids[i];
    return NULL;
}

/* grow the kids array by one and append `child` */
static void add_child(Radix *R, RNode *parent, RNode *child) {
    RNode **nk = (RNode **)xmalloc((parent->n_kids + 1) * sizeof(RNode *));
    for (int i = 0; i < parent->n_kids; i++) nk[i] = parent->kids[i];
    nk[parent->n_kids] = child;
    if (parent->kids) {
        xfree(parent->kids);
        R->bytes -= parent->n_kids * sizeof(RNode *);
    }
    parent->kids = nk;
    parent->n_kids++;
    R->bytes += parent->n_kids * sizeof(RNode *);
}

/* replace the child pointer that equals `oldc` with `newc` (no size change) */
static void replace_child(RNode *parent, RNode *oldc, RNode *newc) {
    for (int i = 0; i < parent->n_kids; i++)
        if (parent->kids[i] == oldc) { parent->kids[i] = newc; return; }
}

/* insert `key` (the remaining suffix) under `node`, whose edge is consumed */
static void radix_insert_rec(Radix *R, RNode *node, const char *key, Title *t) {
    if (*key == '\0') {                       /* key ends exactly at node */
        if (node->t == NULL) R->count++;
        node->t = t;
        return;
    }
    RNode *child = find_child(node, key[0]);
    if (!child) {                             /* no matching edge -> new leaf */
        RNode *leaf = rnode_new(R, key, (int)strlen(key));
        leaf->t = t;
        R->count++;
        add_child(R, node, leaf);
        return;
    }
    int k = common_len(child->edge, key);
    int elen = (int)strlen(child->edge);
    if (k == elen) {                          /* whole edge matched -> descend */
        radix_insert_rec(R, child, key + k, t);
        return;
    }
    /* partial match: split child's edge at k.
     * middle takes the shared prefix; child keeps the rest as its edge. */
    RNode *middle = rnode_new(R, child->edge, k);
    shorten_edge(R, child, k);                /* child->edge now = suffix */
    replace_child(node, child, middle);       /* middle sits where child was */
    add_child(R, middle, child);              /* child hangs under middle */

    if (key[k] == '\0') {                     /* inserted key ends at middle */
        if (middle->t == NULL) R->count++;
        middle->t = t;
    } else {                                  /* diverging suffix -> new leaf */
        RNode *leaf = rnode_new(R, key + k, (int)strlen(key + k));
        leaf->t = t;
        R->count++;
        add_child(R, middle, leaf);
    }
}

static void rx_insert(void *self, Title *t) {
    Radix *R = (Radix *)self;
    radix_insert_rec(R, R->root, t->primaryTitle, t);
}

static Title *rx_search(void *self, const char *key) {
    Radix *R = (Radix *)self;
    RNode *node = R->root;
    const char *p = key;
    while (*p) {
        RNode *child = find_child(node, p[0]);
        if (!child) return NULL;
        int elen = (int)strlen(child->edge);
        if (strncmp(child->edge, p, elen) != 0) return NULL; /* diverged */
        p += elen;
        node = child;
    }
    return node->t;
}

static int rx_remove(void *self, const char *key) {
    Radix *R = (Radix *)self;
    RNode *node = R->root;
    const char *p = key;
    while (*p) {
        RNode *child = find_child(node, p[0]);
        if (!child) return 0;
        int elen = (int)strlen(child->edge);
        if (strncmp(child->edge, p, elen) != 0) return 0;
        p += elen;
        node = child;
    }
    if (node->t == NULL) return 0;            /* same clear-flag policy as Trie */
    node->t = NULL;
    R->count--;
    return 1;
}

static size_t rx_size(void *self)      { return ((Radix *)self)->count; }
static size_t rx_mem_bytes(void *self) { return ((Radix *)self)->bytes; }

static void free_subtree(RNode *n) {
    if (!n) return;
    for (int i = 0; i < n->n_kids; i++) free_subtree(n->kids[i]);
    if (n->kids) xfree(n->kids);
    xfree(n->edge);
    xfree(n);
}

static void rx_destroy(void *self) {
    Radix *R = (Radix *)self;
    free_subtree(R->root);
    xfree(R);
}

static size_t count_terminals(RNode *n) {
    if (!n) return 0;
    size_t c = (n->t != NULL) ? 1 : 0;
    for (int i = 0; i < n->n_kids; i++) c += count_terminals(n->kids[i]);
    return c;
}

size_t radix_count_prefix(Dictionary *d, const char *prefix) {
    Radix *R = (Radix *)d->self;
    RNode *node = R->root;
    const char *p = prefix;
    while (*p) {
        RNode *child = find_child(node, p[0]);
        if (!child) return 0;
        int k = common_len(child->edge, p);
        int elen = (int)strlen(child->edge);
        if (k == elen) { p += elen; node = child; continue; } /* full edge */
        if (p[k] == '\0') { node = child; break; }            /* prefix ends mid-edge */
        return 0;                                             /* diverged */
    }
    return count_terminals(node);
}

static void rcollect_dfs(RNode *n, Title **out, size_t max, size_t *cnt) {
    if (!n || *cnt >= max) return;
    if (n->t) out[(*cnt)++] = n->t;
    for (int i = 0; i < n->n_kids && *cnt < max; i++)
        rcollect_dfs(n->kids[i], out, max, cnt);
}

size_t radix_collect_prefix(Dictionary *d, const char *prefix,
                            Title **out, size_t max) {
    Radix *R = (Radix *)d->self;
    RNode *node = R->root;
    const char *p = prefix;
    while (*p) {
        RNode *child = find_child(node, p[0]);
        if (!child) return 0;
        int k = common_len(child->edge, p);
        int elen = (int)strlen(child->edge);
        if (k == elen) { p += elen; node = child; continue; }
        if (p[k] == '\0') { node = child; break; }
        return 0;
    }
    size_t cnt = 0;
    rcollect_dfs(node, out, max, &cnt);
    return cnt;
}

Dictionary radix_create(void) {
    Radix *R = (Radix *)xmalloc(sizeof(Radix));
    R->count = 0;
    R->bytes = sizeof(Radix);
    R->root = rnode_new(R, "", 0);            /* root: empty edge */

    Dictionary d;
    d.self = R; d.name = "Radix Tree";
    d.insert = rx_insert; d.search = rx_search; d.remove = rx_remove;
    d.size = rx_size; d.mem_bytes = rx_mem_bytes; d.destroy = rx_destroy;
    return d;
}
