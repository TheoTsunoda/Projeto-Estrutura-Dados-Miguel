#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "dict.h"

/* HASH TABLE (em média O(1)) */
/* Tratamento de colisões: chaining, uso de listas ligadas simples para cada índice que contém mais de 1 título*/
/* Inserir: usa o "tconst". Faz o cálculo do índice, procura se já está ocupado. Se está,
 *          procura se dentro da lista ligada tem tconst igual. Se sim, atualiza. Caso não,
 *          adiciona no começo da lista ligada simples.
 * Remoção:  Usa o "tconst". Faz o cálculo do índice, procura. Se encontrar remove e retorna 0, se não, retorna 1.
 * Procura: Usa o "tconst". Procura e se tem devolve a linha de informações do título correspondente,
 * se não devolve NULL*/

Dictionary hashtable_create(void);

#endif //HASHTABLE_H
