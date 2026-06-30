#include "operations.h"
#include "timer.h"
#include "radix.h"      /* prefix collection uses the radix structure */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- configuration ---------------------------------------------------- */

/* Missing runtimes (-1) are INCLUDED per request. Set to 1 to exclude them
 * (recommended for a clean mean/min). One-character change. */
#define EXCLUDE_MISSING_RUNTIME 1

#define PREFIX_LIST_MAX 12      /* titles listed in autocomplete */
#define TOPK 20                /* size of each weighted top list */

/* ---- media category predicates --------------------------------------- */

typedef int (*TypePred)(const Title *);

/* Watchable: movies, TV series, short films. */
static int is_watchable(const Title *t) {
    return strcmp(t->titleType, "movie")    == 0 ||
           strcmp(t->titleType, "tvSeries") == 0 ||
           strcmp(t->titleType, "short")    == 0;
}
/* Playable: video games. */
static int is_playable(const Title *t) {
    return strcmp(t->titleType, "videoGame") == 0;
}
/* "Short" is a titleType, not a real content genre, so we drop it from the
 * predominant-genre analysis. */
static int is_excluded_genre(const char *g) {
    return strcmp(g, "Short") == 0;
}

/* ====================================================================== */
/*  Op 0 — Prefix / autocomplete                                          */
/* ====================================================================== */
void op_prefix_search(Dictionary *prefix_struct, const char *prefix) {
    Title *hits[PREFIX_LIST_MAX];
    uint64_t t0 = now_ns();
    size_t total = radix_count_prefix(prefix_struct, prefix);
    size_t shown = radix_collect_prefix(prefix_struct, prefix, hits, PREFIX_LIST_MAX);
    uint64_t dt = now_ns() - t0;

    printf("\n-- Autocomplete for \"%s\" --\n", prefix);
    if (total == 0) {
        printf("  (no titles begin with that prefix)\n");
    } else {
        printf("  %zu titles match; showing %zu:\n", total, shown);
        for (size_t i = 0; i < shown; i++) {
            Title *t = hits[i];
            printf("   %2zu. %-45.45s (%d)  rating %.1f\n",
                   i + 1, t->primaryTitle,
                   t->startYear == SENTINEL_MISSING ? 0 : t->startYear,
                   t->averageRating);
        }
    }
    printf("  [%.3f ms]\n", dt / 1e6);
}

/* ====================================================================== */
/*  Op 1 — Genre trend by decade + naive prediction (per category)        */
/* ====================================================================== */
#define MAX_DECADE_IDX 220
#define MAX_TREND_GENRES 64

static void genre_trend_for(Title *titles, size_t n, TypePred pred,
                            const char *label) {
    char  *gname[MAX_TREND_GENRES];
    int    ngenres = 0;
    static long counts[MAX_DECADE_IDX][MAX_TREND_GENRES];
    memset(counts, 0, sizeof(counts));
    int firstIdx = MAX_DECADE_IDX, lastIdx = 0;
    long matched = 0;

    for (size_t i = 0; i < n; i++) {
        Title *t = &titles[i];
        if (!pred(t)) continue;
        if (t->startYear == SENTINEL_MISSING || t->startYear < 1850) continue;
        int didx = t->startYear / 10;
        if (didx < 0 || didx >= MAX_DECADE_IDX) continue;
        matched++;
        if (didx < firstIdx) firstIdx = didx;
        if (didx > lastIdx)  lastIdx  = didx;

        for (int g = 0; g < t->genreCount; g++) {
            if (is_excluded_genre(t->genres[g])) continue;   /* skip "Short" */
            int col = -1;
            for (int c = 0; c < ngenres; c++)
                if (strcmp(gname[c], t->genres[g]) == 0) { col = c; break; }
            if (col < 0 && ngenres < MAX_TREND_GENRES) {
                col = ngenres; gname[ngenres++] = t->genres[g];
            }
            if (col >= 0) counts[didx][col]++;
        }
    }

    printf("\n  [%s]\n", label);
    if (matched == 0 || firstIdx > lastIdx) { printf("    (no data)\n"); return; }
    for (int d = firstIdx; d <= lastIdx; d++) {
        int best = -1; long bestc = 0;
        for (int c = 0; c < ngenres; c++)
            if (counts[d][c] > bestc) { bestc = counts[d][c]; best = c; }
        if (best >= 0)
            printf("    %ds : %-12s (%ld titles)\n", d * 10, gname[best], bestc);
    }

    int nextDecade = (lastIdx + 1) * 10;
    int npts = lastIdx - firstIdx + 1;
    int predBest = -1; double predVal = -1.0;
    for (int c = 0; c < ngenres; c++) {
        double Sx = 0, Sy = 0, Sxx = 0, Sxy = 0;
        for (int d = firstIdx; d <= lastIdx; d++) {
            double x = d * 10, y = (double)counts[d][c];
            Sx += x; Sy += y; Sxx += x * x; Sxy += x * y;
        }
        double denom = npts * Sxx - Sx * Sx;
        if (denom == 0) continue;
        double slope = (npts * Sxy - Sx * Sy) / denom;
        double inter = (Sy - slope * Sx) / npts;
        double proj  = inter + slope * nextDecade;
        if (proj > predVal) { predVal = proj; predBest = c; }
    }
    if (predBest >= 0 && predVal > 0)
        printf("    Prediction for %ds: %s (projected ~%.0f, naive linear trend)\n",
               nextDecade, gname[predBest], predVal);
}

void op_genre_trend(Title *titles, size_t n) {
    uint64_t t0 = now_ns();
    printf("\n-- Most frequent genre by decade (\"Short\" genre excluded) --");
    genre_trend_for(titles, n, is_watchable, "Watchable: movies, TV series, shorts");
    genre_trend_for(titles, n, is_playable,  "Playable: video games");
    printf("  [%.3f ms]\n", (now_ns() - t0) / 1e6);
}

/* ====================================================================== */
/*  Op 2 — Runtime statistics                                             */
/* ====================================================================== */
static int cmp_int(const void *a, const void *b) {
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}
typedef struct { char *name; double sum; long cnt; } GenreStat;
static int cmp_gstat(const void *a, const void *b) {
    const GenreStat *A = a, *B = b;
    double ma = A->cnt ? A->sum / A->cnt : 0;
    double mb = B->cnt ? B->sum / B->cnt : 0;
    return (mb > ma) - (mb < ma);
}

void op_runtime_stats(Title *titles, size_t n) {
    uint64_t t0 = now_ns();
    int   *vals = (int *)malloc(n * sizeof(int));
    size_t m = 0, missing = 0;
    double sum = 0;
    GenreStat gs[MAX_TREND_GENRES];
    int ng = 0;

    for (size_t i = 0; i < n; i++) {
        int rt = titles[i].runtimeMinutes;
        if (rt == SENTINEL_MISSING) {
            missing++;
#if EXCLUDE_MISSING_RUNTIME
            continue;
#endif
        }
        vals[m++] = rt; sum += rt;
        for (int g = 0; g < titles[i].genreCount; g++) {
            int col = -1;
            for (int c = 0; c < ng; c++)
                if (strcmp(gs[c].name, titles[i].genres[g]) == 0) { col = c; break; }
            if (col < 0 && ng < MAX_TREND_GENRES) {
                col = ng; gs[ng].name = titles[i].genres[g];
                gs[ng].sum = 0; gs[ng].cnt = 0; ng++;
            }
            if (col >= 0) { gs[col].sum += rt; gs[col].cnt++; }
        }
    }

    printf("\n-- Runtime statistics (minutes) --\n");
#if EXCLUDE_MISSING_RUNTIME
    printf("  (%zu titles with missing runtime EXCLUDED)\n", missing);
#else
    printf("  NOTE: %zu titles have missing runtime stored as -1 and are INCLUDED;\n", missing);
    printf("        this lowers the mean and makes the minimum -1.\n");
#endif
    if (m == 0) { printf("  (no data)\n"); free(vals); return; }
    qsort(vals, m, sizeof(int), cmp_int);
    double mean = sum / m;
    double median = (m % 2) ? vals[m/2] : (vals[m/2 - 1] + vals[m/2]) / 2.0;
    printf("  count=%zu  mean=%.1f  median=%.1f  min=%d  max=%d\n",
           m, mean, median, vals[0], vals[m-1]);
    qsort(gs, ng, sizeof(GenreStat), cmp_gstat);
    printf("  Mean runtime by genre (top 10 by mean):\n");
    for (int c = 0; c < ng && c < 10; c++)
        if (gs[c].cnt)
            printf("   %-12s %.1f min  (%ld titles)\n",
                   gs[c].name, gs[c].sum / gs[c].cnt, gs[c].cnt);
    free(vals);
    printf("  [%.3f ms]\n", (now_ns() - t0) / 1e6);
}

/* ====================================================================== */
/*  Op 3 — Top 10 by Bayesian weighted rating (per category)              */
/* ====================================================================== */
static void top10_for(Title *titles, size_t n, TypePred pred, const char *label) {
    /* C and m are computed within this category only. */
    double sumR = 0, sumV = 0; size_t cat = 0;
    for (size_t i = 0; i < n; i++) {
        if (!pred(&titles[i])) continue;
        sumR += titles[i].averageRating; sumV += (double)titles[i].numVotes; cat++;
    }
    printf("\n  [%s]  (%zu titles)\n", label, cat);
    if (cat == 0) { printf("    (no data)\n"); return; }
    double C = sumR / cat, mthr = sumV / cat;

    Title *best[TOPK]; double bestWR[TOPK]; int filled = 0;
    for (size_t i = 0; i < n; i++) {
        if (!pred(&titles[i])) continue;
        double v = (double)titles[i].numVotes, R = titles[i].averageRating;
        double wr = (v / (v + mthr)) * R + (mthr / (v + mthr)) * C;
        if (filled < TOPK) {
            int j = filled++;
            while (j > 0 && bestWR[j-1] < wr) { bestWR[j]=bestWR[j-1]; best[j]=best[j-1]; j--; }
            bestWR[j] = wr; best[j] = &titles[i];
        } else if (wr > bestWR[TOPK-1]) {
            int j = TOPK-1;
            while (j > 0 && bestWR[j-1] < wr) { bestWR[j]=bestWR[j-1]; best[j]=best[j-1]; j--; }
            bestWR[j] = wr; best[j] = &titles[i];
        }
    }
    printf("    (C = mean rating = %.2f ; m = mean votes = %.0f)\n", C, mthr);
    for (int i = 0; i < filled; i++) {
        Title *t = best[i];
        printf("    %2d. WR %.3f | %.1f stars | %8ld votes | %-40.40s\n",
               i + 1, bestWR[i], t->averageRating, t->numVotes, t->primaryTitle);
    }
}

void op_top10_weighted(Title *titles, size_t n) {
    uint64_t t0 = now_ns();
    printf("\n-- Top %d by Bayesian weighted rating --", TOPK);
    top10_for(titles, n, is_watchable, "Watchable: movies, TV series, shorts");
    top10_for(titles, n, is_playable,  "Playable: video games");
    printf("  [%.3f ms]\n", (now_ns() - t0) / 1e6);
}

/* ====================================================================== */
/*  Interactive sub-menu                                                  */
/* ====================================================================== */
void run_analytics_menu(Title *titles, size_t n, Dictionary *prefix_struct) {
    char line[256];
    for (;;) {
        printf("\n=== Analytics ===\n"
               " 1) Autocomplete (title prefix)\n"
               " 2) Genre trend by decade + prediction\n"
               " 3) Runtime statistics\n"
               " 4) Top 10 by weighted rating\n"
               " 0) Back\n> ");
        if (!fgets(line, sizeof(line), stdin)) return;
        switch (line[0]) {
            case '1': {
                printf("Enter title prefix: ");
                char pfx[256];
                if (fgets(pfx, sizeof(pfx), stdin)) {
                    size_t L = strlen(pfx);
                    while (L && (pfx[L-1]=='\n' || pfx[L-1]=='\r')) pfx[--L]=0;
                    op_prefix_search(prefix_struct, pfx);
                }
                break;
            }
            case '2': op_genre_trend(titles, n);    break;
            case '3': op_runtime_stats(titles, n);  break;
            case '4': op_top10_weighted(titles, n); break;
            case '0': return;
            default:  printf("Unknown option.\n"); break;
        }
    }
}
