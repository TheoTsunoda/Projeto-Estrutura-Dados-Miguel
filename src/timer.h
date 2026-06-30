#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* Monotonic high-resolution clock in nanoseconds.
 *
 * Use it as:
 *     uint64_t t0 = now_ns();
 *     ... operation to time ...
 *     uint64_t elapsed = now_ns() - t0;     // nanoseconds
 *
 * Portable across CLion's common toolchains: uses QueryPerformanceCounter on
 * Windows/MSVC, clock_gettime(CLOCK_MONOTONIC) on Linux/macOS/MinGW.
 */
uint64_t now_ns(void);

#endif /* TIMER_H */
