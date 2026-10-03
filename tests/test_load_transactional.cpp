#include <cstdio>
#include <iostream>
#include <string>

#include "test_check.h"

#include "core/SaveSystem.h"
#include "core/Camera.h"
#include "world/World.h"
#include "world/Tile.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationClock.h"
#include "simulation/Population.h"
#include "simulation/CitizenManager.h"

using namespace urbania;

// Issue #10: the documented transactional contract was honoured during parsing
// but violated during apply -- a throw partway through left a half-overwritten
// city. This test arms a compile-time failpoint that throws after the world
// tiles are committed and asserts the live city is fully restored.
void test_failed_apply_leaves_city_untouched()
{
    const std::string path = "test_save_transactional.dat";

    // City A: district at x=10.
    World worldA;
    for (int x = 10; x <= 20; ++x)
    {
        worldA.getTile(x, 9).type = TileType::Road;
    }
    worldA.getTile(11, 10).type = TileType::Residential;
    worldA.getTile(19, 10).type = TileType::Commercial;
    Simulation simA;
    simA.initialize(worldA);
    simA.getPopulation().getCitizenManager().createCitizen({ 11, 10, true });
    simA.update(0.0f);

    SimulationClock clockA;
    urbania::Camera cameraA;
    CHECK(SaveSystem::save(worldA, simA, clockA, cameraA, path).ok);

    // Live city B: different geometry, different money.
    World worldB;
    for (int x = 40; x <= 50; ++x)
    {
        worldB.getTile(x, 9).type = TileType::Road;
    }
    worldB.getTile(41, 10).type = TileType::Residential;
    worldB.getTile(49, 10).type = TileType::Commercial;
    Simulation simB;
    simB.initialize(worldB);
    simB.getPopulation().getCitizenManager().createCitizen({ 41, 10, true });
    simB.update(0.0f);
    simB.getEconomy().restoreSavedState(54321, 0.0f, 0.0f, 0.0f);

    const int moneyBefore = simB.getEconomy().getMoney();
    const int populationBefore = simB.getPopulation().getCitizens().getCitizenCount();

    // Arm the failpoint and load A into B. The load must fail.
    SaveSystem::setLoadFailpointForTests(true);
    SimulationClock clockB;
    urbania::Camera cameraB;
    const auto failed = SaveSystem::load(worldB, simB, clockB, cameraB, path);
    CHECK(!failed.ok);

    // B must be exactly as it was: money, population, geometry.
    CHECK(simB.getEconomy().getMoney() == moneyBefore);
    CHECK(simB.getPopulation().getCitizens().getCitizenCount() == populationBefore);
    CHECK(worldB.getTile(41, 9).type == TileType::Road);
    CHECK(worldB.getTile(11, 9).type == TileType::Grass);

    // With the failpoint cleared, the same load succeeds.
    SaveSystem::setLoadFailpointForTests(false);
    CHECK(SaveSystem::load(worldB, simB, clockB, cameraB, path).ok);

    std::remove(path.c_str());
}

int main()
{
    std::cout << "=== Running Load Transactionality Tests ===\n";
    test_failed_apply_leaves_city_untouched();
    return testcheck::summary("Load Transactionality Tests");
}
