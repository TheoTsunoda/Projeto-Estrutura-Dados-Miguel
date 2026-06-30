#include "alloc.h"
#include <stdlib.h>
#include <string.h>

/* We over-allocate by a small header that records the block's size, so xfree
 * can subtract the right amount. The header is aligned via max_align_t so the
 * pointer we hand back is suitably aligned for any type. */
typedef union {
    size_t size;
    long double _align;   /* forces worst-case alignment of the payload */
} Header;

static size_t g_current = 0;
static size_t g_peak    = 0;
static size_t g_cap     = 0;

void *xmalloc(size_t size) {
    /* Força a restrição de memória, se houver */
    if (g_cap != 0 && g_current + size > g_cap) {
        return NULL;
    }

    Header *h = (Header *)malloc(sizeof(Header) + size); /*Tamanho da struct Header + tamanho que está sendo pedido*/
    if (!h) {
        return NULL;            /*Se tiver vazio, devolve NULL*/
    }
    h->size = size;

    g_current += size;
    if (g_current > g_peak) {
        g_peak = g_current;
    }

    return (void *)(h + 1);     /* A ordem que foi dada no malloc foi header, depois size. */
                                /*Então h+1 é o ponteiro para o slot de memória do size */
}

void xfree(void *ptr) {
    if (!ptr) {
        return;
    }
    Header *h = ((Header *)ptr) - 1; /* pega o ponteiro ptr, trata-o como um ponteiro de Header e "volta" a quantidade de memória de um header */
    g_current -= h->size;
    free(h);  /*Solta aquela bloco de memória, header + size */
}

size_t xmalloc_current(void) { return g_current; }
size_t xmalloc_peak(void)    { return g_peak; }

void xmalloc_reset_counters(void) {
    g_current = 0;
    g_peak = 0;
}

void xmalloc_set_cap(size_t cap_bytes) {
    g_cap = cap_bytes;
}
