#include "benchmark.h"
#include "timer.h"
#include "dict.h"
#include "linkedlist.h"
#include "hashtable.h"
#include "AVL.h"
#include "skiplist.h"
#include "trie.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ---- configuration ---------------------------------------------------- */
#define RESULTS_DIR "../results"     /* relative to the run dir (cmake-build) */
#define LIST_CAP    20000            /* linked list O(n^2) build: cap its size */
#define N_QUERIES   20000            /* search / query operations timed per cell */
#define MAX_TIERS   32
#define NSPEC       5

/* Metric indices */
enum { M_INSERT, M_SEARCH, M_QUERY, M_REMOVE, M_MEM, M_COUNT };

typedef struct {
    const char *name;
    Dictionary (*create)(void);
    int         key_is_title;        /* 1 = Trie (primaryTitle), 0 = tconst */
    const char *color;
} Spec;

typedef struct {
    double v[M_COUNT];               /* the five measured values */
    int    valid;
} Cell;

static const char *keyof(const Title *t, int title) {
    return title ? t->primaryTitle : t->tconst;
}

/* ---- SVG line-chart generator (log-log) ------------------------------- */
/* Writes a complete <svg> element plotting one polyline per structure. */
static void write_chart(FILE *f, const char *title, const char *ylabel,
                        size_t *T, int nt, Cell res[NSPEC][MAX_TIERS],
                        Spec *specs, int metric) {
    const double W = 820, H = 460;
    const double mL = 78, mR = 200, mT = 46, mB = 60;
    const double pw = W - mL - mR, ph = H - mT - mB;

    /* data ranges */
    double xmin = 1e300, xmax = -1e300, ymin = 1e300, ymax = -1e300;
    for (int ti = 0; ti < nt; ti++) {
        double lx = log10((double)T[ti]);
        if (lx < xmin) xmin = lx;
        if (lx > xmax) xmax = lx;
        for (int s = 0; s < NSPEC; s++) {
            if (!res[s][ti].valid) continue;
            double val = res[s][ti].v[metric];
            if (val <= 0) continue;
            double ly = log10(val);
            if (ly < ymin) ymin = ly;
            if (ly > ymax) ymax = ly;
        }
    }
    if (xmax <= xmin) xmax = xmin + 1;
    if (ymax <= ymin) ymax = ymin + 1;
    ymin = floor(ymin); ymax = ceil(ymax);   /* snap to powers of 10 */

    #define PX(lx) (mL + ((lx) - xmin) / (xmax - xmin) * pw)
    #define PY(ly) (mT + (1.0 - ((ly) - ymin) / (ymax - ymin)) * ph)

    fprintf(f, "<svg viewBox=\"0 0 %.0f %.0f\" xmlns=\"http://www.w3.org/2000/svg\" "
               "font-family=\"system-ui,Arial,sans-serif\" style=\"background:#fff\">\n", W, H);
    fprintf(f, "<text x=\"%.0f\" y=\"24\" font-size=\"18\" font-weight=\"700\">%s</text>\n", mL, title);
    fprintf(f, "<text x=\"%.0f\" y=\"40\" font-size=\"12\" fill=\"#666\">%s vs. number of elements (log-log)</text>\n", mL, ylabel);

    /* y gridlines at each power of 10 */
    for (double ly = ymin; ly <= ymax + 0.001; ly += 1.0) {
        double py = PY(ly);
        fprintf(f, "<line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\" stroke=\"#eee\"/>\n", mL, py, mL+pw, py);
        fprintf(f, "<text x=\"%.1f\" y=\"%.1f\" font-size=\"11\" fill=\"#888\" text-anchor=\"end\">1e%.0f</text>\n", mL-8, py+4, ly);
    }
    /* x gridlines at each power of 10 */
    for (double lx = ceil(xmin); lx <= xmax + 0.001; lx += 1.0) {
        double px = PX(lx);
        fprintf(f, "<line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\" stroke=\"#eee\"/>\n", px, mT, px, mT+ph);
        fprintf(f, "<text x=\"%.1f\" y=\"%.1f\" font-size=\"11\" fill=\"#888\" text-anchor=\"middle\">1e%.0f</text>\n", px, mT+ph+18, lx);
    }
    /* axes */
    fprintf(f, "<line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\" stroke=\"#333\"/>\n", mL, mT, mL, mT+ph);
    fprintf(f, "<line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\" stroke=\"#333\"/>\n", mL, mT+ph, mL+pw, mT+ph);

    /* one polyline per structure */
    for (int s = 0; s < NSPEC; s++) {
        fprintf(f, "<polyline fill=\"none\" stroke=\"%s\" stroke-width=\"2.5\" points=\"", specs[s].color);
        for (int ti = 0; ti < nt; ti++) {
            if (!res[s][ti].valid || res[s][ti].v[metric] <= 0) continue;
            fprintf(f, "%.1f,%.1f ", PX(log10((double)T[ti])), PY(log10(res[s][ti].v[metric])));
        }
        fprintf(f, "\"/>\n");
        for (int ti = 0; ti < nt; ti++) {
            if (!res[s][ti].valid || res[s][ti].v[metric] <= 0) continue;
            fprintf(f, "<circle cx=\"%.1f\" cy=\"%.1f\" r=\"3.5\" fill=\"%s\"/>\n",
                    PX(log10((double)T[ti])), PY(log10(res[s][ti].v[metric])), specs[s].color);
        }
    }
    /* legend */
    for (int s = 0; s < NSPEC; s++) {
        double ly = mT + 10 + s * 22;
        fprintf(f, "<rect x=\"%.0f\" y=\"%.0f\" width=\"14\" height=\"14\" fill=\"%s\"/>\n", mL+pw+20, ly, specs[s].color);
        fprintf(f, "<text x=\"%.0f\" y=\"%.0f\" font-size=\"13\">%s%s</text>\n",
                mL+pw+40, ly+12, specs[s].name, specs[s].key_is_title ? " *" : "");
    }
    fprintf(f, "<text x=\"%.0f\" y=\"%.0f\" font-size=\"11\" fill=\"#999\">* keyed on title</text>\n", mL+pw+20, mT+10+NSPEC*22+6);
    fprintf(f, "</svg>\n");
    #undef PX
    #undef PY
}

static void write_svg_file(const char *path, const char *title, const char *ylabel,
                           size_t *T, int nt, Cell res[NSPEC][MAX_TIERS], Spec *specs, int metric) {
    char full[512];
    snprintf(full, sizeof(full), "%s/%s", RESULTS_DIR, path);
    FILE *f = fopen(full, "w");
    if (!f) { fprintf(stderr, "  (could not write %s)\n", full); return; }
    write_chart(f, title, ylabel, T, nt, res, specs, metric);
    fclose(f);
}

/* ---- main entry ------------------------------------------------------- */
void run_benchmark(Title *titles, size_t count) {
    srand(12345);                    /* reproducible runs */

    Spec specs[NSPEC] = {
        { "Linked List", linkedlist_create, 0, "#dc2626" },
        { "Hash Table",  hashtable_create,  0, "#2563eb" },
        { "AVL Tree",    AVL_create,        0, "#16a34a" },
        { "Skip List",   skiplist_create,   0, "#d97706" },
        { "Trie",        trie_create,       1, "#7c3aed" },
    };

    /* build the tier list, clamped to `count` and de-duplicated */
    size_t wanted[] = {1000,2000,5000,10000,20000,50000,100000,200000,500000, count};
    size_t T[MAX_TIERS]; int nt = 0;
    for (size_t i = 0; i < sizeof(wanted)/sizeof(wanted[0]); i++) {
        size_t n = wanted[i] < count ? wanted[i] : count;
        if (nt > 0 && n <= T[nt-1]) continue;
        T[nt++] = n;
    }

    static Cell res[NSPEC][MAX_TIERS];
    const char **hit  = malloc(N_QUERIES * sizeof(char*));
    const char **mix  = malloc(N_QUERIES * sizeof(char*));
    char (*missbuf)[24] = malloc(N_QUERIES * 24);

    printf("\nBenchmark: %d tiers up to n=%zu. Linked list capped at %d.\n", nt, T[nt-1], LIST_CAP);

    for (int s = 0; s < NSPEC; s++) {
        for (int ti = 0; ti < nt; ti++) {
            size_t n = T[ti];
            res[s][ti].valid = 0;
            if (specs[s].key_is_title == 0 && s == 0 /*list*/ && n > LIST_CAP) {
                printf("  %-12s n=%-7zu  (skipped: exceeds list cap)\n", specs[s].name, n);
                continue;
            }
            int title = specs[s].key_is_title;
            Dictionary d = specs[s].create();

            /* insertion */
            uint64_t t0 = now_ns();
            for (size_t i = 0; i < n; i++) dict_insert(&d, &titles[i]);
            double ins_ns = (double)(now_ns() - t0) / (double)n;

            double mem = (double)dict_mem_bytes(&d);

            /* search (all hits) */
            for (int k = 0; k < N_QUERIES; k++)
                hit[k] = keyof(&titles[rand() % n], title);
            volatile size_t sink = 0;
            t0 = now_ns();
            for (int k = 0; k < N_QUERIES; k++) sink += (size_t)dict_search(&d, hit[k]);
            double srch_ns = (double)(now_ns() - t0) / N_QUERIES;

            /* mixed query: half existing hits, half fabricated misses */
            for (int k = 0; k < N_QUERIES; k++) {
                if (k & 1) {
                    if (title) snprintf(missbuf[k], 24, "\x02zzq%d", k);
                    else       snprintf(missbuf[k], 24, "zz%09d", k);
                    mix[k] = missbuf[k];
                } else {
                    mix[k] = keyof(&titles[rand() % n], title);
                }
            }
            t0 = now_ns();
            for (int k = 0; k < N_QUERIES; k++) sink += (size_t)dict_search(&d, mix[k]);
            double qry_ns = (double)(now_ns() - t0) / N_QUERIES;

            /* removal: distinct existing keys */
            int *perm = malloc(n * sizeof(int));
            for (size_t i = 0; i < n; i++) perm[i] = (int)i;
            for (size_t i = n - 1; i > 0; i--) { size_t j = rand() % (i + 1); int tmp = perm[i]; perm[i] = perm[j]; perm[j] = tmp; }
            size_t kr = n / 2 < 10000 ? n / 2 : 10000;
            if (kr < 1) kr = 1;
            t0 = now_ns();
            for (size_t j = 0; j < kr; j++) sink += dict_remove(&d, keyof(&titles[perm[j]], title));
            double rem_ns = (double)(now_ns() - t0) / (double)kr;
            free(perm);

            (void)sink;
            res[s][ti].v[M_INSERT] = ins_ns;
            res[s][ti].v[M_SEARCH] = srch_ns;
            res[s][ti].v[M_QUERY]  = qry_ns;
            res[s][ti].v[M_REMOVE] = rem_ns;
            res[s][ti].v[M_MEM]    = mem;
            res[s][ti].valid = 1;

            printf("  %-12s n=%-7zu  ins=%.1fns srch=%.1fns qry=%.1fns rem=%.1fns mem=%.2fMB\n",
                   specs[s].name, n, ins_ns, srch_ns, qry_ns, rem_ns, mem/1048576.0);
            dict_destroy(&d);
        }
    }
    free(hit); free(mix); free(missbuf);

    /* ---- write CSV ---- */
    char path[512];
    snprintf(path, sizeof(path), "%s/benchmark.csv", RESULTS_DIR);
    FILE *csv = fopen(path, "w");
    if (csv) {
        fprintf(csv, "structure,key,n,insert_ns,search_ns,query_ns,remove_ns,mem_bytes,mem_mb\n");
        for (int s = 0; s < NSPEC; s++)
            for (int ti = 0; ti < nt; ti++) {
                if (!res[s][ti].valid) continue;
                fprintf(csv, "%s,%s,%zu,%.3f,%.3f,%.3f,%.3f,%.0f,%.4f\n",
                        specs[s].name, specs[s].key_is_title ? "title" : "tconst", T[ti],
                        res[s][ti].v[M_INSERT], res[s][ti].v[M_SEARCH], res[s][ti].v[M_QUERY],
                        res[s][ti].v[M_REMOVE], res[s][ti].v[M_MEM], res[s][ti].v[M_MEM]/1048576.0);
            }
        fclose(csv);
        printf("Wrote %s\n", path);
    } else {
        fprintf(stderr, "Could not open %s (does the results/ folder exist?)\n", path);
    }

    /* ---- write SVG charts ---- */
    write_svg_file("insert.svg", "Insertion time per element", "nanoseconds / insert", T, nt, res, specs, M_INSERT);
    write_svg_file("search.svg", "Search time (hit) per query", "nanoseconds / search", T, nt, res, specs, M_SEARCH);
    write_svg_file("query.svg",  "Average query time (mixed hit/miss)", "nanoseconds / query", T, nt, res, specs, M_QUERY);
    write_svg_file("remove.svg", "Removal time per element", "nanoseconds / remove", T, nt, res, specs, M_REMOVE);
    write_svg_file("memory.svg", "Memory usage", "bytes", T, nt, res, specs, M_MEM);

    /* ---- write HTML dashboard ---- */
    snprintf(path, sizeof(path), "%s/benchmark.html", RESULTS_DIR);
    FILE *h = fopen(path, "w");
    if (h) {
        fprintf(h, "<!doctype html><meta charset=\"utf-8\"><title>Benchmark results</title>");
        fprintf(h, "<body style=\"font-family:system-ui,Arial,sans-serif;max-width:900px;margin:24px auto;color:#222\">");
        fprintf(h, "<h1>Data-structure benchmark</h1>");
        fprintf(h, "<p style=\"color:#555\">Insertion, search, mixed query, removal (per-operation, nanoseconds) and memory, "
                   "across input sizes. Axes are log-log, so a straight line of slope ~0 is O(1), gentle rise is O(log n), "
                   "slope ~1 is O(n), slope ~2 is O(n&sup2;).</p>");
        fprintf(h, "<p style=\"background:#fff7ed;border:1px solid #fed7aa;padding:10px;border-radius:8px;color:#9a3412\">"
                   "<b>Valid comparison group:</b> Linked List, Hash Table, AVL, Skip List (all keyed on <code>tconst</code>). "
                   "The <b>Trie</b> (marked *) is keyed on <code>primaryTitle</code>; its cost is O(key length), not O(element count), "
                   "so its curve is shown for completeness but is not directly comparable to the other four.</p>");
        const char *files[] = {"insert.svg","search.svg","query.svg","remove.svg","memory.svg"};
        for (int i = 0; i < 5; i++) {
            char sp[512]; snprintf(sp, sizeof(sp), "%s/%s", RESULTS_DIR, files[i]);
            FILE *sf = fopen(sp, "r");
            if (sf) { char buf[4096]; size_t r; fprintf(h, "<div style=\"margin:24px 0\">");
                while ((r = fread(buf,1,sizeof(buf),sf)) > 0) fwrite(buf,1,r,h);
                fprintf(h, "</div>"); fclose(sf); }
        }
        fprintf(h, "<h2>Raw data</h2><table border=\"1\" cellpadding=\"6\" style=\"border-collapse:collapse;font-size:13px\">");
        fprintf(h, "<tr style=\"background:#f3f4f6\"><th>structure<th>key<th>n<th>insert ns<th>search ns<th>query ns<th>remove ns<th>mem MB</tr>");
        for (int s = 0; s < NSPEC; s++)
            for (int ti = 0; ti < nt; ti++) {
                if (!res[s][ti].valid) continue;
                fprintf(h, "<tr><td>%s<td>%s<td>%zu<td>%.1f<td>%.1f<td>%.1f<td>%.1f<td>%.2f</tr>",
                        specs[s].name, specs[s].key_is_title?"title":"tconst", T[ti],
                        res[s][ti].v[M_INSERT], res[s][ti].v[M_SEARCH], res[s][ti].v[M_QUERY],
                        res[s][ti].v[M_REMOVE], res[s][ti].v[M_MEM]/1048576.0);
            }
        fprintf(h, "</table></body>");
        fclose(h);
        printf("Wrote %s\n", path);
    }
    printf("Done. Open %s/benchmark.html in a browser.\n", RESULTS_DIR);
}
