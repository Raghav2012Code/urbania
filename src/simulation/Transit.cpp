#include "simulation/Transit.h"

#include <algorithm>

#include "simulation/Economy.h"
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

    busStops.push_back({ nextId++, { x, y, true } });
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

    busStops.push_back({ nextId++, { x, y, true } });
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

const std::vector<BusStop>& Transit::getBusStops() const
{
    return busStops;
}

int Transit::getBusStopCount() const
{
    return static_cast<int>(busStops.size());
}

int Transit::getNextId() const
{
    return nextId;
}

void Transit::syncWithWorld(const World& world)
{
    for (auto it = busStops.begin(); it != busStops.end();)
    {
        if (!it->tile.valid || it->tile.x < 0 || it->tile.x >= world.getWidth() ||
            it->tile.y < 0 || it->tile.y >= world.getHeight() ||
            world.getTile(it->tile.x, it->tile.y).type != TileType::Road)
        {
            it = busStops.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Transit::clear()
{
    busStops.clear();
    nextId = 1;
}

}  // namespace urbania
