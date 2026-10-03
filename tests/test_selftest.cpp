#include <iostream>
#include <string>

#include "test_check.h"

#include "core/SelfTest.h"
#include "world/World.h"
#include "simulation/Simulation.h"
#include "simulation/Economy.h"

using namespace urbania;

// Runs the in-engine self-test (F11) headlessly so the same coverage is
// exercised by CTest. Its assertions use check()/skip() rather than assert(),
// so results are trustworthy in Release too.
int main()
{
    std::cout << "=== Running In-Engine SelfTest ===\n";

    World world;
    Simulation simulation;
    simulation.initialize(world);
    Economy economy;

    SelfTest selfTest;
    selfTest.run(world, economy, simulation);

    CHECK(selfTest.hasRun());
    CHECK(selfTest.getTotal() > 0);
    CHECK(selfTest.allPassed());
    if (!selfTest.allPassed())
    {
        for (const std::string& line : selfTest.getResults())
        {
            std::cerr << line << "\n";
        }
    }

    return testcheck::summary("In-Engine SelfTest");
}
