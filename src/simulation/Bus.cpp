#include "simulation/Bus.h"

#include <algorithm>

#include "simulation/Pathfinder.h"
#include "simulation/RoadNetwork.h"
#include "simulation/Transit.h"

namespace urbania {

void Bus::update(float deltaTime, const Transit& transit, const RoadNetwork& roadNetwork)
{
    if (deltaTime <= 0.0f)
    {
        return;
    }

    const BusRoute* route = transit.getRoute(routeId);
    if (route == nullptr || route->stopIds.size() < Transit::MIN_ROUTE_STOPS)
    {
        active = false;
        path.clear();
        return;
    }

    // If we have no path or current path is invalid/finished, build path to next stop
    if (path.empty() || pathIndex >= static_cast<int>(path.size()) - 1)
    {
        if (!buildNextLeg(transit, roadNetwork))
        {
            active = false;
            return;
        }
    }

    // Verify all remaining tiles in path still exist on the road network
    for (size_t i = static_cast<size_t>(std::max(0, pathIndex)); i < path.size(); ++i)
    {
        if (!roadNetwork.isRoad(path[i].x, path[i].y))
        {
            // Road was demolished underneath our planned route
            path.clear();
            if (!buildNextLeg(transit, roadNetwork))
            {
                active = false;
                return;
            }
            break;
        }
    }

    if (path.empty() || path.size() < 2)
    {
        active = false;
        return;
    }

    active = true;

    movementProgress += speed * deltaTime;
    while (movementProgress >= 1.0f && pathIndex < static_cast<int>(path.size()) - 1)
    {
        movementProgress -= 1.0f;
        ++pathIndex;

        // If we arrived at the end of this leg (the target bus stop)
        if (pathIndex >= static_cast<int>(path.size()) - 1)
        {
            currentStopIndex = (currentStopIndex + 1) % static_cast<int>(route->stopIds.size());
            if (!buildNextLeg(transit, roadNetwork))
            {
                active = false;
                return;
            }
            break;
        }
    }
}

bool Bus::buildNextLeg(const Transit& transit, const RoadNetwork& roadNetwork)
{
    const BusRoute* route = transit.getRoute(routeId);
    if (route == nullptr || route->stopIds.size() < Transit::MIN_ROUTE_STOPS)
    {
        return false;
    }

    const int totalStops = static_cast<int>(route->stopIds.size());
    if (currentStopIndex < 0 || currentStopIndex >= totalStops)
    {
        currentStopIndex = 0;
    }

    const int fromStopId = route->stopIds[currentStopIndex];
    const int toStopIndex = (currentStopIndex + 1) % totalStops;
    const int toStopId = route->stopIds[toStopIndex];

    const BusStop* fromStop = transit.getBusStopById(fromStopId);
    const BusStop* toStop = transit.getBusStopById(toStopId);

    if (fromStop == nullptr || toStop == nullptr || !fromStop->tile.valid || !toStop->tile.valid)
    {
        return false;
    }

    if (!roadNetwork.isRoad(fromStop->tile.x, fromStop->tile.y) ||
        !roadNetwork.isRoad(toStop->tile.x, toStop->tile.y))
    {
        return false;
    }

    // Determine our start tile: if we have a current valid position, start from there, else from fromStop
    TileCoordinate startTile = fromStop->tile;
    if (pathIndex >= 0 && pathIndex < static_cast<int>(path.size()))
    {
        startTile = path[pathIndex];
    }

    std::vector<TileCoordinate> newPath = Pathfinder::findPath(roadNetwork, startTile, toStop->tile);
    if (newPath.empty())
    {
        return false;
    }

    path = std::move(newPath);
    pathIndex = 0;
    movementProgress = 0.0f;
    active = true;
    return true;
}

}  // namespace urbania
