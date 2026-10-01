#pragma once

#include <cstdint>
#include <vector>

#include "world/Tile.h"

namespace urbania {

class CitizenManager;
class RoadNetwork;

}  // namespace urbania

class World;

namespace urbania {

// Calculates and stores home-to-workplace road routes for employed
// citizens. Routes use the existing A* Pathfinder between the road
// tiles adjacent to each endpoint (Up, Right, Down, Left order).
// Routes are data only: no movement happens here.
//
// Recalculation runs only when the road graph revision, the citizen
// roster, or the employment count changes -- never blindly every frame.
// The road graph is tracked by revision rather than node count, because
// demolishing one road and building another leaves the node count
// unchanged while connectivity differs completely. No raylib dependency.
class CommuteSystem {
public:
    void update(const World& world, const RoadNetwork& roadNetwork, CitizenManager& citizens);
    void recalculateAllRoutes(const World& world, const RoadNetwork& roadNetwork,
                              CitizenManager& citizens);

    // Forces the next update() to rebuild every route and clear the
    // derived counters. Required after the road graph is replaced without
    // a revision change being observable, such as
    // Simulation::rebuildAfterLoad() after loading a city.
    void invalidate();

    int getRoutedCitizens() const;
    int getUnroutedCitizens() const;
    const std::vector<TileCoordinate>& getSampleRoute() const;

private:
    static TileCoordinate adjacentRoad(const World& world, const TileCoordinate& tile);

    // Road graph revision is a full 64-bit counter, so -1 is a value it
    // can never take and is a safe "nothing cached yet" sentinel.
    std::uint64_t lastRoadRevision = static_cast<std::uint64_t>(-1);
    int lastCitizenCount = -1;
    int lastEmployedCount = -1;

    int routedCitizens = 0;
    int unroutedCitizens = 0;
    std::vector<TileCoordinate> sampleRoute{};
};

}  // namespace urbania
