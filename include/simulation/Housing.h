#pragma once

#include <map>

#include "world/Tile.h"

class World;
class Population;

namespace urbania {

class LandValue;

// City housing and residential value system.
// Coordinates residential capacity, current occupancy, residential value,
// and housing pressure indicators.
//
// Housing capacity preserves the standard: 1 Residential tile = 10 residents.
// Residential Value is directly derived from LandValue clamped to [0, 100].
// Housing pressure indicates shortage (+100) vs surplus (-100) in [-100, +100].
//
// Stored and calculated separately from Tile data.
// Calculations update once per simulation hour based on simulation time.
// Independent from raylib.
class Housing {
public:
    static constexpr int CAPACITY_PER_TILE = 10;
    static constexpr int MIN_HOUSING_PRESSURE = -100;
    static constexpr int MAX_HOUSING_PRESSURE = 100;
    static constexpr float MIN_RESIDENTIAL_VALUE = 0.0f;
    static constexpr float MAX_RESIDENTIAL_VALUE = 100.0f;
    static constexpr float SIM_SECONDS_PER_HOUR = 3600.0f;

    Housing();

    void update(const World& world, const Population& population,
                const LandValue& landValue, float simulationDeltaTime);

    // Forces an immediate recalculation of housing metrics and residential values.
    void recalculate(const World& world, const Population& population,
                     const LandValue& landValue);

    int getTotalCapacity() const;
    int getTotalResidents() const;
    float getOccupancyRatio() const;
    int getHousingPressure() const;

    float getResidentialValue(int x, int y) const;
    float getResidentialValue(const TileCoordinate& coord) const;
    const std::map<TileCoordinate, float>& getResidentialGrid() const;

private:
    int calculateHousingPressure(int population, int housingCapacity) const;

    std::map<TileCoordinate, float> residentialValues;
    int totalCapacity = 0;
    int totalResidents = 0;
    float occupancyRatio = 0.0f;
    int housingPressure = 0;
    float secondsTowardNextHour = 0.0f;
};

}  // namespace urbania
