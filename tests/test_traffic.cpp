#include <iostream>
#include <vector>

#include "test_check.h"

#include "world/World.h"
#include "world/Tile.h"
#include "simulation/CitizenManager.h"
#include "simulation/Congestion.h"
#include "simulation/RoadNetwork.h"
#include "simulation/Traffic.h"
#include "simulation/Vehicle.h"

using namespace urbania;

// Issue #6: a completed trip deactivated the vehicle, the sweep dropped its
// dedup entry, and the next frame allocated a brand-new Vehicle at pathIndex 0
// for the same citizen -- one per citizen per trip, forever, with nextVehicleId
// climbing without bound. Vehicles now shuttle and IDs stay bounded.
void test_vehicle_ids_stay_bounded_over_soak()
{
    World world;
    for (int x = 10; x <= 20; ++x)
    {
        world.getTile(x, 9).type = TileType::Road;
    }

    RoadNetwork net;
    net.rebuild(world);

    std::vector<TileCoordinate> route;
    for (int x = 10; x <= 20; ++x)
    {
        route.push_back({ x, 9, true });
    }

    CitizenManager citizens;
    for (int i = 0; i < 3; ++i)
    {
        Citizen& citizen = citizens.createCitizen({ 11, 10, true });
        citizen.employed = true;
        citizen.workplace = { 19, 10, true };
        citizen.commutePath = route;
        citizen.pathIndex = 0;
        citizen.commutingToWork = true;
    }

    Traffic traffic;
    Congestion congestion;

    int maxVehicleId = 0;
    int previousPathIndex = -1;
    bool reversedDirection = false;

    for (int frame = 0; frame < 2400; ++frame)
    {
        traffic.update(net, citizens, 0.25f, congestion);
        congestion.update(traffic);

        CHECK(traffic.getVehicleCount() == 3);

        for (const Vehicle& vehicle : traffic.getVehicles())
        {
            if (vehicle.id > maxVehicleId)
            {
                maxVehicleId = vehicle.id;
            }
        }

        if (!traffic.getVehicles().empty())
        {
            const int pathIndex = traffic.getVehicles().front().pathIndex;
            if (previousPathIndex >= 0 && pathIndex < previousPathIndex)
            {
                reversedDirection = true;
            }
            previousPathIndex = pathIndex;
        }
    }

    // Exactly three commuters -> exactly three vehicles, IDs 1..3, ever.
    CHECK(traffic.getVehicleCount() == 3);
    CHECK(maxVehicleId == 3);
    // Shuttling means the vehicle drives both directions of its route.
    CHECK(reversedDirection);
    CHECK(traffic.getActiveVehicleCount() == 3);
}

int main()
{
    std::cout << "=== Running Traffic Regression Tests ===\n";
    test_vehicle_ids_stay_bounded_over_soak();
    return testcheck::summary("Traffic Regression Tests");
}
