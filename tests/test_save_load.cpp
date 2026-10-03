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
#include "simulation/CommuteSystem.h"

using namespace urbania;

namespace {

void buildDistrict(World& world, int xOffset)
{
    const int roadY = 9;
    for (int x = 0; x < 11; ++x)
    {
        world.getTile(xOffset + x, roadY).type = TileType::Road;
    }
    world.getTile(xOffset + 1, roadY + 1).type = TileType::Residential;
    world.getTile(xOffset + 9, roadY + 1).type = TileType::Commercial;
}

}  // namespace

// Issue #2: the loader capped the monotonic citizen ID counter with the cap
// meant for the LIVE population, so after ~100k cumulative creations every save
// was rejected by its own loader. MAX_CITIZENS now bounds the live count only.
void test_next_id_above_max_citizens_round_trips()
{
    const std::string path = "test_save_next_id.dat";

    World world;
    world.getTile(5, 5).type = TileType::Residential;

    Simulation simulation;
    simulation.initialize(world);
    simulation.getPopulation().getCitizenManager().createCitizen({ 5, 5, true });
    simulation.update(0.0f);

    // Force the monotonic counter above the loader's live-count cap while the
    // live population stays at one.
    std::vector<Citizen> live = simulation.getPopulation().getCitizens().getCitizens();
    simulation.getPopulation().getCitizenManager().restoreSaved(std::move(live), 120001);
    CHECK(simulation.getPopulation().getCitizenManager().getNextId() == 120001);

    SimulationClock clock;
    urbania::Camera camera;
    const auto saved = SaveSystem::save(world, simulation, clock, camera, path);
    CHECK(saved.ok);
    if (!saved.ok)
    {
        std::remove(path.c_str());
        return;
    }

    World loadedWorld;
    Simulation loadedSimulation;
    loadedSimulation.initialize(loadedWorld);
    SimulationClock loadedClock;
    urbania::Camera loadedCamera;
    const auto loaded =
        SaveSystem::load(loadedWorld, loadedSimulation, loadedClock, loadedCamera, path);

    CHECK(loaded.ok);
    CHECK(loadedSimulation.getPopulation().getCitizens().getCitizenCount() == 1);
    CHECK(loadedSimulation.getPopulation().getCitizenManager().getNextId() == 120001);

    std::remove(path.c_str());
}

// Issue #8: rebuildAfterLoad() reset traffic/congestion/citizenMovement but not
// CommuteSystem, so loading a city whose counts matched the running one kept the
// previous city's sampleRoute. The loaded city must describe itself.
void test_load_reflects_loaded_city_commute_stats()
{
    const std::string path = "test_save_commute.dat";

    // City A: district at x=10.
    World worldA;
    buildDistrict(worldA, 10);
    Simulation simA;
    simA.initialize(worldA);
    simA.getPopulation().getCitizenManager().createCitizen({ 11, 10, true });
    simA.update(0.0f);
    CHECK(simA.getCommuteSystem().getRoutedCitizens() >= 1);

    SimulationClock clockA;
    urbania::Camera cameraA;
    CHECK(SaveSystem::save(worldA, simA, clockA, cameraA, path).ok);

    // City B live: identical counts, same shape, translated by +30 tiles in x.
    World worldB;
    buildDistrict(worldB, 40);
    Simulation simB;
    simB.initialize(worldB);
    simB.getPopulation().getCitizenManager().createCitizen({ 41, 10, true });
    simB.update(0.0f);
    CHECK(simB.getCommuteSystem().getRoutedCitizens() >= 1);
    CHECK(simB.getRoadNetwork().getNodeCount() == simA.getRoadNetwork().getNodeCount());
    CHECK(simB.getPopulation().getCitizens().getCitizenCount() == 1);

    // Load A into the live B simulation.
    SimulationClock clockB;
    urbania::Camera cameraB;
    CHECK(SaveSystem::load(worldB, simB, clockB, cameraB, path).ok);

    const std::vector<TileCoordinate>& route = simB.getCommuteSystem().getSampleRoute();
    CHECK(!route.empty());
    CHECK(simB.getCommuteSystem().getRoutedCitizens() >= 1);

    // No tile from the previous (x >= 40) city may survive.
    bool anyPreviousCityTile = false;
    for (const TileCoordinate& step : route)
    {
        if (step.x >= 40)
        {
            anyPreviousCityTile = true;
        }
    }
    CHECK(!anyPreviousCityTile);

    std::remove(path.c_str());
}

int main()
{
    std::cout << "=== Running Save/Load Regression Tests ===\n";
    test_next_id_above_max_citizens_round_trips();
    test_load_reflects_loaded_city_commute_stats();
    return testcheck::summary("Save/Load Regression Tests");
}
