#ifndef DICT_H
#define DICT_H

#include "title.h"
#include <stddef.h>

/* Interface comum para todas as estruturas
 *
 * Como as estruturas terão funções em comum entre elas, "procurar, inserir, remover", essa interface reune essas
 * funções, para que assim quando for necessário no menu ou para o benchmark, a forma de chamar é uma só e não precisa
 * ficar criando condições para cada estrutura. Como a linguagem C não permite classes e a característica de herança
 * (como Java) a forma de fazer isso é por uma struct de ponteiros que apontam para funções (ao invés de variáveis, o mais
 * comum). Assim, cada estrutura preencherá essa struct com suas funções mais um ponteiro "self" para si mesmo.
 *
 *
 *
 */

typedef struct Dictionary Dictionary;

struct Dictionary {
    /* Ponteiro void que muda dependendo da estrutura, aponta para os dados de cada estrutura, dizendo onde realizar
     * tais funções. Conceito de polimorfismo aplicado em C. */
    void *self;

    /* Nome da estrutura. */
    const char *name;

    /* Insere um título. */
    void   (*insert)(void *self, Title *t);

    /* Procura no dataset o título pelo código dele (tconst). Retorna a linha de informações dele ou NULl se não
     * encontrar*/
    Title *(*search)(void *self, const char *key);

    /* Remove o valor que tem o códgo tconst inserido. Retorna 1 se algo foi removido, 0 se encontrou. */
    int    (*remove)(void *self, const char *key);

    /* Outras funções importantes */

    /* Números de títulos salvos no momento. */
    size_t (*size)(void *self);

    /* Bytes utilizados pela estrutura, nos seus nós, ponteiros, arrays). Não do dataset */
    size_t (*mem_bytes)(void *self);

    /* Libera a memória usada pela estrutura. */
    void   (*destroy)(void *self);
};

/* Renomeando alguns comandos por conveniência */

static inline void   dict_insert(Dictionary *d, Title *t)        { d->insert(d->self, t); }
static inline Title *dict_search(Dictionary *d, const char *key) { return d->search(d->self, key); }
static inline int    dict_remove(Dictionary *d, const char *key) { return d->remove(d->self, key); }
static inline size_t dict_size(Dictionary *d)                    { return d->size(d->self); }
static inline size_t dict_mem_bytes(Dictionary *d)               { return d->mem_bytes(d->self); }
static inline void   dict_destroy(Dictionary *d)                 { d->destroy(d->self); }

#endif /* DICT_H */
