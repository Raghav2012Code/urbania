#include "simulation/Transit.h"

#include <algorithm>

#include "simulation/Economy.h"
#include "simulation/Pathfinder.h"
#include "simulation/RoadNetwork.h"
#include "world/Tile.h"
#include "world/World.h"

namespace urbania {

Transit::Transit()
{
}

bool Transit::canPlaceBusStop(const World& world, int x, int y) const
{
    if (x < 0 || x >= world.getWidth() || y < 0 || y >= world.getHeight())
    {
        return false;
    }

    if (world.getTile(x, y).type != TileType::Road)
    {
        return false;
    }

    if (hasBusStop(x, y))
    {
        return false;
    }

    return true;
}

bool Transit::canPlaceBusStop(const World& world, const TileCoordinate& coord) const
{
    if (!coord.valid)
    {
        return false;
    }
    return canPlaceBusStop(world, coord.x, coord.y);
}

bool Transit::addBusStop(const World& world, int x, int y, Economy& economy)
{
    if (!canPlaceBusStop(world, x, y))
    {
        return false;
    }

    if (!economy.canAfford(BUS_STOP_COST))
    {
        return false;
    }

    if (!economy.spendMoney(BUS_STOP_COST))
    {
        return false;
    }

    busStops.push_back({ nextStopId++, { x, y, true } });
    return true;
}

bool Transit::addBusStop(const World& world, const TileCoordinate& coord, Economy& economy)
{
    if (!coord.valid)
    {
        return false;
    }
    return addBusStop(world, coord.x, coord.y, economy);
}

bool Transit::addBusStopFree(const World& world, int x, int y)
{
    if (!canPlaceBusStop(world, x, y))
    {
        return false;
    }

    busStops.push_back({ nextStopId++, { x, y, true } });
    return true;
}

bool Transit::addBusStopFree(const World& world, const TileCoordinate& coord)
{
    if (!coord.valid)
    {
        return false;
    }
    return addBusStopFree(world, coord.x, coord.y);
}

bool Transit::removeBusStop(int x, int y)
{
    for (auto it = busStops.begin(); it != busStops.end(); ++it)
    {
        if (it->tile.valid && it->tile.x == x && it->tile.y == y)
        {
            busStops.erase(it);
            cleanupRoutes();
            return true;
        }
    }
    return false;
}

bool Transit::removeBusStop(const TileCoordinate& coord)
{
    if (!coord.valid)
    {
        return false;
    }
    return removeBusStop(coord.x, coord.y);
}

bool Transit::hasBusStop(int x, int y) const
{
    return getBusStop(x, y) != nullptr;
}

bool Transit::hasBusStop(const TileCoordinate& coord) const
{
    if (!coord.valid)
    {
        return false;
    }
    return hasBusStop(coord.x, coord.y);
}

const BusStop* Transit::getBusStop(int x, int y) const
{
    for (const auto& stop : busStops)
    {
        if (stop.tile.valid && stop.tile.x == x && stop.tile.y == y)
        {
            return &stop;
        }
    }
    return nullptr;
}

const BusStop* Transit::getBusStop(const TileCoordinate& coord) const
{
    if (!coord.valid)
    {
        return nullptr;
    }
    return getBusStop(coord.x, coord.y);
}

const BusStop* Transit::getBusStopById(int stopId) const
{
    for (const auto& stop : busStops)
    {
        if (stop.id == stopId)
        {
            return &stop;
        }
    }
    return nullptr;
}

const std::vector<BusStop>& Transit::getBusStops() const
{
    return busStops;
}

int Transit::getBusStopCount() const
{
    return static_cast<int>(busStops.size());
}

int Transit::getNextStopId() const
{
    return nextStopId;
}

bool Transit::validateRoute(const RoadNetwork& roadNetwork,
                            const std::vector<int>& stopIds) const
{
    if (stopIds.size() < MIN_ROUTE_STOPS)
    {
        return false;
    }

    // Check existence and duplicate rules
    for (size_t i = 0; i < stopIds.size(); ++i)
    {
        if (getBusStopById(stopIds[i]) == nullptr)
        {
            return false;
        }

        for (size_t j = i + 1; j < stopIds.size(); ++j)
        {
            if (stopIds[i] == stopIds[j])
            {
                // Only allow matching if closing a loop at the first & last index with >= 3 stops
                if (i == 0 && j == stopIds.size() - 1 && stopIds.size() >= 3)
                {
                    continue;
                }
                return false;
            }
        }
    }

    // Check road connectivity between consecutive pairs
    for (size_t i = 0; i + 1 < stopIds.size(); ++i)
    {
        const BusStop* a = getBusStopById(stopIds[i]);
        const BusStop* b = getBusStopById(stopIds[i + 1]);
        if (a == nullptr || b == nullptr)
        {
            return false;
        }

        if (a->tile == b->tile)
        {
            continue;
        }

        const auto path = Pathfinder::findPath(roadNetwork, a->tile, b->tile);
        if (path.empty() || path.front() != a->tile || path.back() != b->tile)
        {
            return false;
        }
    }

    return true;
}

bool Transit::createRoute(const RoadNetwork& roadNetwork, const std::vector<int>& stopIds)
{
    if (!validateRoute(roadNetwork, stopIds))
    {
        return false;
    }

    routes.push_back({ nextRouteId++, stopIds });
    syncBusesWithRoutes();
    return true;
}

bool Transit::deleteRoute(int routeId)
{
    for (auto it = routes.begin(); it != routes.end(); ++it)
    {
        if (it->id == routeId)
        {
            routes.erase(it);
            syncBusesWithRoutes();
            return true;
        }
    }
    return false;
}

bool Transit::deleteLatestRoute()
{
    if (routes.empty())
    {
        return false;
    }
    routes.pop_back();
    syncBusesWithRoutes();
    return true;
}

const BusRoute* Transit::getRoute(int routeId) const
{
    for (const auto& route : routes)
    {
        if (route.id == routeId)
        {
            return &route;
        }
    }
    return nullptr;
}

const std::vector<BusRoute>& Transit::getRoutes() const
{
    return routes;
}

int Transit::getRouteCount() const
{
    return static_cast<int>(routes.size());
}

int Transit::getNextRouteId() const
{
    return nextRouteId;
}

void Transit::update(float deltaTime, const RoadNetwork& roadNetwork)
{
    syncBusesWithRoutes();
    for (auto& bus : buses)
    {
        bus.update(deltaTime, *this, roadNetwork);
    }
}

const std::vector<Bus>& Transit::getBuses() const
{
    return buses;
}

const Bus* Transit::getBus(int busId) const
{
    for (const auto& bus : buses)
    {
        if (bus.id == busId)
        {
            return &bus;
        }
    }
    return nullptr;
}

int Transit::getBusCount() const
{
    return static_cast<int>(buses.size());
}

int Transit::getActiveBusCount() const
{
    int count = 0;
    for (const auto& bus : buses)
    {
        if (bus.active)
        {
            ++count;
        }
    }
    return count;
}

void Transit::syncWithWorld(const World& world)
{
    bool stopsChanged = false;
    for (auto it = busStops.begin(); it != busStops.end();)
    {
        if (!it->tile.valid || it->tile.x < 0 || it->tile.x >= world.getWidth() ||
            it->tile.y < 0 || it->tile.y >= world.getHeight() ||
            world.getTile(it->tile.x, it->tile.y).type != TileType::Road)
        {
            it = busStops.erase(it);
            stopsChanged = true;
        }
        else
        {
            ++it;
        }
    }

    if (stopsChanged)
    {
        cleanupRoutes();
    }
}

void Transit::cleanupRoutes()
{
    for (auto& route : routes)
    {
        route.stopIds.erase(
            std::remove_if(route.stopIds.begin(), route.stopIds.end(),
                           [this](int stopId) { return getBusStopById(stopId) == nullptr; }),
            route.stopIds.end());
    }

    // Remove routes that fell below the minimum 2 stops
    routes.erase(std::remove_if(routes.begin(), routes.end(),
                                [](const BusRoute& r) {
                                    return r.stopIds.size() < MIN_ROUTE_STOPS;
                                }),
                 routes.end());

    syncBusesWithRoutes();
}

void Transit::syncBusesWithRoutes()
{
    // Remove buses whose route no longer exists
    buses.erase(std::remove_if(buses.begin(), buses.end(),
                               [this](const Bus& bus) {
                                   return getRoute(bus.routeId) == nullptr;
                               }),
                buses.end());

    // Ensure each valid route has exactly one bus
    for (const auto& route : routes)
    {
        bool found = false;
        for (const auto& bus : buses)
        {
            if (bus.routeId == route.id)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            Bus newBus;
            newBus.id = nextBusId++;
            newBus.routeId = route.id;
            newBus.currentStopIndex = 0;
            newBus.pathIndex = 0;
            newBus.movementProgress = 0.0f;
            newBus.speed = Bus::DEFAULT_SPEED;
            newBus.active = false;
            buses.push_back(newBus);
        }
    }
}

void Transit::clear()
{
    busStops.clear();
    routes.clear();
    buses.clear();
    nextStopId = 1;
    nextRouteId = 1;
    nextBusId = 1;
}

}  // namespace urbania
