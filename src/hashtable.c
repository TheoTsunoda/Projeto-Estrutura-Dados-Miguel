#include "hashtable.h"
#include "alloc.h"
#include <string.h>

#define TABLE_SIZE 100003 /* número grande e primo para evitar colisões e clustering*/


static unsigned int hash_function(const char* key) {
    unsigned long int value = 0;
    unsigned int i = 0;
    unsigned int key_len = strlen(key);

    for (; i < key_len; i++) {
        value = value * 37 + key[i];
    }
    return value % TABLE_SIZE;
}


typedef struct hitem { /*O tratamento de colisões é feito criando uma lista ligada simples*/
    Title* t;
    struct hitem* next;
} hitem;

typedef struct {
    hitem** items;
    size_t count;
    size_t bytes;
    size_t collisions;
} hash_table;


static hitem* create_item(Title *t) {
    hitem* item = (hitem*) xmalloc(sizeof(hitem));
    if (!item) return NULL;
    item->t = t;
    item->next = NULL;
    return item;
}

static void ht_insert(void *self, Title *t) {
    hash_table *H = (hash_table *)self;
    unsigned int key = hash_function(t->tconst);
    for (hitem *h = H->items[key]; h != NULL; h = h->next) {
        if (strcmp(h->t->tconst, t->tconst) == 0) {
            h->t = t;
            return;
        }
    }

    hitem *hnode = (hitem *)xmalloc(sizeof(hitem));
    if (!hnode) return;
    hnode->t = t;

    if (H->items[key] != NULL) {
        H->collisions++;
    }
    hnode->next = H->items[key];
    H->items[key] = hnode;

    H->count++;
    H->bytes += sizeof(hitem);
}

static Title* ht_search(void* self, const char* key) {
    hash_table *H = (hash_table *)self;
    unsigned int i = hash_function(key); /*utilizado a variável i, pois key já está com o valor tconst*/
    for (hitem *h = H->items[i]; h != NULL; h = h->next) {
        if (strcmp(h->t->tconst, key) == 0) {
            return h->t;
        }
    }
    return NULL;
}


static int ht_remove(void *self, const char *key) {
    hash_table *H = (hash_table *)self;
    unsigned int index = hash_function(key);

    hitem *prev = NULL;
    hitem *cur = H->items[index];

    while (cur) {

        if (strcmp(cur->t->tconst, key) == 0) {
            if (prev) {
                prev->next = cur->next;
            } else {
                H->items[index] = cur->next;
            }

            xfree(cur);
            H->count--;
            H->bytes -= sizeof(hitem);
            return 1;
        }
        prev = cur;
        cur = cur->next;
    }
    return 0;
}

static size_t ht_size(void *self) {
    return ((hash_table *)self)->count;
}

static size_t ht_mem_bytes(void *self) {
    return ((hash_table *)self)->bytes;
}

static void ht_destroy(void *self) {
    hash_table *H = (hash_table *)self;

    for (int i = 0; i < TABLE_SIZE; i++) {
        hitem *cur = H->items[i];
        while (cur) {
            hitem *next = cur->next;
            xfree(cur);
            cur = next;
        }
    }
    xfree(H->items);
    xfree(H);
}

Dictionary hashtable_create(void) {
    hash_table *H = (hash_table*)xmalloc(sizeof(hash_table));
    H->count = 0;
    H->collisions = 0;
    H->items = (hitem**)xmalloc(TABLE_SIZE * sizeof(hitem*));

    for (int i=0; i<TABLE_SIZE; i++) {
        H->items[i] = NULL;
    }

    H->bytes = sizeof(hash_table) +(TABLE_SIZE*sizeof(hitem*));

    Dictionary d;
    d.self = H;
    d.name = "Hash Table";
    d.insert = ht_insert;
    d.search    = ht_search;
    d.remove    = ht_remove;
    d.size      = ht_size;
    d.mem_bytes = ht_mem_bytes;
    d.destroy   = ht_destroy;
    return d;
}

