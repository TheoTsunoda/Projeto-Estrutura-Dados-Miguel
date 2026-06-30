/* Permite o uso da função clock_gettime / CLOCK_MONOTONIC under strict -std=c11. */
#define _POSIX_C_SOURCE 199309L

#include "timer.h"

#if defined(_WIN32)
#include <windows.h>

uint64_t now_ns(void) {
    static LARGE_INTEGER freq;
    static int initialized = 0;
    if (!initialized) {
        QueryPerformanceFrequency(&freq);
        initialized = 1;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    /* counter / freq = segundos; em nanosegundos. */
    return (uint64_t)((counter.QuadPart * 1000000000ULL) / (uint64_t)freq.QuadPart);
}

#else
/* ---- Linux / macOS / MinGW with POSIX ---- */
#include <time.h>

uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}
#endif
