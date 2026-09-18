#pragma once

#include <vector>

#include "simulation/Bus.h"
#include "simulation/BusRoute.h"
#include "simulation/BusStop.h"
#include "world/Tile.h"

class World;
class Economy;

namespace urbania {

class RoadNetwork;

// Foundation for Urbania's public transit system.
// Manages bus stops, bus routes, and buses.
//
// Bus Stop placement rules:
// - Only on valid Road tiles.
// - At most one bus stop per road tile.
// - Cost: BUS_STOP_COST = 500 (deducted from Economy).
// - Removal: Free (no refund, does not destroy the road).
// - Demolishing a road removes any bus stop on it.
//
// Bus Route rules:
// - Ordered sequence of at least 2 bus stop IDs.
// - Road network connectivity required between consecutive stops.
// - Deterministic 1-based IDs.
//
// Bus vehicle rules:
// - Exactly 1 bus spawned per valid route.
// - Deleted routes or broken routes safely despawn their bus.
// - Moves smoothly along A* road path from stop to stop in sequence and loops.
//
// Independent from raylib rendering.
class Transit {
public:
    static constexpr int BUS_STOP_COST = 500;
    static constexpr size_t MIN_ROUTE_STOPS = 2;

    Transit();

    // Bus Stop management
    bool canPlaceBusStop(const World& world, int x, int y) const;
    bool canPlaceBusStop(const World& world, const TileCoordinate& coord) const;

    bool addBusStop(const World& world, int x, int y, Economy& economy);
    bool addBusStop(const World& world, const TileCoordinate& coord, Economy& economy);

    bool addBusStopFree(const World& world, int x, int y);
    bool addBusStopFree(const World& world, const TileCoordinate& coord);

    bool removeBusStop(int x, int y);
    bool removeBusStop(const TileCoordinate& coord);

    bool hasBusStop(int x, int y) const;
    bool hasBusStop(const TileCoordinate& coord) const;

    const BusStop* getBusStop(int x, int y) const;
    const BusStop* getBusStop(const TileCoordinate& coord) const;
    const BusStop* getBusStopById(int stopId) const;

    const std::vector<BusStop>& getBusStops() const;
    int getBusStopCount() const;
    int getNextStopId() const;

    // Bus Route management
    bool validateRoute(const RoadNetwork& roadNetwork, const std::vector<int>& stopIds) const;
    bool createRoute(const RoadNetwork& roadNetwork, const std::vector<int>& stopIds);
    bool deleteRoute(int routeId);
    bool deleteLatestRoute();

    const BusRoute* getRoute(int routeId) const;
    const std::vector<BusRoute>& getRoutes() const;
    int getRouteCount() const;
    int getNextRouteId() const;

    // Bus vehicle management
    void update(float deltaTime, const RoadNetwork& roadNetwork);
    const std::vector<Bus>& getBuses() const;
    const Bus* getBus(int busId) const;
    int getBusCount() const;
    int getActiveBusCount() const;

    void syncWithWorld(const World& world);
    void clear();

private:
    void cleanupRoutes();
    void syncBusesWithRoutes();

    std::vector<BusStop> busStops;
    std::vector<BusRoute> routes;
    std::vector<Bus> buses;
    int nextStopId = 1;
    int nextRouteId = 1;
    int nextBusId = 1;
};

}  // namespace urbania
