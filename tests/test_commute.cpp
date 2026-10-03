#include <iostream>

#include "test_check.h"

#include "world/World.h"
#include "world/Tile.h"
#include "simulation/Simulation.h"
#include "simulation/CommuteSystem.h"
#include "simulation/Population.h"
#include "simulation/CitizenManager.h"
#include "simulation/Economy.h"
#include "simulation/RoadNetwork.h"

using namespace urbania;

namespace {

void checkNoStaleCommutePath(const Simulation& simulation)
{
    const RoadNetwork& net = simulation.getRoadNetwork();
    for (const Citizen& citizen : simulation.getPopulation().getCitizens().getCitizens())
    {
        for (const TileCoordinate& step : citizen.commutePath)
        {
            CHECK(step.valid);
            CHECK(net.isRoad(step.x, step.y));
        }
    }
}

}  // namespace

// Issue #7: the commute cache was keyed on road NODE COUNT, so demolishing one
// road and building another (net-zero count) left every stored path pointing at
// tiles that are no longer roads. It is now keyed on a monotonic graph revision
// and each path is re-validated against the network.
void test_no_stale_path_after_net_zero_edit()
{
    World world;
    for (int x = 10; x <= 20; ++x)
    {
        world.getTile(x, 9).type = TileType::Road;
    }
    world.getTile(11, 10).type = TileType::Residential;
    world.getTile(19, 10).type = TileType::Commercial;

    Simulation simulation;
    simulation.initialize(world);
    simulation.getPopulation().getCitizenManager().createCitizen({ 11, 10, true });
    simulation.update(0.0f);

    CHECK(simulation.getCommuteSystem().getRoutedCitizens() >= 1);

    const int nodesBefore = simulation.getRoadNetwork().getNodeCount();

    // Net-zero edit: demolish one road on the route, build an unrelated road.
    CHECK(simulation.getEconomy().tryDemolish(world.getTile(15, 9)));
    CHECK(simulation.getEconomy().tryBuild(world.getTile(40, 50), TileType::Road));
    simulation.getRoadNetwork().rebuild(world);

    CHECK(simulation.getRoadNetwork().getNodeCount() == nodesBefore);

    simulation.update(0.0f);
    checkNoStaleCommutePath(simulation);

    // onWorldModified() must reach the same conclusion.
    simulation.onWorldModified();
    checkNoStaleCommutePath(simulation);
}

int main()
{
    std::cout << "=== Running Commute Regression Tests ===\n";
    test_no_stale_path_after_net_zero_edit();
    return testcheck::summary("Commute Regression Tests");
}
