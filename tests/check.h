#pragma once
// Tiny test helpers: no framework, no window. Failures are counted and reported by test_main.cpp.
#include <cmath>
#include <cstdio>

extern int g_checks;
extern int g_failures;

#define CHECK(cond)                                                                   \
    do {                                                                              \
        ++g_checks;                                                                   \
        if (!(cond)) {                                                                \
            ++g_failures;                                                             \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);               \
        }                                                                             \
    } while (0)

#define CHECK_NEAR(a, b, eps)                                                         \
    do {                                                                              \
        ++g_checks;                                                                   \
        const double a_ = (double)(a), b_ = (double)(b);                              \
        if (std::fabs(a_ - b_) > (eps)) {                                             \
            ++g_failures;                                                             \
            std::printf("FAIL %s:%d: %s = %g, expected %s = %g\n", __FILE__, __LINE__, \
                        #a, a_, #b, b_);                                              \
        }                                                                             \
    } while (0)
