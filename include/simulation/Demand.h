#pragma once

#include "world/Tile.h"

class World;
class Population;

namespace urbania {
class Employment;
}

enum class DemandType
{
    Residential,
    Commercial,
    Industrial
};

// City zoning and growth demand system.
// Calculates normalized demand values in range [-100, +100] for
// Residential, Commercial, and Industrial categories.
//
// -100 = very low demand (excess unused capacity / surplus)
//    0 = neutral
// +100 = very high demand (shortage / full capacity / growing need)
//
// Demand is strictly informational for the player and future systems.
// Calculations update once per simulation hour based on simulation time.
class Demand {
public:
    static constexpr int MIN_DEMAND = -100;
    static constexpr int MAX_DEMAND = 100;
    static constexpr float SIM_SECONDS_PER_HOUR = 3600.0f;

    Demand();

    int getResidentialDemand() const;
    int getCommercialDemand() const;
    int getIndustrialDemand() const;
    int getDemand(DemandType type) const;

    // Advances the demand timer on simulation time. Call once per frame
    // with the scaled simulation delta (0 while paused).
    void update(const World& world, const Population& population,
                const urbania::Employment& employment, float simulationDeltaTime);

    // Forces an immediate recalculation of demand figures.
    void recalculate(const World& world, const Population& population,
                     const urbania::Employment& employment);

private:
    int calculateResidentialDemand(int population, int housingCapacity) const;
    int calculateCommercialDemand(int population, int totalJobs, int occupiedJobs) const;
    int calculateIndustrialDemand(int population, int totalJobs, int occupiedJobs) const;

    int residentialDemand = 0;
    int commercialDemand = 0;
    int industrialDemand = 0;
    float secondsTowardNextHour = 0.0f;
};
