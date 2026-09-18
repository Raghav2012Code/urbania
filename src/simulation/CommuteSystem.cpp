#include "simulation/CommuteSystem.h"

#include "simulation/CitizenManager.h"
#include "simulation/Pathfinder.h"
#include "simulation/RoadNetwork.h"
#include "world/Tile.h"
#include "world/World.h"

namespace urbania {

void CommuteSystem::update(const World& world, const RoadNetwork& roadNetwork,
                           CitizenManager& citizens)
{
    // Recalculate only when something relevant changed: road graph
    // size, citizen roster, or employment count. Per-frame single-tile
    // edits always pass through a changed count, so no change slips by.
    const int roadNodes = roadNetwork.getNodeCount();
    int employed = 0;
    for (const Citizen& citizen : citizens.getCitizens())
    {
        if (citizen.employed)
        {
            ++employed;
        }
    }

    if (roadNodes == lastRoadNodeCount && citizens.getCitizenCount() == lastCitizenCount &&
        employed == lastEmployedCount)
    {
        return;
    }

    recalculateAllRoutes(world, roadNetwork, citizens);

    lastRoadNodeCount = roadNetwork.getNodeCount();
    lastCitizenCount = citizens.getCitizenCount();

    int employedNow = 0;
    for (const Citizen& citizen : citizens.getCitizens())
    {
        if (citizen.employed)
        {
            ++employedNow;
        }
    }
    lastEmployedCount = employedNow;
}

void CommuteSystem::recalculateAllRoutes(const World& world, const RoadNetwork& roadNetwork,
                                         CitizenManager& citizens)
{
    routedCitizens = 0;
    unroutedCitizens = 0;
    sampleRoute.clear();

    for (Citizen& citizen : citizens.getCitizens())
    {
        if (!citizen.employed)
        {
            citizen.commutePath.clear();
            continue;
        }

        const TileCoordinate from = adjacentRoad(world, citizen.home);
        const TileCoordinate to = adjacentRoad(world, citizen.workplace);
        if (!from.valid || !to.valid)
        {
            citizen.commutePath.clear();
            unroutedCitizens += 1;
            continue;
        }

        citizen.commutePath = Pathfinder::findPath(roadNetwork, from, to);
        if (citizen.commutePath.empty())
        {
            unroutedCitizens += 1;
        }
        else
        {
            routedCitizens += 1;
            if (sampleRoute.empty())
            {
                sampleRoute = citizen.commutePath;
            }
        }
    }
}

int CommuteSystem::getRoutedCitizens() const
{
    return routedCitizens;
}

int CommuteSystem::getUnroutedCitizens() const
{
    return unroutedCitizens;
}

const std::vector<TileCoordinate>& CommuteSystem::getSampleRoute() const
{
    return sampleRoute;
}

TileCoordinate CommuteSystem::adjacentRoad(const World& world, const TileCoordinate& tile)
{
    if (!tile.valid)
    {
        return {};
    }

    // Fixed Up, Right, Down, Left order for determinism.
    static constexpr int DX[4] = { 0, 1, 0, -1 };
    static constexpr int DY[4] = { -1, 0, 1, 0 };

    for (int i = 0; i < 4; ++i)
    {
        const int nx = tile.x + DX[i];
        const int ny = tile.y + DY[i];
        if (nx < 0 || nx >= world.getWidth() || ny < 0 || ny >= world.getHeight())
        {
            continue;
        }
        if (world.getTile(nx, ny).type == TileType::Road)
        {
            return { nx, ny, true };
        }
    }

    return {};
}

}  // namespace urbania
