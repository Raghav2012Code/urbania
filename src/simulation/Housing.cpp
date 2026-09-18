#include "simulation/Housing.h"

#include <algorithm>
#include <cmath>

#include "simulation/LandValue.h"
#include "simulation/Population.h"
#include "world/Tile.h"
#include "world/World.h"

namespace urbania {

Housing::Housing()
{
}

void Housing::update(const World& world, const Population& population,
                     const LandValue& landValue, float simulationDeltaTime)
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
        recalculate(world, population, landValue);
    }
}

void Housing::recalculate(const World& world, const Population& population,
                          const LandValue& landValue)
{
    totalCapacity = population.getTotalHousingCapacity();
    totalResidents = population.getTotalPopulation();

    occupancyRatio = (totalCapacity > 0)
                         ? (static_cast<float>(totalResidents) / static_cast<float>(totalCapacity))
                         : 0.0f;

    housingPressure = calculateHousingPressure(totalResidents, totalCapacity);

    residentialValues.clear();
    const int width = world.getWidth();
    const int height = world.getHeight();

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            if (world.getTile(x, y).type == TileType::Residential)
            {
                const float lv = landValue.getLandValue(x, y);
                residentialValues[{ x, y, true }] =
                    std::clamp(lv, MIN_RESIDENTIAL_VALUE, MAX_RESIDENTIAL_VALUE);
            }
        }
    }
}

int Housing::getTotalCapacity() const
{
    return totalCapacity;
}

int Housing::getTotalResidents() const
{
    return totalResidents;
}

float Housing::getOccupancyRatio() const
{
    return occupancyRatio;
}

int Housing::getHousingPressure() const
{
    return housingPressure;
}

float Housing::getResidentialValue(int x, int y) const
{
    const auto it = residentialValues.find({ x, y, true });
    if (it != residentialValues.end())
    {
        return it->second;
    }
    return 0.0f;
}

float Housing::getResidentialValue(const TileCoordinate& coord) const
{
    if (!coord.valid)
    {
        return 0.0f;
    }
    return getResidentialValue(coord.x, coord.y);
}

const std::map<TileCoordinate, float>& Housing::getResidentialGrid() const
{
    return residentialValues;
}

int Housing::calculateHousingPressure(int population, int housingCapacity) const
{
    // Handle zero capacity safely:
    // - If population is 0, pressure is neutral (0).
    // - If citizens exist with no housing, pressure is max (+100).
    if (housingCapacity <= 0)
    {
        return (population > 0) ? MAX_HOUSING_PRESSURE : 0;
    }

    // Occupancy ratio: 0.0 (all empty) to 1.0 (full capacity).
    // - Occupancy 0.0 (large surplus / empty homes) yields -100 pressure.
    // - Occupancy 0.5 (half full / balanced) yields 0 pressure.
    // - Occupancy 1.0 (full / shortage) yields +100 pressure.
    const float occupancy =
        static_cast<float>(population) / static_cast<float>(housingCapacity);
    const float score = (occupancy - 0.5f) * 200.0f;
    return std::clamp(static_cast<int>(std::round(score)), MIN_HOUSING_PRESSURE,
                      MAX_HOUSING_PRESSURE);
}

}  // namespace urbania
