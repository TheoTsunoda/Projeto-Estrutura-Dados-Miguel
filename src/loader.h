#ifndef LOADER_H
#define LOADER_H

#include "title.h"
#include <stddef.h>

/* Carrega o dataset para o formato de array.
 *
 *  - path      : caminho para a pasta TSV
 *  - out_count : Recebe o número de registros carregados
 *  - limit     : if > 0, para de até a certa quantidade de dados; 0 = passa por todos os dados.
 *
 * Parsing rules:
 *  - Primeira linha (com os títulos) é pulada
 *  - Os campos são separados por tabs (exatamente 11)
 *  - O campo de gêneros é separado por vírgulas em Title.genres[]
 *  - Qualquer campo NULL é colocado a sentinela (-1 para números,
 *    lista vazia para lista de gêneros)
 */
Title *load_titles(const char *path, size_t *out_count, size_t limit);

/* "Free" a array e toda string e números que pertence a cada título. */
void   free_titles(Title *titles, size_t count);

#endif /* LOADER_H */
