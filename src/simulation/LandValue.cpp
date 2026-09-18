#include "simulation/LandValue.h"

#include <algorithm>
#include <cmath>

#include "simulation/Congestion.h"
#include "simulation/Pollution.h"
#include "world/World.h"

namespace urbania {

LandValue::LandValue()
{
}

void LandValue::update(const World& world, const Pollution& pollution,
                       const Congestion& congestion, float simulationDeltaTime)
{
    // Zero or negative delta (e.g. paused) halts simulation progression.
    if (simulationDeltaTime <= 0.0f)
    {
        return;
    }

    secondsTowardNextHour += simulationDeltaTime;
    while (secondsTowardNextHour >= SIM_SECONDS_PER_HOUR)
    {
        secondsTowardNextHour -= SIM_SECONDS_PER_HOUR;
        recalculate(world, pollution, congestion);
    }
}

void LandValue::recalculate(const World& world, const Pollution& pollution,
                            const Congestion& congestion)
{
    const int width = world.getWidth();
    const int height = world.getHeight();
    const int totalTiles = width * height;
    if (totalTiles <= 0)
    {
        averageLandValue = BASE_LAND_VALUE;
        return;
    }

    grid.clear();
    float sum = 0.0f;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const float val = calculateTileLandValue(x, y, world, pollution, congestion);
            grid[{ x, y, true }] = val;
            sum += val;
        }
    }

    averageLandValue = sum / static_cast<float>(totalTiles);
}

float LandValue::getLandValue(int x, int y) const
{
    const auto it = grid.find({ x, y, true });
    if (it != grid.end())
    {
        return it->second;
    }
    return BASE_LAND_VALUE;
}

float LandValue::getLandValue(const TileCoordinate& coord) const
{
    if (!coord.valid)
    {
        return BASE_LAND_VALUE;
    }
    return getLandValue(coord.x, coord.y);
}

float LandValue::getAverageLandValue() const
{
    return averageLandValue;
}

const std::map<TileCoordinate, float>& LandValue::getLandValueGrid() const
{
    return grid;
}

float LandValue::calculateTileLandValue(int x, int y, const World& world,
                                        const Pollution& pollution,
                                        const Congestion& congestion) const
{
    float score = BASE_LAND_VALUE;

    // 1. Parks modifier (+15 if at least one park within radius 5)
    if (hasNearbyPark(x, y, world))
    {
        score += PARK_LAND_VALUE_BONUS;
    }

    // 2. Pollution penalty (linear: up to -35 at 100 pollution)
    const float tilePollution = pollution.getPollution(x, y);
    const float pollutionPenalty = tilePollution * POLLUTION_PENALTY_FACTOR;
    score -= pollutionPenalty;

    // 3. Traffic congestion penalty from nearby roads
    const float nearbyCongestion = getNearbyCongestion(x, y, world, congestion);
    const float congestionPenalty =
        std::min(MAX_CONGESTION_PENALTY, nearbyCongestion * CONGESTION_PENALTY_FACTOR);
    score -= congestionPenalty;

    return std::clamp(score, MIN_LAND_VALUE, MAX_LAND_VALUE);
}

bool LandValue::hasNearbyPark(int tileX, int tileY, const World& world) const
{
    const int minX = std::max(0, tileX - PARK_VALUE_RADIUS);
    const int maxX = std::min(world.getWidth() - 1, tileX + PARK_VALUE_RADIUS);
    const int minY = std::max(0, tileY - PARK_VALUE_RADIUS);
    const int maxY = std::min(world.getHeight() - 1, tileY + PARK_VALUE_RADIUS);

    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            if (world.getTile(x, y).type == TileType::Park)
            {
                return true;
            }
        }
    }

    return false;
}

float LandValue::getNearbyCongestion(int tileX, int tileY, const World& world,
                                     const Congestion& congestion) const
{
    const int minX = std::max(0, tileX - CONGESTION_VALUE_RADIUS);
    const int maxX = std::min(world.getWidth() - 1, tileX + CONGESTION_VALUE_RADIUS);
    const int minY = std::max(0, tileY - CONGESTION_VALUE_RADIUS);
    const int maxY = std::min(world.getHeight() - 1, tileY + CONGESTION_VALUE_RADIUS);

    float maxCong = 0.0f;
    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            if (world.getTile(x, y).type == TileType::Road)
            {
                const float c = congestion.getCongestion(x, y);
                if (c > maxCong)
                {
                    maxCong = c;
                }
            }
        }
    }

    return maxCong;
}

}  // namespace urbania
