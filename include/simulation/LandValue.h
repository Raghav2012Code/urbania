#pragma once

#include <map>

#include "world/Tile.h"

class World;

namespace urbania {

class Congestion;
class Pollution;

// City land value system.
// Calculates tile desirability in range [0.0f, 100.0f] based on nearby
// parks, environmental pollution, and traffic congestion.
//
// 0.0f   = extremely undesirable
// 50.0f  = neutral baseline
// 100.0f = extremely desirable
//
// Stored in a separate data structure keyed by TileCoordinate.
// Informational only for the player and future housing/demand systems.
// Calculations update once per simulation hour based on simulation time.
class LandValue {
public:
    static constexpr float MIN_LAND_VALUE = 0.0f;
    static constexpr float MAX_LAND_VALUE = 100.0f;
    static constexpr float BASE_LAND_VALUE = 50.0f;

    static constexpr float PARK_LAND_VALUE_BONUS = 15.0f;
    static constexpr int PARK_VALUE_RADIUS = 5;

    static constexpr float POLLUTION_PENALTY_FACTOR = 0.35f; // 100 pollution -> -35 penalty

    static constexpr int CONGESTION_VALUE_RADIUS = 3;
    static constexpr float CONGESTION_PENALTY_FACTOR = 10.0f;
    static constexpr float MAX_CONGESTION_PENALTY = 15.0f;

    static constexpr float SIM_SECONDS_PER_HOUR = 3600.0f;

    LandValue();

    void update(const World& world, const Pollution& pollution, const Congestion& congestion,
                float simulationDeltaTime);

    // Forces an immediate recalculation of all tiles' land values.
    void recalculate(const World& world, const Pollution& pollution,
                     const Congestion& congestion);

    float getLandValue(int x, int y) const;
    float getLandValue(const TileCoordinate& coord) const;
    float getAverageLandValue() const;
    const std::map<TileCoordinate, float>& getLandValueGrid() const;

private:
    float calculateTileLandValue(int x, int y, const World& world, const Pollution& pollution,
                                 const Congestion& congestion) const;
    bool hasNearbyPark(int tileX, int tileY, const World& world) const;
    float getNearbyCongestion(int tileX, int tileY, const World& world,
                              const Congestion& congestion) const;

    std::map<TileCoordinate, float> grid;
    float averageLandValue = BASE_LAND_VALUE;
    float secondsTowardNextHour = 0.0f;
};

}  // namespace urbania
