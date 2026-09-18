#include "simulation/Congestion.h"

#include "simulation/Traffic.h"
#include "simulation/Vehicle.h"

namespace urbania {

void Congestion::update(const Traffic& traffic)
{
    usage.clear();
    congestedRoadCount = 0;
    maxCongestion = 0.0f;

    for (const Vehicle& vehicle : traffic.getVehicles())
    {
        if (!vehicle.active || vehicle.path.empty())
        {
            continue;
        }

        int index = vehicle.pathIndex;
        if (index < 0)
        {
            index = 0;
        }
        const int last = static_cast<int>(vehicle.path.size()) - 1;
        if (index > last)
        {
            index = last;
        }

        const TileCoordinate& tile = vehicle.path[index];
        if (tile.valid)
        {
            usage[{ tile.x, tile.y }] += 1;
        }
    }

    for (const auto& entry : usage)
    {
        const float congestion =
            static_cast<float>(entry.second) / static_cast<float>(ROAD_CAPACITY);
        if (congestion > 1.0f)
        {
            ++congestedRoadCount;
        }
        if (congestion > maxCongestion)
        {
            maxCongestion = congestion;
        }
    }
}

int Congestion::getCapacity()
{
    return ROAD_CAPACITY;
}

int Congestion::getVehicleCount(int x, int y) const
{
    const auto it = usage.find({ x, y });
    if (it == usage.end())
    {
        return 0;
    }
    return it->second;
}

float Congestion::getCongestion(int x, int y) const
{
    return static_cast<float>(getVehicleCount(x, y)) / static_cast<float>(ROAD_CAPACITY);
}

int Congestion::getCongestedRoadCount() const
{
    return congestedRoadCount;
}

float Congestion::getMaxCongestion() const
{
    return maxCongestion;
}

float Congestion::speedMultiplier(float congestion)
{
    // Free flow at 1.0, linearly down to 0.6 at capacity, then
    // hyperbolically toward zero under overload. Never negative.
    if (congestion <= 1.0f)
    {
        const float multiplier = 1.0f - congestion * 0.4f;
        return multiplier < 0.0f ? 0.0f : multiplier;
    }
    return 0.6f / congestion;
}

}  // namespace urbania
