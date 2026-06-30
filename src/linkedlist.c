#include "linkedlist.h"
#include "alloc.h"
#include <stdlib.h>
#include <string.h>

/* Criando o "esqueleto" de um nó */
typedef struct LLNode {
    Title         *t;     /* Nó apontando para o título, typedef criado no title.h*/
    struct LLNode *next;
} LLNode;

/*Uma struct da lista ligada, é necessário para dizer para onde o void *self aponta*/
typedef struct {
    LLNode *head;
    size_t  count;        /* Número de nós */
    size_t  bytes;        /* Bytes da estrutura de dado (nós+self) */
} LinkedList;



static void ll_insert(void *self, Title *t) {
    LinkedList *L = (LinkedList *)self;

    /* Evita duplicatas no mesmo tconst. Se já houver ele na lista, o nó é atualizado, substituido */
    for (LLNode *n = L->head; n; n = n->next) {
        if (strcmp(n->t->tconst, t->tconst) == 0) {
            n->t = t;
            return;
        }
    }

    /*Cria nó, coloca o novo valor dele, coloca esse nó no começo da lista, e aumenta o contador e nº de byes*/
    LLNode *node = (LLNode *)xmalloc(sizeof(LLNode));
    if (!node) return;
    node->t = t;
    node->next = L->head;
    L->head = node;
    L->count++;
    L->bytes += sizeof(LLNode);
}

static Title *ll_search(void *self, const char *key) {
    LinkedList *L = (LinkedList *)self;
    for (LLNode *n = L->head; n; n = n->next) {
        if (strcmp(n->t->tconst, key) == 0)
            return n->t;
    }
    return NULL;
}

static int ll_remove(void *self, const char *key) {
    LinkedList *L = (LinkedList *)self;
    LLNode *prev = NULL, *cur = L->head;
    while (cur) {
        if (strcmp(cur->t->tconst, key) == 0) {
            if (prev) prev->next = cur->next;
            else      L->head    = cur->next;
            xfree(cur);
            L->count--;
            L->bytes -= sizeof(LLNode);
            return 1;
        }
        prev = cur;
        cur = cur->next;
    }
    return 0;
}

static size_t ll_size(void *self)      { return ((LinkedList *)self)->count; }
static size_t ll_mem_bytes(void *self) { return ((LinkedList *)self)->bytes; }

static void ll_destroy(void *self) {
    LinkedList *L = (LinkedList *)self;
    LLNode *n = L->head;
    while (n) {
        LLNode *next = n->next;
        xfree(n);
        n = next;
    }
    xfree(L);
}



Dictionary linkedlist_create(void) {
    LinkedList *L = (LinkedList *)xmalloc(sizeof(LinkedList));
    L->head  = NULL;
    L->count = 0;
    L->bytes = sizeof(LinkedList);

    Dictionary d;
    d.self      = L;
    d.name      = "Linked List";
    d.insert    = ll_insert;
    d.search    = ll_search;
    d.remove    = ll_remove;
    d.size      = ll_size;
    d.mem_bytes = ll_mem_bytes;
    d.destroy   = ll_destroy;
    return d;
}
