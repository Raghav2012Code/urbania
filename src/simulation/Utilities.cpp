#include "simulation/Utilities.h"

#include <algorithm>
#include <cmath>

#include "world/Tile.h"
#include "world/World.h"

namespace urbania {

Utilities::Utilities()
{
}

void Utilities::getDemandsForType(TileType type, int& outElec, int& outWater, int& outSewage)
{
    switch (type)
    {
    case TileType::Residential:
        outElec = RESIDENTIAL_ELECTRICITY_DEMAND;
        outWater = RESIDENTIAL_WATER_DEMAND;
        outSewage = RESIDENTIAL_SEWAGE_DEMAND;
        break;
    case TileType::Commercial:
        outElec = COMMERCIAL_ELECTRICITY_DEMAND;
        outWater = COMMERCIAL_WATER_DEMAND;
        outSewage = COMMERCIAL_SEWAGE_DEMAND;
        break;
    case TileType::Industrial:
        outElec = INDUSTRIAL_ELECTRICITY_DEMAND;
        outWater = INDUSTRIAL_WATER_DEMAND;
        outSewage = INDUSTRIAL_SEWAGE_DEMAND;
        break;
    default:
        outElec = 0;
        outWater = 0;
        outSewage = 0;
        break;
    }
}

bool Utilities::isConnectedToRoad(const World& world, int x, int y) const
{
    for (int dy = -UTILITY_CONNECTION_RADIUS; dy <= UTILITY_CONNECTION_RADIUS; ++dy)
    {
        for (int dx = -UTILITY_CONNECTION_RADIUS; dx <= UTILITY_CONNECTION_RADIUS; ++dx)
        {
            const int nx = x + dx;
            const int ny = y + dy;
            if (nx >= 0 && nx < world.getWidth() && ny >= 0 && ny < world.getHeight())
            {
                if (world.getTile(nx, ny).type == TileType::Road)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool Utilities::isConnectedToRoad(const World& world, const TileCoordinate& coord) const
{
    if (!coord.valid)
    {
        return false;
    }
    return isConnectedToRoad(world, coord.x, coord.y);
}

void Utilities::update(const World& world, float simulationDeltaTime)
{
    // Paused (zero/negative delta) => state cannot have changed via sim time;
    // skip the full O(W*H*25) rescan. World edits use onWorldModified().
    if (simulationDeltaTime <= 0.0f)
    {
        return;
    }
    recalculate(world);
}

void Utilities::recalculate(const World& world)
{
    electricityDemand = 0;
    waterDemand = 0;
    sewageDemand = 0;

    suppliedBuildings = 0;
    unsuppliedBuildings = 0;
    totalDevelopedBuildings = 0;

    statusGrid.clear();

    int allocatedElec = 0;
    int allocatedWater = 0;
    int allocatedSewage = 0;

    for (int y = 0; y < world.getHeight(); ++y)
    {
        for (int x = 0; x < world.getWidth(); ++x)
        {
            const Tile& tile = world.getTile(x, y);
            if (tile.type != TileType::Residential &&
                tile.type != TileType::Commercial &&
                tile.type != TileType::Industrial)
            {
                continue;
            }

            ++totalDevelopedBuildings;

            int e = 0;
            int w = 0;
            int s = 0;
            getDemandsForType(tile.type, e, w, s);

            electricityDemand += e;
            waterDemand += w;
            sewageDemand += s;

            TileUtilityStatus status;
            status.connected = isConnectedToRoad(world, x, y);

            if (status.connected)
            {
                // All-or-nothing: only consume capacity when the whole
                // building fits, so one starved resource never wastes the
                // other two on a building that stays unsupplied.
                const bool fitsElec = (allocatedElec + e <= electricityCapacity);
                const bool fitsWater = (allocatedWater + w <= waterCapacity);
                const bool fitsSewage = (allocatedSewage + s <= sewageCapacity);
                if (fitsElec && fitsWater && fitsSewage)
                {
                    status.hasElectricity = true;
                    status.hasWater = true;
                    status.hasSewage = true;
                    allocatedElec += e;
                    allocatedWater += w;
                    allocatedSewage += s;
                    status.isFullySupplied = true;
                    ++suppliedBuildings;
                }
                else
                {
                    status.isFullySupplied = false;
                    ++unsuppliedBuildings;
                }
            }
            else
            {
                status.isFullySupplied = false;
                ++unsuppliedBuildings;
            }

            statusGrid[TileCoordinate::validCoord(x, y)] = status;
        }
    }
}

TileUtilityStatus Utilities::getTileStatus(int x, int y) const
{
    auto it = statusGrid.find(TileCoordinate::validCoord(x, y));
    if (it != statusGrid.end())
    {
        return it->second;
    }
    return {};
}

TileUtilityStatus Utilities::getTileStatus(const TileCoordinate& coord) const
{
    if (!coord.valid)
    {
        return {};
    }
    return getTileStatus(coord.x, coord.y);
}

bool Utilities::isTileSupplied(int x, int y) const
{
    auto it = statusGrid.find(TileCoordinate::validCoord(x, y));
    if (it != statusGrid.end())
    {
        return it->second.isFullySupplied;
    }
    return false;
}

void Utilities::setElectricityCapacity(int capacity)
{
    electricityCapacity = std::max(0, capacity);
}

void Utilities::setWaterCapacity(int capacity)
{
    waterCapacity = std::max(0, capacity);
}

void Utilities::setSewageCapacity(int capacity)
{
    sewageCapacity = std::max(0, capacity);
}

bool Utilities::isTileSupplied(const TileCoordinate& coord) const
{
    if (!coord.valid)
    {
        return false;
    }
    return isTileSupplied(coord.x, coord.y);
}

int Utilities::getElectricityDemand() const
{
    return electricityDemand;
}

int Utilities::getWaterDemand() const
{
    return waterDemand;
}

int Utilities::getSewageDemand() const
{
    return sewageDemand;
}

int Utilities::getElectricityCapacity() const
{
    return electricityCapacity;
}

int Utilities::getWaterCapacity() const
{
    return waterCapacity;
}

int Utilities::getSewageCapacity() const
{
    return sewageCapacity;
}

int Utilities::getSuppliedBuildingCount() const
{
    return suppliedBuildings;
}

int Utilities::getUnsuppliedBuildingCount() const
{
    return unsuppliedBuildings;
}

int Utilities::getTotalDevelopedBuildingCount() const
{
    return totalDevelopedBuildings;
}

float Utilities::getDailyMaintenanceCost() const
{
    return TOTAL_DAILY_MAINTENANCE;
}

const std::map<TileCoordinate, TileUtilityStatus>& Utilities::getStatusGrid() const
{
    return statusGrid;
}

}  // namespace urbania
