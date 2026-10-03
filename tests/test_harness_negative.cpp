#include "test_check.h"

// Deliberately failing probe. Built only when -DURBANIA_NEGATIVE_TESTS=ON and
// run by CI to prove the CHECK-based harness can actually report failure (the
// defect fixed in #9 was that Release test binaries could never fail). This
// file must never be registered as a CTest test.
int main()
{
    CHECK(1 == 2);
    CHECK(2 + 2 == 4);
    return testcheck::summary("Negative Harness Probe");
}
