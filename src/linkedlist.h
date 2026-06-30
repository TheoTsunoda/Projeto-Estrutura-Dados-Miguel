#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include "dict.h"

/* ============================================================
 *  Lista ligada simples — O(n)
 * ============================================================
 *
 *
 * Essa lista busca pelo tconst:
 *   - insert : Começa no "Head", 0(1) - mas checa se essa chave que está tentando ser inserida
 *              já existe e atualiza a linha se sim, tornando-se O(n).
 *              (Fazer esse método torna o dataset mais impo)
 *   - search : Scaneamento linear, O(n).
 *   - remove : Scaneamento linar + remover, O(n).
 */
Dictionary linkedlist_create(void);

#endif /* LINKEDLIST_H */
