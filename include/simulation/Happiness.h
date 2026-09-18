#pragma once

#include "world/Tile.h"

class World;

namespace urbania {

class Citizen;
class CitizenManager;
class Pollution;

// City citizen happiness system.
// Calculates individual citizen happiness in range [0.0f, 100.0f]
// based on employment, nearby parks, commute routes, pollution, and housing.
//
// 0.0f   = extremely unhappy
// 50.0f  = neutral base
// 100.0f = extremely happy
//
// Updates citizen.happiness in place on simulation time (every 3600 sim seconds).
class Happiness {
public:
    static constexpr float MIN_HAPPINESS = 0.0f;
    static constexpr float MAX_HAPPINESS = 100.0f;
    static constexpr float BASE_HAPPINESS = 50.0f;

    static constexpr float EMPLOYED_BONUS = 15.0f;
    static constexpr float UNEMPLOYED_PENALTY = -15.0f;
    static constexpr float PARK_BONUS = 15.0f;
    static constexpr int PARK_HAPPINESS_RADIUS = 5;

    static constexpr float POLLUTION_PENALTY_FACTOR = 0.30f; // 100 pollution -> -30 penalty
    static constexpr float NO_ROUTE_PENALTY = -10.0f;
    static constexpr float COMMUTE_PENALTY_PER_STEP = 0.5f;
    static constexpr float MAX_COMMUTE_PENALTY = 10.0f;

    static constexpr float VALID_HOUSING_BONUS = 5.0f;
    static constexpr float INVALID_HOUSING_PENALTY = -20.0f;

    static constexpr float SIM_SECONDS_PER_HOUR = 3600.0f;

    Happiness();

    void update(const World& world, CitizenManager& citizens, const Pollution& pollution,
                float simulationDeltaTime);

    // Forces an immediate recalculation of all citizens' happiness.
    void recalculate(const World& world, CitizenManager& citizens, const Pollution& pollution);

    float getAverageHappiness() const;

private:
    float calculateCitizenHappiness(const Citizen& citizen, const World& world,
                                    const Pollution& pollution) const;
    bool hasNearbyPark(int homeX, int homeY, const World& world) const;

    float averageHappiness = BASE_HAPPINESS;
    float secondsTowardNextHour = 0.0f;
};

}  // namespace urbania
