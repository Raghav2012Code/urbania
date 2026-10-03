#include <iostream>
#include <vector>

#include "test_check.h"

#include "world/World.h"
#include "world/Tile.h"
#include "simulation/RoadNetwork.h"
#include "simulation/Transit.h"
#include "simulation/Bus.h"
#include "simulation/BusStop.h"

using namespace urbania;

namespace {

void buildRoadRow(World& world, int y, int x0, int x1)
{
    for (int x = x0; x <= x1; ++x)
    {
        world.getTile(x, y).type = TileType::Road;
    }
}

}  // namespace

// Issue #1: a bus on a loop-closing route must survive the wrap-around
// degenerate leg and keep completing circuits. Before the fix it committed
// a 1-tile A->A path, tripped its own path.size() < 2 guard, and died forever.
void test_loop_route_bus_survives()
{
    World world;
    buildRoadRow(world, 10, 10, 14);

    RoadNetwork net;
    net.rebuild(world);

    Transit transit;
    CHECK(transit.addBusStopFree(world, 10, 10));
    CHECK(transit.addBusStopFree(world, 12, 10));
    CHECK(transit.addBusStopFree(world, 14, 10));

    const BusStop* a = transit.getBusStop(10, 10);
    const BusStop* b = transit.getBusStop(12, 10);
    const BusStop* c = transit.getBusStop(14, 10);
    CHECK(a != nullptr && b != nullptr && c != nullptr);
    if (a == nullptr || b == nullptr || c == nullptr)
    {
        return;
    }

    // [A, B, C, A] is the exact loop-closing route the editor offers.
    CHECK(transit.createRoute(net, { a->id, b->id, c->id, a->id }));
    CHECK(transit.getRouteCount() == 1);
    CHECK(transit.getBusCount() == 1);

    int stopVisits[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int previousIndex = -1;
    int framesAlive = 0;

    for (int frame = 0; frame < 600; ++frame)
    {
        transit.update(0.5f, net);
        CHECK(transit.getBusCount() == 1);
        if (transit.getBuses().empty())
        {
            break;
        }

        const Bus& bus = transit.getBuses().front();
        if (bus.active)
        {
            ++framesAlive;
            // A live bus always drives a real leg of 2+ tiles.
            CHECK(bus.path.size() >= 2);
            const int idx = bus.currentStopIndex;
            if (idx >= 0 && idx < 8 && idx != previousIndex)
            {
                ++stopVisits[idx];
            }
            previousIndex = idx;
        }
        else
        {
            previousIndex = -1;
        }
    }

    // Never dies: the whole point of the fix.
    CHECK(framesAlive == 600);
    // Reaching stops 0, 1 and 2 at least twice means at least two full circuits.
    CHECK(stopVisits[0] >= 2);
    CHECK(stopVisits[1] >= 2);
    CHECK(stopVisits[2] >= 2);
}

// Issue #11: validateRoute must check every pair the bus actually drives,
// including the wrap-around leg. Road connectivity is transitive, so a route
// that passes the consecutive checks cannot have an unreachable wrap; this is
// defensive hardening and locks the validator's observable contract.
void test_validate_route_contract()
{
    World world;
    buildRoadRow(world, 10, 10, 14);

    RoadNetwork net;
    net.rebuild(world);

    Transit transit;
    transit.addBusStopFree(world, 10, 10);
    transit.addBusStopFree(world, 12, 10);
    transit.addBusStopFree(world, 14, 10);

    const BusStop* a = transit.getBusStop(10, 10);
    const BusStop* b = transit.getBusStop(12, 10);
    const BusStop* c = transit.getBusStop(14, 10);
    CHECK(a != nullptr && b != nullptr && c != nullptr);
    if (a == nullptr || b == nullptr || c == nullptr)
    {
        return;
    }

    // Valid routes: a 2-stop route (wrap leg is B->A) and a 3-stop circuit
    // (wrap leg is C->A) must both be accepted.
    CHECK(transit.validateRoute(net, { a->id, b->id }));
    CHECK(transit.validateRoute(net, { a->id, b->id, c->id }));
    CHECK(transit.validateRoute(net, { a->id, b->id, c->id, a->id }));

    // Duplicate stop that is not a first/last loop closure is rejected.
    CHECK(!transit.validateRoute(net, { a->id, b->id, b->id }));

    // Unknown stop is rejected.
    CHECK(!transit.validateRoute(net, { a->id, 9999 }));

    // A route whose endpoints are on disconnected road islands is rejected.
    World islands;
    islands.getTile(10, 10).type = TileType::Road;
    islands.getTile(10, 11).type = TileType::Road;
    islands.getTile(20, 20).type = TileType::Road;
    islands.getTile(20, 21).type = TileType::Road;

    RoadNetwork islandNet;
    islandNet.rebuild(islands);

    Transit islandTransit;
    islandTransit.addBusStopFree(islands, 10, 10);
    islandTransit.addBusStopFree(islands, 20, 20);
    const BusStop* d = islandTransit.getBusStop(10, 10);
    const BusStop* e = islandTransit.getBusStop(20, 20);
    CHECK(d != nullptr && e != nullptr);
    if (d != nullptr && e != nullptr)
    {
        CHECK(!islandTransit.validateRoute(islandNet, { d->id, e->id }));
    }
}

// Issue #13: validateRoute must reject an over-long route at creation time so
// the game cannot accept a city the loader would then refuse. Uses distinct,
// mutually connected stops on a full road grid so only the length limit can be
// the reason for rejection.
void test_route_length_cap()
{
    World world;
    for (int y = 0; y < world.getHeight(); ++y)
    {
        for (int x = 0; x < world.getWidth(); ++x)
        {
            world.getTile(x, y).type = TileType::Road;
        }
    }

    RoadNetwork net;
    net.rebuild(world);

    Transit transit;
    std::vector<int> stopIds;
    stopIds.reserve(Transit::MAX_STOPS_PER_ROUTE + 1);
    for (int y = 0; y < world.getHeight() && stopIds.size() <= Transit::MAX_STOPS_PER_ROUTE; ++y)
    {
        for (int x = 0; x < world.getWidth() && stopIds.size() <= Transit::MAX_STOPS_PER_ROUTE; ++x)
        {
            if (transit.addBusStopFree(world, x, y))
            {
                stopIds.push_back(transit.getBusStop(x, y)->id);
            }
        }
    }

    CHECK(Transit::MIN_ROUTE_STOPS == 2);
    CHECK(Transit::MAX_ROUTES == 2000);
    CHECK(Transit::MAX_STOPS_PER_ROUTE == 5000);
    CHECK(stopIds.size() == Transit::MAX_STOPS_PER_ROUTE + 1);
    CHECK(!transit.validateRoute(net, stopIds));
}

int main()
{
    std::cout << "=== Running Transit Regression Tests ===\n";
    test_loop_route_bus_survives();
    test_validate_route_contract();
    test_route_length_cap();
    return testcheck::summary("Transit Regression Tests");
}
