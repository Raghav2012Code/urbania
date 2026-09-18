#include "simulation/Happiness.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "simulation/Citizen.h"
#include "simulation/CitizenManager.h"
#include "simulation/Pollution.h"
#include "simulation/Utilities.h"
#include "world/World.h"

namespace urbania {

Happiness::Happiness()
{
}

void Happiness::update(const World& world, CitizenManager& citizens, const Pollution& pollution,
                       const Utilities& utilities, float simulationDeltaTime)
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
        recalculate(world, citizens, pollution, utilities);
    }
}

void Happiness::recalculate(const World& world, CitizenManager& citizens,
                            const Pollution& pollution, const Utilities& utilities)
{
    const int count = citizens.getCitizenCount();
    if (count == 0)
    {
        averageHappiness = BASE_HAPPINESS;
        return;
    }

    float sum = 0.0f;
    for (Citizen& citizen : citizens.getCitizens())
    {
        citizen.happiness = calculateCitizenHappiness(citizen, world, pollution, utilities);
        sum += citizen.happiness;
    }

    averageHappiness = sum / static_cast<float>(count);
}

float Happiness::getAverageHappiness() const
{
    return averageHappiness;
}

float Happiness::calculateCitizenHappiness(const Citizen& citizen, const World& world,
                                           const Pollution& pollution,
                                           const Utilities& utilities) const
{
    float score = BASE_HAPPINESS;

    // 1. Employment modifier (+15 employed, -15 unemployed)
    if (citizen.employed)
    {
        score += EMPLOYED_BONUS;
    }
    else
    {
        score += UNEMPLOYED_PENALTY;
    }

    // 2. Housing modifier (+5 valid home, -20 missing/demolished home)
    const bool validHomeBounds = citizen.home.valid && citizen.home.x >= 0 &&
                                 citizen.home.y >= 0 && citizen.home.x < world.getWidth() &&
                                 citizen.home.y < world.getHeight();
    if (validHomeBounds &&
        world.getTile(citizen.home.x, citizen.home.y).type == TileType::Residential)
    {
        score += VALID_HOUSING_BONUS;
    }
    else
    {
        score += INVALID_HOUSING_PENALTY;
    }

    // 3. Parks modifier (+15 if at least one park within radius 5 of home)
    if (validHomeBounds && hasNearbyPark(citizen.home.x, citizen.home.y, world))
    {
        score += PARK_BONUS;
    }

    // 4. Pollution penalty (0 at 0 pollution up to -30 at 100 pollution)
    if (validHomeBounds)
    {
        const float homePollution = pollution.getPollution(citizen.home.x, citizen.home.y);
        const float pollutionPenalty = homePollution * POLLUTION_PENALTY_FACTOR;
        score -= pollutionPenalty;
    }

    // 5. Commute condition (for employed citizens only)
    if (citizen.employed)
    {
        if (citizen.commutePath.empty())
        {
            score += NO_ROUTE_PENALTY; // -10 for disconnected/unroutable workplace
        }
        else
        {
            // Short commutes (<= 5 steps) incur no penalty; longer commutes
            // scale gently at 0.5 per extra step up to a max -10 penalty.
            const float routeLength = static_cast<float>(citizen.commutePath.size());
            const float excess = std::max(0.0f, routeLength - 5.0f);
            const float commutePenalty =
                std::min(MAX_COMMUTE_PENALTY, excess * COMMUTE_PENALTY_PER_STEP);
            score -= commutePenalty;
        }
    }

    // 6. Utility services modifier (-20 if home lacks one or more basic utilities)
    if (validHomeBounds)
    {
        if (!utilities.isTileSupplied(citizen.home.x, citizen.home.y))
        {
            score += UNPOWERED_UTILITY_PENALTY;
        }
    }

    return std::clamp(score, MIN_HAPPINESS, MAX_HAPPINESS);
}

bool Happiness::hasNearbyPark(int homeX, int homeY, const World& world) const
{
    const int minX = std::max(0, homeX - PARK_HAPPINESS_RADIUS);
    const int maxX = std::min(world.getWidth() - 1, homeX + PARK_HAPPINESS_RADIUS);
    const int minY = std::max(0, homeY - PARK_HAPPINESS_RADIUS);
    const int maxY = std::min(world.getHeight() - 1, homeY + PARK_HAPPINESS_RADIUS);

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

}  // namespace urbania
