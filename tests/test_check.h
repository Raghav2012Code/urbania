#pragma once

// Minimal, dependency-free test harness that survives -DNDEBUG.
//
// The project previously used bare <cassert>, so Release builds (-DNDEBUG)
// compiled every check out and the test binaries could never fail. CHECK()
// always evaluates its condition, records failures, prints them, and lets
// main() return a non-zero exit code so CTest and CI can see the failure.

#include <cmath>
#include <cstdio>

namespace testcheck {

inline int& failures()
{
    static int value = 0;
    return value;
}

inline int& checks()
{
    static int value = 0;
    return value;
}

inline void report(bool ok, const char* expr, const char* file, int line)
{
    ++checks();
    if (!ok)
    {
        ++failures();
        std::fprintf(stderr, "FAIL %s:%d: %s\n", file, line, expr);
    }
}

inline int summary(const char* suite)
{
    if (failures() == 0)
    {
        std::printf("=== %s: all %d checks passed ===\n", suite, checks());
        return 0;
    }
    std::fprintf(stderr, "=== %s: %d of %d checks FAILED ===\n", suite, failures(), checks());
    return 1;
}

}  // namespace testcheck

#define CHECK(cond) \
    ::testcheck::report(static_cast<bool>(cond), #cond, __FILE__, __LINE__)

#define CHECK_EQ(a, b) \
    ::testcheck::report((a) == (b), #a " == " #b, __FILE__, __LINE__)

#define CHECK_NEAR(a, b, eps)                                                       \
    ::testcheck::report(std::fabs(static_cast<double>(a) - static_cast<double>(b)) <= \
                            static_cast<double>(eps),                               \
                        #a " ~= " #b, __FILE__, __LINE__)
