#pragma once

#include <vector>

#include "simulation/BusStop.h"
#include "world/Tile.h"

class World;
class Economy;

namespace urbania {

// Foundation for Urbania's public transit system.
// Manages bus stops: placement, removal, validation, and spatial queries.
//
// Placement rules:
// - Only on valid Road tiles.
// - At most one bus stop per road tile.
// - Cost: BUS_STOP_COST = 500 (deducted from Economy).
// - Removal: Free (no refund, does not destroy the road).
// - Demolishing a road removes any bus stop on it.
//
// IDs are deterministic and start at 1.
// Independent from raylib rendering.
class Transit {
public:
    static constexpr int BUS_STOP_COST = 500;

    Transit();

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

    const std::vector<BusStop>& getBusStops() const;
    int getBusStopCount() const;
    int getNextId() const;

    void syncWithWorld(const World& world);
    void clear();

private:
    std::vector<BusStop> busStops;
    int nextId = 1;
};

}  // namespace urbania
