#pragma once

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
// Recalculation runs only when the world, citizens, or employment
// change, never blindly every frame. No raylib dependency here.
class CommuteSystem {
public:
    void update(const World& world, const RoadNetwork& roadNetwork, CitizenManager& citizens);
    void recalculateAllRoutes(const World& world, const RoadNetwork& roadNetwork,
                              CitizenManager& citizens);

    int getRoutedCitizens() const;
    int getUnroutedCitizens() const;
    const std::vector<TileCoordinate>& getSampleRoute() const;

private:
    static TileCoordinate adjacentRoad(const World& world, const TileCoordinate& tile);

    int lastRoadNodeCount = -1;
    int lastCitizenCount = -1;
    int lastEmployedCount = -1;

    int routedCitizens = 0;
    int unroutedCitizens = 0;
    std::vector<TileCoordinate> sampleRoute{};
};

}  // namespace urbania
