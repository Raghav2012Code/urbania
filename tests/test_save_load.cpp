#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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
#include "simulation/Transit.h"
#include "simulation/BusRoute.h"
#include "simulation/BusStop.h"

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

// Rewrites the <scale> token of the CLOCK line in a save file.
void setClockScaleInFile(const std::string& path, const std::string& scale)
{
    std::ifstream in(path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line))
    {
        if (line.rfind("CLOCK ", 0) == 0)
        {
            std::istringstream ss(line);
            std::string tag, time, storedScale, paused;
            ss >> tag >> time >> storedScale >> paused;
            line = "CLOCK " + time + " " + scale + " " + paused;
        }
        lines.push_back(line);
    }
    in.close();

    std::ofstream out(path, std::ios::trunc);
    for (const std::string& stored : lines)
    {
        out << stored << "\n";
    }
}

// Rewrites a v2 save as v1: drops the HOURCLOCKS record and the version.
void downgradeSaveToV1(const std::string& path)
{
    std::ifstream in(path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line))
    {
        if (line.rfind("URBANIA_SAVE ", 0) == 0)
        {
            line = "URBANIA_SAVE 1";
        }
        if (line.rfind("HOURCLOCKS ", 0) == 0)
        {
            continue;
        }
        lines.push_back(line);
    }
    in.close();

    std::ofstream out(path, std::ios::trunc);
    for (const std::string& stored : lines)
    {
        out << stored << "\n";
    }
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

// Issue #15: clockScale was accepted as any finite float and then silently
// rewritten to 1x, breaking the "corrupt files keep the current city intact"
// guarantee. It is now validated against the clock's supported speeds.
void test_clock_scale_validated_on_load()
{
    const std::string path = "test_save_clock.dat";

    CHECK(SimulationClock::isSupportedSpeed(SimulationClock::NORMAL_SPEED));
    CHECK(SimulationClock::isSupportedSpeed(SimulationClock::FAST_SPEED));
    CHECK(SimulationClock::isSupportedSpeed(SimulationClock::VERY_FAST_SPEED));
    CHECK(SimulationClock::isSupportedSpeed(SimulationClock::EXTREMELY_FAST_SPEED));
    CHECK(!SimulationClock::isSupportedSpeed(7.5f));
    CHECK(!SimulationClock::isSupportedSpeed(-4.0f));

    World world;
    world.getTile(5, 5).type = TileType::Residential;
    Simulation simulation;
    simulation.initialize(world);
    SimulationClock clock;
    urbania::Camera camera;
    CHECK(SaveSystem::save(world, simulation, clock, camera, path).ok);

    setClockScaleInFile(path, "7.5");
    World worldBad;
    Simulation simBad;
    simBad.initialize(worldBad);
    SimulationClock clockBad;
    urbania::Camera cameraBad;
    CHECK(!SaveSystem::load(worldBad, simBad, clockBad, cameraBad, path).ok);

    setClockScaleInFile(path, "4.0");
    World worldGood;
    Simulation simGood;
    simGood.initialize(worldGood);
    SimulationClock clockGood;
    urbania::Camera cameraGood;
    CHECK(SaveSystem::load(worldGood, simGood, clockGood, cameraGood, path).ok);

    std::remove(path.c_str());
}

// Issue #13: the loader capped route count/length but Transit enforced neither
// at creation, so a legally-built city could become unsaveable. Both limits now
// live on Transit and are enforced on creation, and the loader uses the same
// constants, so a city at the cap round-trips.
void test_route_cap_enforced_and_round_trips()
{
    const std::string path = "test_save_routes.dat";

    World world;
    world.getTile(5, 5).type = TileType::Road;
    world.getTile(6, 5).type = TileType::Road;

    Simulation simulation;
    simulation.initialize(world);
    Transit& transit = simulation.getTransit();

    CHECK(transit.addBusStopFree(world, 5, 5));
    CHECK(transit.addBusStopFree(world, 6, 5));
    const BusStop* a = transit.getBusStop(5, 5);
    const BusStop* b = transit.getBusStop(6, 5);
    CHECK(a != nullptr && b != nullptr);
    if (a == nullptr || b == nullptr)
    {
        return;
    }

    // Fill directly to the cap (createRoute at this scale would be needlessly
    // quadratic); restoreSaved is the loader's own entry point.
    std::vector<BusStop> stops = transit.getBusStops();
    std::vector<BusRoute> routes;
    routes.reserve(Transit::MAX_ROUTES);
    for (size_t i = 0; i < Transit::MAX_ROUTES; ++i)
    {
        BusRoute route;
        route.id = static_cast<int>(i) + 1;
        route.stopIds = { a->id, b->id };
        routes.push_back(std::move(route));
    }
    transit.restoreSaved(stops, routes, 10000, static_cast<int>(Transit::MAX_ROUTES) + 1, 1);
    CHECK(transit.getRouteCount() == static_cast<int>(Transit::MAX_ROUTES));

    // One more route is refused rather than appended.
    CHECK(!transit.createRoute(simulation.getRoadNetwork(), { a->id, b->id }));
    CHECK(transit.getRouteCount() == static_cast<int>(Transit::MAX_ROUTES));

    // A city at the cap saves and loads again.
    SimulationClock clock;
    urbania::Camera camera;
    CHECK(SaveSystem::save(world, simulation, clock, camera, path).ok);

    World loadedWorld;
    Simulation loadedSimulation;
    loadedSimulation.initialize(loadedWorld);
    SimulationClock loadedClock;
    urbania::Camera loadedCamera;
    CHECK(SaveSystem::load(loadedWorld, loadedSimulation, loadedClock, loadedCamera, path).ok);
    CHECK(loadedSimulation.getTransit().getRouteCount() == static_cast<int>(Transit::MAX_ROUTES));

    std::remove(path.c_str());
}

// Issue #12: the hourly accumulators for LandValue, Housing, Happiness and
// Demand were neither persisted nor reset, so a loaded city ticked on the
// previous session's schedule. They now round-trip, and v1 files still load.
void test_hour_accumulators_round_trip()
{
    const std::string path = "test_save_hourclocks.dat";

    World world;
    world.getTile(5, 5).type = TileType::Residential;
    Simulation simulation;
    simulation.initialize(world);
    simulation.update(1800.0f);

    const float landValue = simulation.getLandValue().getSecondsTowardNextHour();
    const float housing = simulation.getHousing().getSecondsTowardNextHour();
    const float happiness = simulation.getHappiness().getSecondsTowardNextHour();
    const float demand = simulation.getDemand().getSecondsTowardNextHour();
    CHECK(landValue == 1800.0f);
    CHECK(housing == 1800.0f);
    CHECK(happiness == 1800.0f);
    CHECK(demand == 1800.0f);

    SimulationClock clock;
    urbania::Camera camera;
    CHECK(SaveSystem::save(world, simulation, clock, camera, path).ok);

    World loadedWorld;
    Simulation loadedSimulation;
    loadedSimulation.initialize(loadedWorld);
    SimulationClock loadedClock;
    urbania::Camera loadedCamera;
    CHECK(SaveSystem::load(loadedWorld, loadedSimulation, loadedClock, loadedCamera, path).ok);
    CHECK(loadedSimulation.getLandValue().getSecondsTowardNextHour() == landValue);
    CHECK(loadedSimulation.getHousing().getSecondsTowardNextHour() == housing);
    CHECK(loadedSimulation.getHappiness().getSecondsTowardNextHour() == happiness);
    CHECK(loadedSimulation.getDemand().getSecondsTowardNextHour() == demand);

    // A v1 save has no HOURCLOCKS record: it still loads, accumulators at zero.
    downgradeSaveToV1(path);
    World v1World;
    Simulation v1Simulation;
    v1Simulation.initialize(v1World);
    SimulationClock v1Clock;
    urbania::Camera v1Camera;
    CHECK(SaveSystem::load(v1World, v1Simulation, v1Clock, v1Camera, path).ok);
    CHECK(v1Simulation.getLandValue().getSecondsTowardNextHour() == 0.0f);
    CHECK(v1Simulation.getHousing().getSecondsTowardNextHour() == 0.0f);
    CHECK(v1Simulation.getHappiness().getSecondsTowardNextHour() == 0.0f);
    CHECK(v1Simulation.getDemand().getSecondsTowardNextHour() == 0.0f);

    std::remove(path.c_str());
}

int main()
{
    std::cout << "=== Running Save/Load Regression Tests ===\n";
    test_next_id_above_max_citizens_round_trips();
    test_load_reflects_loaded_city_commute_stats();
    test_clock_scale_validated_on_load();
    test_route_cap_enforced_and_round_trips();
    test_hour_accumulators_round_trip();
    return testcheck::summary("Save/Load Regression Tests");
}
