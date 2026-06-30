#include "loader.h"
#include "alloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Cria uma duplicata da string em um novo espaço de memória */
static char *dup_str(const char *s) {
    size_t n = strlen(s) + 1; /* +1 porque strlen não inclui o \0 essencial em strings*/
    char *p = (char *)xmalloc(n);
    if (p) memcpy(p, s, n); /* Copia bit a bit de s em p, n bits (incluindo \0 */
    return p;
}

/* Retorna 1 se o campo estiver vazio, 0 caso contrário */
static int is_null(const char *s) {
    return s[0] == '\\' && s[1] == 'N' && s[2] == '\0';
}

/* Retorna se há sentinelas faltando */
static int parse_int(const char *s) {
    if (is_null(s)) return SENTINEL_MISSING;
    return atoi(s);
}

/* Arruma a linha de strings, colocando \0 entre tipos de valor e \t para indicar fim do título específico.
 * Além disso, coloca o array de ponteiros apontando para cada variável */
static int split_tabs(char *line, char **out, int max) {
    int n = 0;
    char *p = line;
    char *start = line;
    while (n < max) {
        if (*p == '\t' || *p == '\0') {
            int end = (*p == '\0');
            *p = '\0';
            out[n++] = start;
            if (end) break;
            start = p + 1;
        }
        p++;
    }
    return n;
}

/* Faz a separação dos gêneros por vírgula e coloc, de cada título, os gêneros e o contador de gêneros */
static void parse_genres(Title *t, char *field) {
    t->genreCount = 0;
    if (field[0] == '\0' || is_null(field)) {
        return;                 /* Sem gênero */
    }
    char *start = field;
    char *p = field;
    for (;;) {
        if (*p == ',' || *p == '\0') {
            int end = (*p == '\0');
            *p = '\0';
            if (t->genreCount < MAX_GENRES) {
                t->genres[t->genreCount++] = dup_str(start);
            }
            if (end) break;
            start = p + 1;
        }
        p++;
    }
}

Title *load_titles(const char *path, size_t *out_count, size_t limit) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "ERROR: could not open '%s'\n", path);
        return NULL;
    }

    size_t cap = 1024, count = 0;
    Title *arr = (Title *)malloc(cap * sizeof(Title));
    if (!arr) { fclose(f); return NULL; }

    char line[8192];
    int first = 1;

    while (fgets(line, sizeof(line), f)) {

        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }

        if (first) { first = 0; continue; }   /* pula a primeira linha */
        if (len == 0) continue;                /* pula linhas em branco */

        char *fields[11];
        int nf = split_tabs(line, fields, 11);
        if (nf < 11) continue;                 /* se não tiver exatas 11 colunas, pula */

        if (count == cap) {
            cap *= 2;
            Title *bigger = (Title *)realloc(arr, cap * sizeof(Title));
            if (!bigger) { free_titles(arr, count); fclose(f); return NULL; }
            arr = bigger;
        }

        Title *t = &arr[count];
        t->tconst        = dup_str(fields[0]);
        t->titleType     = dup_str(fields[1]);
        t->primaryTitle  = dup_str(fields[2]);
        t->originalTitle  = dup_str(fields[3]);
        t->isAdult       = parse_int(fields[4]);
        t->startYear     = parse_int(fields[5]);
        t->endYear       = parse_int(fields[6]);
        t->runtimeMinutes = parse_int(fields[7]);
        parse_genres(t, fields[8]);
        t->averageRating = is_null(fields[9]) ? 0.0 : atof(fields[9]);
        t->numVotes      = is_null(fields[10]) ? 0 : atol(fields[10]);

        count++;
        if (limit > 0 && count >= limit) break;
    }

    fclose(f);
    *out_count = count;
    return arr;
}

void free_titles(Title *titles, size_t count) {
    if (!titles) return;
    for (size_t i = 0; i < count; i++) {
        Title *t = &titles[i];
        xfree(t->tconst);
        xfree(t->titleType);
        xfree(t->primaryTitle);
        xfree(t->originalTitle);
        for (int g = 0; g < t->genreCount; g++) {
            xfree(t->genres[g]);
        }
    }
    free(titles);
}
