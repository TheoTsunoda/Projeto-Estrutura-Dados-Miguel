#ifndef TITLE_H
#define TITLE_H

/* One IMDb record = one row of imdb_clean.tsv.
 *
 * Column order in the file:
 *   tconst  titleType  primaryTitle  originalTitle  isAdult
 *   startYear  endYear  runtimeMinutes  genres  averageRating  numVotes
 *
 * Missing numeric fields are stored as SENTINEL_MISSING (-1). Any operation
 * that uses such a field must skip records where it equals the sentinel
 */

#define SENTINEL_MISSING (-1)
#define MAX_GENRES 8          /* IMDb lists at most 3, but we leave headroom. */

typedef struct {
    char  *tconst;            /* unique key, e.g. "tt0000001"  (owned, heap)   */
    char  *titleType;         /* "movie", "short", "tvSeries", ... (owned)     */
    char  *primaryTitle;      /* display/Trie key (owned)                      */
    char  *originalTitle;     /* (owned)                                       */
    int    isAdult;           /* 0 or 1                                        */
    int    startYear;         /* year, or SENTINEL_MISSING                     */
    int    endYear;           /* year, or SENTINEL_MISSING                     */
    int    runtimeMinutes;    /* minutes, or SENTINEL_MISSING                  */
    char  *genres[MAX_GENRES];/* array of genre strings (owned)                */
    int    genreCount;        /* how many entries in genres[] are valid        */
    double averageRating;     /* 0.0 .. 10.0                                   */
    long   numVotes;          /* vote count                                    */
} Title;

#endif /* TITLE_H */
