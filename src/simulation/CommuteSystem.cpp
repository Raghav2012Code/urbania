#include "simulation/CommuteSystem.h"

#include "simulation/CitizenManager.h"
#include "simulation/Pathfinder.h"
#include "simulation/RoadNetwork.h"
#include "world/Tile.h"
#include "world/World.h"

namespace urbania {

void CommuteSystem::invalidate()
{
    lastRoadRevision = static_cast<std::uint64_t>(-1);
    lastCitizenCount = -1;
    lastEmployedCount = -1;

    routedCitizens = 0;
    unroutedCitizens = 0;
    sampleRoute.clear();
}

void CommuteSystem::update(const World& world, const RoadNetwork& roadNetwork,
                           CitizenManager& citizens)
{
    // Recalculate only when something relevant changed. The road graph is
    // keyed on its rebuild revision, not its node count: demolishing one
    // road and building another keeps the count identical while
    // connectivity changes completely, and a count-based check would leave
    // every stored path pointing at tiles that are no longer roads.
    const std::uint64_t roadRevision = roadNetwork.revision();
    int employed = 0;
    for (const Citizen& citizen : citizens.getCitizens())
    {
        if (citizen.employed)
        {
            ++employed;
        }
    }

    if (roadRevision == lastRoadRevision && citizens.getCitizenCount() == lastCitizenCount &&
        employed == lastEmployedCount)
    {
        return;
    }

    recalculateAllRoutes(world, roadNetwork, citizens);

    lastRoadRevision = roadRevision;
    lastCitizenCount = citizens.getCitizenCount();
    lastEmployedCount = employed;
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

        // Defence in depth: never keep a path that is not entirely on the
        // road network. Pathfinder only walks road tiles, but validating
        // here means a stale path cannot survive even if the revision
        // check above were ever bypassed.
        bool fullyOnRoad = !citizen.commutePath.empty();
        for (const TileCoordinate& step : citizen.commutePath)
        {
            if (!step.valid || !roadNetwork.isRoad(step.x, step.y))
            {
                fullyOnRoad = false;
                break;
            }
        }

        if (!fullyOnRoad)
        {
            citizen.commutePath.clear();
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
